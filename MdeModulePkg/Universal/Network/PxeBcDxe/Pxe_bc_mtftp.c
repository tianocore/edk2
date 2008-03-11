/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    pxe_bc_mtftp.c

Abstract:
  TFTP and MTFTP (multicast TFTP) implementation.

Revision History


**/

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// The following #define is used to create a version that does not wait to
// open after a listen.  This is just for a special regression test of MTFTP
// server to make sure multiple opens are handled correctly.  Normally this
// next line should be a comment.
// #define SpecialNowaitVersion    // comment out for normal operation
//

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "Bc.h"

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
UINT64
Swap64 (
  UINT64 n
  )
{
  union {
    UINT64  n;
    UINT8   b[8];
  } u;

  UINT8 t;

  u.n     = n;

  t       = u.b[0];
  u.b[0]  = u.b[7];
  u.b[7]  = t;

  t       = u.b[1];
  u.b[1]  = u.b[6];
  u.b[6]  = t;

  t       = u.b[2];
  u.b[2]  = u.b[5];
  u.b[5]  = t;

  t       = u.b[3];
  u.b[3]  = u.b[4];
  u.b[4]  = t;

  return u.n;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return EFI_SUCCESS :=
  @return EFI_TFTP_ERROR :=
  @return other :=

**/
STATIC
EFI_STATUS
TftpUdpRead (
  PXE_BASECODE_DEVICE         *Private,
  UINT16                      Operation,
  VOID                        *HeaderPtr,
  UINTN                       *BufferSizePtr,
  VOID                        *BufferPtr,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_IP_ADDRESS              *OurIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *OurPortPtr,
  UINT16                      Timeout
  )
{
  EFI_PXE_BASE_CODE_MODE  *PxeBcMode;
  EFI_STATUS              Status;
  EFI_EVENT               TimeoutEvent;
  UINTN                   HeaderSize;

  //
  //
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &TimeoutEvent
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->SetTimer (
                  TimeoutEvent,
                  TimerRelative,
                  Timeout * 10000000 + 1000000
                  );

  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (TimeoutEvent);
    return Status;
  }
  //
  //
  //
  HeaderSize = Private->BigBlkNumFlag ? sizeof (struct Tftpv4Ack8) : sizeof (struct Tftpv4Ack);

#define ERROR_MESSAGE_PTR ((struct Tftpv4Error *) HeaderPtr)

  Status = UdpRead (
            Private,
            Operation,
            OurIpPtr,
            OurPortPtr,
            ServerIpPtr,
            ServerPortPtr,
            &HeaderSize,
            HeaderPtr,
            BufferSizePtr,
            BufferPtr,
            TimeoutEvent
            );

  if (Status != EFI_SUCCESS || ERROR_MESSAGE_PTR->OpCode != HTONS (TFTP_ERROR)) {
    gBS->CloseEvent (TimeoutEvent);
    return Status;
  }
  //
  // got an error packet
  // write one byte error code followed by error message
  //
  PxeBcMode                       = Private->EfiBc.Mode;
  PxeBcMode->TftpErrorReceived    = TRUE;
  PxeBcMode->TftpError.ErrorCode  = (UINT8) NTOHS (ERROR_MESSAGE_PTR->ErrCode);
  HeaderSize                      = MIN (*BufferSizePtr, sizeof PxeBcMode->TftpError.ErrorString);
  CopyMem (PxeBcMode->TftpError.ErrorString, BufferPtr, HeaderSize);

  gBS->CloseEvent (TimeoutEvent);
  return EFI_TFTP_ERROR;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
VOID
SendError (
  PXE_BASECODE_DEVICE         *Private,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *OurPortPtr
  )
{
  struct Tftpv4Error  *ErrStr;
  UINTN               Len;

  ErrStr            = (VOID *) Private->TftpErrorBuffer;
  Len               = sizeof *ErrStr;

  ErrStr->OpCode    = HTONS (TFTP_ERROR);
  ErrStr->ErrCode   = HTONS (TFTP_ERR_OPTION);
  ErrStr->ErrMsg[0] = 0;

  UdpWrite (
    Private,
    EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT,
    ServerIpPtr,
    ServerPortPtr,
    0,
    0,
    OurPortPtr,
    0,
    0,
    &Len,
    ErrStr
    );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
SendAckAndGetData (
  PXE_BASECODE_DEVICE         *Private,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_IP_ADDRESS              *ReplyIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *OurPortPtr,
  UINT16                      Timeout,
  UINTN                       *ReplyLenPtr,
  UINT8                       *PxeBcMode,
  UINT64                      *BlockNumPtr,
  BOOLEAN                     AckOnly
  )
{
  struct Tftpv4Data DataBuffer;
  struct Tftpv4Ack  *Ack2Ptr;
  struct Tftpv4Ack8 *Ack8Ptr;
  EFI_STATUS        Status;
  UINTN             Len;

  Ack2Ptr = (VOID *) Private->TftpAckBuffer;
  Ack8Ptr = (VOID *) Private->TftpAckBuffer;

  if (Private->BigBlkNumFlag) {
    Len               = sizeof (struct Tftpv4Ack8);

    Ack8Ptr->OpCode   = HTONS (TFTP_ACK8);
    Ack8Ptr->BlockNum = Swap64 (*BlockNumPtr);

    Status = UdpWrite (
              Private,
              EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT,
              ServerIpPtr,
              ServerPortPtr,
              0,
              0,
              OurPortPtr,
              0,
              0,
              &Len,
              Ack8Ptr
              );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    Len               = sizeof (struct Tftpv4Ack);

    Ack2Ptr->OpCode   = HTONS (TFTP_ACK);
    Ack2Ptr->BlockNum = HTONS ((UINT16) *BlockNumPtr);

    Status = UdpWrite (
              Private,
              EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT,
              ServerIpPtr,
              ServerPortPtr,
              0,
              0,
              OurPortPtr,
              0,
              0,
              &Len,
              Ack2Ptr
              );

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (AckOnly) {
    //
    // ACK of last packet.  This is just a courtesy.
    // Do not wait for response.
    //
    return EFI_SUCCESS;
  }
  //
  // read reply
  //
  Status = TftpUdpRead (
            Private,
            0,
            &DataBuffer,
            ReplyLenPtr,
            PxeBcMode,
            ServerIpPtr,
            ServerPortPtr,
            ReplyIpPtr,
            OurPortPtr,
            Timeout
            );

  if (EFI_ERROR (Status) && Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }
  //
  // got a good reply (so far)
  // check for next data packet
  //
  if (!Private->BigBlkNumFlag && DataBuffer.Header.OpCode == HTONS (TFTP_DATA)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      SendError (Private, ServerIpPtr, ServerPortPtr, OurPortPtr);
    }

    *BlockNumPtr = NTOHS (DataBuffer.Header.BlockNum);
    return Status;
  }

  if (Private->BigBlkNumFlag && DataBuffer.Header.OpCode == HTONS (TFTP_DATA8)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      SendError (Private, ServerIpPtr, ServerPortPtr, OurPortPtr);
    }

    *BlockNumPtr = Swap64 (*(UINT64 *) &DataBuffer.Header.BlockNum);
    return Status;
  }

  return EFI_PROTOCOL_ERROR;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
LockStepReceive (
  PXE_BASECODE_DEVICE         *Private,
  UINTN                       PacketSize,
  UINT64                      *BufferSizePtr,
  UINT64                      Offset,
  UINT8                       *BufferPtr,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_IP_ADDRESS              *ReplyIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *OurPortPtr,
  UINT64                      LastBlock,
  UINT16                      Timeout,
  IN BOOLEAN                  DontUseBuffer
  )
{
  EFI_STATUS  Status;
  UINT64      BlockNum;
  UINT64      BufferSize;
  UINTN       Retries;
  UINTN       SaveLen;
  UINTN       ReplyLen;

  ReplyLen  = PacketSize;
  BlockNum  = LastBlock;

  DEBUG ((DEBUG_INFO, "\nLockStepReceive()  PacketSize = %d", PacketSize));

  if (DontUseBuffer) {
    BufferSize = PacketSize;
  } else {
    BufferSize = *BufferSizePtr - Offset;
    BufferPtr += Offset;
  }

  while (ReplyLen >= 512 && ReplyLen == PacketSize) {
    if (BufferSize < PacketSize) {
      ReplyLen = (UINTN) ((BufferSize > 0) ? BufferSize : 0);
    }

    SaveLen = ReplyLen;

    //
    // write an ack packet and get data - retry up to NUM_ACK_RETRIES on timeout
    //
    Retries = NUM_ACK_RETRIES;

    do {
      ReplyLen = SaveLen;

      Status = SendAckAndGetData (
                Private,
                ServerIpPtr,
                ServerPortPtr,
                ReplyIpPtr,
                OurPortPtr,
                Timeout,
                (UINTN *) &ReplyLen,
                BufferPtr,
                &BlockNum,
                FALSE
                );

      if (!EFI_ERROR (Status) || Status == EFI_BUFFER_TOO_SMALL) {
        if (BlockNum == LastBlock) {
          DEBUG ((DEBUG_NET, "\nresend"));
          //
          // a resend - continue
          //
          Status = EFI_TIMEOUT;
        } else if (Private->BigBlkNumFlag) {
          if (BlockNum != ++LastBlock) {
            DEBUG ((DEBUG_NET, "\nLockStepReceive()  Exit #1a"));
            //
            // not correct blocknum - error
            //
            return EFI_PROTOCOL_ERROR;
          }
        } else {
          LastBlock = (LastBlock + 1) & 0xFFFF;
          if (BlockNum != LastBlock) {
            DEBUG ((DEBUG_NET, "\nLockStepReceive()  Exit #1b"));
            return EFI_PROTOCOL_ERROR;
            //
            // not correct blocknum - error
            //
          }
        }
      }
    } while (Status == EFI_TIMEOUT && --Retries);

    if (EFI_ERROR (Status)) {
      if (Status != EFI_BUFFER_TOO_SMALL) {
        SendError (Private, ServerIpPtr, ServerPortPtr, OurPortPtr);
      }

      return Status;
    }

    if (DontUseBuffer) {
      BufferSize += ReplyLen;
    } else {
      BufferPtr += ReplyLen;
      BufferSize -= ReplyLen;
    }
  }
  //
  // while (ReplyLen == PacketSize);
  //
  if (DontUseBuffer) {
    if (BufferSizePtr != NULL) {
      *BufferSizePtr = (BufferSize - PacketSize);
    }
  } else {
    *BufferSizePtr -= BufferSize;
  }

  /* Send ACK of last packet. */
  ReplyLen = 0;

  SendAckAndGetData (
    Private,
    ServerIpPtr,
    ServerPortPtr,
    ReplyIpPtr,
    OurPortPtr,
    Timeout,
    (UINTN *) &ReplyLen,
    BufferPtr,
    &BlockNum,
    TRUE
    );

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// some literals
//
STATIC UINT8                      Mode[]          = MODE_BINARY;
STATIC UINT8                      BlockSizeOp[]   = OP_BLKSIZE;
STATIC UINT8                      TsizeOp[]       = OP_TFRSIZE;
STATIC UINT8                      OverwriteOp[]   = OP_OVERWRITE;
STATIC UINT8                      BigBlkNumOp[]   = OP_BIGBLKNUM;
STATIC EFI_PXE_BASE_CODE_UDP_PORT TftpRequestPort = TFTP_OPEN_PORT;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return Pointer to value field if option found or NULL if not found.

**/
STATIC
UINT8 *
FindOption (
  UINT8 *OptionPtr,
  INTN  OpLen,
  UINT8 *OackPtr,
  INTN  OackSize
  )
{
  if ((OackSize -= OpLen) <= 0) {
    return NULL;
  }

  do {
    if (!CompareMem (OackPtr, OptionPtr, OpLen)) {
      return OackPtr + OpLen;
    }

    ++OackPtr;
  } while (--OackSize);

  return NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#define BKSZOP      1 // block size
#define TSIZEOP     2 // transfer size
#define OVERWRITEOP 4 // overwrite
#define BIGBLKNUMOP 8 // big block numbers
STATIC
EFI_STATUS
TftpRwReq (
  UINT16                      Req,
  UINT16                      Options,
  PXE_BASECODE_DEVICE         *Private,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *OurPortPtr,
  UINT8                       *FilenamePtr,
  UINTN                       *PacketSizePtr,
  VOID                        *Buffer
  )
{
  union {
    UINT8             Data[514];
    struct Tftpv4Req  ReqStr;
  } *u;

  UINT16  OpFlags;
  INTN    Len;
  INTN    TotalLen;
  UINT8   *Ptr;

  if (*OurPortPtr == 0) {
    OpFlags = EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT | EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT;
  } else {
    OpFlags = EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT;
  }
  //
  // build the basic request - opcode, filename, mode
  //
  u                 = Buffer;
  u->ReqStr.OpCode  = HTONS (Req);
  TotalLen = sizeof (Mode) + sizeof (u->ReqStr.OpCode) + (Len = 1 + AsciiStrLen ((CHAR8 *) FilenamePtr));

  CopyMem (u->ReqStr.FileName, FilenamePtr, Len);
  Ptr = (UINT8 *) (u->ReqStr.FileName + Len);

  CopyMem (Ptr, Mode, sizeof (Mode));
  Ptr += sizeof (Mode);

  if (Options & BKSZOP) {
    CopyMem (Ptr, BlockSizeOp, sizeof (BlockSizeOp));
    UtoA10 (*PacketSizePtr, Ptr + sizeof (BlockSizeOp));

    TotalLen += (Len = 1 + AsciiStrLen ((CHAR8 *) (Ptr + sizeof (BlockSizeOp))) + sizeof (BlockSizeOp));

    Ptr += Len;
  }

  if (Options & TSIZEOP) {
    CopyMem (Ptr, TsizeOp, sizeof (TsizeOp));
    CopyMem (Ptr + sizeof (TsizeOp), "0", 2);
    TotalLen += sizeof (TsizeOp) + 2;
    Ptr += sizeof (TsizeOp) + 2;
  }

  if (Options & OVERWRITEOP) {
    CopyMem (Ptr, OverwriteOp, sizeof (OverwriteOp));
    CopyMem (Ptr + sizeof (OverwriteOp), "1", 2);
    TotalLen += sizeof (OverwriteOp) + 2;
    Ptr += sizeof (OverwriteOp) + 2;
  }

  if (Options & BIGBLKNUMOP) {
    CopyMem (Ptr, BigBlkNumOp, sizeof (BigBlkNumOp));
    CopyMem (Ptr + sizeof (BigBlkNumOp), "8", 2);
    TotalLen += sizeof (BigBlkNumOp) + 2;
    Ptr += sizeof (BigBlkNumOp) + 2;
  }
  //
  // send it
  //
  return UdpWrite (
          Private,
          OpFlags,
          ServerIpPtr,
          ServerPortPtr,
          0,
          0,
          OurPortPtr,
          0,
          0,
          (UINTN *) &TotalLen,
          u
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
TftpRwReqwResp (
  UINT16                      Req,
  UINT16                      Options,
  PXE_BASECODE_DEVICE         *Private,
  VOID                        *HeaderPtr,
  UINTN                       *PacketSizePtr,
  UINTN                       *ReplyLenPtr,
  VOID                        *BufferPtr,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerPortPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *ServerReplyPortPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  *OurPortPtr,
  UINT8                       *FilenamePtr,
  UINT16                      Timeout
  )
{
  EFI_STATUS  Status;
  UINTN       SaveReplyLen;
  INTN        Retries;
  UINT8       Buffer[514];

  SaveReplyLen            = *ReplyLenPtr;
  Retries                 = 3;
  Private->BigBlkNumFlag  = FALSE;
  *OurPortPtr             = 0;
  //
  // generate random
  //
  do {
    if (*OurPortPtr != 0) {
      if (++ *OurPortPtr == 0) {
        *OurPortPtr = PXE_RND_PORT_LOW;
      }
    }
    //
    // send request from our Ip = StationIp
    //
    if ((Status = TftpRwReq (
                    Req,
                    Options,
                    Private,
                    ServerIpPtr,
                    ServerPortPtr,
                    OurPortPtr,
                    FilenamePtr,
                    PacketSizePtr,
                    Buffer
                    )) != EFI_SUCCESS) {
      DEBUG (
        (DEBUG_WARN,
        "\nTftpRwReqwResp()  Exit #1  %xh (%r)",
        Status,
        Status)
        );

      return Status;
    }
    //
    // read reply to our Ip = StationIp
    //
    *ReplyLenPtr = SaveReplyLen;

    Status = TftpUdpRead (
              Private,
              EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT,
              HeaderPtr,
              ReplyLenPtr,
              BufferPtr,
              ServerIpPtr,
              ServerReplyPortPtr,
              0,
              OurPortPtr,
              Timeout
              );
  } while (Status == EFI_TIMEOUT && --Retries);

  if (!Options || Status != EFI_TFTP_ERROR) {
    DEBUG (
      (DEBUG_WARN,
      "\nTftpRwReqwResp()  Exit #2  %xh (%r)",
      Status,
      Status)
      );
    return Status;
  }

  Status = TftpRwReqwResp (
            Req,
            0,
            Private,
            HeaderPtr,
            PacketSizePtr,
            ReplyLenPtr,
            BufferPtr,
            ServerIpPtr,
            ServerPortPtr,
            ServerReplyPortPtr,
            OurPortPtr,
            FilenamePtr,
            Timeout
            );

  DEBUG ((DEBUG_WARN, "\nTftpRwReqwResp()  Exit #3  %xh (%r)", Status, Status));

  return Status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

//
// mtftp listen
// read on mcast ip, cport, from sport, for data packet
// returns success if gets multicast last packet or all up to last block
// if not missing, then finished
//

/**


**/
STATIC
EFI_STATUS
MtftpListen (
  PXE_BASECODE_DEVICE           *Private,
  UINT64                        *BufferSizePtr,
  UINT8                         *BufferPtr,
  EFI_IP_ADDRESS                *ServerIpPtr,
  EFI_PXE_BASE_CODE_MTFTP_INFO  *MtftpInfoPtr,
  UINT64                        *StartBlockPtr,
  UINTN                         *NumMissedPtr,
  UINT16                        TransTimeout,
  UINT16                        ListenTimeout,
  UINT64                        FinalBlock,
  IN BOOLEAN                    DontUseBuffer
  )
{
  EFI_STATUS        Status;
  struct Tftpv4Ack  Header;
  UINT64            Offset;
  UINT64            BlockNum;
  UINT64            LastBlockNum;
  UINT64            BufferSize;
  UINTN             NumMissed;
  UINTN             PacketSize;
  UINTN             SaveReplyLen;
  UINTN             ReplyLen;
  UINT16            Timeout;

  LastBlockNum  = *StartBlockPtr;
  Timeout       = ListenTimeout;
  *NumMissedPtr = 0;
  PacketSize    = 0;
  BufferSize    = *BufferSizePtr;
  ReplyLen      = MAX_TFTP_PKT_SIZE;;

  //
  // receive
  //
  do {
    if ((SaveReplyLen = ReplyLen) > BufferSize) {
      SaveReplyLen = 0;
    }

    /* %%TBD - add big block number support */

    //
    // get data - loop on resends
    //
    do {
      ReplyLen = SaveReplyLen;

      if ((Status = TftpUdpRead (
                      Private,
                      0,
                      &Header,
                      &ReplyLen,
                      BufferPtr,
                      ServerIpPtr,
                      &MtftpInfoPtr->SPort,
                      &MtftpInfoPtr->MCastIp,
                      &MtftpInfoPtr->CPort,
                      Timeout
                      )) != EFI_SUCCESS) {
        return Status;
      }
      //
      // make sure a data packet
      //
      if (Header.OpCode != HTONS (TFTP_DATA)) {
        return EFI_PROTOCOL_ERROR;
      }
    } while ((BlockNum = NTOHS (Header.BlockNum)) == LastBlockNum);

    //
    // make sure still going up
    //
    if (LastBlockNum > BlockNum) {
      return EFI_PROTOCOL_ERROR;
    }

    if (BlockNum - LastBlockNum > 0xFFFFFFFF) {
      return EFI_PROTOCOL_ERROR;
    } else {
      NumMissed = (UINTN) (BlockNum - LastBlockNum - 1);
    }

    LastBlockNum = BlockNum;

    //
    // if first time through, some reinitialization
    //
    if (!PacketSize) {
      *StartBlockPtr  = BlockNum;
      PacketSize      = ReplyLen;
      Timeout         = TransTimeout;
    } else {
      *NumMissedPtr = (UINT16) (*NumMissedPtr + NumMissed);
    }
    //
    // if missed packets, update start block,
    // etc. and move packet to proper place in buffer
    //
    if (NumMissed) {
      *StartBlockPtr = BlockNum;
      if (!DontUseBuffer) {
        Offset = NumMissed * PacketSize;
        CopyMem (BufferPtr + Offset, BufferPtr, ReplyLen);
        BufferPtr += Offset;
        BufferSize -= Offset;
      }
    }

    if (!DontUseBuffer) {
      BufferPtr += ReplyLen;
      BufferSize -= ReplyLen;
    }
  } while (ReplyLen == PacketSize && BlockNum != FinalBlock);

  *BufferSizePtr = BufferSize;

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return // mtftp open session
  @return // return code EFI_SUCCESS
  @return //      and *CompletionStatusPtr = GOTUNI | GOTMULTI means done
  @return //      and *CompletionStatusPtr = GOTMULTI means got first two multicast packets, use listen for rest
  @return //      and *CompletionStatusPtr = 0 means did not get first two multicast packets, use listen for all
  @retval GOTUNI  returns NO_DATA go will go to TFTP session)

**/
STATIC
EFI_STATUS
MtftpOpen (
  PXE_BASECODE_DEVICE                                               * Private,
  UINT64                                                            *BufferSizePtr,
  UINT8                                                             *BufferPtr,
  UINTN                                                             *PacketSizePtr,
  EFI_IP_ADDRESS                                                    * ServerIpPtr,
  UINT8                                                             *FilenamePtr,
  EFI_PXE_BASE_CODE_MTFTP_INFO                                      * MtftpInfoPtr,
  UINT8                                                             *CompletionStatusPtr,
#define GOTUNI 1
#define GOTMULTI 2
  IN BOOLEAN                    DontUseBuffer
  )
{
  EFI_STATUS        Status;
  EFI_IP_ADDRESS    OurReplyIp;
  struct Tftpv4Ack  Header;
  INTN              ReplyLen;
  INTN              Retries;
  UINT8             *BufferPtr2;
  UINT8             TmpBuf[514];

  Retries         = NUM_MTFTP_OPEN_RETRIES;
  BufferPtr2      = BufferPtr;
  *PacketSizePtr  = (UINTN) (MIN (*BufferSizePtr, MAX_TFTP_PKT_SIZE));

  do {
    //
    // send a read request
    //
    *CompletionStatusPtr = 0;

    if ((Status = TftpRwReq (
                    TFTP_RRQ,
                    0,
                    Private,
                    ServerIpPtr,
                    &MtftpInfoPtr->SPort,
                    &MtftpInfoPtr->CPort,
                    FilenamePtr,
                    PacketSizePtr,
                    TmpBuf
                    )) != EFI_SUCCESS) {
      return Status;
    }

    for (;;) {
      //
      // read reply
      //
      ZeroMem (&OurReplyIp, Private->IpLength);
      ReplyLen = *PacketSizePtr;

      if ((Status = TftpUdpRead (
                      Private,
                      EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER,
                      &Header,
                      (UINTN *) &ReplyLen,
                      BufferPtr2,
                      ServerIpPtr,
                      &MtftpInfoPtr->SPort,
                      &OurReplyIp,
                      &MtftpInfoPtr->CPort,
                      MtftpInfoPtr->TransmitTimeout
                      )) == EFI_SUCCESS) {
        //
        // check for first data packet
        //
        if (Header.OpCode != HTONS (TFTP_DATA)) {
          return EFI_PROTOCOL_ERROR;
        }
        //
        // check block num
        //
        if (Header.BlockNum != HTONS (1)) {
          //
          // it's not first
          // if we are not the primary client,
          // we probably got first and now second
          // multicast but no unicast, so
          // *CompletionStatusPtr = GOTMULTI - if this is
          // the second, can just go on to listen
          // starting with 2 as the last block
          // received
          //
          if (Header.BlockNum != HTONS (2)) {
            //
            // not second
            //
            *CompletionStatusPtr = 0;
          }

          return Status;
        }

        //
        // now actual
        //
        *PacketSizePtr = ReplyLen;
        //
        // see if a unicast data packet
        //
        if (!CompareMem (
              &OurReplyIp,
              &Private->EfiBc.Mode->StationIp,
              Private->IpLength
              )) {
          *CompletionStatusPtr |= GOTUNI;
          //
          // it is
          // if already got multicast packet,
          // got em both
          //
          if (*CompletionStatusPtr & GOTMULTI) {
            break;
          }
        } else if (!CompareMem (
                    &OurReplyIp,
                    &MtftpInfoPtr->MCastIp,
                    Private->IpLength
                    )) {
          //
          // otherwise see if a multicast data packet
          //
          *CompletionStatusPtr |= GOTMULTI;
          //
          // it is
          // got first - bump pointer so that if
          // second multi comes along, we're OK
          //
          if (!DontUseBuffer) {
            BufferPtr2 = (UINT8 *) BufferPtr + ReplyLen;
          }
          //
          // if already got unicast packet,
          // got em both
          //
          if (*CompletionStatusPtr & GOTUNI) {
            break;
          }
        } else {
          //
          // else protocol error
          //
          return EFI_PROTOCOL_ERROR;
        }
      } else if (Status == EFI_TIMEOUT) {
        //
        // bad return code - if timed out, retry
        //
        break;
      } else {
        //
        // else just bad - failed MTFTP open
        //
        return Status;
      }
    }
  } while (Status == EFI_TIMEOUT && --Retries);

  if (Status != EFI_SUCCESS) {
    //
    // open failed
    //
    return Status;
  }
  //
  // got em both - go into receive mode
  // routine to read rest of file after a successful open (TFTP or MTFTP)
  // sends ACK and gets next data packet until short packet arrives,
  // then sends ACK and (hopefully) times out
  //
  return LockStepReceive (
          Private,
          (UINT16) ReplyLen,
          BufferSizePtr,
          ReplyLen,
          BufferPtr,
          ServerIpPtr,
          &MtftpInfoPtr->SPort,
          &MtftpInfoPtr->MCastIp,
          &MtftpInfoPtr->CPort,
          1,
          MtftpInfoPtr->TransmitTimeout,
          DontUseBuffer
          );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
MtftpDownload (
  PXE_BASECODE_DEVICE           *Private,
  UINT64                        *BufferSizePtr,
  UINT8                         *BufferPtr,
  EFI_IP_ADDRESS                *ServerIpPtr,
  UINT8                         *FilenamePtr,
  EFI_PXE_BASE_CODE_MTFTP_INFO  *MtftpInfoPtr,
  IN BOOLEAN                    DontUseBuffer
  )
{
  EFI_PXE_BASE_CODE_IP_FILTER Filter;
  EFI_STATUS                  Status;
  UINT64                      StartBlock;
  UINT64                      LastBlock;
  UINT64                      LastStartBlock;
  UINT64                      BufferSize;
  UINTN                       Offset;
  UINTN                       NumMissed;
  UINT16                      TransTimeout;
  UINT16                      ListenTimeout;
  UINT8                       *BufferPtrLocal;

  TransTimeout      = MtftpInfoPtr->TransmitTimeout;
  ListenTimeout     = MtftpInfoPtr->ListenTimeout;
  LastBlock         = 0;
  LastStartBlock    = 0;
  Offset            = 0;

  Filter.Filters    = EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST;
  Filter.IpCnt      = 2;
  CopyMem (&Filter.IpList[0], &Private->EfiBc.Mode->StationIp, sizeof (EFI_IP_ADDRESS));
  CopyMem (&Filter.IpList[1], &MtftpInfoPtr->MCastIp, sizeof (EFI_IP_ADDRESS));

  if ((Status = IpFilter (Private, &Filter)) != EFI_SUCCESS) {
    return Status;
  }

  for (;;) {
    StartBlock  = LastStartBlock;
    BufferSize  = *BufferSizePtr - Offset;

    if (DontUseBuffer) {
    //
    // overwrie the temp buf
    //
      BufferPtrLocal = BufferPtr;
    } else {
      BufferPtrLocal = BufferPtr + Offset;

    }
    //
    // special !!! do not leave enabled in saved version on Source Safe
    // Following code put in in order to create a special version for regression
    // test of MTFTP server to make sure it handles mulitple opens correctly.
    // This code should NOT be enabled normally.
    //
    if (((Status = MtftpListen (
                      Private,
                      &BufferSize,
                      BufferPtrLocal,
                      ServerIpPtr,
                      MtftpInfoPtr,
                      &StartBlock,
                      &NumMissed,
                      TransTimeout,
                      ListenTimeout,
                      LastBlock,
                      DontUseBuffer
                      )) != EFI_SUCCESS) && (Status != EFI_TIMEOUT)) {
        return Status;
        //
        // failed
        //
    }
    //
    // if none were received, start block is not reset
    //
    if (StartBlock == LastStartBlock) {
      UINT8 CompStat;

      //
      // timed out with none received - try MTFTP open
      //
      if ((Status = MtftpOpen (
                      Private,
                      BufferSizePtr,
                      BufferPtr,
                      &Offset,
                      ServerIpPtr,
                      FilenamePtr,
                      MtftpInfoPtr,
                      &CompStat,
                      DontUseBuffer
                      )) != EFI_SUCCESS) {
        //
        // open failure - try TFTP
        //
        return Status;
      }
      //
      // return code EFI_SUCCESS
      // and *CompletionStatusPtr = GOTUNI | GOTMULTI means done
      // and *CompletionStatusPtr = GOTMULTI means got first two multicast packets, use listen for rest
      // and *CompletionStatusPtr = 0 means did not get first two multicast packets, use listen for all
      // (do not get = GOTUNI - returns NO_DATA go will go to TFTP session)
      //
      if (CompStat == (GOTUNI | GOTMULTI)) {
      //
      // finished - got it all
      //
        return Status;
      }

      if (CompStat) {
        //
        // offset is two packet lengths
        //
        Offset <<= 1;
        //
        // last block received
        //
        LastStartBlock = 2;
      } else {
        Offset          = 0;
        LastStartBlock  = 0;
      }

      ListenTimeout = TransTimeout;
      continue;
    }
    //
    // did we get the last block
    //
    if (Status == EFI_SUCCESS) {
      //
      // yes - set the file size if this was first time
      //
      if (!LastBlock) {
        *BufferSizePtr -= BufferSize;
      }
      //
      // if buffer was too small, finished
      //
      if (!DontUseBuffer) {
        return EFI_BUFFER_TOO_SMALL;
      }
      //
      // if we got them all, finished
      //
      if (!NumMissed && StartBlock == LastStartBlock + 1) {
        return Status;
      }
      //
      // did not get them all - set last block
      //
      LastBlock = (UINT16) (StartBlock - 1);
    }
    //
    // compute listen timeout
    //
    ListenTimeout = (UINT16) ((NumMissed > MtftpInfoPtr->ListenTimeout) ? 0 : (MtftpInfoPtr->ListenTimeout - NumMissed));

    //
    // reset
    //
    Offset          = 0;
    LastStartBlock  = 0;
  }
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
TftpInfo (
  PXE_BASECODE_DEVICE         *Private,
  UINT64                      *BufferSizePtr,
  EFI_IP_ADDRESS              *ServerIpPtr,
  EFI_PXE_BASE_CODE_UDP_PORT  SrvPort,
  UINT8                       *FilenamePtr,
  UINTN                       *PacketSizePtr
  )
{
  EFI_PXE_BASE_CODE_UDP_PORT  OurPort;
  EFI_PXE_BASE_CODE_UDP_PORT  ServerReplyPort;
  EFI_STATUS                  Status;
  UINT64                      BlockNum;
  UINTN                       Offset;
  UINTN                       ReplyLen;
  UINT8                       *Ptr;

  union {
    struct Tftpv4Oack OAck2Ptr;
    struct Tftpv4Ack  Ack2Ptr;
    struct Tftpv4Data Datastr;
  } u;

  OurPort         = 0;
  ServerReplyPort = 0;
  ReplyLen        = sizeof (u.Datastr.Data);

  //
  // send a write request with the blocksize option -
  // sets our IP and port - and receive reply - sets his port
  // will retry operation up to 3 times if no response,
  // and will retry without options on an error reply
  //
  if ((Status = TftpRwReqwResp (
                  TFTP_RRQ,
                  /* BIGBLKNUMOP | */BKSZOP | TSIZEOP,
                  Private,
                  &u,
                  PacketSizePtr,
                  &ReplyLen,
                  u.Datastr.Data,
                  ServerIpPtr,
                  &SrvPort,
                  &ServerReplyPort,
                  &OurPort,
                  FilenamePtr,
                  REQ_RESP_TIMEOUT
                  )) != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "\nTftpInfo()  Exit #1"));
    return Status;
  }
  //
  // check for good OACK
  //
  if (u.OAck2Ptr.OpCode == HTONS (TFTP_OACK)) {
    //
    // now parse it for options
    // bigblk#
    //
    Ptr = FindOption (
            BigBlkNumOp,
            sizeof (BigBlkNumOp),
            u.OAck2Ptr.OpAck[0].Option,
            ReplyLen + sizeof (u.Ack2Ptr.BlockNum)
            );

    if (Ptr != NULL) {
      if (AtoU (Ptr) == 8) {
        Private->BigBlkNumFlag = TRUE;
      } else {
        return EFI_PROTOCOL_ERROR;
      }
    }
    //
    // blksize
    //
    Ptr = FindOption (
            BlockSizeOp,
            sizeof (BlockSizeOp),
            u.OAck2Ptr.OpAck[0].Option,
            ReplyLen += sizeof (u.Ack2Ptr.BlockNum)
            );

    *PacketSizePtr = (Ptr) ? AtoU (Ptr) : 512;

    //
    // tsize
    //
    Ptr = FindOption (
            TsizeOp,
            sizeof (TsizeOp),
            u.OAck2Ptr.OpAck[0].Option,
            ReplyLen
            );

    if (Ptr != NULL) {
      *BufferSizePtr = AtoU64 (Ptr);

      //
      // teminate session with error
      //
      SendError (Private, ServerIpPtr, &ServerReplyPort, &OurPort);

      return EFI_SUCCESS;
    }

    Offset    = 0;
    BlockNum  = 0;
  } else {
    //
    // if MTFTP get filesize, return unsupported
    //
    if (SrvPort != TftpRequestPort) {
      SendError (Private, ServerIpPtr, &ServerReplyPort, &OurPort);
      DEBUG ((DEBUG_WARN, "\nTftpInfo()  Exit #3"));
      return EFI_UNSUPPORTED;
    }

    Offset    = ReplyLen;
    //
    // last block received
    //
    BlockNum  = 1;
  }
  //
  // does not support the option - do a download with no buffer
  //
  *BufferSizePtr = 0;

  Status = LockStepReceive (
            Private,
            (UINT16) ReplyLen,
            BufferSizePtr,
            Offset,
            (UINT8 *) &u,
            ServerIpPtr,
            &ServerReplyPort,
            &Private->EfiBc.Mode->StationIp,
            &OurPort,
            BlockNum,
            ACK_TIMEOUT,
            TRUE
            );

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "\nTftpInfo()  LockStepReceive() == %Xh", Status));
  }

  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
TftpDownload (
  PXE_BASECODE_DEVICE         *Private,
  UINT64                      *BufferSizePtr,
  UINT8                       *BufferPtr,
  EFI_IP_ADDRESS              *ServerIpPtr,
  UINT8                       *FilenamePtr,
  UINTN                       *PacketSizePtr,
  EFI_PXE_BASE_CODE_UDP_PORT  SrvPort,
  UINT16                      Req,
  IN BOOLEAN                  DontUseBuffer
  )
{
  EFI_PXE_BASE_CODE_UDP_PORT  OurPort;
  EFI_PXE_BASE_CODE_UDP_PORT  ServerReplyPort;
  EFI_STATUS                  Status;
  UINT64                      Offset;
  UINT64                      BlockNum;
  UINTN                       ReplyLen;
  UINT8                       *Ptr;

  union {
    struct Tftpv4Ack    Ack2Ptr;
    struct Tftpv4Oack   OAck2Ptr;
    struct Tftpv4Data   Data;
    struct Tftpv4Ack8   Ack8Ptr;
    struct Tftpv4Data8  Data8;
  } U;

  OurPort         = 0;
  ServerReplyPort = 0;
  ReplyLen        = (UINTN) ((*BufferSizePtr > 0xFFFF) ? 0xFFFF : *BufferSizePtr);

  //
  // send a read request with the blocksize option - sets our IP and port
  // - and receive reply - sets his port will retry operation up to 3
  // times if no response, and will retry without options on an error
  // reply
  //
  if ((Status = TftpRwReqwResp (
                  Req,
                  /* BIGBLKNUMOP | */BKSZOP,
                  Private,
                  &U,
                  PacketSizePtr,
                  &ReplyLen,
                  BufferPtr,
                  ServerIpPtr,
                  &SrvPort,
                  &ServerReplyPort,
                  &OurPort,
                  FilenamePtr,
                  REQ_RESP_TIMEOUT
                  )) != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "\nTftpDownload()  Exit #1  %xh (%r)", Status, Status));
    return Status;
  }
  //
  // check for OACK
  //
  if (U.OAck2Ptr.OpCode == HTONS (TFTP_OACK)) {
    //
    // get the OACK
    //
    CopyMem (U.Data.Data, BufferPtr, ReplyLen);

    Ptr = FindOption (
            BigBlkNumOp,
            sizeof (BigBlkNumOp),
            U.OAck2Ptr.OpAck[0].Option,
            ReplyLen + sizeof (U.Ack2Ptr.BlockNum)
            );

    if (Ptr != NULL) {
      if (AtoU (Ptr) == 8) {
        Private->BigBlkNumFlag = TRUE;
      } else {
        return EFI_PROTOCOL_ERROR;
      }
    }
    //
    // now parse it for blocksize option
    //
    Ptr = FindOption (
            BlockSizeOp,
            sizeof (BlockSizeOp),
            U.OAck2Ptr.OpAck[0].Option,
            ReplyLen += sizeof (U.Ack2Ptr.BlockNum)
            );

    ReplyLen  = (Ptr != NULL) ? AtoU (Ptr) : 512;

    Offset    = 0;
    //
    // last block received
    //
    BlockNum  = 0;
  } else if (U.Ack2Ptr.OpCode != HTONS (TFTP_DATA) || U.Ack2Ptr.BlockNum != HTONS (1)) {
    //
    // or data
    //
    DEBUG ((DEBUG_WARN, "\nTftpDownload()  Exit #2  %xh (%r)", Status, Status));

    return EFI_PROTOCOL_ERROR;
  } else {
    //
    // got good data packet
    //
    Offset    = ReplyLen;
    //
    // last block received
    //
    BlockNum  = 1;
  }

  if (PacketSizePtr != NULL) {
    *PacketSizePtr = ReplyLen;
  }
  //
  // routine to read rest of file after a successful open (TFTP or MTFTP)
  // sends ACK and gets next data packet until short packet arrives, then sends
  // ACK and (hopefully) times out
  // if first packet has been read, BufferPtr and BufferSize must reflect fact
  //
  Status = LockStepReceive (
            Private,
            ReplyLen,
            BufferSizePtr,
            Offset,
            BufferPtr,
            ServerIpPtr,
            &ServerReplyPort,
            &Private->EfiBc.Mode->StationIp,
            &OurPort,
            BlockNum,
            ACK_TIMEOUT,
            DontUseBuffer
            );

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_WARN, "\nTftpDownload()  Exit #3  %xh (%r)", Status, Status));

    if (Status == EFI_BUFFER_TOO_SMALL) {
      Status = TftpInfo (
                Private,
                BufferSizePtr,
                ServerIpPtr,
                SrvPort,
                FilenamePtr,
                PacketSizePtr
                );

      if (!EFI_ERROR (Status)) {
        Status = EFI_BUFFER_TOO_SMALL;
      }
    }
  }

  return Status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**


**/
STATIC
EFI_STATUS
TftpUpload (
  PXE_BASECODE_DEVICE *Private,
  UINT64              *BufferSizePtr,
  VOID                *BufferPtr,
  EFI_IP_ADDRESS      *ServerIpPtr,
  UINT8               *FilenamePtr,
  UINTN               *PacketSizePtr,
  BOOLEAN             Overwrite
  )
{
  struct Tftpv4Ack            Header;
  EFI_PXE_BASE_CODE_UDP_PORT  OurPort;
  EFI_PXE_BASE_CODE_UDP_PORT  ServerReplyPort;
  EFI_STATUS                  Status;
  UINT64                      BlockNum;
  UINT64                      TransferSize;
  UINTN                       ReplyLen;
  UINTN                       TransferLen;
  UINT16                      Options;
  UINT8                       *Ptr;

  union {
    struct Tftpv4Oack OAck2Ptr;
    struct Tftpv4Ack  Ack2Ptr;
    struct Tftpv4Data Datastr;
  } u;

  OurPort         = 0;
  ServerReplyPort = 0;
  TransferSize    = *BufferSizePtr;
  ReplyLen        = sizeof (u.Datastr.Data);
  Options         = (UINT16) ((Overwrite) ? OVERWRITEOP | BKSZOP : BKSZOP);

  //
  // send a write request with the blocksize option - sets our IP and port -
  // and receive reply - sets his port
  // will retry operation up to 3 times if no response, and will retry without
  // options on an error reply
  //
  if ((Status = TftpRwReqwResp (
                  TFTP_WRQ,
                  Options,
                  Private,
                  &u,
                  PacketSizePtr,
                  &ReplyLen,
                  u.Datastr.Data,
                  ServerIpPtr,
                  &TftpRequestPort,
                  &ServerReplyPort,
                  &OurPort,
                  FilenamePtr,
                  REQ_RESP_TIMEOUT
                  )) != EFI_SUCCESS) {
    return Status;
  }
  //
  // check for OACK
  //
  if (u.OAck2Ptr.OpCode == HTONS (TFTP_OACK)) {
    //
    // parse it for blocksize option
    //
    Ptr = FindOption (
            BlockSizeOp,
            sizeof (BlockSizeOp),
            u.OAck2Ptr.OpAck[0].Option,
            ReplyLen += sizeof (u.Ack2Ptr.BlockNum)
            );
    *PacketSizePtr = (Ptr) ? AtoU (Ptr) : 512;
  }
  //
  // or ACK
  //
  else if (u.Ack2Ptr.OpCode == HTONS (TFTP_ACK)) {
    //
    // option was not supported
    //
    *PacketSizePtr = 512;
  } else {
    return EFI_PROTOCOL_ERROR;
  }
  //
  // loop
  //
  Header.OpCode   = HTONS (TFTP_DATA);
  BlockNum        = 1;
  Header.BlockNum = HTONS (1);

  do {
    UINTN HeaderSize;
    INTN  Retries;

    Retries     = NUM_ACK_RETRIES;
    HeaderSize  = sizeof (Header);
    TransferLen = (UINTN) (MIN (*PacketSizePtr, TransferSize));

    //
    // write a data packet and get an ack
    //
    do {
      //
      // write
      //
      if ((Status = UdpWrite (
                      Private,
                      EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT,
                      ServerIpPtr,
                      &ServerReplyPort,
                      0,
                      0,
                      &OurPort,
                      &HeaderSize,
                      &Header,
                      &TransferLen,
                      BufferPtr
                      )) != EFI_SUCCESS) {
        return Status;
      }
      //
      // read reply
      //
      ReplyLen = sizeof (u.Datastr.Data);

      if ((Status = TftpUdpRead (
                      Private,
                      0,
                      &u,
                      &ReplyLen,
                      u.Datastr.Data,
                      ServerIpPtr,
                      &ServerReplyPort,
                      0,
                      &OurPort,
                      ACK_TIMEOUT
                      )) == EFI_SUCCESS) {
        //
        // check for ACK for this data packet
        //
        if (u.Ack2Ptr.OpCode != HTONS (TFTP_ACK)) {
          return EFI_PROTOCOL_ERROR;
        }

        if (u.Ack2Ptr.BlockNum != Header.BlockNum) {
          //
          // not for this packet - continue
          //
          Status = EFI_TIMEOUT;
        }
      }
    } while (Status == EFI_TIMEOUT && --Retries);

    if (Status != EFI_SUCCESS) {
      return Status;
    }

    BufferPtr = (VOID *) ((UINT8 *) (BufferPtr) + TransferLen);
    TransferSize -= TransferLen;
    ++BlockNum;
    Header.BlockNum = HTONS ((UINT16) BlockNum);
  } while (TransferLen == *PacketSizePtr);

  return EFI_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return *  EFI_INVALID_PARAMETER
  @return *  EFI_OUT_OF_RESOURCES
  @return *  EFI_BAD_BUFFER_SIZE
  @return *  Status is also returned from IpFilter(), TftpInfo(), MtftpDownload(),
  @return *  TftpDownload() and TftpUpload().

**/
EFI_STATUS
PxeBcMtftp (
  PXE_BASECODE_DEVICE               *Private,
  IN EFI_PXE_BASE_CODE_TFTP_OPCODE  Operation,
  UINT64                            *BufferSizePtr,
  VOID                              *BufferPtr,
  EFI_IP_ADDRESS                    *ServerIpPtr,
  UINT8                             *FilenamePtr,
  UINTN                             *PacketSizePtr,
  IN EFI_PXE_BASE_CODE_MTFTP_INFO   *MtftpInfoPtr, OPTIONAL
  IN BOOLEAN                        Overwrite,
  IN BOOLEAN                        DontUseBuffer
  )
{
  EFI_PXE_BASE_CODE_IP_FILTER Filter;
  EFI_STATUS                  StatCode;
  EFI_STATUS                  Status;
  UINT64                      BufferSizeLocal;
  UINTN                       PacketSize;
  UINT8                       *BufferPtrLocal;

  Filter.Filters  = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP;
  Filter.IpCnt    = 0;
  Filter.reserved = 0;

  /* No error has occurred, yet. */
  Private->EfiBc.Mode->TftpErrorReceived = FALSE;

  /* We must at least have an MTFTP server IP address and
   * a pointer to the buffer size.
   */
  if (!ServerIpPtr || !BufferSizePtr) {
    DEBUG ((DEBUG_WARN, "\nPxeBcMtftp()  Exit #1"));

    return EFI_INVALID_PARAMETER;
  }

  Private->Function = EFI_PXE_BASE_CODE_FUNCTION_MTFTP;

  //
  // make sure filter set to unicast at start
  //
  if ((StatCode = IpFilter (Private, &Filter)) != EFI_SUCCESS) {
    DEBUG (
      (DEBUG_NET,
      "\nPxeBcMtftp()  Exit  IpFilter() == %Xh",
      StatCode)
      );

    return StatCode;
  }
  //
  // set unset parms to default values
  //
  if (!PacketSizePtr) {
    *(PacketSizePtr = &PacketSize) = MAX_TFTP_PKT_SIZE;
  }

  if ((*PacketSizePtr > *BufferSizePtr) &&
    (Operation != EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE) &&
    (Operation != EFI_PXE_BASE_CODE_MTFTP_GET_FILE_SIZE)) {
    *PacketSizePtr = MAX ((UINTN) *BufferSizePtr, MIN_TFTP_PKT_SIZE);
  }

  if (*PacketSizePtr < MIN_TFTP_PKT_SIZE) {
    *PacketSizePtr = MIN_TFTP_PKT_SIZE;
    return EFI_INVALID_PARAMETER;
  }

  if (*PacketSizePtr > BUFFER_ALLOCATE_SIZE) {
    *PacketSizePtr = BUFFER_ALLOCATE_SIZE;
  }

  if (*PacketSizePtr > MAX_TFTP_PKT_SIZE) {
    *PacketSizePtr = MAX_TFTP_PKT_SIZE;
  }

  if (Operation == EFI_PXE_BASE_CODE_TFTP_GET_FILE_SIZE) {
    StatCode = TftpInfo (
                Private,
                BufferSizePtr,
                ServerIpPtr,
                TftpRequestPort,
                FilenamePtr,
                PacketSizePtr
                );

    if (StatCode != EFI_SUCCESS) {
      DEBUG (
        (DEBUG_WARN,
        "\nPxeBcMtftp()  Exit  TftpInfo() == %Xh",
        StatCode)
        );
    }

    return StatCode;
  }

  if (Operation == EFI_PXE_BASE_CODE_MTFTP_GET_FILE_SIZE) {
    if (!MtftpInfoPtr || !MtftpInfoPtr->SPort) {
      DEBUG ((DEBUG_WARN, "\nPxeBcMtftp()  Exit #2"));
      return EFI_INVALID_PARAMETER;
    } else {
      StatCode = TftpInfo (
                  Private,
                  BufferSizePtr,
                  ServerIpPtr,
                  MtftpInfoPtr->SPort,
                  FilenamePtr,
                  PacketSizePtr
                  );

      gBS->Stall (10000);

      if (StatCode != EFI_SUCCESS) {
        DEBUG (
          (DEBUG_WARN,
          "\nPxeBcMtftp()  Exit  TftpInfo() == %Xh",
          StatCode)
          );
      }

      return StatCode;
    }
  }

  if (!BufferPtr && !DontUseBuffer) {
    //
    // if dontusebuffer is false and no buffer???
    //
    DEBUG ((DEBUG_WARN, "\nPxeBcMtftp()  Exit #3"));
    //
    // DontUseBuffer can be true only for read_file operation
    //
    return EFI_INVALID_PARAMETER;
  }

  if (DontUseBuffer) {
    Status = gBS->AllocatePool (
                    EfiBootServicesData,
                    BUFFER_ALLOCATE_SIZE,
                    (VOID **) &BufferPtrLocal
                    );

    if (EFI_ERROR (Status) || BufferPtrLocal == NULL) {
      DEBUG ((DEBUG_NET, "\nPxeBcMtftp()  Exit #4"));
      return EFI_OUT_OF_RESOURCES;
    }

    BufferSizeLocal = BUFFER_ALLOCATE_SIZE;
  } else {
    if (!*BufferSizePtr && Operation != EFI_PXE_BASE_CODE_TFTP_WRITE_FILE) {
      DEBUG ((DEBUG_WARN, "\nPxeBcMtftp()  Exit #5"));
      return EFI_BAD_BUFFER_SIZE;
    }

    BufferPtrLocal  = BufferPtr;
    BufferSizeLocal = *BufferSizePtr;
  }

  switch (Operation) {
  case EFI_PXE_BASE_CODE_MTFTP_READ_FILE:
    if (FilenamePtr == NULL ||
        MtftpInfoPtr == NULL ||
        MtftpInfoPtr->MCastIp.Addr[0] == 0 ||
        MtftpInfoPtr->SPort == 0 ||
        MtftpInfoPtr->CPort == 0 ||
        MtftpInfoPtr->ListenTimeout == 0 ||
        MtftpInfoPtr->TransmitTimeout == 0
        ) {
      StatCode = EFI_INVALID_PARAMETER;
      break;
    }
    //
    // try MTFTP - if fails, drop into TFTP read
    //
    if ((StatCode = MtftpDownload (
                      Private,
                      &BufferSizeLocal,
                      BufferPtrLocal,
                      ServerIpPtr,
                      FilenamePtr,
                      MtftpInfoPtr,
                      DontUseBuffer
                      )) == EFI_SUCCESS || StatCode == EFI_BUFFER_TOO_SMALL) {
      if (BufferSizePtr /* %% !DontUseBuffer */ ) {
        *BufferSizePtr = BufferSizeLocal;
      }

      break;
    }
    //
    // go back to unicast
    //
    if ((StatCode = IpFilter (Private, &Filter)) != EFI_SUCCESS) {
      break;
    }

  /* fall thru */
  case EFI_PXE_BASE_CODE_TFTP_READ_FILE:
    if (FilenamePtr == NULL) {
      StatCode = EFI_INVALID_PARAMETER;
      break;
    }

    StatCode = TftpDownload (
                Private,
                &BufferSizeLocal,
                BufferPtrLocal,
                ServerIpPtr,
                FilenamePtr,
                PacketSizePtr,
                TftpRequestPort,
                TFTP_RRQ,
                DontUseBuffer
                );

    if (StatCode == EFI_SUCCESS || StatCode == EFI_BUFFER_TOO_SMALL) {
      if (BufferSizePtr /* !DontUseBuffer */ ) {
        *BufferSizePtr = BufferSizeLocal;
      }
    }

    break;

  case EFI_PXE_BASE_CODE_TFTP_WRITE_FILE:
    if (FilenamePtr == NULL || DontUseBuffer) {
      //
      // not a valid option
      //
      StatCode = EFI_INVALID_PARAMETER;
      break;
    }

    StatCode = TftpUpload (
                Private,
                BufferSizePtr,
                BufferPtr,
                ServerIpPtr,
                FilenamePtr,
                PacketSizePtr,
                Overwrite
                );

    if (StatCode != EFI_SUCCESS) {
      DEBUG (
        (DEBUG_WARN,
        "\nPxeBcMtftp()  Exit #6  %xh (%r)",
        StatCode,
        StatCode)
        );
    }

    return StatCode;

  case EFI_PXE_BASE_CODE_TFTP_READ_DIRECTORY:
    if (FilenamePtr == NULL || DontUseBuffer) {
      //
      // not a valid option
      //
      StatCode = EFI_INVALID_PARAMETER;
      break;
    }

    StatCode = TftpDownload (
                Private,
                BufferSizePtr,
                BufferPtr,
                ServerIpPtr,
                FilenamePtr,
                PacketSizePtr,
                TftpRequestPort,
                TFTP_DIR,
                DontUseBuffer
                );

    if (StatCode != EFI_SUCCESS) {
      DEBUG (
        (DEBUG_WARN,
        "\nPxeBcMtftp()  Exit #7  %xh (%r)",
        StatCode,
        StatCode)
        );
    }

    return StatCode;

  case EFI_PXE_BASE_CODE_MTFTP_READ_DIRECTORY:
    if (DontUseBuffer) {
      StatCode = EFI_INVALID_PARAMETER;
      break;
    }

    if (MtftpInfoPtr == NULL || !MtftpInfoPtr->SPort) {
      DEBUG (
        (DEBUG_WARN,
        "\nPxeBcMtftp()  Exit #9  %xh (%r)",
        EFI_INVALID_PARAMETER,
        EFI_INVALID_PARAMETER)
        );

      return EFI_INVALID_PARAMETER;
    }

    StatCode = TftpDownload (
                Private,
                BufferSizePtr,
                BufferPtr,
                ServerIpPtr,
                (UINT8 *) "/",
                PacketSizePtr,
                MtftpInfoPtr->SPort,
                TFTP_DIR,
                DontUseBuffer
                );

    break;

  default:
    StatCode = EFI_INVALID_PARAMETER;
  }

  if (DontUseBuffer) {
    gBS->FreePool (BufferPtrLocal);
  }

  if (StatCode != EFI_SUCCESS) {
    DEBUG (
      (DEBUG_WARN,
      "\nPxeBcMtftp()  Exit #8  %xh (%r)",
      StatCode,
      StatCode)
      );
  }

  gBS->Stall (10000);

  return StatCode;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/**

  @return *  EFI_INVALID_PARAMETER
  @return *  Status is also returned from PxeBcMtftp();

**/
EFI_STATUS
EFIAPI
BcMtftp (
  IN EFI_PXE_BASE_CODE_PROTOCOL       * This,
  IN EFI_PXE_BASE_CODE_TFTP_OPCODE    Operation,
  IN OUT VOID                         *BufferPtr,
  IN BOOLEAN                          Overwrite,
  IN OUT UINT64                       *BufferSizePtr,
  IN UINTN                            *BlockSizePtr OPTIONAL,
  IN EFI_IP_ADDRESS                   * ServerIpPtr,
  IN UINT8                            *FilenamePtr,
  IN EFI_PXE_BASE_CODE_MTFTP_INFO     * MtftpInfoPtr OPTIONAL,
  IN BOOLEAN                          DontUseBuffer
  )
{
  EFI_PXE_BASE_CODE_IP_FILTER Filter;
  EFI_STATUS                  StatCode;
  PXE_BASECODE_DEVICE         *Private;

  //
  // Lock the instance data and make sure started
  //
  StatCode = EFI_SUCCESS;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "BC *This pointer == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  Private = CR (This, PXE_BASECODE_DEVICE, EfiBc, PXE_BASECODE_DEVICE_SIGNATURE);

  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "PXE_BASECODE_DEVICE poiner == NULL"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_INADDR_UNICAST (ServerIpPtr)) {
      //
      // The station IP is not a unicast address.
      //
      return EFI_INVALID_PARAMETER;
  }

  EfiAcquireLock (&Private->Lock);

  if (This->Mode == NULL || !This->Mode->Started) {
    DEBUG ((DEBUG_ERROR, "BC was not started."));
    EfiReleaseLock (&Private->Lock);
    return EFI_NOT_STARTED;
  }
  //
  // Issue BC command
  //
  Filter.Filters  = EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP;
  Filter.IpCnt    = 0;
  Filter.reserved = 0;

  DEBUG ((DEBUG_WARN, "\nBcMtftp()  Op=%d  Buf=%Xh", Operation, BufferPtr));

  StatCode = PxeBcMtftp (
              Private,
              Operation,
              BufferSizePtr,
              BufferPtr,
              ServerIpPtr,
              FilenamePtr,
              BlockSizePtr,
              MtftpInfoPtr,
              Overwrite,
              DontUseBuffer
              );

  //
  // restore to unicast
  //
  IpFilter (Private, &Filter);

  //
  // Unlock the instance data
  //
  EfiReleaseLock (&Private->Lock);
  return StatCode;
}

/* eof - PxeBcMtftp.c */
