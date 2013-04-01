/** @file
  Socket header file.

Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php<BR>

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <Uefi.h>

#include <Protocol/Ip4.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Udp4.h>

#include <Library/NetLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DpcLib.h>
#include <Library/PrintLib.h>

#define SOCK_SND_BUF        0
#define SOCK_RCV_BUF        1

#define SOCK_BUFF_LOW_WATER (2 * 1024)
#define SOCK_RCV_BUFF_SIZE  (8 * 1024)
#define SOCK_SND_BUFF_SIZE  (8 * 1024)
#define SOCK_BACKLOG        5

#define PROTO_RESERVED_LEN  20

#define SO_NO_MORE_DATA     0x0001

//
//
//
// When a socket is created it enters into SO_UNCONFIGURED,
// no actions can be taken on this socket, only after calling
// SockConfigure. The state transition diagram of socket is
// as following:
//
// SO_UNCONFIGURED --- SO_CONFIGURED --- SO_CONNECTING
//  ^      |                                    |
//  |      --->  SO_LISTENING                   |
//  |                                           |
//  |------------------SO_DISCONNECTING<-- SO_CONNECTED
//
// A passive socket can only go into SO_LISTENING and
// SO_UNCONFIGURED state. SO_XXXING state is a middle state
// when a socket is undergoing a protocol procedure such
// as requesting a TCP connection.
//
//
//

///
/// Socket state
///
#define SO_CLOSED        0
#define SO_LISTENING     1
#define SO_CONNECTING    2
#define SO_CONNECTED     3
#define SO_DISCONNECTING 4

///
/// Socket configure state
///
#define SO_UNCONFIGURED        0
#define SO_CONFIGURED_ACTIVE   1
#define SO_CONFIGURED_PASSIVE  2
#define SO_NO_MAPPING          3

/**
  Set socket SO_NO_MORE_DATA flag.
  
  @param Sock               Pointer to the socket
  
**/
#define SOCK_NO_MORE_DATA(Sock)     ((Sock)->Flag |= SO_NO_MORE_DATA)

/**
  Check whether the socket is unconfigured.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is unconfigued
  @retval False              The socket is not unconfigued
  
**/
#define SOCK_IS_UNCONFIGURED(Sock)  ((Sock)->ConfigureState == SO_UNCONFIGURED)

/**
  Check whether the socket is configured.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is configued
  @retval False              The socket is not configued
  
**/
#define SOCK_IS_CONFIGURED(Sock) \
    (((Sock)->ConfigureState == SO_CONFIGURED_ACTIVE) || \
    ((Sock)->ConfigureState == SO_CONFIGURED_PASSIVE))

/**
  Check whether the socket is configured to active mode.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is configued to active mode
  @retval False              The socket is not configued to active mode
  
**/
#define SOCK_IS_CONFIGURED_ACTIVE(Sock) \
  ((Sock)->ConfigureState == SO_CONFIGURED_ACTIVE)

/**
  Check whether the socket is configured to passive mode.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is configued to passive mode
  @retval False              The socket is not configued to passive mode
  
**/
#define SOCK_IS_CONNECTED_PASSIVE(Sock) \
  ((Sock)->ConfigureState == SO_CONFIGURED_PASSIVE)

/**
  Check whether the socket is mapped.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is no mapping
  @retval False              The socket is mapped
  
**/
#define SOCK_IS_NO_MAPPING(Sock) \
  ((Sock)->ConfigureState == SO_NO_MAPPING)

/**
  Check whether the socket is closed.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is closed
  @retval False              The socket is not closed
  
**/
#define SOCK_IS_CLOSED(Sock)          ((Sock)->State == SO_CLOSED)

/**
  Check whether the socket is listening.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is listening
  @retval False              The socket is not listening
  
**/
#define SOCK_IS_LISTENING(Sock)       ((Sock)->State == SO_LISTENING)

/**
  Check whether the socket is connecting.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is connecting
  @retval False              The socket is not connecting
  
**/
#define SOCK_IS_CONNECTING(Sock)      ((Sock)->State == SO_CONNECTING)

