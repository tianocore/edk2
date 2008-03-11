/** @file

Copyright (c) 2005 - 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Socket.h

Abstract:


**/

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <PiDxe.h>

#include <Protocol/Ip4.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Udp4.h>

#include <Library/NetLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

#define SOCK_SND_BUF        0
#define SOCK_RCV_BUF        1

#define SOCK_BUFF_LOW_WATER 2 * 1024
#define SOCK_RCV_BUFF_SIZE  8 * 1024
#define SOCK_SND_BUFF_SIZE  8 * 1024
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
typedef enum {
  SO_CLOSED       = 0,
  SO_LISTENING,
  SO_CONNECTING,
  SO_CONNECTED,
  SO_DISCONNECTING
} SOCK_STATE;

typedef enum {
  SO_UNCONFIGURED       = 0,
  SO_CONFIGURED_ACTIVE,
  SO_CONFIGURED_PASSIVE,
  SO_NO_MAPPING
} SOCK_CONFIGURE_STATE;

#define SOCK_NO_MORE_DATA(Sock)     ((Sock)->Flag |= SO_NO_MORE_DATA)

#define SOCK_IS_UNCONFIGURED(Sock)  ((Sock)->ConfigureState == SO_UNCONFIGURED)

#define SOCK_IS_CONFIGURED(Sock) \
    (((Sock)->ConfigureState == SO_CONFIGURED_ACTIVE) || \
    ((Sock)->ConfigureState == SO_CONFIGURED_PASSIVE))

#define SOCK_IS_CONFIGURED_ACTIVE(Sock) \
  ((Sock)->ConfigureState == SO_CONFIGURED_ACTIVE)

#define SOCK_IS_CONNECTED_PASSIVE(Sock) \
  ((Sock)->ConfigureState == SO_CONFIGURED_PASSIVE)

#define SOCK_IS_NO_MAPPING(Sock) \
  ((Sock)->ConfigureState == SO_NO_MAPPING)

#define SOCK_IS_CLOSED(Sock)         ((Sock)->State == SO_CLOSED)

#define SOCK_IS_LISTENING(Sock)      ((Sock)->State == SO_LISTENING)

#define SOCK_IS_CONNECTING(Sock)     ((Sock)->State == SO_CONNECTING)

#define SOCK_IS_CONNECTED(Sock)      ((Sock)->State == SO_CONNECTED)

#define SOCK_IS_DISCONNECTING(Sock)  ((Sock)->State == SO_DISCONNECTING)

#define SOCK_IS_NO_MORE_DATA(Sock)   (0 != ((Sock)->Flag & SO_NO_MORE_DATA))

#define SOCK_SIGNATURE           EFI_SIGNATURE_32 ('S', 'O', 'C', 'K')

#define SOCK_FROM_THIS(a)        CR ((a), SOCKET, NetProtocol, SOCK_SIGNATURE)

#define SET_RCV_BUFFSIZE(Sock, Size)  ((Sock)->RcvBuffer.HighWater = (Size))

#define GET_RCV_BUFFSIZE(Sock)        ((Sock)->RcvBuffer.HighWater)

#define GET_RCV_DATASIZE(Sock)        (((Sock)->RcvBuffer.DataQueue)->BufSize)

#define SET_SND_BUFFSIZE(Sock, Size)  ((Sock)->SndBuffer.HighWater = (Size))

#define GET_SND_BUFFSIZE(Sock)        ((Sock)->SndBuffer.HighWater)

#define GET_SND_DATASIZE(Sock)        (((Sock)->SndBuffer.DataQueue)->BufSize)

#define SET_BACKLOG(Sock, Value)      ((Sock)->BackLog = (Value))

#define GET_BACKLOG(Sock)             ((Sock)->BackLog)

#define SOCK_ERROR(Sock, Error)       ((Sock)->SockError = (Error))

#define SND_BUF_HDR_LEN(Sock) \
  ((SockBufFirst (&((Sock)->SndBuffer)))->TotalSize)

#define RCV_BUF_HDR_LEN(Sock) \
  ((SockBufFirst (&((Sock)->RcvBuffer)))->TotalSize)

#define SOCK_FROM_TOKEN(Token)        (((SOCK_TOKEN *) (Token))->Sock)

#define PROTO_TOKEN_FORM_SOCK(SockToken, Type) \
  ((Type *) (((SOCK_TOKEN *) (SockToken))->Token))

typedef struct _SOCKET SOCKET;

typedef struct _SOCK_COMPLETION_TOKEN {
  EFI_EVENT   Event;
  EFI_STATUS  Status;
} SOCK_COMPLETION_TOKEN;

typedef struct _SOCK_IO_TOKEN {
  SOCK_COMPLETION_TOKEN Token;
  union {
    VOID  *RxData;
    VOID  *TxData;
  } Packet;
} SOCK_IO_TOKEN;

