/** @file
  Routines to process Rrq (download).

(C) Copyright 2014 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp4Impl.h"

/**
  The packet process callback for MTFTP download.

  @param  UdpPacket             The packet received
  @param  EndPoint              The local/remote access point of the packet
  @param  IoStatus              The status of the receiving
  @param  Context               Opaque parameter, which is the MTFTP session

**/
VOID
EFIAPI
Mtftp4RrqInput (
  IN NET_BUF        *UdpPacket,
  IN UDP_END_POINT  *EndPoint,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  );

/**
  Start the MTFTP session to download.

  It will first initialize some of the internal states then build and send a RRQ
  request packet, at last, it will start receive for the downloading.

  @param  Instance              The Mtftp session
  @param  Operation             The MTFTP opcode, it may be a EFI_MTFTP4_OPCODE_RRQ
                                or EFI_MTFTP4_OPCODE_DIR.

  @retval EFI_SUCCESS           The mtftp download session is started.
  @retval Others                Failed to start downloading.

**/
EFI_STATUS
Mtftp4RrqStart (
  IN MTFTP4_PROTOCOL  *Instance,
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
  Status = Mtftp4InitBlockRange (&Instance->Blocks, 1, 0xffff);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mtftp4SendRequest (Instance);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return UdpIoRecvDatagram (Instance->UnicastPort, Mtftp4RrqInput, Instance, 0);
}

/**
  Build and send a ACK packet for the download session.

  @param  Instance              The Mtftp session
  @param  BlkNo                 The BlkNo to ack.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the packet
  @retval EFI_SUCCESS           The ACK has been sent
  @retval Others                Failed to send the ACK.

**/
EFI_STATUS
Mtftp4RrqSendAck (
  IN MTFTP4_PROTOCOL  *Instance,
  IN UINT16           BlkNo
  )
{
  EFI_MTFTP4_PACKET  *Ack;
  NET_BUF            *Packet;
  EFI_STATUS         Status;

  Status = EFI_SUCCESS;

  Packet = NetbufAlloc (sizeof (EFI_MTFTP4_ACK_HEADER));
  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ack = (EFI_MTFTP4_PACKET *)NetbufAllocSpace (
                               Packet,
                               sizeof (EFI_MTFTP4_ACK_HEADER),
                               FALSE
                               );
  ASSERT (Ack != NULL);

  Ack->Ack.OpCode   = HTONS (EFI_MTFTP4_OPCODE_ACK);
  Ack->Ack.Block[0] = HTONS (BlkNo);

  Status = Mtftp4SendPacket (Instance, Packet);
  if (!EFI_ERROR (Status)) {
    Instance->AckedBlock = Instance->TotalBlock;
  }

  return Status;
}

