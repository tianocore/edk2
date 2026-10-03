/** @file
  Routines to process Wrq (upload).

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp4Impl.h"

/**
  Build then send a MTFTP data packet for the MTFTP upload session.

  @param  Instance              The MTFTP upload session.
  @param  BlockNum              The block number to send.

  @retval EFI_OUT_OF_RESOURCES  Failed to build the packet.
  @retval EFI_ABORTED           The consumer of this child directs to abort the
                                transmission by return an error through PacketNeeded.
  @retval EFI_SUCCESS           The data is sent.

**/
EFI_STATUS
Mtftp4WrqSendBlock (
  IN OUT MTFTP4_PROTOCOL  *Instance,
  IN     UINT16           BlockNum
  )
{
  EFI_MTFTP4_PACKET  *Packet;
  EFI_MTFTP4_TOKEN   *Token;
  NET_BUF            *UdpPacket;
  EFI_STATUS         Status;
  UINT16             DataLen;
  UINT8              *DataBuf;
  UINT64             Start;

  //
  // Allocate a buffer to hold the user data
  //
  UdpPacket = NetbufAlloc (Instance->BlkSize + MTFTP4_DATA_HEAD_LEN);

  if (UdpPacket == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Packet = (EFI_MTFTP4_PACKET *)NetbufAllocSpace (UdpPacket, MTFTP4_DATA_HEAD_LEN, FALSE);
  if (Packet == NULL) {
    ASSERT (Packet != NULL);
    NetbufFree (UdpPacket);
    return EFI_OUT_OF_RESOURCES;
  }

  Packet->Data.OpCode = HTONS (EFI_MTFTP4_OPCODE_DATA);
  Packet->Data.Block  = HTONS (BlockNum);

  //
  // Read the block from either the buffer or PacketNeeded callback
  //
  Token   = Instance->Token;
  DataLen = Instance->BlkSize;

  if (Token->Buffer != NULL) {
    Start = MultU64x32 (BlockNum - 1, Instance->BlkSize);

    if (Token->BufferSize < Start + Instance->BlkSize) {
      DataLen             = (UINT16)(Token->BufferSize - Start);
      Instance->LastBlock = BlockNum;
      Mtftp4SetLastBlockNum (&Instance->Blocks, BlockNum);
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
    Status  = Token->PacketNeeded (
                       &Instance->Mtftp4,
                       Token,
                       &DataLen,
                       (VOID **)&DataBuf
                       );

    if (EFI_ERROR (Status) || (DataLen > Instance->BlkSize)) {
      if (DataBuf != NULL) {
        FreePool (DataBuf);
      }

      if (UdpPacket != NULL) {
        NetbufFree (UdpPacket);
      }

      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_REQUEST_DENIED,
        (UINT8 *)"User aborted the transfer"
        );

      return EFI_ABORTED;
    }

    if (DataLen < Instance->BlkSize) {
      Instance->LastBlock = BlockNum;
      Mtftp4SetLastBlockNum (&Instance->Blocks, BlockNum);
    }

    if (DataLen > 0) {
      NetbufAllocSpace (UdpPacket, DataLen, FALSE);
      CopyMem (Packet->Data.Data, DataBuf, DataLen);
      FreePool (DataBuf);
    }
  }

  return Mtftp4SendPacket (Instance, UdpPacket);
}

/**
  Function to handle received ACK packet.

  If the ACK number matches the expected block number, and there are more
  data pending, send the next block. Otherwise tell the caller that we are done.

  @param  Instance              The MTFTP upload session
  @param  Packet                The MTFTP packet received
  @param  Len                   The packet length
  @param  Completed             Return whether the upload has finished.

  @retval EFI_SUCCESS           The ACK is successfully processed.
  @retval EFI_TFTP_ERROR        The block number loops back.
  @retval Others                Failed to transmit the next data packet.

**/
EFI_STATUS
Mtftp4WrqHandleAck (
  IN     MTFTP4_PROTOCOL    *Instance,
  IN     EFI_MTFTP4_PACKET  *Packet,
  IN     UINT32             Len,
  OUT BOOLEAN               *Completed
  )
{
  UINT16  AckNum;
  INTN    Expected;
  UINT64  BlockCounter;

  *Completed = FALSE;
  AckNum     = NTOHS (Packet->Ack.Block[0]);
  Expected   = Mtftp4GetNextBlockNum (&Instance->Blocks);

  ASSERT (Expected >= 0);

  //
  // Get an unwanted ACK, return EFI_SUCCESS to let Mtftp4WrqInput
  // restart receive.
  //
  if (Expected != AckNum) {
    return EFI_SUCCESS;
  }

  //
  // Remove the acked block number, if this is the last block number,
  // tell the Mtftp4WrqInput to finish the transfer. This is the last
  // block number if the block range are empty.
  //
  Mtftp4RemoveBlockNum (&Instance->Blocks, AckNum, *Completed, &BlockCounter);

  Expected = Mtftp4GetNextBlockNum (&Instance->Blocks);

  if (Expected < 0) {
    //
    // The block range is empty. It may either because the last
    // block has been ACKed, or the sequence number just looped back,
    // that is, there is more than 0xffff blocks.
    //
    if (Instance->LastBlock == AckNum) {
      ASSERT (Instance->LastBlock >= 1);
      *Completed = TRUE;
      return EFI_SUCCESS;
    } else {
      Mtftp4SendError (
        Instance,
        EFI_MTFTP4_ERRORCODE_REQUEST_DENIED,
        (UINT8 *)"Block number rolls back, not supported, try blksize option"
        );

      return EFI_TFTP_ERROR;
    }
  }

  return Mtftp4WrqSendBlock (Instance, (UINT16)Expected);
}

/**
  Check whether the received OACK is valid.

  The OACK is valid only if:
  1. It only include options requested by us
  2. It can only include a smaller block size
  3. It can't change the proposed time out value.
  4. Other requirements of the individal MTFTP options as required.

  @param  Reply                 The options included in the OACK
  @param  Request               The options we requested

  @retval TRUE                  The options included in OACK is valid.
  @retval FALSE                 The options included in OACK is invalid.

**/
BOOLEAN
Mtftp4WrqOackValid (
  IN MTFTP4_OPTION  *Reply,
  IN MTFTP4_OPTION  *Request
  )
{
  //
  // It is invalid for server to return options we don't request
  //
  if ((Reply->Exist & ~Request->Exist) != 0) {
    return FALSE;
  }

  //
  // Server can only specify a smaller block size to be used and
  // return the timeout matches that requested.
  //
  if ((((Reply->Exist & MTFTP4_BLKSIZE_EXIST) != 0) && (Reply->BlkSize > Request->BlkSize)) ||
      (((Reply->Exist & MTFTP4_TIMEOUT_EXIST) != 0) && (Reply->Timeout != Request->Timeout)))
  {
    return FALSE;
  }

  return TRUE;
}

/**
  Function to handle the MTFTP OACK packet.

  It parses the packet's options, and update the internal states of the session.

  @param  Instance              The MTFTP session
  @param  Packet                The received OACK packet
  @param  Len                   The length of the packet
  @param  Completed             Whether the transmission has completed. NOT used by
                                this function.

  @retval EFI_SUCCESS           The OACK process is OK
  @retval EFI_TFTP_ERROR        Some error occurred, and the session reset.

**/
EFI_STATUS
Mtftp4WrqHandleOack (
  IN OUT MTFTP4_PROTOCOL    *Instance,
  IN     EFI_MTFTP4_PACKET  *Packet,
  IN     UINT32             Len,
  OUT BOOLEAN               *Completed
  )
{
  MTFTP4_OPTION      Reply;
  EFI_MTFTP4_PACKET  Bogus;
  EFI_STATUS         Status;
  INTN               Expected;

  *Completed = FALSE;

  //
  // Ignore the OACK if already started the upload
  //
  Expected = Mtftp4GetNextBlockNum (&Instance->Blocks);

  if (Expected != 0) {
    return EFI_SUCCESS;
  }

  //
  // Parse and validate the options from server
  //
  ZeroMem (&Reply, sizeof (MTFTP4_OPTION));
  Status = Mtftp4ParseOptionOack (Packet, Len, Instance->Operation, &Reply);

  if (EFI_ERROR (Status) || !Mtftp4WrqOackValid (&Reply, &Instance->RequestOption)) {
    //
    // Don't send a MTFTP error packet when out of resource, it can
    // only make it worse.
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

  if (Reply.BlkSize != 0) {
    Instance->BlkSize = Reply.BlkSize;
  }

  if (Reply.Timeout != 0) {
    Instance->Timeout = Reply.Timeout;
  }

  //
  // Build a bogus ACK0 packet then pass it to the Mtftp4WrqHandleAck,
  // which will start the transmission of the first data block.
  //
  Bogus.Ack.OpCode   = HTONS (EFI_MTFTP4_OPCODE_ACK);
  Bogus.Ack.Block[0] = 0;

  Status = Mtftp4WrqHandleAck (
             Instance,
             &Bogus,
             sizeof (EFI_MTFTP4_ACK_HEADER),
             Completed
             );

  return Status;
}

/**
  The input process routine for MTFTP upload.

  @param  UdpPacket             The received MTFTP packet.
  @param  EndPoint              The local/remote access point
  @param  IoStatus              The result of the packet receiving
  @param  Context               Opaque parameter for the callback, which is the
                                MTFTP session.
**/
VOID
EFIAPI
Mtftp4WrqInput (
  IN NET_BUF        *UdpPacket,
  IN UDP_END_POINT  *EndPoint,
  IN EFI_STATUS     IoStatus,
  IN VOID           *Context
  )
{
  MTFTP4_PROTOCOL    *Instance;
  EFI_MTFTP4_PACKET  *Packet;
  BOOLEAN            Completed;
  EFI_STATUS         Status;
  UINT32             Len;
  UINT16             Opcode;

  Instance = (MTFTP4_PROTOCOL *)Context;
  NET_CHECK_SIGNATURE (Instance, MTFTP4_PROTOCOL_SIGNATURE);

  Completed = FALSE;
  Packet    = NULL;
  Status    = EFI_SUCCESS;

  if (EFI_ERROR (IoStatus)) {
    Status = IoStatus;
    goto ON_EXIT;
  }

  ASSERT (UdpPacket != NULL);

  if (UdpPacket->TotalSize < MTFTP4_OPCODE_LEN) {
    goto ON_EXIT;
  }

  //
  // Client send initial request to server's listening port. Server
  // will select a UDP port to communicate with the client.
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
    if (Packet == NULL) {
      ASSERT (Packet != NULL);
      Status = EFI_OUT_OF_RESOURCES;
      goto ON_EXIT;
    }
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
    case EFI_MTFTP4_OPCODE_ACK:
      if (Len != MTFTP4_OPCODE_LEN + MTFTP4_BLKNO_LEN) {
        goto ON_EXIT;
      }

      Status = Mtftp4WrqHandleAck (Instance, Packet, Len, &Completed);
      break;

    case EFI_MTFTP4_OPCODE_OACK:
      if (Len <= MTFTP4_OPCODE_LEN) {
        goto ON_EXIT;
      }

      Status = Mtftp4WrqHandleOack (Instance, Packet, Len, &Completed);
      break;

    case EFI_MTFTP4_OPCODE_ERROR:
      Status = EFI_TFTP_ERROR;
      break;

    default:
      break;
  }

ON_EXIT:
  //
  // Free the resources, then if !EFI_ERROR (Status) and not completed,
  // restart the receive, otherwise end the session.
  //
  if ((Packet != NULL) && (UdpPacket->BlockOpNum > 1)) {
    FreePool (Packet);
  }

  if (UdpPacket != NULL) {
    NetbufFree (UdpPacket);
  }

  if (!EFI_ERROR (Status) && !Completed) {
    Status = UdpIoRecvDatagram (Instance->UnicastPort, Mtftp4WrqInput, Instance, 0);
  }

  //
  // Status may have been updated by UdpIoRecvDatagram
  //
  if (EFI_ERROR (Status) || Completed) {
    Mtftp4CleanOperation (Instance, Status);
  }
}

/**
  Start the MTFTP session for upload.

  It will first init some states, then send the WRQ request packet,
  and start receiving the packet.

  @param  Instance              The MTFTP session
  @param  Operation             Redundant parameter, which is always
                                EFI_MTFTP4_OPCODE_WRQ here.

  @retval EFI_SUCCESS           The upload process has been started.
  @retval Others                Failed to start the upload.

**/
EFI_STATUS
Mtftp4WrqStart (
  IN MTFTP4_PROTOCOL  *Instance,
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
  Status = Mtftp4InitBlockRange (&Instance->Blocks, 0, 0xffff);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = Mtftp4SendRequest (Instance);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return UdpIoRecvDatagram (Instance->UnicastPort, Mtftp4WrqInput, Instance, 0);
}
