/** @file
  Mtftp6 Rrq process functions implementation.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp6Impl.h"

/**
  Build and send a ACK packet for download.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  BlockNum              The block number to be acked.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the packet.
  @retval EFI_SUCCESS           The ACK has been sent.
  @retval Others                Failed to send the ACK.

**/
EFI_STATUS
Mtftp6RrqSendAck (
  IN MTFTP6_INSTANCE  *Instance,
  IN UINT16           BlockNum
  )
{
  EFI_MTFTP6_PACKET  *Ack;
  NET_BUF            *Packet;
  EFI_STATUS         Status;

  Status = EFI_SUCCESS;

  //
  // Allocate net buffer to create ack packet.
  //
  Packet = NetbufAlloc (sizeof (EFI_MTFTP6_ACK_HEADER));

  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ack = (EFI_MTFTP6_PACKET *)NetbufAllocSpace (
                               Packet,
                               sizeof (EFI_MTFTP6_ACK_HEADER),
                               FALSE
                               );
  ASSERT (Ack != NULL);

  Ack->Ack.OpCode   = HTONS (EFI_MTFTP6_OPCODE_ACK);
  Ack->Ack.Block[0] = HTONS (BlockNum);

  //
  // Reset current retry count of the instance.
  //
  Instance->CurRetry   = 0;
  Instance->LastPacket = Packet;

  Status = Mtftp6TransmitPacket (Instance, Packet);
  if (!EFI_ERROR (Status)) {
    Instance->AckedBlock = Instance->TotalBlock;
  }

  return Status;
}

/**
  Deliver the received data block to the user, which can be saved
  in the user provide buffer or through the CheckPacket callback.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Packet                The pointer to the received packet.
  @param[in]  Len                   The packet length.
  @param[out] UdpPacket             The net buf of the received packet.

  @retval EFI_SUCCESS           The data was saved successfully.
  @retval EFI_ABORTED           The user tells to abort by return an error through
                                CheckPacket.
  @retval EFI_BUFFER_TOO_SMALL  The user's buffer is too small, and buffer length is
                                updated to the actual buffer size needed.

**/
EFI_STATUS
Mtftp6RrqSaveBlock (
  IN  MTFTP6_INSTANCE    *Instance,
  IN  EFI_MTFTP6_PACKET  *Packet,
  IN  UINT32             Len,
  OUT NET_BUF            **UdpPacket
  )
{
  EFI_MTFTP6_TOKEN  *Token;
  EFI_STATUS        Status;
  UINT16            Block;
  UINT64            Start;
  UINT32            DataLen;
  UINT64            BlockCounter;
  BOOLEAN           Completed;

  Completed = FALSE;
  Token     = Instance->Token;
  Block     = NTOHS (Packet->Data.Block);
  DataLen   = Len - MTFTP6_DATA_HEAD_LEN;

  //
  // This is the last block, save the block num
  //
  if (DataLen < Instance->BlkSize) {
    Completed         = TRUE;
    Instance->LastBlk = Block;
    Mtftp6SetLastBlockNum (&Instance->BlkList, Block);
  }

  //
  // Remove this block number from the file hole. If Mtftp6RemoveBlockNum
  // returns EFI_NOT_FOUND, the block has been saved, don't save it again.
  // Note that : For bigger files, allowing the block counter to roll over
  // to accept transfers of unlimited size. So BlockCounter is memorised as
  // continuous block counter.
  //
  Status = Mtftp6RemoveBlockNum (&Instance->BlkList, Block, Completed, &BlockCounter);

  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  } else if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Token->CheckPacket != NULL) {
    //
    // Callback to the check packet routine with the received packet.
    //
    Status = Token->CheckPacket (&Instance->Mtftp6, Token, (UINT16)Len, Packet);

    if (EFI_ERROR (Status)) {
      //
      // Free the received packet before send new packet in ReceiveNotify,
      // since the Udp6Io might need to be reconfigured.
      //
      NetbufFree (*UdpPacket);
      *UdpPacket = NULL;
      //
      // Send the Mtftp6 error message if user aborted the current session.
      //
      Mtftp6SendError (
        Instance,
        EFI_MTFTP6_ERRORCODE_ILLEGAL_OPERATION,
        (UINT8 *)"User aborted download"
        );

      return EFI_ABORTED;
    }
  }

  if (Token->Buffer != NULL) {
    Start = MultU64x32 (BlockCounter - 1, Instance->BlkSize);
    if (Start + DataLen <= Token->BufferSize) {
      CopyMem ((UINT8 *)Token->Buffer + Start, Packet->Data.Data, DataLen);
      //
      // Update the file size when received the last block
      //
      if ((Instance->LastBlk == Block) && Completed) {
        Token->BufferSize = Start + DataLen;
      }
    } else if (Instance->LastBlk != 0) {
      //
      // Don't save the data if the buffer is too small, return
      // EFI_BUFFER_TOO_SMALL if received the last packet. This
      // will give a accurate file length.
      //
      Token->BufferSize = Start + DataLen;

      //
      // Free the received packet before send new packet in ReceiveNotify,
      // since the udpio might need to be reconfigured.
      //
      NetbufFree (*UdpPacket);
      *UdpPacket = NULL;
      //
      // Send the Mtftp6 error message if no enough buffer.
      //
      Mtftp6SendError (
        Instance,
        EFI_MTFTP6_ERRORCODE_DISK_FULL,
        (UINT8 *)"User provided memory block is too small"
        );

      return EFI_BUFFER_TOO_SMALL;
    }
  }

  return EFI_SUCCESS;
}

