/** @file
  Mtftp6 Wrq process functions implementation.

  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp6Impl.h"

/**
  Build and send a Mtftp6 data packet for upload.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  BlockNum              The block num to be sent.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the packet.
  @retval EFI_SUCCESS           The data packet was sent.
  @retval EFI_ABORTED           The user aborted this process.

**/
EFI_STATUS
Mtftp6WrqSendBlock (
  IN MTFTP6_INSTANCE  *Instance,
  IN UINT16           BlockNum
  )
{
  EFI_MTFTP6_PACKET  *Packet;
  EFI_MTFTP6_TOKEN   *Token;
  NET_BUF            *UdpPacket;
  EFI_STATUS         Status;
  UINT16             DataLen;
  UINT8              *DataBuf;
  UINT64             Start;

  //
  // Allocate net buffer to create data packet.
  //
  UdpPacket = NetbufAlloc (Instance->BlkSize + MTFTP6_DATA_HEAD_LEN);

  if (UdpPacket == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Packet = (EFI_MTFTP6_PACKET *)NetbufAllocSpace (
                                  UdpPacket,
                                  MTFTP6_DATA_HEAD_LEN,
                                  FALSE
                                  );
  ASSERT (Packet != NULL);

  Packet->Data.OpCode = HTONS (EFI_MTFTP6_OPCODE_DATA);
  Packet->Data.Block  = HTONS (BlockNum);

  //
  // Read the block from either the buffer or PacketNeeded callback
  //
  Token   = Instance->Token;
  DataLen = Instance->BlkSize;

  if (Token->Buffer != NULL) {
    Start = MultU64x32 (BlockNum - 1, Instance->BlkSize);

    if (Token->BufferSize < Start + Instance->BlkSize) {
      DataLen           = (UINT16)(Token->BufferSize - Start);
      Instance->LastBlk = BlockNum;
      Mtftp6SetLastBlockNum (&Instance->BlkList, BlockNum);
    }

    if (DataLen > 0) {
      NetbufAllocSpace (UdpPacket, DataLen, FALSE);
      CopyMem (Packet->Data.Data, (UINT8 *)Token->Buffer + Start, DataLen);
    }
  } else {
    //
    // Get data from PacketNeeded
    //
    DataBuf = NULL;
    Status  = Token->PacketNeeded (&Instance->Mtftp6, Token, &DataLen, (VOID *)&DataBuf);

    if (EFI_ERROR (Status) || (DataLen > Instance->BlkSize)) {
      if (DataBuf != NULL) {
        gBS->FreePool (DataBuf);
      }

      //
      // The received packet has already been freed.
      //
      Mtftp6SendError (
        Instance,
        EFI_MTFTP6_ERRORCODE_REQUEST_DENIED,
        (UINT8 *)"User aborted the transfer"
        );

      return EFI_ABORTED;
    }

    if (DataLen < Instance->BlkSize) {
      Instance->LastBlk = BlockNum;
      Mtftp6SetLastBlockNum (&Instance->BlkList, BlockNum);
    }

    if (DataLen > 0) {
      NetbufAllocSpace (UdpPacket, DataLen, FALSE);
      CopyMem (Packet->Data.Data, DataBuf, DataLen);
      gBS->FreePool (DataBuf);
    }
  }

  //
  // Reset current retry count of the instance.
  //
  Instance->CurRetry = 0;

  return Mtftp6TransmitPacket (Instance, UdpPacket);
}

/**
  Function to handle received ACK packet. If the ACK number matches the
  expected block number, with more data pending, send the next
  block. Otherwise, tell the caller that we are done.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Packet                The pointer to the received packet.
  @param[in]  Len                   The length of the packet.
  @param[out] UdpPacket             The net buf of received packet.
  @param[out] IsCompleted           If TRUE, the upload has been completed.
                                    Otherwise, the upload has not been completed.

  @retval EFI_SUCCESS           The ACK packet successfully processed.
  @retval EFI_TFTP_ERROR        The block number loops back.
  @retval Others                Failed to transmit the next data packet.

**/
EFI_STATUS
Mtftp6WrqHandleAck (
  IN  MTFTP6_INSTANCE    *Instance,
  IN  EFI_MTFTP6_PACKET  *Packet,
  IN  UINT32             Len,
  OUT NET_BUF            **UdpPacket,
  OUT BOOLEAN            *IsCompleted
  )
{
  UINT16  AckNum;
  INTN    Expected;
  UINT64  BlockCounter;

  *IsCompleted = FALSE;
  AckNum       = NTOHS (Packet->Ack.Block[0]);
  Expected     = Mtftp6GetNextBlockNum (&Instance->BlkList);

  ASSERT (Expected >= 0);

  //
  // Get an unwanted ACK, return EFI_SUCCESS to let Mtftp6WrqInput
  // restart receive.
  //
  if (Expected != AckNum) {
    return EFI_SUCCESS;
  }

  //
  // Remove the acked block number, if this is the last block number,
  // tell the Mtftp6WrqInput to finish the transfer. This is the last
  // block number if the block range are empty.
  //
  Mtftp6RemoveBlockNum (&Instance->BlkList, AckNum, *IsCompleted, &BlockCounter);

  Expected = Mtftp6GetNextBlockNum (&Instance->BlkList);

  if (Expected < 0) {
    //
    // The block range is empty. It may either because the last
    // block has been ACKed, or the sequence number just looped back,
    // that is, there is more than 0xffff blocks.
    //
    if (Instance->LastBlk == AckNum) {
      ASSERT (Instance->LastBlk >= 1);
      *IsCompleted = TRUE;
      return EFI_SUCCESS;
    } else {
      //
      // Free the received packet before send new packet in ReceiveNotify,
      // since the udpio might need to be reconfigured.
      //
      NetbufFree (*UdpPacket);
      *UdpPacket = NULL;
      //
      // Send the Mtftp6 error message if block number rolls back.
      //
      Mtftp6SendError (
        Instance,
        EFI_MTFTP6_ERRORCODE_REQUEST_DENIED,
        (UINT8 *)"Block number rolls back, not supported, try blksize option"
        );

      return EFI_TFTP_ERROR;
    }
  }

  //
  // Free the receive buffer before send new packet since it might need
  // reconfigure udpio.
  //
  NetbufFree (*UdpPacket);
  *UdpPacket = NULL;

  return Mtftp6WrqSendBlock (Instance, (UINT16)Expected);
}

/**
  Check whether the received OACK is valid. The OACK is valid
  only if:
  1. It only include options requested by us.
  2. It can only include a smaller block size.
  3. It can't change the proposed time out value.
  4. Other requirements of the individal MTFTP6 options as required.

  @param[in]  ReplyInfo             The pointer to options information in reply packet.
  @param[in]  RequestInfo           The pointer to requested options information.

  @retval     TRUE                  If the option in OACK is valid.
  @retval     FALSE                 If the option is invalid.

**/
BOOLEAN
Mtftp6WrqOackValid (
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
  // Server can only specify a smaller block size to be used and
  // return the timeout matches that requested.
  //
  if ((((ReplyInfo->BitMap & MTFTP6_OPT_BLKSIZE_BIT) != 0) && (ReplyInfo->BlkSize > RequestInfo->BlkSize)) ||
      (((ReplyInfo->BitMap & MTFTP6_OPT_TIMEOUT_BIT) != 0) && (ReplyInfo->Timeout != RequestInfo->Timeout))
      )
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Process the OACK packet for Wrq.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Packet                The pointer to the received packet.
  @param[in]  Len                   The length of the packet.
  @param[out] UdpPacket             The net buf of received packet.
  @param[out] IsCompleted           If TRUE, the upload has been completed.
                                    Otherwise, the upload has not been completed.

  @retval EFI_SUCCESS           The OACK packet successfully processed.
  @retval EFI_TFTP_ERROR        An TFTP communication error happened.
  @retval Others                Failed to process the OACK packet.

**/
EFI_STATUS
Mtftp6WrqHandleOack (
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
  EFI_MTFTP6_PACKET       Dummy;
  EFI_STATUS              Status;
  INTN                    Expected;

  *IsCompleted = FALSE;
  Options      = NULL;

  //
  // Ignore the OACK if already started the upload
  //
  Expected = Mtftp6GetNextBlockNum (&Instance->BlkList);

  if (Expected != 0) {
    return EFI_SUCCESS;
  }

  //
  // Parse and validate the options from server
  //
  ZeroMem (&ExtInfo, sizeof (MTFTP6_EXT_OPTION_INFO));

  Status = Mtftp6ParseStart (Packet, Len, &Count, &Options);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Options != NULL);

  Status = Mtftp6ParseExtensionOption (Options, Count, FALSE, Instance->Operation, &ExtInfo);

  if (EFI_ERROR (Status) || !Mtftp6WrqOackValid (&ExtInfo, &Instance->ExtInfo)) {
    //
    // Don't send a MTFTP error packet when out of resource, it can
    // only make it worse.
    //
    if (Status != EFI_OUT_OF_RESOURCES) {
      //
      // Free the received packet before send new packet in ReceiveNotify,
      // since the udpio might need to be reconfigured.
      //
      NetbufFree (*UdpPacket);
      *UdpPacket = NULL;
      //
      // Send the Mtftp6 error message if invalid Oack packet received.
      //
      Mtftp6SendError (
        Instance,
        EFI_MTFTP6_ERRORCODE_ILLEGAL_OPERATION,
        (UINT8 *)"Malformatted OACK packet"
        );
    }

    return EFI_TFTP_ERROR;
  }

  if (ExtInfo.BlkSize != 0) {
    Instance->BlkSize = ExtInfo.BlkSize;
  }

  if (ExtInfo.Timeout != 0) {
    Instance->Timeout = ExtInfo.Timeout;
  }

  //
  // Build a bogus ACK0 packet then pass it to the Mtftp6WrqHandleAck,
  // which will start the transmission of the first data block.
  //
  Dummy.Ack.OpCode   = HTONS (EFI_MTFTP6_OPCODE_ACK);
  Dummy.Ack.Block[0] = 0;

  return Mtftp6WrqHandleAck (
           Instance,
           &Dummy,
           sizeof (EFI_MTFTP6_ACK_HEADER),
           UdpPacket,
           IsCompleted
           );
}

/**
  The packet process callback for Mtftp6 upload.

  @param[in]  UdpPacket             The pointer to the packet received.
  @param[in]  UdpEpt                The pointer to the Udp6 access point.
  @param[in]  IoStatus              The status from Udp6 instance.
  @param[in]  Context               The pointer to the context.

**/
VOID
EFIAPI
Mtftp6WrqInput (
  IN NET_BUF        *UdpPacket,
  IN UDP_END_POINT  *UdpEpt,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  )
{
  MTFTP6_INSTANCE    *Instance;
  EFI_MTFTP6_PACKET  *Packet;
  BOOLEAN            IsCompleted;
  EFI_STATUS         Status;
  UINT32             TotalNum;
  UINT32             Len;
  UINT16             Opcode;

  Instance = (MTFTP6_INSTANCE *)Context;

  NET_CHECK_SIGNATURE (Instance, MTFTP6_INSTANCE_SIGNATURE);

  IsCompleted = FALSE;
  Packet      = NULL;
  Status      = EFI_SUCCESS;
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
  // Client send initial request to server's listening port. Server
  // will select a UDP port to communicate with the client.
  //
  if (UdpEpt->RemotePort != Instance->ServerDataPort) {
    if (Instance->ServerDataPort != 0) {
      goto ON_EXIT;
    } else {
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
    case EFI_MTFTP6_OPCODE_ACK:
      if (Len != MTFTP6_OPCODE_LEN + MTFTP6_BLKNO_LEN) {
        goto ON_EXIT;
      }

      //
      // Handle the Ack packet of Wrq.
      //
      Status = Mtftp6WrqHandleAck (Instance, Packet, Len, &UdpPacket, &IsCompleted);
      break;

    case EFI_MTFTP6_OPCODE_OACK:
      if (Len <= MTFTP6_OPCODE_LEN) {
        goto ON_EXIT;
      }

      //
      // Handle the Oack packet of Wrq.
      //
      Status = Mtftp6WrqHandleOack (Instance, Packet, Len, &UdpPacket, &IsCompleted);
      break;

    default:
      //
      // Drop and return eror if received error message.
      //
      Status = EFI_TFTP_ERROR;
      break;
  }

ON_EXIT:
  //
  // Free the resources, then if !EFI_ERROR (Status) and not completed,
  // restart the receive, otherwise end the session.
  //
  if ((Packet != NULL) && (TotalNum > 1)) {
    FreePool (Packet);
  }

  if (UdpPacket != NULL) {
    NetbufFree (UdpPacket);
  }

  if (!EFI_ERROR (Status) && !IsCompleted) {
    Status = UdpIoRecvDatagram (
               Instance->UdpIo,
               Mtftp6WrqInput,
               Instance,
               0
               );
  }

  //
  // Clean up the current session if failed to continue.
  //
  if (EFI_ERROR (Status) || IsCompleted) {
    Mtftp6OperationClean (Instance, Status);
  }
}

/**
  Start the Mtftp6 instance to upload. It will first init some states,
  then send the WRQ request packet, and start to receive the packet.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Operation             The operation code of the current packet.

  @retval EFI_SUCCESS           The Mtftp6 was started to upload.
  @retval Others                Failed to start to upload.

**/
EFI_STATUS
Mtftp6WrqStart (
  IN MTFTP6_INSTANCE  *Instance,
  IN UINT16           Operation
  )
{
  EFI_STATUS  Status;

  //
  // The valid block number range are [0, 0xffff]. For example:
  // the client sends an WRQ request to the server, the server
  // ACK with an ACK0 to let client start transfer the first
  // packet.
  //
  Status = Mtftp6InitBlockRange (&Instance->BlkList, 0, 0xffff);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mtftp6SendRequest (Instance, Operation);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return UdpIoRecvDatagram (
           Instance->UdpIo,
           Mtftp6WrqInput,
           Instance,
           0
           );
}
