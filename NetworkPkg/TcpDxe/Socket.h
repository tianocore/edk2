/** @file
  Common head file for TCP socket.

  Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <Uefi.h>

#include <Protocol/Tcp4.h>
#include <Protocol/Tcp6.h>

#include <Library/NetLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DpcLib.h>

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

/**
  Set socket SO_NO_MORE_DATA flag.

  @param[in] Sock            Pointer to the socket

**/
#define SOCK_NO_MORE_DATA(Sock)     ((Sock)->Flag |= SO_NO_MORE_DATA)

/**
  Check whether the socket is unconfigured.

  @param[in]  Sock           Pointer to the socket.

  @retval TRUE               The socket is unconfigured.
  @retval FALSE              The socket is not unconfigured.

**/
#define SOCK_IS_UNCONFIGURED(Sock)  ((Sock)->ConfigureState == SO_UNCONFIGURED)

/**
  Check whether the socket is configured.

  @param[in] Sock            Pointer to the socket

  @retval TRUE               The socket is configured
  @retval FALSE              The socket is not configured

**/
#define SOCK_IS_CONFIGURED(Sock) \
    (((Sock)->ConfigureState == SO_CONFIGURED_ACTIVE) || \
    ((Sock)->ConfigureState == SO_CONFIGURED_PASSIVE))

/**
  Check whether the socket is configured to active mode.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is configured to active mode.
  @retval FALSE              The socket is not configured to active mode.

**/
#define SOCK_IS_CONFIGURED_ACTIVE(Sock) ((Sock)->ConfigureState == SO_CONFIGURED_ACTIVE)

/**
  Check whether the socket is configured to passive mode.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is configured to passive mode.
  @retval FALSE              The socket is not configured to passive mode.

**/
#define SOCK_IS_CONNECTED_PASSIVE(Sock) ((Sock)->ConfigureState == SO_CONFIGURED_PASSIVE)

/**
  Check whether the socket is mapped.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is not mapping.
  @retval FALSE              The socket is mapped.

**/
#define SOCK_IS_NO_MAPPING(Sock)  ((Sock)->ConfigureState == SO_NO_MAPPING)

/**
  Check whether the socket is closed.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is closed.
  @retval FALSE              The socket is not closed.

**/
#define SOCK_IS_CLOSED(Sock)          ((Sock)->State == SO_CLOSED)

/**
  Check whether the socket is listening.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is listening.
  @retval FALSE              The socket is not listening.

**/
#define SOCK_IS_LISTENING(Sock)       ((Sock)->State == SO_LISTENING)

/**
  Check whether the socket is connecting.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is connecting.
  @retval FALSE              The socket is not connecting.

**/
#define SOCK_IS_CONNECTING(Sock)      ((Sock)->State == SO_CONNECTING)

/**
  Check whether the socket has connected.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket has connected.
  @retval FALSE              The socket has not connected.

**/
#define SOCK_IS_CONNECTED(Sock)       ((Sock)->State == SO_CONNECTED)

/**
  Check whether the socket is disconnecting.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is disconnecting.
  @retval FALSE              The socket is not disconnecting.

**/
#define SOCK_IS_DISCONNECTING(Sock)   ((Sock)->State == SO_DISCONNECTING)

/**
  Check whether the socket is no more data.

  @param[in] Sock            Pointer to the socket.

  @retval TRUE               The socket is no more data.
  @retval FALSE              The socket still has data.

**/
#define SOCK_IS_NO_MORE_DATA(Sock)    (0 != ((Sock)->Flag & SO_NO_MORE_DATA))

/**
  Set the size of the receive buffer.

  @param[in] Sock            Pointer to the socket.
  @param[in] Size            The size to set.

**/
#define SET_RCV_BUFFSIZE(Sock, Size)  ((Sock)->RcvBuffer.HighWater = (Size))

/**
  Get the size of the receive buffer.

  @param[in] Sock            Pointer to the socket.

  @return The receive buffer size.

**/
#define GET_RCV_BUFFSIZE(Sock)        ((Sock)->RcvBuffer.HighWater)