/**
  Process the received data packets. It will save the block
  then send back an ACK if it is active.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Packet                The pointer to the received packet.
  @param[in]  Len                   The length of the packet.
  @param[out] UdpPacket             The net buf of received packet.
  @param[out] IsCompleted           If TRUE, the download has been completed.
                                    Otherwise, the download has not been completed.

  @retval EFI_SUCCESS           The data packet was successfully processed.
  @retval EFI_ABORTED           The download was aborted by the user.
  @retval EFI_BUFFER_TOO_SMALL  The user-provided buffer is too small.

**/
EFI_STATUS
Mtftp6RrqHandleData (
  IN  MTFTP6_INSTANCE    *Instance,
  IN  EFI_MTFTP6_PACKET  *Packet,
  IN  UINT32             Len,
  OUT NET_BUF            **UdpPacket,
  OUT BOOLEAN            *IsCompleted
  )
{
  EFI_STATUS  Status;
  UINT16      BlockNum;
  INTN        Expected;

  *IsCompleted = FALSE;
  Status       = EFI_SUCCESS;
  BlockNum     = NTOHS (Packet->Data.Block);
  Expected     = Mtftp6GetNextBlockNum (&Instance->BlkList);

  ASSERT (Expected >= 0);

  //
  // If we are active (Master) and received an unexpected packet, transmit
  // the ACK for the block we received, then restart receiving the
  // expected one. If we are passive (Slave), save the block.
  //
  if (Instance->IsMaster && (Expected != BlockNum)) {
    //
    // Free the received packet before send new packet in ReceiveNotify,
    // since the udpio might need to be reconfigured.
    //
    NetbufFree (*UdpPacket);
    *UdpPacket = NULL;

    //
    // If Expected is 0, (UINT16) (Expected - 1) is also the expected Ack number (65535).
    //
    return Mtftp6RrqSendAck (Instance, (UINT16)(Expected - 1));
  }

  Status = Mtftp6RrqSaveBlock (Instance, Packet, Len, UdpPacket);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Record the total received and saved block number.
  //
  Instance->TotalBlock++;

  //
  // Reset the passive client's timer whenever it received a valid data packet.
  //
  if (!Instance->IsMaster) {
    Instance->PacketToLive = Instance->Timeout * 2;
  }

  //
  // Check whether we have received all the blocks. Send the ACK if we
  // are active (unicast client or master client for multicast download).
  // If we have received all the blocks, send an ACK even if we are passive
  // to tell the server that we are done.
  //
  Expected = Mtftp6GetNextBlockNum (&Instance->BlkList);

  if (Instance->IsMaster || (Expected < 0)) {
    if (Expected < 0) {
      //
      // If we are passive client, then the just received Block maybe
      // isn't the last block. We need to send an ACK to the last block
      // to inform the server that we are done. If we are active client,
      // the Block == Instance->LastBlock.
      //
      BlockNum     = Instance->LastBlk;
      *IsCompleted = TRUE;
    } else {
      BlockNum = (UINT16)(Expected - 1);
    }

    //
    // Free the received packet before send new packet in ReceiveNotify,
    // since the udpio might need to be reconfigured.
    //
    NetbufFree (*UdpPacket);
    *UdpPacket = NULL;

    if ((Instance->WindowSize == (Instance->TotalBlock - Instance->AckedBlock)) || (Expected < 0)) {
      Status = Mtftp6RrqSendAck (Instance, BlockNum);
    }
  }

  return Status;
}

