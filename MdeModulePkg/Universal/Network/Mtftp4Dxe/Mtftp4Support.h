/** @file
  Support routines for MTFTP.
  
Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __EFI_MTFTP4_SUPPORT_H__
#define __EFI_MTFTP4_SUPPORT_H__

//
// The structure representing a range of block numbers, [Start, End].
// It is used to remember the holes in the MTFTP block space. If all
// the holes are filled in, then the download or upload has completed.
//
typedef struct {
  LIST_ENTRY                Link;
  INTN                      Start;
  INTN                      End;
  INTN                      Round;
  INTN                      Bound;
} MTFTP4_BLOCK_RANGE;


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
  );

/**
  Get the first valid block number on the range list.

  @param  Head                  The block range head

  @return The first valid block number, -1 if the block range is empty. 

**/
INTN
Mtftp4GetNextBlockNum (
  IN LIST_ENTRY             *Head
  );

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
  );

/**
  Remove the block number from the block range list.

  @param  Head                  The block range list to remove from
  @param  Num                   The block number to remove
  @param  Completed             Wether Num is the last block number
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
  );

/**
  Set the timeout for the instance. User a longer time for passive instances.

  @param  Instance              The Mtftp session to set time out

**/
VOID
Mtftp4SetTimeout (
  IN OUT MTFTP4_PROTOCOL        *Instance
  );

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
  );

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
  );

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
  );

/**
  Retransmit the last packet for the instance.

  @param  Instance              The Mtftp instance

  @retval EFI_SUCCESS           The last packet is retransmitted.
  @retval Others                Failed to retransmit.

**/
EFI_STATUS
Mtftp4Retransmit (
  IN MTFTP4_PROTOCOL        *Instance
  );

/**
  The timer ticking function in TPL_NOTIFY level for the Mtftp service instance.

  @param  Event                 The ticking event
  @param  Context               The Mtftp service instance

**/
VOID
EFIAPI
Mtftp4OnTimerTickNotifyLevel (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );

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
  );
#endif