/**
  Get the size of the receive data.

  @param[in] Sock            Pointer to the socket.

  @return The received data size.

**/
#define GET_RCV_DATASIZE(Sock)        (((Sock)->RcvBuffer.DataQueue)->BufSize)

/**
  Set the size of the send buffer.

  @param[in] Sock            Pointer to the socket.
  @param[in] Size            The size to set.

**/
#define SET_SND_BUFFSIZE(Sock, Size)  ((Sock)->SndBuffer.HighWater = (Size))

/**
  Get the size of the send buffer.

  @param[in] Sock            Pointer to the socket.

  @return The send buffer size.

**/
#define GET_SND_BUFFSIZE(Sock)        ((Sock)->SndBuffer.HighWater)

/**
  Get the size of the send data.

  @param[in] Sock            Pointer to the socket.

  @return The send data size.

**/
#define GET_SND_DATASIZE(Sock)        (((Sock)->SndBuffer.DataQueue)->BufSize)

/**
  Set the backlog value of the socket.

  @param[in] Sock            Pointer to the socket.
  @param[in] Value           The value to set.

**/
#define SET_BACKLOG(Sock, Value)      ((Sock)->BackLog = (Value))

/**
  Get the backlog value of the socket.

  @param[in] Sock            Pointer to the socket.

  @return The backlog value.

**/
#define GET_BACKLOG(Sock)             ((Sock)->BackLog)

/**
  Set the socket with error state.

  @param[in] Sock            Pointer to the socket.
  @param[in] Error           The error state.

**/
#define SOCK_ERROR(Sock, Error)       ((Sock)->SockError = (Error))

#define SOCK_SIGNATURE                SIGNATURE_32 ('S', 'O', 'C', 'K')

#define SOCK_FROM_THIS(a)             CR ((a), SOCKET, NetProtocol, SOCK_SIGNATURE)

#define SOCK_FROM_TOKEN(Token)        (((SOCK_TOKEN *) (Token))->Sock)

#define PROTO_TOKEN_FORM_SOCK(SockToken, Type)  ((Type *) (((SOCK_TOKEN *) (SockToken))->Token))

typedef struct _TCP_SOCKET SOCKET;

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
  UINT32        LowWater;   ///< The low water mark of sock_buffer
  NET_BUF_QUEUE *DataQueue; ///< The queue to buffer data
} SOCK_BUFFER;

/**
  The handler of protocol for request from socket.

  @param[in] Socket          The socket issuing the request to protocol.
  @param[in] Request         The request issued by socket.
  @param[in] RequestData     The request related data.

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

/**
  The Callback function called after the TCP socket is created.

  @param[in]  This            Pointer to the socket just created.
  @param[in]  Context         Context of the socket.

  @retval EFI_SUCCESS         This protocol installed successfully.
  @retval other               Some error occurred.

**/
typedef
EFI_STATUS
(*SOCK_CREATE_CALLBACK) (
  IN SOCKET  *This,
  IN VOID    *Context
  );

/**
  The callback function called before the TCP socket is to be destroyed.

  @param[in]  This                   The TCP socket to be destroyed.
  @param[in]  Context                The context.

**/
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
  SOCK_TYPE              Type;
  UINT8                  State;

  SOCKET                 *Parent;        ///< The parent of this socket
  UINT32                 BackLog;        ///< The connection limit for listening socket
  UINT32                 SndBufferSize;  ///< The high water mark of send buffer
  UINT32                 RcvBufferSize;  ///< The high water mark of receive buffer
  UINT8                  IpVersion;
  VOID                   *Protocol;      ///< The pointer to protocol function template
                                         ///< wanted to install on socket

  //
  // Callbacks after socket is created and before socket is to be destroyed.
  //
  SOCK_CREATE_CALLBACK   CreateCallback;  ///< Callback after created
  SOCK_DESTROY_CALLBACK  DestroyCallback; ///< Callback before destroyed
  VOID                   *Context;        ///< The context of the callback

  //
  // Opaque protocol data.
  //
  VOID                   *ProtoData;
  UINT32                 DataSize;

  SOCK_PROTO_HANDLER     ProtoHandler;    ///< The handler of protocol for socket request

  EFI_HANDLE             DriverBinding;   ///< The driver binding handle
} SOCK_INIT_DATA;