/**
  Check whether the socket has connected.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket has connected
  @retval False              The socket has not connected
  
**/
#define SOCK_IS_CONNECTED(Sock)       ((Sock)->State == SO_CONNECTED)

/**
  Check whether the socket is disconnecting.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is disconnecting
  @retval False              The socket is not disconnecting
  
**/
#define SOCK_IS_DISCONNECTING(Sock)   ((Sock)->State == SO_DISCONNECTING)

/**
  Check whether the socket is no more data.
  
  @param  Sock               Pointer to the socket
  
  @retval True               The socket is no more data
  @retval False              The socket still has data
  
**/
#define SOCK_IS_NO_MORE_DATA(Sock)    (0 != ((Sock)->Flag & SO_NO_MORE_DATA))

/**
  Set the size of the receive buffer.
  
  @param  Sock               Pointer to the socket
  @param  Size               The size to set

**/
#define SET_RCV_BUFFSIZE(Sock, Size)  ((Sock)->RcvBuffer.HighWater = (Size))

/**
  Get the size of the receive buffer.
  
  @param  Sock               Pointer to the socket
  
  @return The receive buffer size

**/
#define GET_RCV_BUFFSIZE(Sock)        ((Sock)->RcvBuffer.HighWater)

/**
  Get the size of the receive data.
  
  @param  Sock               Pointer to the socket
  
  @return The received data size

**/
#define GET_RCV_DATASIZE(Sock)        (((Sock)->RcvBuffer.DataQueue)->BufSize)

/**
  Set the size of the send buffer.
  
  @param  Sock               Pointer to the socket
  @param  Size               The size to set

**/
#define SET_SND_BUFFSIZE(Sock, Size)  ((Sock)->SndBuffer.HighWater = (Size))

/**
  Get the size of the send buffer.
  
  @param  Sock               Pointer to the socket
  
  @return The send buffer size

**/
#define GET_SND_BUFFSIZE(Sock)        ((Sock)->SndBuffer.HighWater)

/**
  Get the size of the send data.
  
  @param  Sock               Pointer to the socket
  
  @return The send data size

**/
#define GET_SND_DATASIZE(Sock)        (((Sock)->SndBuffer.DataQueue)->BufSize)

/**
  Set the backlog value of the socket.
  
  @param  Sock               Pointer to the socket
  @param  Value              The value to set

**/
#define SET_BACKLOG(Sock, Value)      ((Sock)->BackLog = (Value))

/**
  Get the backlog value of the socket.
  
  @param  Sock               Pointer to the socket
  
  @return The backlog value

**/
#define GET_BACKLOG(Sock)             ((Sock)->BackLog)

/**
  Set the socket with error state.
  
  @param  Sock               Pointer to the socket
  @param  Error              The error state

**/
#define SOCK_ERROR(Sock, Error)       ((Sock)->SockError = (Error))

#define SND_BUF_HDR_LEN(Sock) \
  ((SockBufFirst (&((Sock)->SndBuffer)))->TotalSize)

#define RCV_BUF_HDR_LEN(Sock) \
  ((SockBufFirst (&((Sock)->RcvBuffer)))->TotalSize)

#define SOCK_SIGNATURE                SIGNATURE_32 ('S', 'O', 'C', 'K')

#define SOCK_FROM_THIS(a)             CR ((a), SOCKET, NetProtocol, SOCK_SIGNATURE)

#define SOCK_FROM_TOKEN(Token)        (((SOCK_TOKEN *) (Token))->Sock)

#define PROTO_TOKEN_FORM_SOCK(SockToken, Type) \
  ((Type *) (((SOCK_TOKEN *) (SockToken))->Token))

typedef struct _SOCKET SOCKET;

///
/// Socket completion token
///
typedef struct _SOCK_COMPLETION_TOKEN {
  EFI_EVENT   Event;            ///< The event to be issued
  EFI_STATUS  Status;           ///< The status to be issued
} SOCK_COMPLETION_TOKEN;

typedef union {
  VOID  *RxData;
  VOID  *TxData;
} SOCK_IO_DATA;

