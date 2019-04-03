/** @file
  Mtftp6 support functions declaration.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_MTFTP6_SUPPORT_H__
#define __EFI_MTFTP6_SUPPORT_H__

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
} MTFTP6_BLOCK_RANGE;


/**
  Initialize the block range for either RRQ or WRQ. RRQ and WRQ have
  different requirements for Start and End. For example, during startup,
  WRQ initializes its whole valid block range to [0, 0xffff]. This
  is because the server will send an ACK0 to inform the user to start the
  upload. When the client receives an ACK0, it will remove 0 from the range,
  get the next block number, which is 1, then upload the BLOCK1. For RRQ
  without option negotiation, the server will directly send us the BLOCK1
  in response to the client's RRQ. When BLOCK1 is received, the client will
  remove it from the block range and send an ACK. It also works if there
  is option negotiation.

  @param[in]  Head                   The block range head to initialize.
  @param[in]  Start                  The Start block number.
  @param[in]  End                    The last block number.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for initial block range.
  @retval EFI_SUCCESS            The initial block range is created.

**/
EFI_STATUS
Mtftp6InitBlockRange (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Start,
  IN UINT16                 End
  );


/**
  Get the first valid block number on the range list.

  @param[in]  Head                   The block range head.

  @retval     ==-1                   If the block range is empty.
  @retval     >-1                    The first valid block number.

**/
INTN
Mtftp6GetNextBlockNum (
  IN LIST_ENTRY             *Head
  );


/**
  Set the last block number of the block range list. It
  removes all the blocks after the Last. MTFTP initialize the
  block range to the maximum possible range, such as [0, 0xffff]
  for WRQ. When it gets the last block number, it calls
  this function to set the last block number.

  @param[in]  Head                   The block range list.
  @param[in]  Last                   The last block number.

**/
VOID
Mtftp6SetLastBlockNum (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Last
  );


/**
  Remove the block number from the block range list.

  @param[in]  Head                   The block range list to remove from.
  @param[in]  Num                    The block number to remove.
  @param[in]  Completed              Whether Num is the last block number.
  @param[out] BlockCounter           The continuous block counter instead of the value after roll-over.

  @retval EFI_NOT_FOUND          The block number isn't in the block range list.
  @retval EFI_SUCCESS            The block number has been removed from the list.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
Mtftp6RemoveBlockNum (
  IN LIST_ENTRY             *Head,
  IN UINT16                 Num,
  IN BOOLEAN                Completed,
  OUT UINT64                *BlockCounter
  );


/**
  Build and transmit the request packet for the Mtftp6 instance.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  Operation              The operation code of this packet.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for the request.
  @retval EFI_SUCCESS            The request was built and sent.
  @retval Others                 Failed to transmit the packet.

**/
EFI_STATUS
Mtftp6SendRequest (
  IN MTFTP6_INSTANCE        *Instance,
  IN UINT16                 Operation
  );


/**
  Build and send an error packet.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  ErrCode                The error code in the packet.
  @param[in]  ErrInfo                The error message in the packet.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory for the error packet.
  @retval EFI_SUCCESS            The error packet was transmitted.
  @retval Others                 Failed to transmit the packet.

**/
EFI_STATUS
Mtftp6SendError (
  IN MTFTP6_INSTANCE        *Instance,
  IN UINT16                 ErrCode,
  IN UINT8*                 ErrInfo
  );


/**
  Send the packet for the Mtftp6 instance.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  Packet                 The pointer to the packet to be sent.

  @retval EFI_SUCCESS            The packet was sent out
  @retval Others                 Failed to transmit the packet.

**/
EFI_STATUS
Mtftp6TransmitPacket (
  IN MTFTP6_INSTANCE        *Instance,
  IN NET_BUF                *Packet
  );


/**
  Check packet for GetInfo callback routine.

  @param[in]  This                   The pointer to the Mtftp6 protocol.
  @param[in]  Token                  The pointer to the Mtftp6 token.
  @param[in]  PacketLen              The length of the packet
  @param[in]  Packet                 The pointer to the received packet.

  @retval EFI_SUCCESS            The check process passed successfully.
  @retval EFI_ABORTED            Abort the Mtftp6 operation.

**/
EFI_STATUS
EFIAPI
Mtftp6CheckPacket (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_TOKEN       *Token,
  IN UINT16                 PacketLen,
  IN EFI_MTFTP6_PACKET      *Packet
  );