///
///  The union type of TCP4 and TCP6 protocol.
///
typedef union _NET_PROTOCOL {
  EFI_TCP4_PROTOCOL      Tcp4Protocol;    ///< Tcp4 protocol
  EFI_TCP6_PROTOCOL      Tcp6Protocol;    ///< Tcp6 protocol
} NET_PROTOCOL;
///
/// The socket structure representing a network service access point.
///
struct _TCP_SOCKET {
  //
  // Socket description information
  //
  UINT32                    Signature;      ///< Signature of the socket
  EFI_HANDLE                SockHandle;     ///< The virtual handle of the socket
  EFI_HANDLE                DriverBinding;  ///< Socket's driver binding protocol
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  LIST_ENTRY                Link;
  UINT8                     ConfigureState;
  SOCK_TYPE                 Type;
  UINT8                     State;
  UINT16                    Flag;
  EFI_LOCK                  Lock;           ///< The lock of socket
  SOCK_BUFFER               SndBuffer;      ///< Send buffer of application's data
  SOCK_BUFFER               RcvBuffer;      ///< Receive buffer of received data
  EFI_STATUS                SockError;      ///< The error returned by low layer protocol
  BOOLEAN                   InDestroy;

  //
  // Fields used to manage the connection request
  //
  UINT32                    BackLog;        ///< the limit of connection to this socket
  UINT32                    ConnCnt;        ///< the current count of connections to it
  SOCKET                    *Parent;        ///< listening parent that accept the connection
  LIST_ENTRY                ConnectionList; ///< the connections maintained by this socket
  //
  // The queue to buffer application's asynchronous token
  //
  LIST_ENTRY                ListenTokenList;
  LIST_ENTRY                RcvTokenList;
  LIST_ENTRY                SndTokenList;
  LIST_ENTRY                ProcessingSndTokenList;

  SOCK_COMPLETION_TOKEN     *ConnectionToken; ///< app's token to signal if connected
  SOCK_COMPLETION_TOKEN     *CloseToken;      ///< app's token to signal if closed
  //
  // Interface for low level protocol
  //
  SOCK_PROTO_HANDLER        ProtoHandler;     ///< The request handler of protocol
  UINT8                     ProtoReserved[PROTO_RESERVED_LEN];  ///< Data fields reserved for protocol
  UINT8                     IpVersion;
  NET_PROTOCOL              NetProtocol;                        ///< TCP4 or TCP6 protocol socket used
  //
  // Callbacks after socket is created and before socket is to be destroyed.
  //
  SOCK_CREATE_CALLBACK      CreateCallback;   ///< Callback after created
  SOCK_DESTROY_CALLBACK     DestroyCallback;  ///< Callback before destroyed
  VOID                      *Context;         ///< The context of the callback
};

///
///  The token structure buffered in socket layer.
///
typedef struct _SOCK_TOKEN {
  LIST_ENTRY            TokenList;      ///< The entry to add in the token list
  SOCK_COMPLETION_TOKEN *Token;         ///< The application's token
  UINT32                RemainDataLen;  ///< Unprocessed data length
  SOCKET                *Sock;          ///< The pointer to the socket this token
                                        ///< belongs to
} SOCK_TOKEN;

///
/// Reserved data to access the NET_BUF delivered by TCP driver.
///
typedef struct _TCP_RSV_DATA {
  UINT32 UrgLen;
} TCP_RSV_DATA;

//
// Socket provided operations for low layer protocol implemented in SockImpl.c
//

/**
  Set the state of the socket.

  @param[in, out]  Sock                  Pointer to the socket.
  @param[in]       State                 The new socket state to be set.

**/
VOID
SockSetState (
  IN OUT SOCKET     *Sock,
  IN     UINT8      State
  );

