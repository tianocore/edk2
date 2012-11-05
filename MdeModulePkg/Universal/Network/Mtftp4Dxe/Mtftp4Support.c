/** @file
  Support routines for Mtftp.
  
Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Mtftp4Impl.h"


/**
  Allocate a MTFTP4 block range, then init it to the range of [Start, End]

  @param  Start                 The start block number
  @param  End                   The last block number in the range

  @return Pointer to the created block range, NULL if failed to allocate memory.

**/
MTFTP4_BLOCK_RANGE *
Mtftp4AllocateRange (
  IN UINT16                 Start,
  IN UINT16                 End
  )
{
  MTFTP4_BLOCK_RANGE        *Range;

  Range = AllocateZeroPool (sizeof (MTFTP4_BLOCK_RANGE));

  if (Range == NULL) {
    return NULL;
  }

  InitializeListHead (&Range->Link);
  Range->Start  = Start;
  Range->End    = End;
  Range->Bound  = End;

  return Range;
}


/**
  Initialize the block range for either RRQ or WRQ. 
  
  RRQ and WRQ have different requirements for Start and End. 
  For example, during start up, WRQ initializes its whole valid block range 
  to [0, 0xffff]. This is bacause the server will send us a ACK0 to inform us 
  to start the upload. When the client received ACK0, it will remove 0 from the 
  range, get the next block number, which is 1, then upload the BLOCK1. For RRQ
  without option negotiation, the server will directly send us the BLOCK1 in 
  response to the client's RRQ. When received BLOCK1, the client will remove 
  it from the block range and send an ACK. It also works if there is option 
  negotiation.

  @param  Head                  The block range head to initialize
  @param  Start                 The Start block number.
  @param  End                   The last block number.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for initial block range
  @retval EFI_SUCCESS           The initial block range is created.

**/
EFI_STATUS
Mtftp4InitBlockRange (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Start,
  IN UINT16                 End
  )
{
  MTFTP4_BLOCK_RANGE        *Range;

  Range = Mtftp4AllocateRange (Start, End);

  if (Range == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InsertTailList (Head, &Range->Link);
  return EFI_SUCCESS;
}


/**
  Get the first valid block number on the range list.

  @param  Head                  The block range head

  @return The first valid block number, -1 if the block range is empty. 

**/
INTN
Mtftp4GetNextBlockNum (
  IN LIST_ENTRY             *Head
  )
{
  MTFTP4_BLOCK_RANGE  *Range;

  if (IsListEmpty (Head)) {
    return -1;
  }

  Range = NET_LIST_HEAD (Head, MTFTP4_BLOCK_RANGE, Link);
  return Range->Start;
}


/**
  Set the last block number of the block range list. 
  
  It will remove all the blocks after the Last. MTFTP initialize the block range
  to the maximum possible range, such as [0, 0xffff] for WRQ. When it gets the 
  last block number, it will call this function to set the last block number.

  @param  Head                  The block range list
  @param  Last                  The last block number

**/
VOID
Mtftp4SetLastBlockNum (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Last
  )
{
  MTFTP4_BLOCK_RANGE        *Range;

  //
  // Iterate from the tail to head to remove the block number
  // after the last.
  //
  while (!IsListEmpty (Head)) {
    Range = NET_LIST_TAIL (Head, MTFTP4_BLOCK_RANGE, Link);

    if (Range->Start > Last) {
      RemoveEntryList (&Range->Link);
      FreePool (Range);
      continue;
    }

    if (Range->End > Last) {
      Range->End = Last;
    }

    return ;
  }
}


/**
  Remove the block number from the block range list.

  @param  Head                  The block range list to remove from
  @param  Num                   The block number to remove
  @param  Completed             Whether Num is the last block number
  @param  TotalBlock            The continuous block number in all 

  @retval EFI_NOT_FOUND         The block number isn't in the block range list
  @retval EFI_SUCCESS           The block number has been removed from the list
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource

**/
EFI_STATUS
Mtftp4RemoveBlockNum (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Num,
  IN BOOLEAN                Completed,
  OUT UINT64                *TotalBlock
  )
{
  MTFTP4_BLOCK_RANGE        *Range;
  MTFTP4_BLOCK_RANGE        *NewRange;
  LIST_ENTRY                *Entry;

  NET_LIST_FOR_EACH (Entry, Head) {

    //
    // Each block represents a hole [Start, End] in the file,
    // skip to the first range with End >= Num
    //
    Range = NET_LIST_USER_STRUCT (Entry, MTFTP4_BLOCK_RANGE, Link);

    if (Range->End < Num) {
      continue;
    }

    //
    // There are three different cases for Start
    // 1. (Start > Num) && (End >= Num):
    //    because all the holes before this one has the condition of
    //    End < Num, so this block number has been removed.
    //
    // 2. (Start == Num) && (End >= Num):
    //    Need to increase the Start by one, and if End == Num, this
    //    hole has been removed completely, remove it.
    //
    // 3. (Start < Num) && (End >= Num):
    //    if End == Num, only need to decrease the End by one because
    //    we have (Start < Num) && (Num == End), so (Start <= End - 1).
    //    if (End > Num), the hold is splited into two holes, with
    //    [Start, Num - 1] and [Num + 1, End].
    //
    if (Range->Start > Num) {
      return EFI_NOT_FOUND;

    } else if (Range->Start == Num) {
      Range->Start++;

      //
      // Note that: RFC 1350 does not mention block counter roll-over, 
      // but several TFTP hosts implement the roll-over be able to accept 
      // transfers of unlimited size. There is no consensus, however, whether 
      // the counter should wrap around to zero or to one. Many implementations 
      // wrap to zero, because this is the simplest to implement. Here we choose 
      // this solution.
      //
	  *TotalBlock  = Num;
	  
      if (Range->Round > 0) {
	    *TotalBlock += Range->Bound +  MultU64x32 ((UINTN) (Range->Round -1), (UINT32) (Range->Bound + 1)) + 1;
	  }

      if (Range->Start > Range->Bound) {
	  	  Range->Start = 0;
		  Range->Round ++;
      }

      if ((Range->Start > Range->End) || Completed) {
        RemoveEntryList (&Range->Link);
        FreePool (Range);
      }

      return EFI_SUCCESS;

    } else {
      if (Range->End == Num) {
        Range->End--;
      } else {
        NewRange = Mtftp4AllocateRange ((UINT16) (Num + 1), (UINT16) Range->End);

        if (NewRange == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        Range->End = Num - 1;
        NetListInsertAfter (&Range->Link, &NewRange->Link);
      }

      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Build then transmit the request packet for the MTFTP session.

  @param  Instance              The Mtftp session

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the request
  @retval EFI_SUCCESS           The request is built and sent
  @retval Others                Failed to transmit the packet.

**/
EFI_STATUS
Mtftp4SendRequest (
  IN MTFTP4_PROTOCOL        *Instance
  )
{
  EFI_MTFTP4_PACKET         *Packet;
  EFI_MTFTP4_OPTION         *Options;
  EFI_MTFTP4_TOKEN          *Token;
  NET_BUF                   *Nbuf;
  UINT8                     *Mode;
  UINT8                     *Cur;
  UINT32                    Len;
  UINTN                     Index;
  UINT32                    Len1;
  UINT32                    Len2;

  Token   = Instance->Token;
  Options = Token->OptionList;
  Mode    = Instance->Token->ModeStr;

  if (Mode == NULL) {
    Mode = (UINT8 *) "octet";
  }

  //
  // Compute the packet length
  //
  Len1 = (UINT32) AsciiStrLen ((CHAR8 *) Token->Filename);
  Len2 = (UINT32) AsciiStrLen ((CHAR8 *) Mode);
  Len  = (Len1 + Len2 + 4);

  for (Index = 0; Index < Token->OptionCount; Index++) {
    Len1 = (UINT32) AsciiStrLen ((CHAR8 *) Options[Index].OptionStr);
    Len2 = (UINT32) AsciiStrLen ((CHAR8 *) Options[Index].ValueStr);
    Len += Len1 + Len2 + 2;
  }

  //
  // Allocate a packet then copy the data over
  //
  if ((Nbuf = NetbufAlloc (Len)) == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Packet         = (EFI_MTFTP4_PACKET *) NetbufAllocSpace (Nbuf, Len, FALSE);
  ASSERT (Packet != NULL);

  Packet->OpCode = HTONS (Instance->Operation);
  Cur            = Packet->Rrq.Filename;
  Cur            = (UINT8 *) AsciiStrCpy ((CHAR8 *) Cur, (CHAR8 *) Token->Filename);
  Cur           += AsciiStrLen ((CHAR8 *) Token->Filename) + 1;
  Cur            = (UINT8 *) AsciiStrCpy ((CHAR8 *) Cur, (CHAR8 *) Mode);
  Cur           += AsciiStrLen ((CHAR8 *) Mode) + 1;

  for (Index = 0; Index < Token->OptionCount; ++Index) {
    Cur  = (UINT8 *) AsciiStrCpy ((CHAR8 *) Cur, (CHAR8 *) Options[Index].OptionStr);
    Cur += AsciiStrLen ((CHAR8 *) Options[Index].OptionStr) + 1;

    Cur  = (UINT8 *) AsciiStrCpy ((CHAR8 *) Cur, (CHAR8 *) Options[Index].ValueStr);
    Cur += AsciiStrLen ((CHAR8 *) (CHAR8 *) Options[Index].ValueStr) + 1;
  }

  return Mtftp4SendPacket (Instance, Nbuf);
}


/**
  Build then send an error message.

  @param  Instance              The MTFTP session
  @param  ErrCode               The error code  
  @param  ErrInfo               The error message

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for the error packet
  @retval EFI_SUCCESS           The error packet is transmitted.
  @retval Others                Failed to transmit the packet.

**/
EFI_STATUS
Mtftp4SendError (
  IN MTFTP4_PROTOCOL        *Instance,
  IN UINT16                 ErrCode,
  IN UINT8                  *ErrInfo
  )
{
  NET_BUF                   *Packet;
  EFI_MTFTP4_PACKET         *TftpError;
  UINT32                    Len;

  Len     = (UINT32) (AsciiStrLen ((CHAR8 *) ErrInfo) + sizeof (EFI_MTFTP4_ERROR_HEADER));
  Packet  = NetbufAlloc (Len);
  if (Packet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TftpError         = (EFI_MTFTP4_PACKET *) NetbufAllocSpace (Packet, Len, FALSE);
  ASSERT (TftpError != NULL);

  TftpError->OpCode = HTONS (EFI_MTFTP4_OPCODE_ERROR);
  TftpError->Error.ErrorCode = HTONS (ErrCode);

  AsciiStrCpy ((CHAR8 *) TftpError->Error.ErrorMessage, (CHAR8 *) ErrInfo);

  return Mtftp4SendPacket (Instance, Packet);
}


/**
  The callback function called when the packet is transmitted.
  
  It simply frees the packet.

  @param  Packet                The transmitted (or failed to) packet
  @param  EndPoint              The local and remote UDP access point
  @param  IoStatus              The result of the transmission
  @param  Context               Opaque parameter to the callback

**/
VOID
EFIAPI
Mtftp4OnPacketSent (
  IN NET_BUF                   *Packet,
  IN UDP_END_POINT             *EndPoint,
  IN EFI_STATUS                IoStatus,
  IN VOID                      *Context
  )
{
  NetbufFree (Packet);
}


/**
  Set the timeout for the instance. User a longer time for passive instances.

  @param  Instance              The Mtftp session to set time out

**/
VOID
Mtftp4SetTimeout (
  IN OUT MTFTP4_PROTOCOL        *Instance
  )
{
  if (Instance->Master) {
    Instance->PacketToLive = Instance->Timeout;
  } else {
    Instance->PacketToLive = Instance->Timeout * 2;
  }
}


/**
  Send the packet for the instance. 
  
  It will first save a reference to the packet for later retransmission. 
  Then determine the destination port, listen port for requests, and connected 
  port for others. At last, send the packet out.

  @param  Instance              The Mtftp instance
  @param  Packet                The packet to send

  @retval EFI_SUCCESS           The packet is sent out
  @retval Others                Failed to transmit the packet.

**/
EFI_STATUS
Mtftp4SendPacket (
  IN OUT MTFTP4_PROTOCOL        *Instance,
  IN OUT NET_BUF                *Packet
  )
{
  UDP_END_POINT             UdpPoint;
  EFI_STATUS                Status;
  UINT16                    OpCode;
  UINT8                     *Buffer;

  //
  // Save the packet for retransmission
  //
  if (Instance->LastPacket != NULL) {
    NetbufFree (Instance->LastPacket);
  }

  Instance->LastPacket        = Packet;

  Instance->CurRetry          = 0;
  Mtftp4SetTimeout (Instance);

  ZeroMem (&UdpPoint, sizeof (UdpPoint));
  UdpPoint.RemoteAddr.Addr[0] = Instance->ServerIp;

  //
  // Send the requests to the listening port, other packets
  // to the connected port
  //
  Buffer = NetbufGetByte (Packet, 0, NULL);
  ASSERT (Buffer != NULL);
  OpCode = NTOHS (*(UINT16 *)Buffer);

  if ((OpCode == EFI_MTFTP4_OPCODE_RRQ) || 
      (OpCode == EFI_MTFTP4_OPCODE_DIR) ||
      (OpCode == EFI_MTFTP4_OPCODE_WRQ)) {
    UdpPoint.RemotePort = Instance->ListeningPort;
  } else {
    UdpPoint.RemotePort = Instance->ConnectedPort;
  }

  NET_GET_REF (Packet);

  Status = UdpIoSendDatagram (
             Instance->UnicastPort,
             Packet,
             &UdpPoint,
             NULL,
             Mtftp4OnPacketSent,
             Instance
             );

  if (EFI_ERROR (Status)) {
    NET_PUT_REF (Packet);
  }

  return Status;
}


/**
  Retransmit the last packet for the instance.

  @param  Instance              The Mtftp instance

  @retval EFI_SUCCESS           The last packet is retransmitted.
  @retval Others                Failed to retransmit.

**/
EFI_STATUS
Mtftp4Retransmit (
  IN MTFTP4_PROTOCOL        *Instance
  )
{
  UDP_END_POINT             UdpPoint;
  EFI_STATUS                Status;
  UINT16                    OpCode;
  UINT8                     *Buffer;

  ASSERT (Instance->LastPacket != NULL);

  ZeroMem (&UdpPoint, sizeof (UdpPoint));
  UdpPoint.RemoteAddr.Addr[0] = Instance->ServerIp;

  //
  // Set the requests to the listening port, other packets to the connected port
  //
  Buffer = NetbufGetByte (Instance->LastPacket, 0, NULL);
  ASSERT (Buffer != NULL);
  OpCode = NTOHS (*(UINT16 *) Buffer);

  if ((OpCode == EFI_MTFTP4_OPCODE_RRQ) || (OpCode == EFI_MTFTP4_OPCODE_DIR) ||
      (OpCode == EFI_MTFTP4_OPCODE_WRQ)) {
    UdpPoint.RemotePort = Instance->ListeningPort;
  } else {
    UdpPoint.RemotePort = Instance->ConnectedPort;
  }

  NET_GET_REF (Instance->LastPacket);

  Status = UdpIoSendDatagram (
             Instance->UnicastPort,
             Instance->LastPacket,
             &UdpPoint,
             NULL,
             Mtftp4OnPacketSent,
             Instance
             );

  if (EFI_ERROR (Status)) {
    NET_PUT_REF (Instance->LastPacket);
  }

  return Status;
}


/**
  The timer ticking function for the Mtftp service instance.

  @param  Event                 The ticking event
  @param  Context               The Mtftp service instance

**/
VOID
EFIAPI
Mtftp4OnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  MTFTP4_SERVICE            *MtftpSb;
  LIST_ENTRY                *Entry;
  LIST_ENTRY                *Next;
  MTFTP4_PROTOCOL           *Instance;
  EFI_MTFTP4_TOKEN          *Token;

  MtftpSb = (MTFTP4_SERVICE *) Context;

  //
  // Iterate through all the children of the Mtftp service instance. Time
  // out the packet. If maximum retries reached, clean the session up.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, Next, &MtftpSb->Children) {
    Instance = NET_LIST_USER_STRUCT (Entry, MTFTP4_PROTOCOL, Link);

    if ((Instance->PacketToLive == 0) || (--Instance->PacketToLive > 0)) {
      continue;
    }

    //
    // Call the user's time out handler
    //
    Token = Instance->Token;

    if ((Token->TimeoutCallback != NULL) &&
        EFI_ERROR (Token->TimeoutCallback (&Instance->Mtftp4, Token))) {

      Mtftp4SendError (
         Instance,
         EFI_MTFTP4_ERRORCODE_REQUEST_DENIED,
         (UINT8 *) "User aborted the transfer in time out"
         );

      Mtftp4CleanOperation (Instance, EFI_ABORTED);
      continue;
    }

    //
    // Retransmit the packet if haven't reach the maxmium retry count,
    // otherwise exit the transfer.
    //
    if (++Instance->CurRetry < Instance->MaxRetry) {
      Mtftp4Retransmit (Instance);
      Mtftp4SetTimeout (Instance);
    } else {
      Mtftp4CleanOperation (Instance, EFI_TIMEOUT);
      continue;
    }
  }
}