///
/// The application token with data packet
///
typedef struct _SOCK_IO_TOKEN {
  SOCK_COMPLETION_TOKEN Token;
  SOCK_IO_DATA          Packet;
} SOCK_IO_TOKEN;

///
///  The request issued from socket layer to protocol layer.  
///
#define SOCK_ATTACH     0    ///< Attach current socket to a new PCB
#define SOCK_DETACH     1    ///< Detach current socket from the PCB
#define SOCK_CONFIGURE  2    ///< Configure attached PCB
#define SOCK_FLUSH      3    ///< Flush attached PCB
#define SOCK_SND        4    ///< Need protocol to send something
#define SOCK_SNDPUSH    5    ///< Need protocol to send pushed data
#define SOCK_SNDURG     6    ///< Need protocol to send urgent data
#define SOCK_CONSUMED   7    ///< Application has retrieved data from socket
#define SOCK_CONNECT    8    ///< Need to connect to a peer
#define SOCK_CLOSE      9    ///< Need to close the protocol process
#define SOCK_ABORT      10   ///< Need to reset the protocol process
#define SOCK_POLL       11   ///< Need to poll to the protocol layer
#define SOCK_ROUTE      12   ///< Need to add a route information
#define SOCK_MODE       13   ///< Need to get the mode data of the protocol
#define SOCK_GROUP      14   ///< Need to join a mcast group

///
///  The socket type.
///
typedef enum {
  SockDgram, ///< This socket providing datagram service
  SockStream ///< This socket providing stream service
} SOCK_TYPE;

///
///  The buffer structure of rcvd data and send data used by socket.
///
typedef struct _SOCK_BUFFER {
  UINT32        HighWater;  ///< The buffersize upper limit of sock_buffer
  UINT32        LowWater;   ///< The low warter mark of sock_buffer
  NET_BUF_QUEUE *DataQueue; ///< The queue to buffer data
} SOCK_BUFFER;

/**
  The handler of protocol for request from socket.
  
  @param Socket              The socket issuing the request to protocol
  @param Request             The request issued by socket
  @param RequestData         The request related data
  
  @retval EFI_SUCCESS        The socket request is completed successfully.
  @retval other              The error status returned by the corresponding TCP
                             layer function.
                             
**/
typedef
EFI_STATUS
(*SOCK_PROTO_HANDLER) (
  IN SOCKET       *Socket,
  IN UINT8        Request,
  IN VOID         *RequestData
  );
  
  
//
// Socket provided oprerations for low layer protocol
//

//
// Socket provided operations for user interface
//

/**
  Set the state of the socket.

  @param  Sock                  Pointer to the socket.
  @param  State                 The new socket state to be set.

**/
VOID
SockSetState (
  IN OUT SOCKET     *Sock,
  IN     UINT8      State
  );

/**
  Called by the low layer protocol to indicate the socket a connection is 
  established. 
  
  This function just changes the socket's state to SO_CONNECTED 
  and signals the token used for connection establishment.

  @param  Sock                  Pointer to the socket associated with the
                                established connection.
                                
**/
VOID
SockConnEstablished (
  IN SOCKET *Sock
  );

/**
  Called by the low layer protocol to indicate the connection is closed.
  
  This function flushes the socket, sets the state to SO_CLOSED and signals 
  the close token.

  @param  Sock                  Pointer to the socket associated with the closed
                                connection.
                                
**/
VOID
SockConnClosed (
  IN OUT SOCKET *Sock
  );

/**
  Called by low layer protocol to indicate that some data is sent or processed.
   
  This function trims the sent data in the socket send buffer, signals the data 
  token if proper.

  @param  Sock                  Pointer to the socket.
  @param  Count                 The length of the data processed or sent, in bytes.

**/
VOID
SockDataSent (
  IN SOCKET     *Sock,
  IN UINT32     Count
  );

/**
  Called by the low layer protocol to copy some data in socket send
  buffer starting from the specific offset to a buffer provided by
  the caller.

  @param  Sock                  Pointer to the socket.
  @param  Offset                The start point of the data to be copied.
  @param  Len                   The length of the data to be copied.
  @param  Dest                  Pointer to the destination to copy the data.

  @return The data size copied.

**/
UINT32
SockGetDataToSend (
  IN SOCKET      *Sock,
  IN UINT32      Offset,
  IN UINT32      Len,
  IN UINT8       *Dest
  );