/**
  Deliver the received data block to the user, which can be saved
  in the user provide buffer or through the CheckPacket callback.

  @param  Instance              The Mtftp session
  @param  Packet                The received data packet
  @param  Len                   The packet length

  @retval EFI_SUCCESS           The data is saved successfully
  @retval EFI_ABORTED           The user tells to abort by return an error  through
                                CheckPacket
  @retval EFI_BUFFER_TOO_SMALL  The user's buffer is too small and buffer length is
                                 updated to the actual buffer size needed.

**/
EFI_STATUS
Mtftp4RrqSaveBlock (
  IN OUT MTFTP4_PROTOCOL    *Instance,
  IN     EFI_MTFTP4_PACKET  *Packet,
  IN     UINT32             Len
  )
{
  EFI_MTFTP4_TOKEN  *Token;
  EFI_STATUS        Status;
  UINT16            Block;
  UINT64            Start;
  UINT32            DataLen;
  UINT64            BlockCounter;
  BOOLEAN           Completed;

  Completed = FALSE;
  Token     = Instance->Token;
  Block     = NTOHS (Packet->Data.Block);
  DataLen   = Len - MTFTP4_DATA_HEAD_LEN;

  //
  // This is the last block, save the block no
  //
  if (DataLen < Instance->BlkSize) {
    Completed           = TRUE;
    Instance->LastBlock = Block;
    Mtftp4SetLastBlockNum (&Instance->Blocks, Block);
  }

  //
  // Remove this block number from the file hole. If Mtftp4RemoveBlockNum
  // returns EFI_NOT_FOUND, the block has been saved, don't save it again.
  // Note that : For bigger files, allowing the block counter to roll over
  // to accept transfers of unlimited size. So BlockCounter is memorised as
  // continuous block counter.
  //
  Status = Mtftp4RemoveBlockNum (&Instance->Blocks, Block, Completed, &BlockCounter);

  if (Status == EFI_NOT_FOUND) {
    return EFI_SUCCESS;
  } else if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Token->CheckPacket != NULL) {
    Status = Token->CheckPacket (&Instance->Mtftp4, Token, (UINT16)Len, Packet);

    if (EFI_ERROR (Status)) {
      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_ILLEGAL_OPERATION,
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
      if ((Instance->LastBlock == Block) && Completed) {
        Token->BufferSize = Start + DataLen;
      }
    } else if (Instance->LastBlock != 0) {
      //
      // Don't save the data if the buffer is too small, return
      // EFI_BUFFER_TOO_SMALL if received the last packet. This
      // will give a accurate file length.
      //
      Token->BufferSize = Start + DataLen;

      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_DISK_FULL,
        (UINT8 *)"User provided memory block is too small"
        );

      return EFI_BUFFER_TOO_SMALL;
    }
  }

  return EFI_SUCCESS;
}

/**
  Function to process the received data packets.

  It will save the block then send back an ACK if it is active.

  @param  Instance              The downloading MTFTP session
  @param  Packet                The packet received
  @param  Len                   The length of the packet
  @param  Multicast             Whether this packet is multicast or unicast
  @param  Completed             Return whether the download has completed

  @retval EFI_SUCCESS           The data packet is successfully processed
  @retval EFI_ABORTED           The download is aborted by the user
  @retval EFI_BUFFER_TOO_SMALL  The user provided buffer is too small

**/
EFI_STATUS
Mtftp4RrqHandleData (
  IN     MTFTP4_PROTOCOL    *Instance,
  IN     EFI_MTFTP4_PACKET  *Packet,
  IN     UINT32             Len,
  IN     BOOLEAN            Multicast,
  OUT BOOLEAN               *Completed
  )
{
  EFI_STATUS  Status;
  UINT16      BlockNum;
  INTN        Expected;

  *Completed = FALSE;
  Status     = EFI_SUCCESS;
  BlockNum   = NTOHS (Packet->Data.Block);
  Expected   = Mtftp4GetNextBlockNum (&Instance->Blocks);

  ASSERT (Expected >= 0);

  //
  // If we are active (Master) and received an unexpected packet, transmit
  // the ACK for the block we received, then restart receiving the
  // expected one. If we are passive (Slave), save the block.
  //
  if (Instance->Master && (Expected != BlockNum)) {
    //
    // If Expected is 0, (UINT16) (Expected - 1) is also the expected Ack number (65535).
    //
    return Mtftp4RrqSendAck (Instance, (UINT16)(Expected - 1));
  }

  Status = Mtftp4RrqSaveBlock (Instance, Packet, Len);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Record the total received and saved block number.
  //
  Instance->TotalBlock++;

  //
  // Reset the passive client's timer whenever it received a
  // valid data packet.
  //
  if (!Instance->Master) {
    Mtftp4SetTimeout (Instance);
  }

  //
  // Check whether we have received all the blocks. Send the ACK if we
  // are active (unicast client or master client for multicast download).
  // If we have received all the blocks, send an ACK even if we are passive
  // to tell the server that we are done.
  //
  Expected = Mtftp4GetNextBlockNum (&Instance->Blocks);

  if (Instance->Master || (Expected < 0)) {
    if (Expected < 0) {
      //
      // If we are passive client, then the just received Block maybe
      // isn't the last block. We need to send an ACK to the last block
      // to inform the server that we are done. If we are active client,
      // the Block == Instance->LastBlock.
      //
      BlockNum   = Instance->LastBlock;
      *Completed = TRUE;
    } else {
      BlockNum = (UINT16)(Expected - 1);
    }

    if ((Instance->WindowSize == (Instance->TotalBlock - Instance->AckedBlock)) || (Expected < 0)) {
      Status = Mtftp4RrqSendAck (Instance, BlockNum);
    }
  }

  return Status;
}