//
// the request issued from socket layer to protocol layer
//
typedef enum {
  SOCK_ATTACH,    // attach current socket to a new PCB
  SOCK_DETACH,    // detach current socket from the PCB
  SOCK_CONFIGURE, // configure attached PCB
  SOCK_FLUSH,     // flush attached PCB
  SOCK_SND,       // need protocol to send something
  SOCK_SNDPUSH,   // need protocol to send pushed data
  SOCK_SNDURG,    // need protocol to send urgent data
  SOCK_CONSUMED,  // application has retrieved data from socket
  SOCK_CONNECT,   // need to connect to a peer
  SOCK_CLOSE,     // need to close the protocol process
  SOCK_ABORT,     // need to reset the protocol process
  SOCK_POLL,      // need to poll to the protocol layer
  SOCK_ROUTE,     // need to add a route information
  SOCK_MODE,      // need to get the mode data of the protocol
  SOCK_GROUP      // need to join a mcast group
} SOCK_REQUEST;

//
// the socket type
//
typedef enum {
  SOCK_DGRAM, // this socket providing datagram service
  SOCK_STREAM // this socket providing stream service
} SOCK_TYPE;

//
// the handler of protocol for request from socket
//
typedef
EFI_STATUS
(*SOCK_PROTO_HANDLER) (
  IN SOCKET       * Socket,  // the socket issuing the request to protocol
  IN SOCK_REQUEST Request,   // the request issued by socket
  IN VOID *RequestData       // the request related data
  );

//
// the buffer structure of rcvd data and send data used by socket
//
typedef struct _SOCK_BUFFER {
  UINT32        HighWater;  // the buffersize upper limit of sock_buffer
  UINT32        LowWater;   // the low warter mark of sock_buffer
  NET_BUF_QUEUE *DataQueue; // the queue to buffer data
} SOCK_BUFFER;


//
// socket provided oprerations for low layer protocol
//

//
// socket provided operations for user interface
//
VOID
SockSetState (
  IN SOCKET     *Sock,
  IN SOCK_STATE State
  );

//
// when the connection establishment process for a Sock
// is finished low layer protocol calling this function
// to notify socket layer
//
VOID
SockConnEstablished (
  IN SOCKET *Sock
  );

VOID
SockConnClosed (
  IN SOCKET *Sock
  );

//
// called by low layer protocol to trim send buffer of
// Sock, when Count data is sent out completely
//
VOID
SockDataSent (
  IN SOCKET  *Sock,
  IN UINT32  Count
  );

//
// called by low layer protocol to get Len of data from
// socket to send and copy it in Dest
//
UINT32
SockGetDataToSend (
  IN SOCKET *Sock,
  IN UINT32 Offset,
  IN UINT32 Len,
  IN UINT8  *Dest
  );

//
//  called by low layer protocol to notify socket no more data can be
//  received
//
VOID
SockNoMoreData (
  IN SOCKET *Sock
  );

//
// called by low layer protocol to append a NetBuffer
// to rcv buffer of sock
//
VOID
SockDataRcvd (
  IN SOCKET    *Sock,
  IN NET_BUF   *NetBuffer,
  IN UINT32     UrgLen
  );

UINT32
SockGetFreeSpace (
  IN SOCKET   *Sock,
  IN UINT32   Which
  );

SOCKET  *
SockClone (
  IN SOCKET *Sock
  );

VOID
SockRcvdErr (
  IN SOCKET       *Sock,
  IN EFI_STATUS   Error
  );

typedef
EFI_STATUS
(*SOCK_CREATE_CALLBACK) (
  IN SOCKET  *This,
  IN VOID    *Context
  );

typedef
VOID
(*SOCK_DESTROY_CALLBACK) (
  IN SOCKET  *This,
  IN VOID    *Context
  );

//
// the initialize data for create a new socket
//
typedef struct _SOCK_INIT_DATA {
  SOCK_TYPE   Type;
  SOCK_STATE  State;

  SOCKET      *Parent;        // the parent of this socket
  UINT32      BackLog;        // the connection limit for listening socket
  UINT32      SndBufferSize;  // the high warter mark of send buffer
  UINT32      RcvBufferSize;  // the high warter mark of receive buffer
  VOID        *Protocol;      // the pointer to protocol function template
                              // wanted to install on socket

  //
  // Callbacks after socket is created and before socket is to be destroyed.
  //
  SOCK_CREATE_CALLBACK   CreateCallback;
  SOCK_DESTROY_CALLBACK  DestroyCallback;
  VOID                   *Context;

  //
  // Opaque protocol data.
  //
  VOID                   *ProtoData;
  UINT32                 DataSize;

  SOCK_PROTO_HANDLER     ProtoHandler;

  EFI_HANDLE   DriverBinding; // the driver binding handle
} SOCK_INIT_DATA;
//
// the socket structure representing a network service access point
//
struct _SOCKET {