/**
  Called by the low layer protocol to indicate that there
  will be no more data from the communication peer.
  
  This function set the socket's state to SO_NO_MORE_DATA and
  signal all queued IO tokens with the error status EFI_CONNECTION_FIN.

  @param  Sock                  Pointer to the socket.

**/
VOID
SockNoMoreData (
  IN OUT SOCKET *Sock
  );

/**
  Called by the low layer protocol to deliver received data to socket layer.
  
  This function will append the data to the socket receive buffer, set ther 
  urgent data length and then check if any receive token can be signaled.

  @param  Sock                  Pointer to the socket.
  @param  NetBuffer             Pointer to the buffer that contains the received
                                data.
  @param  UrgLen                The length of the urgent data in the received data.

**/
VOID
SockDataRcvd (
  IN     SOCKET    *Sock,
  IN OUT NET_BUF   *NetBuffer,
  IN     UINT32    UrgLen
  );

/**
  Get the length of the free space of the specific socket buffer.

  @param  Sock                  Pointer to the socket.
  @param  Which                 Flag to indicate which socket buffer to check,
                                either send buffer or receive buffer.

  @return The length of the free space, in bytes.

**/
UINT32
SockGetFreeSpace (
  IN SOCKET  *Sock,
  IN UINT32  Which
  );

/**
  Clone a new socket including its associated protocol control block.

  @param  Sock                  Pointer to the socket to be cloned.

  @return Pointer to the newly cloned socket. If NULL, error condition occurred.

**/
SOCKET *
SockClone (
  IN SOCKET *Sock
  );

/**
  Signal the receive token with the specific error or
  set socket error code after error is received.

  @param  Sock                  Pointer to the socket.
  @param  Error                 The error code received.

**/
VOID
SockRcvdErr (
  IN OUT SOCKET       *Sock,
  IN     EFI_STATUS   Error
  );

///
/// Proto type of the create callback
///
typedef
EFI_STATUS
(*SOCK_CREATE_CALLBACK) (
  IN SOCKET  *This,
  IN VOID    *Context
  );
  
///
/// Proto type of the destroy callback 
///
typedef
VOID
(*SOCK_DESTROY_CALLBACK) (
  IN SOCKET  *This,
  IN VOID    *Context
  );

///
///  The initialize data for create a new socket.
///
typedef struct _SOCK_INIT_DATA {
  SOCK_TYPE   Type;
  UINT8       State;

  SOCKET      *Parent;        ///< The parent of this socket
  UINT32      BackLog;        ///< The connection limit for listening socket
  UINT32      SndBufferSize;  ///< The high warter mark of send buffer
  UINT32      RcvBufferSize;  ///< The high warter mark of receive buffer
  VOID        *Protocol;      ///< The pointer to protocol function template
                              ///< wanted to install on socket

  //
  // Callbacks after socket is created and before socket is to be destroyed.
  //
  SOCK_CREATE_CALLBACK   CreateCallback;  ///< Callback after created
  SOCK_DESTROY_CALLBACK  DestroyCallback; ///< Callback before destroied
  VOID                   *Context;        ///< The context of the callback

  //
  // Opaque protocol data.
  //
  VOID                   *ProtoData;
  UINT32                 DataSize;

  SOCK_PROTO_HANDLER     ProtoHandler;  ///< The handler of protocol for socket request

  EFI_HANDLE   DriverBinding;           ///< The driver binding handle
} SOCK_INIT_DATA;

///
///  The union type of TCP and UDP protocol.
///  
typedef union _NET_PROTOCOL {
  EFI_TCP4_PROTOCOL TcpProtocol;   ///< Tcp protocol
  EFI_UDP4_PROTOCOL UdpProtocol;   ///< Udp protocol
} NET_PROTOCOL;

///
/// The socket structure representing a network service access point
///
struct _SOCKET {