/**
  Validate whether the options received in the server's OACK packet is valid.
  The options are valid only if:
  1. The server doesn't include options not requested by us.
  2. The server can only use smaller blksize than that is requested.
  3. The server can only use the same timeout as requested.
  4. The server doesn't change its multicast channel.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  ReplyInfo             The pointer to options information in reply packet.
  @param[in]  RequestInfo           The pointer to requested options info.

  @retval     TRUE                  If the option in the OACK is valid.
  @retval     FALSE                 If the option is invalid.

**/
BOOLEAN
Mtftp6RrqOackValid (
  IN MTFTP6_INSTANCE         *Instance,
  IN MTFTP6_EXT_OPTION_INFO  *ReplyInfo,
  IN MTFTP6_EXT_OPTION_INFO  *RequestInfo
  )
{
  //
  // It is invalid for server to return options we don't request
  //
  if ((ReplyInfo->BitMap & ~RequestInfo->BitMap) != 0) {
    return FALSE;
  }

  //
  // Server can only specify a smaller block size and windowsize to be used and
  // return the timeout matches that requested.
  //
  if ((((ReplyInfo->BitMap & MTFTP6_OPT_BLKSIZE_BIT) != 0) && (ReplyInfo->BlkSize > RequestInfo->BlkSize)) ||
      (((ReplyInfo->BitMap & MTFTP6_OPT_WINDOWSIZE_BIT) != 0) && (ReplyInfo->BlkSize > RequestInfo->BlkSize)) ||
      (((ReplyInfo->BitMap & MTFTP6_OPT_TIMEOUT_BIT) != 0) && (ReplyInfo->Timeout != RequestInfo->Timeout))
      )
  {
    return FALSE;
  }

  //
  // The server can send ",,master" to client to change its master
  // setting. But if it use the specific multicast channel, it can't
  // change the setting.
  //
  if (((ReplyInfo->BitMap & MTFTP6_OPT_MCAST_BIT) != 0) && !NetIp6IsUnspecifiedAddr (&Instance->McastIp)) {
    if (!NetIp6IsUnspecifiedAddr (&ReplyInfo->McastIp) && (CompareMem (
                                                             &ReplyInfo->McastIp,
                                                             &Instance->McastIp,
                                                             sizeof (EFI_IPv6_ADDRESS)
                                                             ) != 0))
    {
      return FALSE;
    }

    if ((ReplyInfo->McastPort != 0) && (ReplyInfo->McastPort != Instance->McastPort)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Configure Udp6Io to receive a packet from a multicast address.

  @param[in]  McastIo               The pointer to the mcast Udp6Io.
  @param[in]  Context               The pointer to the context.

  @retval EFI_SUCCESS           The mcast Udp6Io was successfully configured.
  @retval Others                Failed to configure the Udp6Io.

**/
EFI_STATUS
EFIAPI
Mtftp6RrqConfigMcastUdpIo (
  IN UDP_IO  *McastIo,
  IN VOID    *Context
  )
{
  EFI_STATUS            Status;
  EFI_UDP6_PROTOCOL     *Udp6;
  EFI_UDP6_CONFIG_DATA  *Udp6Cfg;
  EFI_IPv6_ADDRESS      Group;
  MTFTP6_INSTANCE       *Instance;

  Udp6     = McastIo->Protocol.Udp6;
  Udp6Cfg  = &(McastIo->Config.Udp6);
  Instance = (MTFTP6_INSTANCE *)Context;

  //
  // Set the configure data for the mcast Udp6Io.
  //
  ZeroMem (Udp6Cfg, sizeof (EFI_UDP6_CONFIG_DATA));

  Udp6Cfg->AcceptPromiscuous  = FALSE;
  Udp6Cfg->AcceptAnyPort      = FALSE;
  Udp6Cfg->AllowDuplicatePort = FALSE;
  Udp6Cfg->TrafficClass       = 0;
  Udp6Cfg->HopLimit           = 128;
  Udp6Cfg->ReceiveTimeout     = 0;
  Udp6Cfg->TransmitTimeout    = 0;
  Udp6Cfg->StationPort        = Instance->McastPort;
  Udp6Cfg->RemotePort         = 0;

  CopyMem (
    &Udp6Cfg->RemoteAddress,
    &Instance->ServerIp,
    sizeof (EFI_IPv6_ADDRESS)
    );

  //
  // Configure the mcast Udp6Io.
  //
  Status = Udp6->Configure (Udp6, Udp6Cfg);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Join the multicast group
  //
  CopyMem (&Group, &Instance->McastIp, sizeof (EFI_IPv6_ADDRESS));

  return Udp6->Groups (Udp6, TRUE, &Group);
}

/**
  Process the OACK packet for Rrq.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Packet                The pointer to the received packet.
  @param[in]  Len                   The length of the packet.
  @param[out] UdpPacket             The net buf of received packet.
  @param[out] IsCompleted           If TRUE, the download has been completed.
                                    Otherwise, the download has not been completed.

  @retval EFI_DEVICE_ERROR      Failed to create/start a multicast Udp6 child.
  @retval EFI_TFTP_ERROR        An error happened during the process.
  @retval EFI_SUCCESS           The OACK packet successfully processed.

**/
EFI_STATUS
Mtftp6RrqHandleOack (
  IN  MTFTP6_INSTANCE    *Instance,
  IN  EFI_MTFTP6_PACKET  *Packet,
  IN  UINT32             Len,
  OUT NET_BUF            **UdpPacket,
  OUT BOOLEAN            *IsCompleted
  )
{
  EFI_MTFTP6_OPTION       *Options;
  UINT32                  Count;
  MTFTP6_EXT_OPTION_INFO  ExtInfo;
  EFI_STATUS              Status;
  INTN                    Expected;
  EFI_UDP6_PROTOCOL       *Udp6;

  *IsCompleted = FALSE;
  Options      = NULL;

  //
  // If already started the master download, don't change the
  // setting. Master download always succeeds.
  //
  Expected = Mtftp6GetNextBlockNum (&Instance->BlkList);
  ASSERT (Expected != -1);

  if (Instance->IsMaster && (Expected != 1)) {
    return EFI_SUCCESS;
  }

  ZeroMem (&ExtInfo, sizeof (MTFTP6_EXT_OPTION_INFO));

  //
  // Parse the options in the packet.
  //
  Status = Mtftp6ParseStart (Packet, Len, &Count, &Options);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Options != NULL);

  //
  // Parse the extensive options in the packet.
  //
  Status = Mtftp6ParseExtensionOption (Options, Count, FALSE, Instance->Operation, &ExtInfo);

  if (EFI_ERROR (Status) || !Mtftp6RrqOackValid (Instance, &ExtInfo, &Instance->ExtInfo)) {
    //
    // Don't send an ERROR packet if the error is EFI_OUT_OF_RESOURCES.
    //
    if (Status != EFI_OUT_OF_RESOURCES) {
      //
      // Free the received packet before send new packet in ReceiveNotify,
      // since the udpio might need to be reconfigured.
      //
      NetbufFree (*UdpPacket);
      *UdpPacket = NULL;
      //
      // Send the Mtftp6 error message if invalid packet.
      //
      Mtftp6SendError (
        Instance,
        EFI_MTFTP6_ERRORCODE_ILLEGAL_OPERATION,
        (UINT8 *)"Malformatted OACK packet"
        );
    }

    return EFI_TFTP_ERROR;
  }

  if ((ExtInfo.BitMap & MTFTP6_OPT_MCAST_BIT) != 0) {
    //
    // Save the multicast info. Always update the Master, only update the
    // multicast IP address, block size, window size, timeoute at the first time. If IP
    // address is updated, create a UDP child to receive the multicast.
    //
    Instance->IsMaster = ExtInfo.IsMaster;

    if (NetIp6IsUnspecifiedAddr (&Instance->McastIp)) {
      if (NetIp6IsUnspecifiedAddr (&ExtInfo.McastIp) || (ExtInfo.McastPort == 0)) {
        //
        // Free the received packet before send new packet in ReceiveNotify,
        // since the udpio might need to be reconfigured.
        //
        NetbufFree (*UdpPacket);
        *UdpPacket = NULL;
        //
        // Send the Mtftp6 error message if invalid multi-cast setting.
        //
        Mtftp6SendError (
          Instance,
          EFI_MTFTP6_ERRORCODE_ILLEGAL_OPERATION,
          (UINT8 *)"Illegal multicast setting"
          );

        return EFI_TFTP_ERROR;
      }

      //
      // Create a UDP child then start receive the multicast from it.
      //
      CopyMem (
        &Instance->McastIp,
        &ExtInfo.McastIp,
        sizeof (EFI_IP_ADDRESS)
        );

      Instance->McastPort = ExtInfo.McastPort;
      if (Instance->McastUdpIo == NULL) {
        Instance->McastUdpIo = UdpIoCreateIo (
                                 Instance->Service->Controller,
                                 Instance->Service->Image,
                                 Mtftp6RrqConfigMcastUdpIo,
                                 UDP_IO_UDP6_VERSION,
                                 Instance
                                 );
        if (Instance->McastUdpIo != NULL) {
          Status = gBS->OpenProtocol (
                          Instance->McastUdpIo->UdpHandle,
                          &gEfiUdp6ProtocolGuid,
                          (VOID **)&Udp6,
                          Instance->Service->Image,
                          Instance->Handle,
                          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                          );
          if (EFI_ERROR (Status)) {
            UdpIoFreeIo (Instance->McastUdpIo);
            Instance->McastUdpIo = NULL;
            return EFI_DEVICE_ERROR;
          }
        }
      }

      if (Instance->McastUdpIo == NULL) {
        return EFI_DEVICE_ERROR;
      }

      Status = UdpIoRecvDatagram (
                 Instance->McastUdpIo,
                 Mtftp6RrqInput,
                 Instance,
                 0
                 );

      if (EFI_ERROR (Status)) {
        //
        // Free the received packet before send new packet in ReceiveNotify,
        // since the udpio might need to be reconfigured.
        //
        NetbufFree (*UdpPacket);
        *UdpPacket = NULL;
        //
        // Send the Mtftp6 error message if failed to create Udp6Io to receive.
        //
        Mtftp6SendError (
          Instance,
          EFI_MTFTP6_ERRORCODE_ACCESS_VIOLATION,
          (UINT8 *)"Failed to create socket to receive multicast packet"
          );

        return Status;
      }

      //
      // Update the parameters used.
      //
      if (ExtInfo.BlkSize != 0) {
        Instance->BlkSize = ExtInfo.BlkSize;
      }

      if (ExtInfo.WindowSize != 0) {
        Instance->WindowSize = ExtInfo.WindowSize;
      }

      if (ExtInfo.Timeout != 0) {
        Instance->Timeout = ExtInfo.Timeout;
      }
    }
  } else {
    Instance->IsMaster = TRUE;

    if (ExtInfo.BlkSize != 0) {
      Instance->BlkSize = ExtInfo.BlkSize;
    }

    if (ExtInfo.WindowSize != 0) {
      Instance->WindowSize = ExtInfo.WindowSize;
    }

    if (ExtInfo.Timeout != 0) {
      Instance->Timeout = ExtInfo.Timeout;
    }
  }

  //
  // Free the received packet before send new packet in ReceiveNotify,
  // since the udpio might need to be reconfigured.
  //
  NetbufFree (*UdpPacket);
  *UdpPacket = NULL;
  //
  // Send an ACK to (Expected - 1) which is 0 for unicast download,
  // or tell the server we want to receive the Expected block.
  //
  return Mtftp6RrqSendAck (Instance, (UINT16)(Expected - 1));
}

/**
  The packet process callback for Mtftp6 download.

  @param[in]  UdpPacket             The pointer to the packet received.
  @param[in]  UdpEpt                The pointer to the Udp6 access point.
  @param[in]  IoStatus              The status from Udp6 instance.
  @param[in]  Context               The pointer to the context.

**/
VOID
EFIAPI
Mtftp6RrqInput (
  IN NET_BUF        *UdpPacket,
  IN UDP_END_POINT  *UdpEpt,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  )
{
  MTFTP6_INSTANCE    *Instance;
  EFI_MTFTP6_PACKET  *Packet;
  BOOLEAN            IsCompleted;
  BOOLEAN            IsMcast;
  EFI_STATUS         Status;
  UINT16             Opcode;
  UINT32             TotalNum;
  UINT32             Len;

  Instance = (MTFTP6_INSTANCE *)Context;

  NET_CHECK_SIGNATURE (Instance, MTFTP6_INSTANCE_SIGNATURE);

  Status      = EFI_SUCCESS;
  Packet      = NULL;
  IsCompleted = FALSE;
  IsMcast     = FALSE;
  TotalNum    = 0;

  //
  // Return error status if Udp6 instance failed to receive.
  //
  if (EFI_ERROR (IoStatus)) {
    Status = IoStatus;
    goto ON_EXIT;
  }

  ASSERT (UdpPacket != NULL);

  if (UdpPacket->TotalSize < MTFTP6_OPCODE_LEN) {
    goto ON_EXIT;
  }

  //
  // Find the port this packet is from to restart receive correctly.
  //
  if (CompareMem (
        Ip6Swap128 (&UdpEpt->LocalAddr.v6),
        &Instance->McastIp,
        sizeof (EFI_IPv6_ADDRESS)
        ) == 0)
  {
    IsMcast = TRUE;
  } else {
    IsMcast = FALSE;
  }

  //
  // Client send initial request to server's listening port. Server
  // will select a UDP port to communicate with the client. The server
  // is required to use the same port as RemotePort to multicast the
  // data.
  //
  if (UdpEpt->RemotePort != Instance->ServerDataPort) {
    if (Instance->ServerDataPort != 0) {
      goto ON_EXIT;
    } else {
      //
      // For the subsequent exchange of requests, reconfigure the udpio as
      // (serverip, serverport, localip, localport).
      // Usually, the client set serverport as 0 to receive and reset it
      // once the first packet arrives to send ack.
      //
      Instance->ServerDataPort = UdpEpt->RemotePort;
    }
  }

  //
  // Copy the MTFTP packet to a continuous buffer if it isn't already so.
  //
  Len      = UdpPacket->TotalSize;
  TotalNum = UdpPacket->BlockOpNum;

  if (TotalNum > 1) {
    Packet = AllocateZeroPool (Len);

    if (Packet == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    NetbufCopy (UdpPacket, 0, Len, (UINT8 *)Packet);
  } else {
    Packet = (EFI_MTFTP6_PACKET *)NetbufGetByte (UdpPacket, 0, NULL);
    ASSERT (Packet != NULL);
  }

  Opcode = NTOHS (Packet->OpCode);

  //
  // Callback to the user's CheckPacket if provided. Abort the transmission
  // if CheckPacket returns an EFI_ERROR code.
  //
  if ((Instance->Token->CheckPacket != NULL) &&
      ((Opcode == EFI_MTFTP6_OPCODE_OACK) || (Opcode == EFI_MTFTP6_OPCODE_ERROR))
      )
  {
    Status = Instance->Token->CheckPacket (
                                &Instance->Mtftp6,
                                Instance->Token,
                                (UINT16)Len,
                                Packet
                                );

    if (EFI_ERROR (Status)) {
      //
      // Send an error message to the server to inform it
      //
      if (Opcode != EFI_MTFTP6_OPCODE_ERROR) {
        //
        // Free the received packet before send new packet in ReceiveNotify,
        // since the udpio might need to be reconfigured.
        //
        NetbufFree (UdpPacket);
        UdpPacket = NULL;
        //
        // Send the Mtftp6 error message if user aborted the current session.
        //
        Mtftp6SendError (
          Instance,
          EFI_MTFTP6_ERRORCODE_REQUEST_DENIED,
          (UINT8 *)"User aborted the transfer"
          );
      }

      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

  //
  // Switch the process routines by the operation code.
  //
  switch (Opcode) {
    case EFI_MTFTP6_OPCODE_DATA:
      if ((Len > (UINT32)(MTFTP6_DATA_HEAD_LEN + Instance->BlkSize)) || (Len < (UINT32)MTFTP6_DATA_HEAD_LEN)) {
        goto ON_EXIT;
      }

      //
      // Handle the data packet of Rrq.
      //
      Status = Mtftp6RrqHandleData (
                 Instance,
                 Packet,
                 Len,
                 &UdpPacket,
                 &IsCompleted
                 );
      break;

    case EFI_MTFTP6_OPCODE_OACK:
      if (IsMcast || (Len <= MTFTP6_OPCODE_LEN)) {
        goto ON_EXIT;
      }

      //
      // Handle the Oack packet of Rrq.
      //
      Status = Mtftp6RrqHandleOack (
                 Instance,
                 Packet,
                 Len,
                 &UdpPacket,
                 &IsCompleted
                 );
      break;

    default:
      //
      // Drop and return error if received error message.
      //
      Status = EFI_TFTP_ERROR;
      break;
  }

ON_EXIT:
  //
  // Free the resources, then if !EFI_ERROR (Status), restart the
  // receive, otherwise end the session.
  //
  if ((Packet != NULL) && (TotalNum > 1)) {
    FreePool (Packet);
  }

  if (UdpPacket != NULL) {
    NetbufFree (UdpPacket);
  }

  if (!EFI_ERROR (Status) && !IsCompleted) {
    if (IsMcast) {
      Status = UdpIoRecvDatagram (
                 Instance->McastUdpIo,
                 Mtftp6RrqInput,
                 Instance,
                 0
                 );
    } else {
      Status = UdpIoRecvDatagram (
                 Instance->UdpIo,
                 Mtftp6RrqInput,
                 Instance,
                 0
                 );
    }
  }

  //
  // Clean up the current session if failed to continue.
  //
  if (EFI_ERROR (Status) || IsCompleted) {
    Mtftp6OperationClean (Instance, Status);
  }
}

/**
  Start the Mtftp6 instance to download. It first initializes some
  of the internal states, then builds and sends an RRQ request packet.
  Finally, it starts receive for the downloading.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Operation             The operation code of current packet.

  @retval EFI_SUCCESS           The Mtftp6 is started to download.
  @retval Others                Failed to start to download.

**/
EFI_STATUS
Mtftp6RrqStart (
  IN MTFTP6_INSTANCE  *Instance,
  IN UINT16           Operation
  )
{
  EFI_STATUS  Status;

  //
  // The valid block number range are [1, 0xffff]. For example:
  // the client sends an RRQ request to the server, the server
  // transfers the DATA1 block. If option negotiation is ongoing,
  // the server will send back an OACK, then client will send ACK0.
  //
  Status = Mtftp6InitBlockRange (&Instance->BlkList, 1, 0xffff);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mtftp6SendRequest (Instance, Operation);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return UdpIoRecvDatagram (
           Instance->UdpIo,
           Mtftp6RrqInput,
           Instance,
           0
           );
}