/**
  Validate whether the options received in the server's OACK packet is valid.

  The options are valid only if:
  1. The server doesn't include options not requested by us
  2. The server can only use smaller blksize than that is requested
  3. The server can only use the same timeout as requested
  4. The server doesn't change its multicast channel.

  @param  This                  The downloading Mtftp session
  @param  Reply                 The options in the OACK packet
  @param  Request               The requested options

  @retval TRUE                  The options in the OACK is OK.
  @retval FALSE                 The options in the OACK is invalid.

**/
BOOLEAN
Mtftp4RrqOackValid (
  IN MTFTP4_PROTOCOL  *This,
  IN MTFTP4_OPTION    *Reply,
  IN MTFTP4_OPTION    *Request
  )
{
  //
  // It is invalid for server to return options we don't request
  //
  if ((Reply->Exist &~Request->Exist) != 0) {
    return FALSE;
  }

  //
  // Server can only specify a smaller block size and window size to be used and
  // return the timeout matches that requested.
  //
  if ((((Reply->Exist & MTFTP4_BLKSIZE_EXIST) != 0) && (Reply->BlkSize > Request->BlkSize)) ||
      (((Reply->Exist & MTFTP4_WINDOWSIZE_EXIST) != 0) && (Reply->WindowSize > Request->WindowSize)) ||
      (((Reply->Exist & MTFTP4_TIMEOUT_EXIST) != 0) && (Reply->Timeout != Request->Timeout))
      )
  {
    return FALSE;
  }

  //
  // The server can send ",,master" to client to change its master
  // setting. But if it use the specific multicast channel, it can't
  // change the setting.
  //
  if (((Reply->Exist & MTFTP4_MCAST_EXIST) != 0) && (This->McastIp != 0)) {
    if ((Reply->McastIp != 0) && (Reply->McastIp != This->McastIp)) {
      return FALSE;
    }

    if ((Reply->McastPort != 0) && (Reply->McastPort != This->McastPort)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Configure a UDP IO port to receive the multicast.

  @param  McastIo               The UDP IO to configure
  @param  Context               The opaque parameter to the function which is the
                                MTFTP session.

  @retval EFI_SUCCESS           The UDP child is successfully configured.
  @retval Others                Failed to configure the UDP child.

**/
EFI_STATUS
EFIAPI
Mtftp4RrqConfigMcastPort (
  IN UDP_IO  *McastIo,
  IN VOID    *Context
  )
{
  MTFTP4_PROTOCOL         *Instance;
  EFI_MTFTP4_CONFIG_DATA  *Config;
  EFI_UDP4_CONFIG_DATA    UdpConfig;
  EFI_IPv4_ADDRESS        Group;
  EFI_STATUS              Status;
  IP4_ADDR                Ip;

  Instance = (MTFTP4_PROTOCOL *)Context;
  Config   = &Instance->Config;

  UdpConfig.AcceptBroadcast    = FALSE;
  UdpConfig.AcceptPromiscuous  = FALSE;
  UdpConfig.AcceptAnyPort      = FALSE;
  UdpConfig.AllowDuplicatePort = FALSE;
  UdpConfig.TypeOfService      = 0;
  UdpConfig.TimeToLive         = 64;
  UdpConfig.DoNotFragment      = FALSE;
  UdpConfig.ReceiveTimeout     = 0;
  UdpConfig.TransmitTimeout    = 0;
  UdpConfig.UseDefaultAddress  = Config->UseDefaultSetting;
  IP4_COPY_ADDRESS (&UdpConfig.StationAddress, &Config->StationIp);
  IP4_COPY_ADDRESS (&UdpConfig.SubnetMask, &Config->SubnetMask);
  UdpConfig.StationPort = Instance->McastPort;
  UdpConfig.RemotePort  = 0;

  Ip = HTONL (Instance->ServerIp);
  IP4_COPY_ADDRESS (&UdpConfig.RemoteAddress, &Ip);

  Status = McastIo->Protocol.Udp4->Configure (McastIo->Protocol.Udp4, &UdpConfig);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (!Config->UseDefaultSetting &&
      !EFI_IP4_EQUAL (&mZeroIp4Addr, &Config->GatewayIp))
  {
    //
    // The station IP address is manually configured and the Gateway IP is not 0.
    // Add the default route for this UDP instance.
    //
    Status = McastIo->Protocol.Udp4->Routes (
                                       McastIo->Protocol.Udp4,
                                       FALSE,
                                       &mZeroIp4Addr,
                                       &mZeroIp4Addr,
                                       &Config->GatewayIp
                                       );

    if (EFI_ERROR (Status)) {
      McastIo->Protocol.Udp4->Configure (McastIo->Protocol.Udp4, NULL);
      return Status;
    }
  }

  //
  // join the multicast group
  //
  Ip = HTONL (Instance->McastIp);
  IP4_COPY_ADDRESS (&Group, &Ip);

  return McastIo->Protocol.Udp4->Groups (McastIo->Protocol.Udp4, TRUE, &Group);
}

/**
  Function to process the OACK.

  It will first validate the OACK packet, then update the various negotiated parameters.

  @param  Instance              The download MTFTP session
  @param  Packet                The packet received
  @param  Len                   The packet length
  @param  Multicast             Whether this packet is received as a multicast
  @param  Completed             Returns whether the download has completed. NOT
                                used  by this function.

  @retval EFI_DEVICE_ERROR      Failed to create/start a multicast UDP child
  @retval EFI_TFTP_ERROR        Some error happened during the process
  @retval EFI_SUCCESS           The OACK is successfully processed.

**/
EFI_STATUS
Mtftp4RrqHandleOack (
  IN OUT MTFTP4_PROTOCOL    *Instance,
  IN     EFI_MTFTP4_PACKET  *Packet,
  IN     UINT32             Len,
  IN     BOOLEAN            Multicast,
  OUT BOOLEAN               *Completed
  )
{
  MTFTP4_OPTION      Reply;
  EFI_STATUS         Status;
  INTN               Expected;
  EFI_UDP4_PROTOCOL  *Udp4;

  *Completed = FALSE;

  //
  // If already started the master download, don't change the
  // setting. Master download always succeeds.
  //
  Expected = Mtftp4GetNextBlockNum (&Instance->Blocks);
  ASSERT (Expected != -1);

  if (Instance->Master && (Expected != 1)) {
    return EFI_SUCCESS;
  }

  //
  // Parse and validate the options from server
  //
  ZeroMem (&Reply, sizeof (MTFTP4_OPTION));

  Status = Mtftp4ParseOptionOack (Packet, Len, Instance->Operation, &Reply);

  if (EFI_ERROR (Status) ||
      !Mtftp4RrqOackValid (Instance, &Reply, &Instance->RequestOption))
  {
    //
    // Don't send an ERROR packet if the error is EFI_OUT_OF_RESOURCES.
    //
    if (Status != EFI_OUT_OF_RESOURCES) {
      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_ILLEGAL_OPERATION,
        (UINT8 *)"Malformatted OACK packet"
        );
    }

    return EFI_TFTP_ERROR;
  }

  if ((Reply.Exist & MTFTP4_MCAST_EXIST) != 0) {
    //
    // Save the multicast info. Always update the Master, only update the
    // multicast IP address, block size, window size, timeout at the first time.
    // If IP address is updated, create a UDP child to receive the multicast.
    //
    Instance->Master = Reply.Master;

    if (Instance->McastIp == 0) {
      if ((Reply.McastIp == 0) || (Reply.McastPort == 0)) {
        Mtftp4SendError (
          Instance,
          EFI_MTFTP4_ERRORCODE_ILLEGAL_OPERATION,
          (UINT8 *)"Illegal multicast setting"
          );

        return EFI_TFTP_ERROR;
      }

      //
      // Create a UDP child then start receive the multicast from it.
      //
      Instance->McastIp   = Reply.McastIp;
      Instance->McastPort = Reply.McastPort;
      if (Instance->McastUdpPort == NULL) {
        Instance->McastUdpPort = UdpIoCreateIo (
                                   Instance->Service->Controller,
                                   Instance->Service->Image,
                                   Mtftp4RrqConfigMcastPort,
                                   UDP_IO_UDP4_VERSION,
                                   Instance
                                   );
        if (Instance->McastUdpPort != NULL) {
          Status = gBS->OpenProtocol (
                          Instance->McastUdpPort->UdpHandle,
                          &gEfiUdp4ProtocolGuid,
                          (VOID **)&Udp4,
                          Instance->Service->Image,
                          Instance->Handle,
                          EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                          );
          if (EFI_ERROR (Status)) {
            UdpIoFreeIo (Instance->McastUdpPort);
            Instance->McastUdpPort = NULL;
            return EFI_DEVICE_ERROR;
          }
        }
      }

      if (Instance->McastUdpPort == NULL) {
        return EFI_DEVICE_ERROR;
      }

      Status = UdpIoRecvDatagram (Instance->McastUdpPort, Mtftp4RrqInput, Instance, 0);

      if (EFI_ERROR (Status)) {
        Mtftp4SendError (
          Instance,
          EFI_MTFTP4_ERRORCODE_ACCESS_VIOLATION,
          (UINT8 *)"Failed to create socket to receive multicast packet"
          );

        return Status;
      }

      //
      // Update the parameters used.
      //
      if (Reply.BlkSize != 0) {
        Instance->BlkSize = Reply.BlkSize;
      }

      if (Reply.WindowSize != 0) {
        Instance->WindowSize = Reply.WindowSize;
      }

      if (Reply.Timeout != 0) {
        Instance->Timeout = Reply.Timeout;
      }
    }
  } else {
    Instance->Master = TRUE;

    if (Reply.BlkSize != 0) {
      Instance->BlkSize = Reply.BlkSize;
    }

    if (Reply.WindowSize != 0) {
      Instance->WindowSize = Reply.WindowSize;
    }

    if (Reply.Timeout != 0) {
      Instance->Timeout = Reply.Timeout;
    }
  }

  //
  // Send an ACK to (Expected - 1) which is 0 for unicast download,
  // or tell the server we want to receive the Expected block.
  //
  return Mtftp4RrqSendAck (Instance, (UINT16)(Expected - 1));
}

/**
  The packet process callback for MTFTP download.

  @param  UdpPacket             The packet received
  @param  EndPoint              The local/remote access point of the packet
  @param  IoStatus              The status of the receiving
  @param  Context               Opaque parameter, which is the MTFTP session

**/
VOID
EFIAPI
Mtftp4RrqInput (
  IN NET_BUF        *UdpPacket,
  IN UDP_END_POINT  *EndPoint,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  )
{
  MTFTP4_PROTOCOL    *Instance;
  EFI_MTFTP4_PACKET  *Packet;
  BOOLEAN            Completed;
  BOOLEAN            Multicast;
  EFI_STATUS         Status;
  UINT16             Opcode;
  UINT32             Len;

  Instance = (MTFTP4_PROTOCOL *)Context;
  NET_CHECK_SIGNATURE (Instance, MTFTP4_PROTOCOL_SIGNATURE);

  Status    = EFI_SUCCESS;
  Packet    = NULL;
  Completed = FALSE;
  Multicast = FALSE;

  if (EFI_ERROR (IoStatus)) {
    Status = IoStatus;
    goto ON_EXIT;
  }

  ASSERT (UdpPacket != NULL);

  //
  // Find the port this packet is from to restart receive correctly.
  //
  Multicast = (BOOLEAN)(EndPoint->LocalAddr.Addr[0] == Instance->McastIp);

  if (UdpPacket->TotalSize < MTFTP4_OPCODE_LEN) {
    goto ON_EXIT;
  }

  //
  // Client send initial request to server's listening port. Server
  // will select a UDP port to communicate with the client. The server
  // is required to use the same port as RemotePort to multicast the
  // data.
  //
  if (EndPoint->RemotePort != Instance->ConnectedPort) {
    if (Instance->ConnectedPort != 0) {
      goto ON_EXIT;
    } else {
      Instance->ConnectedPort = EndPoint->RemotePort;
    }
  }

  //
  // Copy the MTFTP packet to a continuous buffer if it isn't already so.
  //
  Len = UdpPacket->TotalSize;

  if (UdpPacket->BlockOpNum > 1) {
    Packet = AllocatePool (Len);

    if (Packet == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }

    NetbufCopy (UdpPacket, 0, Len, (UINT8 *)Packet);
  } else {
    Packet = (EFI_MTFTP4_PACKET *)NetbufGetByte (UdpPacket, 0, NULL);
    ASSERT (Packet != NULL);
  }

  Opcode = NTOHS (Packet->OpCode);

  //
  // Call the user's CheckPacket if provided. Abort the transmission
  // if CheckPacket returns an EFI_ERROR code.
  //
  if ((Instance->Token->CheckPacket != NULL) &&
      ((Opcode == EFI_MTFTP4_OPCODE_OACK) || (Opcode == EFI_MTFTP4_OPCODE_ERROR)))
  {
    Status = Instance->Token->CheckPacket (
                                &Instance->Mtftp4,
                                Instance->Token,
                                (UINT16)Len,
                                Packet
                                );

    if (EFI_ERROR (Status)) {
      //
      // Send an error message to the server to inform it
      //
      if (Opcode != EFI_MTFTP4_OPCODE_ERROR) {
        Mtftp4SendError (
          Instance,
          EFI_MTFTP4_ERRORCODE_REQUEST_DENIED,
          (UINT8 *)"User aborted the transfer"
          );
      }

      Status = EFI_ABORTED;
      goto ON_EXIT;
    }
  }

  switch (Opcode) {
    case EFI_MTFTP4_OPCODE_DATA:
      if ((Len > (UINT32)(MTFTP4_DATA_HEAD_LEN + Instance->BlkSize)) ||
          (Len < (UINT32)MTFTP4_DATA_HEAD_LEN))
      {
        goto ON_EXIT;
      }

      Status = Mtftp4RrqHandleData (Instance, Packet, Len, Multicast, &Completed);
      break;

    case EFI_MTFTP4_OPCODE_OACK:
      if (Multicast || (Len <= MTFTP4_OPCODE_LEN)) {
        goto ON_EXIT;
      }

      Status = Mtftp4RrqHandleOack (Instance, Packet, Len, Multicast, &Completed);
      break;

    case EFI_MTFTP4_OPCODE_ERROR:
      Status = EFI_TFTP_ERROR;
      break;

    default:
      break;
  }

ON_EXIT:

  //
  // Free the resources, then if !EFI_ERROR (Status), restart the
  // receive, otherwise end the session.
  //
  if ((Packet != NULL) && (UdpPacket->BlockOpNum > 1)) {
    FreePool (Packet);
  }

  if (UdpPacket != NULL) {
    NetbufFree (UdpPacket);
  }

  if (!EFI_ERROR (Status) && !Completed) {
    if (Multicast) {
      Status = UdpIoRecvDatagram (Instance->McastUdpPort, Mtftp4RrqInput, Instance, 0);
    } else {
      Status = UdpIoRecvDatagram (Instance->UnicastPort, Mtftp4RrqInput, Instance, 0);
    }
  }

  if (EFI_ERROR (Status) || Completed) {
    Mtftp4CleanOperation (Instance, Status);
  }
}