  //
  // socket description information
  //
  UINT32                Signature;
  EFI_HANDLE            SockHandle;     // the virtual handle of the socket
  EFI_HANDLE            DriverBinding;  // socket't driver binding protocol
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  LIST_ENTRY                Link;  
  SOCK_CONFIGURE_STATE  ConfigureState;
  SOCK_TYPE             Type;
  SOCK_STATE            State;
  UINT16                Flag;
  EFI_LOCK              Lock;       // the lock of socket
  SOCK_BUFFER           SndBuffer;  // send buffer of application's data
  SOCK_BUFFER           RcvBuffer;  // receive buffer of received data
  EFI_STATUS            SockError;  // the error returned by low layer protocol
  BOOLEAN               IsDestroyed;

  //
  // fields used to manage the connection request
  //
  UINT32          BackLog;        // the limit of connection to this socket
  UINT32          ConnCnt;        // the current count of connections to it
  SOCKET          *Parent;        // listening parent that accept the connection
  LIST_ENTRY      ConnectionList; // the connections maintained by this socket
  //
  // the queue to buffer application's asynchronous token
  //
  LIST_ENTRY      ListenTokenList;
  LIST_ENTRY      RcvTokenList;
  LIST_ENTRY      SndTokenList;
  LIST_ENTRY      ProcessingSndTokenList;

  SOCK_COMPLETION_TOKEN *ConnectionToken; // app's token to signal if connected
  SOCK_COMPLETION_TOKEN *CloseToken;      // app's token to signal if closed

  //
  // interface for low level protocol
  //
  SOCK_PROTO_HANDLER    ProtoHandler; // the request handler of protocol
  UINT8 ProtoReserved[PROTO_RESERVED_LEN];  // Data fields reserved for protocol
  union {
    EFI_TCP4_PROTOCOL TcpProtocol;
    EFI_UDP4_PROTOCOL UdpProtocol;
  } NetProtocol;

  //
  // Callbacks.
  //
  SOCK_CREATE_CALLBACK   CreateCallback;
  SOCK_DESTROY_CALLBACK  DestroyCallback;
  VOID                   *Context;
};

//
// the token structure buffered in socket layer
//
typedef struct _SOCK_TOKEN {
  LIST_ENTRY            TokenList;      // the entry to add in the token list
  SOCK_COMPLETION_TOKEN *Token;         // The application's token
  UINT32                RemainDataLen;  // unprocessed data length
  SOCKET                *Sock;          // the poninter to the socket this token
                                        // belongs to
} SOCK_TOKEN;

//
// reserved data to access the NET_BUF delivered by UDP driver
//
typedef struct _UDP_RSV_DATA {
  EFI_TIME              TimeStamp;
  EFI_UDP4_SESSION_DATA Session;
} UDP_RSV_DATA;

//
// reserved data to access the NET_BUF delivered by TCP driver
//
typedef struct _TCP_RSV_DATA {
  UINT32 UrgLen;
} TCP_RSV_DATA;

//
// call it to creat a socket and attach it to a PCB
//
SOCKET  *
SockCreateChild (
  IN SOCK_INIT_DATA *SockInitData
  );

//
// call it to destroy a socket and its related PCB
//
EFI_STATUS
SockDestroyChild (
  IN SOCKET *Sock
  );

//
// call it to configure a socket and its related PCB
//
EFI_STATUS
SockConfigure (
  IN SOCKET *Sock,
  IN VOID   *ConfigData
  );

//
// call it to connect a socket to the peer
//
EFI_STATUS
SockConnect (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

//
// call it to issue an asynchronous listen token to the socket
//
EFI_STATUS
SockAccept (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

//
// Call it to send data using this socket
//
EFI_STATUS
SockSend (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

//
// Call it to receive data from this socket
//
EFI_STATUS
SockRcv (
  IN SOCKET *Sock,
  IN VOID   *Token
  );

//
// Call it to flush a socket
//
EFI_STATUS
SockFlush (
  IN SOCKET *Sock
  );

//
// Call it to close a socket in the light of policy in Token
//
EFI_STATUS
SockClose (
  IN SOCKET  *Sock,
  IN VOID    *Token,
  IN BOOLEAN OnAbort
  );

//
// Call it to get the mode data of low layer protocol
//
EFI_STATUS
SockGetMode (
  IN SOCKET *Sock,
  IN VOID   *Mode
  );

//
// call it to add this socket instance into a group
//
EFI_STATUS
SockGroup (
  IN SOCKET *Sock,
  IN VOID   *GroupInfo
  );

//
// call it to add a route entry for this socket instance
//
EFI_STATUS
SockRoute (
  IN SOCKET    *Sock,
  IN VOID      *RouteInfo
  );

//
// Supporting function to operate on socket buffer
//
NET_BUF *
SockBufFirst (
  IN SOCK_BUFFER *Sockbuf
  );

NET_BUF *
SockBufNext (
  IN SOCK_BUFFER *Sockbuf,
  IN NET_BUF     *SockEntry
  );

#endif