  //
  // Socket description information
  //
  UINT32                Signature;      ///< Signature of the socket
  EFI_HANDLE            SockHandle;     ///< The virtual handle of the socket
  EFI_HANDLE            DriverBinding;  ///< Socket's driver binding protocol
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  LIST_ENTRY                Link;  
  UINT8                 ConfigureState;
  SOCK_TYPE             Type;
  UINT8                 State;
  UINT16                Flag;
  EFI_LOCK              Lock;       ///< The lock of socket
  SOCK_BUFFER           SndBuffer;  ///< Send buffer of application's data
  SOCK_BUFFER           RcvBuffer;  ///< Receive buffer of received data
  EFI_STATUS            SockError;  ///< The error returned by low layer protocol
  BOOLEAN               InDestroy;

  //
  // Fields used to manage the connection request
  //
  UINT32          BackLog;        ///< the limit of connection to this socket
  UINT32          ConnCnt;        ///< the current count of connections to it
  SOCKET          *Parent;        ///< listening parent that accept the connection
  LIST_ENTRY      ConnectionList; ///< the connections maintained by this socket
  
  //
  // The queue to buffer application's asynchronous token
  //
  LIST_ENTRY      ListenTokenList;
  LIST_ENTRY      RcvTokenList;
  LIST_ENTRY      SndTokenList;
  LIST_ENTRY      ProcessingSndTokenList;

  SOCK_COMPLETION_TOKEN *ConnectionToken; ///< app's token to signal if connected
  SOCK_COMPLETION_TOKEN *CloseToken;      ///< app's token to signal if closed

  //
  // Interface for low level protocol
  //
  SOCK_PROTO_HANDLER    ProtoHandler;       ///< The request handler of protocol
  UINT8 ProtoReserved[PROTO_RESERVED_LEN];  ///< Data fields reserved for protocol
  NET_PROTOCOL          NetProtocol;        ///< TCP or UDP protocol socket used

  //
  // Callbacks after socket is created and before socket is to be destroyed.
  //
  SOCK_CREATE_CALLBACK   CreateCallback;    ///< Callback after created
  SOCK_DESTROY_CALLBACK  DestroyCallback;   ///< Callback before destroied
  VOID                   *Context;          ///< The context of the callback
};

///
///  The token structure buffered in socket layer.
///
typedef struct _SOCK_TOKEN {
  LIST_ENTRY            TokenList;      ///< The entry to add in the token list
  SOCK_COMPLETION_TOKEN *Token;         ///< The application's token
  UINT32                RemainDataLen;  ///< Unprocessed data length
  SOCKET                *Sock;          ///< The poninter to the socket this token
                                        ///< belongs to
} SOCK_TOKEN;

///
/// Reserved data to access the NET_BUF delivered by UDP driver.
///
typedef struct _UDP_RSV_DATA {
  EFI_TIME              TimeStamp;
  EFI_UDP4_SESSION_DATA Session;
} UDP_RSV_DATA;

///
/// Reserved data to access the NET_BUF delivered by TCP driver.
///
typedef struct _TCP_RSV_DATA {
  UINT32 UrgLen;
} TCP_RSV_DATA;

/**
  Create a socket and its associated protocol control block
  with the intial data SockInitData and protocol specific
  data ProtoData.

  @param  SockInitData         Inital data to setting the socket.
  
  @return Pointer to the newly created socket. If NULL, error condition occured.

**/
SOCKET *
SockCreateChild (
  IN SOCK_INIT_DATA *SockInitData
  );

/**
  Destroy the socket Sock and its associated protocol control block.

  @param  Sock                 The socket to be destroyed.

  @retval EFI_SUCCESS          The socket Sock is destroyed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.

**/
EFI_STATUS
SockDestroyChild (
  IN   SOCKET *Sock
  );

/**
  Configure the specific socket Sock using configuration data ConfigData.

  @param  Sock                 Pointer to the socket to be configured.
  @param  ConfigData           Pointer to the configuration data.

  @retval EFI_SUCCESS          The socket is configured successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket or the
                               socket is already configured.

**/
EFI_STATUS
SockConfigure (
  IN SOCKET *Sock,
  IN VOID   *ConfigData
  );