/**
  Clone a new socket including its associated protocol control block.

  @param[in]  Sock                  Pointer to the socket to be cloned.

  @return Pointer to the newly cloned socket. If NULL, an error condition occurred.

**/
SOCKET *
SockClone (
  IN SOCKET *Sock
  );

/**
  Called by the low layer protocol to indicate the socket a connection is
  established.

  This function just changes the socket's state to SO_CONNECTED
  and signals the token used for connection establishment.

  @param[in, out]  Sock         Pointer to the socket associated with the
                                established connection.

**/
VOID
SockConnEstablished (
  IN OUT SOCKET *Sock
  );

/**
  Called by the low layer protocol to indicate that the connection is closed.

  This function flushes the socket, sets the state to SO_CLOSED, and signals
  the close token.

  @param[in, out]  Sock         Pointer to the socket associated with the closed
                                connection.

**/
VOID
SockConnClosed (
  IN OUT SOCKET *Sock
  );

/**
  Called by low layer protocol to indicate that some data is sent or processed.

  This function trims the sent data in the socket send buffer and signals the data
  token, if proper.

  @param[in, out]  Sock      Pointer to the socket.
  @param[in]       Count     The length of the data processed or sent, in bytes.

**/
VOID
SockDataSent (
  IN OUT SOCKET     *Sock,
  IN     UINT32     Count
  );

/**
  Called by the low layer protocol to copy some data in socket send
  buffer starting from the specific offset to a buffer provided by
  the caller.

  @param[in]  Sock                  Pointer to the socket.
  @param[in]  Offset                The start point of the data to be copied.
  @param[in]  Len                   The length of the data to be copied.
  @param[out] Dest                  Pointer to the destination to copy the data.

  @return The data size copied.

**/
UINT32
SockGetDataToSend (
  IN  SOCKET      *Sock,
  IN  UINT32      Offset,
  IN  UINT32      Len,
  OUT UINT8       *Dest
  );

/**
  Called by the low layer protocol to deliver received data to socket layer.

  This function appends the data to the socket receive buffer, set the
  urgent data length, then checks if any receive token can be signaled.

  @param[in, out]  Sock       Pointer to the socket.
  @param[in, out]  NetBuffer  Pointer to the buffer that contains the received data.
  @param[in]       UrgLen     The length of the urgent data in the received data.

**/
VOID
SockDataRcvd (
  IN OUT SOCKET    *Sock,
  IN OUT NET_BUF   *NetBuffer,
  IN     UINT32    UrgLen
  );

/**
  Get the length of the free space of the specific socket buffer.

  @param[in]  Sock              Pointer to the socket.
  @param[in]  Which             Flag to indicate which socket buffer to check:
                                either send buffer or receive buffer.

  @return The length of the free space, in bytes.

**/
UINT32
SockGetFreeSpace (
  IN SOCKET  *Sock,
  IN UINT32  Which
  );

/**
  Called by the low layer protocol to indicate that there will be no more data
  from the communication peer.

  This function sets the socket's state to SO_NO_MORE_DATA and signals all queued
  IO tokens with the error status EFI_CONNECTION_FIN.

  @param[in, out]  Sock                  Pointer to the socket.

**/
VOID
SockNoMoreData (
  IN OUT SOCKET *Sock
  );

//
// Socket provided operations for user interface implemented in SockInterface.c
//

/**
  Create a socket and its associated protocol control block
  with the initial data SockInitData and protocol specific
  data ProtoData.

  @param[in]  SockInitData         Initial data to setting the socket.

  @return Pointer to the newly created socket. If NULL, an error condition occurred.

**/
SOCKET *
SockCreateChild (
  IN SOCK_INIT_DATA *SockInitData
  );

/**
  Destroy the socket Sock and its associated protocol control block.

  @param[in, out]  Sock                 The socket to be destroyed.

  @retval EFI_SUCCESS          The socket Sock was destroyed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.

**/
EFI_STATUS
SockDestroyChild (
  IN OUT SOCKET *Sock
  );