/**
  The dummy configure routine for create a new Udp6 Io.

  @param[in]  UdpIo                  The pointer to the Udp6 Io.
  @param[in]  Context                The pointer to the context.

  @retval EFI_SUCCESS                The value is always returned.

**/
EFI_STATUS
EFIAPI
Mtftp6ConfigDummyUdpIo (
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  );


/**
  The configure routine for the Mtftp6 instance to transmit/receive.

  @param[in]  UdpIo                  The pointer to the Udp6 Io.
  @param[in]  ServerIp               The pointer to the server address.
  @param[in]  ServerPort             The pointer to the server port.
  @param[in]  LocalIp                The pointer to the local address.
  @param[in]  LocalPort              The pointer to the local port.

  @retval EFI_SUCCESS            Configure the Udp6 Io for Mtftp6 successfully.
  @retval EFI_NO_MAPPING         The corresponding Ip6 instance has not been
                                 configured yet.

**/
EFI_STATUS
Mtftp6ConfigUdpIo (
  IN UDP_IO                 *UdpIo,
  IN EFI_IPv6_ADDRESS       *ServerIp,
  IN UINT16                 ServerPort,
  IN EFI_IPv6_ADDRESS       *LocalIp,
  IN UINT16                 LocalPort
  );


/**
  Clean up the current Mtftp6 operation.

  @param[in]  Instance               The pointer to the Mtftp6 instance.
  @param[in]  Result                 The result to be returned to the user.

**/
VOID
Mtftp6OperationClean (
  IN MTFTP6_INSTANCE        *Instance,
  IN EFI_STATUS             Result
  );


/**
  Start the Mtftp6 instance to perform the operation, such as read file,
  write file, and read directory.

  @param[in]  This                   The MTFTP session
  @param[in]  Token                  The token that encapsulates the user's request.
  @param[in]  OpCode                 The operation to perform.

  @retval EFI_INVALID_PARAMETER  Some of the parameters are invalid.
  @retval EFI_NOT_STARTED        The MTFTP session hasn't been configured.
  @retval EFI_ALREADY_STARTED    There is pending operation for the session.
  @retval EFI_SUCCESS            The operation was successfully started.

**/
EFI_STATUS
Mtftp6OperationStart (
  IN EFI_MTFTP6_PROTOCOL    *This,
  IN EFI_MTFTP6_TOKEN       *Token,
  IN UINT16                 OpCode
  );


/**
  The timer ticking routine for the Mtftp6 instance.

  @param[in]  Event                  The pointer to the ticking event.
  @param[in]  Context                The pointer to the context.

**/
VOID
EFIAPI
Mtftp6OnTimerTick (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  );


/**
  The packet process callback for Mtftp6 upload.

  @param[in]  UdpPacket             The pointer to the packet received.
  @param[in]  UdpEpt                The pointer to the Udp6 access point.
  @param[in]  IoStatus              The status from the Udp6 instance.
  @param[in]  Context               The pointer to the context.

**/
VOID
EFIAPI
Mtftp6WrqInput (
  IN NET_BUF                *UdpPacket,
  IN UDP_END_POINT          *UdpEpt,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );


/**
  Start the Mtftp6 instance to upload. It will first init some states,
  then send the WRQ request packet, and start to receive the packet.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Operation             The operation code of current packet.

  @retval EFI_SUCCESS           The Mtftp6 was started to upload.
  @retval Others                Failed to start to upload.

**/
EFI_STATUS
Mtftp6WrqStart (
  IN MTFTP6_INSTANCE        *Instance,
  IN UINT16                 Operation
  );


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
  IN NET_BUF                *UdpPacket,
  IN UDP_END_POINT          *UdpEpt,
  IN EFI_STATUS             IoStatus,
  IN VOID                   *Context
  );


/**
  Start the Mtftp6 instance to download. It first initializes some
  of the internal states then builds and sends an RRQ reqeuest packet.
  Finally, it starts receive for the downloading.

  @param[in]  Instance              The pointer to the Mtftp6 instance.
  @param[in]  Operation             The operation code of current packet.

  @retval EFI_SUCCESS           The Mtftp6 was started to download.
  @retval Others                Failed to start to download.

**/
EFI_STATUS
Mtftp6RrqStart (
  IN MTFTP6_INSTANCE        *Instance,
  IN UINT16                 Operation
  );

#endif