/**
  Initiate a connection establishment process.

  @param  Sock                 Pointer to the socket to initiate the initate the
                               connection.
  @param  Token                Pointer to the token used for the connection
                               operation.

  @retval EFI_SUCCESS          The connection is initialized successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not configured to
                               be an active one, or the token is already in one of
                               this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockConnect (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

/**
  Issue a listen token to get an existed connected network instance
  or wait for a connection if there is none.

  @param  Sock                 Pointer to the socket to accept connections.
  @param  Token                The token to accept a connection.

  @retval EFI_SUCCESS          Either a connection is accpeted or the Token is
                               buffered for further acception.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not configured to
                               be a passive one, or the token is already in one of
                               this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the Token due to memory limit.

**/
EFI_STATUS
SockAccept (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

/**
  Issue a token with data to the socket to send out.

  @param  Sock                 Pointer to the socket to process the token with
                               data.
  @param  Token                The token with data that needs to send out.

  @retval EFI_SUCCESS          The token is processed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the token due to memory limit.

**/
EFI_STATUS
SockSend (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

/**
  Issue a token to get data from the socket.

  @param  Sock                 Pointer to the socket to get data from.
  @param  Token                The token to store the received data from the
                               socket.

  @retval EFI_SUCCESS          The token is processed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_CONNECTION_FIN   The connection is closed and there is no more data.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the token due to memory limit.

**/
EFI_STATUS
SockRcv (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

/**
  Reset the socket and its associated protocol control block.

  @param  Sock                 Pointer to the socket to be flushed.

  @retval EFI_SUCCESS          The socket is flushed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.

**/
EFI_STATUS
SockFlush (
  IN SOCKET *Sock
  );

/**
  Close or abort the socket associated connection.

  @param  Sock                 Pointer to the socket of the connection to close or
                               abort.
  @param  Token                The token for close operation.
  @param  OnAbort              TRUE for aborting the connection, FALSE to close it.

  @retval EFI_SUCCESS          The close or abort operation is initialized
                               successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockClose (
  IN SOCKET  *Sock,
  IN VOID    *Token,
  IN BOOLEAN OnAbort
  );

/**
  Get the mode data of the low layer protocol.

  @param  Sock                 Pointer to the socket to get mode data from.
  @param  Mode                 Pointer to the data to store the low layer mode
                               information.

  @retval EFI_SUCCESS          The mode data is got successfully.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockGetMode (
  IN     SOCKET *Sock,
  IN OUT VOID   *Mode
  );

/**
  Configure the low level protocol to join a multicast group for
  this socket's connection.

  @param  Sock                 Pointer to the socket of the connection to join the
                               specific multicast group.
  @param  GroupInfo            Pointer to the multicast group info.

  @retval EFI_SUCCESS          The configuration is done successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockGroup (
  IN SOCKET *Sock,
  IN VOID   *GroupInfo
  );

/**
  Add or remove route information in IP route table associated
  with this socket.

  @param  Sock                 Pointer to the socket associated with the IP route
                               table to operate on.
  @param  RouteInfo            Pointer to the route information to be processed.

  @retval EFI_SUCCESS          The route table is updated successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.
  @retval EFI_NO_MAPPING       The IP address configuration operation is  not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockRoute (
  IN SOCKET    *Sock,
  IN VOID      *RouteInfo
  );

//
// Supporting function to operate on socket buffer
//

/**
  Get the first buffer block in the specific socket buffer.

  @param  Sockbuf               Pointer to the socket buffer.

  @return Pointer to the first buffer in the queue. NULL if the queue is empty.

**/
NET_BUF *
SockBufFirst (
  IN SOCK_BUFFER *Sockbuf
  );

/**
  Get the next buffer block in the specific socket buffer.

  @param  Sockbuf               Pointer to the socket buffer.
  @param  SockEntry             Pointer to the buffer block prior to the required
                                one.

  @return Pointer to the buffer block next to SockEntry. NULL if SockEntry is 
          the tail or head entry.

**/
NET_BUF *
SockBufNext (
  IN SOCK_BUFFER *Sockbuf,
  IN NET_BUF     *SockEntry
  );

#endif