/**
  Configure the specific socket Sock using configuration data ConfigData.

  @param[in]  Sock             Pointer to the socket to be configured.
  @param[in]  ConfigData       Pointer to the configuration data.

  @retval EFI_SUCCESS          The socket configured successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is already configured.

**/
EFI_STATUS
SockConfigure (
  IN SOCKET *Sock,
  IN VOID   *ConfigData
  );

/**
  Initiate a connection establishment process.

  @param[in]  Sock             Pointer to the socket to initiate the
                               connection.
  @param[in]  Token            Pointer to the token used for the connection
                               operation.

  @retval EFI_SUCCESS          The connection initialized successfully.
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
  Issue a listen token to get an existed connected network instance,
  or wait for a connection if there is none.

  @param[in]  Sock             Pointer to the socket to accept connections.
  @param[in]  Token            The token to accept a connection.

  @retval EFI_SUCCESS          Either a connection is accepted or the Token is
                               buffered for further acceptance.
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

  @param[in]  Sock             Pointer to the socket to process the token with
                               data.
  @param[in]  Token            The token with data that needs to send out.

  @retval EFI_SUCCESS          The token processed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the token due to a memory limit.

**/
EFI_STATUS
SockSend (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

/**
  Issue a token to get data from the socket.

  @param[in]  Sock             Pointer to the socket to get data from.
  @param[in]  Token            The token to store the received data from the
                               socket.

  @retval EFI_SUCCESS          The token processed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket, or the
                               socket is closed, or the socket is not in a
                               synchronized state , or the token is already in one
                               of this socket's lists.
  @retval EFI_NO_MAPPING       The IP address configuration operation is not
                               finished.
  @retval EFI_NOT_STARTED      The socket is not configured.
  @retval EFI_CONNECTION_FIN   The connection is closed and there is no more data.
  @retval EFI_OUT_OF_RESOURCE  Failed to buffer the token due to a memory limit.

**/
EFI_STATUS
SockRcv (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

/**
  Reset the socket and its associated protocol control block.

  @param[in, out]  Sock        Pointer to the socket to be flushed.

  @retval EFI_SUCCESS          The socket flushed successfully.
  @retval EFI_ACCESS_DENIED    Failed to get the lock to access the socket.

**/
EFI_STATUS
SockFlush (
  IN OUT SOCKET *Sock
  );

/**
  Close or abort the socket associated connection.

  @param[in, out]  Sock        Pointer to the socket of the connection to close
                               or abort.
  @param[in]  Token            The token for close operation.
  @param[in]  OnAbort          TRUE for aborting the connection, FALSE to close it.

  @retval EFI_SUCCESS          The close or abort operation initialized
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
  IN OUT SOCKET  *Sock,
  IN     VOID    *Token,
  IN     BOOLEAN OnAbort
  );

/**
  Abort the socket associated connection, listen, transmission or receive request.

  @param[in, out]  Sock        Pointer to the socket to abort.
  @param[in]       Token       Pointer to a token that has been issued by
                               Connect(), Accept(), Transmit() or Receive(). If
                               NULL, all pending tokens issued by the four
                               functions listed above will be aborted.

  @retval EFI_UNSUPPORTED      The operation is not supported in the current
                               implementation.
**/
EFI_STATUS
SockCancel (
  IN OUT SOCKET  *Sock,
  IN     VOID    *Token
  );

/**
  Get the mode data of the low layer protocol.

  @param[in]       Sock        Pointer to the socket to get mode data from.
  @param[in, out]  Mode        Pointer to the data to store the low layer mode
                               information.

  @retval EFI_SUCCESS          The mode data was obtained successfully.
  @retval EFI_NOT_STARTED      The socket is not configured.

**/
EFI_STATUS
SockGetMode (
  IN     SOCKET *Sock,
  IN OUT VOID   *Mode
  );

/**
  Add or remove route information in IP route table associated
  with this socket.

  @param[in]  Sock             Pointer to the socket associated with the IP route
                               table to operate on.
  @param[in]  RouteInfo        Pointer to the route information to be processed.

  @retval EFI_SUCCESS          The route table updated successfully.
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

#endif
