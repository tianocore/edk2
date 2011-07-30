/** @file
  Definitions for the Socket layer driver.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <Efi/EfiSocketLib.h>

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

#define DEBUG_SOCKET        0x20000000  ///<  Display Socket related messages
#define DEBUG_BIND          0x10000000  ///<  Display bind related messages
#define DEBUG_LISTEN        0x08000000  ///<  Display listen related messages
#define DEBUG_CONNECTION    0x04000000  ///<  Display connection list related messages
#define DEBUG_POLL          0x02000000  ///<  Display poll messages
#define DEBUG_ACCEPT        0x01000000  ///<  Display accept related messages
#define DEBUG_RX            0x00800000  ///<  Display receive messages
#define DEBUG_TX            0x00400000  ///<  Display transmit messages
#define DEBUG_CLOSE         0x00200000  ///<  Display close messages
#define DEBUG_CONNECT       0x00100000  ///<  Display connect messages

#define MAX_PENDING_CONNECTIONS     1   ///<  Maximum connection FIFO depth
#define MAX_RX_DATA         65536       ///<  Maximum receive data size
#define MAX_TX_DATA         ( MAX_RX_DATA * 2 )
#define RX_PACKET_DATA      16384       ///<  Maximum number of bytes in a RX packet

#define LAYER_SIGNATURE             SIGNATURE_32('S','k','t','L') ///<  DT_LAYER memory signature
#define SERVICE_SIGNATURE           SIGNATURE_32('S','k','t','S') ///<  DT_SERVICE memory signature
#define SOCKET_SIGNATURE            SIGNATURE_32('S','c','k','t') ///<  DT_SOCKET memory signature
#define PORT_SIGNATURE              SIGNATURE_32('P','o','r','t') ///<  DT_PORT memory signature

typedef enum
{
  NETWORK_TYPE_UNKNOWN = 0,
  NETWORK_TYPE_RAW,
  NETWORK_TYPE_TCP4,
  NETWORK_TYPE_TCP6,
  NETWORK_TYPE_UDP4,
  NETWORK_TYPE_UDP6
} NETWORK_TYPE;

typedef enum
{
  SOCKET_STATE_NOT_CONFIGURED = 0,  ///<  socket call was successful
  SOCKET_STATE_BOUND,               ///<  bind call was successful
  SOCKET_STATE_LISTENING,           ///<  listen call was successful
  SOCKET_STATE_NO_PORTS,            ///<  No ports available
  SOCKET_STATE_IN_FIFO,             ///<  Socket on FIFO
  SOCKET_STATE_CONNECTING,          ///<  Connecting to a remote system
  SOCKET_STATE_CONNECTED,           ///<  Accept or connect call was successful

  //
  //  Close state must be the last in the list
  //
  SOCKET_STATE_CLOSED               ///<  Close call was successful
} SOCKET_STATE;

typedef enum
{
  PORT_STATE_ALLOCATED = 0, ///<  Port allocated
  PORT_STATE_OPEN,          ///<  Port opened
  PORT_STATE_RX_ERROR,      ///<  Receive error detected

  //
  //  Close state must be last in the list
  //
  PORT_STATE_CLOSE_STARTED, ///<  Close started on port
  PORT_STATE_CLOSE_TX_DONE, ///<  Transmits shutdown
  PORT_STATE_CLOSE_RX_DONE, ///<  Receives shutdown
  PORT_STATE_CLOSE_DONE     ///<  Port close operation complete
} PORT_STATE;

//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

typedef struct _DT_PACKET DT_PACKET;    ///<  Forward declaration
typedef struct _DT_PORT DT_PORT;        ///<  Forward declaration
typedef struct _DT_SOCKET DT_SOCKET;    ///<  Forward declaration

typedef struct
{
  EFI_TCP4_RECEIVE_DATA RxData;         ///<  Receive operation description
  size_t ValidBytes;                    ///<  Length of valid data in bytes
  UINT8 * pBuffer;                      ///<  Current data pointer
  UINT8 Buffer [ RX_PACKET_DATA ];      ///<  Data buffer
} DT_TCP4_RX_DATA;

typedef struct
{
  EFI_TCP4_TRANSMIT_DATA TxData;        ///<  Transmit operation description
  UINT8 Buffer [ 1 ];                   ///<  Data buffer
} DT_TCP4_TX_DATA;

typedef struct
{
  EFI_UDP4_SESSION_DATA Session;        ///< * Remote network address
  EFI_UDP4_RECEIVE_DATA * pRxData;      ///< * Receive operation description
} DT_UDP4_RX_DATA;

typedef struct
{
  EFI_UDP4_SESSION_DATA Session;        ///<  Remote network address
  EFI_UDP4_TRANSMIT_DATA TxData;        ///<  Transmit operation description
  UINT8 Buffer [ 1 ];                   ///<  Data buffer
} DT_UDP4_TX_DATA;

typedef struct _DT_PACKET {
  DT_PACKET * pNext;                    ///<  Next packet in the receive list
  size_t PacketSize;                    ///<  Size of this data structure
  union {
    DT_TCP4_RX_DATA Tcp4Rx;             ///<  Receive operation description
    DT_TCP4_TX_DATA Tcp4Tx;             ///<  Transmit operation description
    DT_UDP4_RX_DATA Udp4Rx;             ///<  Receive operation description
    DT_UDP4_TX_DATA Udp4Tx;             ///<  Transmit operation description
  } Op;
} GCC_DT_PACKET;

/**
  Service control structure

  The driver uses this structure to manage the network devices.
**/
typedef struct _DT_SERVICE {
  UINTN Signature;          ///<  Structure identification

  //
  //  Links
  //
  DT_SERVICE * pNext;       ///<  Next service in the service list

  //
  //  Service data
  //
  CONST DT_SOCKET_BINDING * pSocketBinding; ///<  Name and shutdown routine
  EFI_HANDLE Controller;    ///<  Controller for the service
  VOID * pInterface;        ///<  Network layer service binding interface

  //
  //  Network data
  //
  NETWORK_TYPE NetworkType; ///<  Type of network service
  DT_PORT * pPortList;      ///<  List of ports using this service
}GCC_DT_SERVICE;

/**
  Start the close operation on a TCP4 port.

  @param [in] pPort       Address of the port structure.
  @param [in] bAbort      Set TRUE to abort active transfers
  @param [in] DebugFlags  Flags for debug messages

**/
typedef
EFI_STATUS
PFN_PORT_CLOSE_START (
  IN DT_PORT * pPort,
  IN BOOLEAN bAbort,
  IN UINTN DebugFlags
  );

/**
  TCP4 context structure

  The driver uses this structure to manage the TCP4 connections.
**/
typedef struct {
  //
  //  TCP4 context
  //
  EFI_HANDLE Handle;                ///<  TCP4 port handle
  EFI_TCP4_PROTOCOL * pProtocol;    ///<  TCP4 protocol pointer
  EFI_TCP4_CONFIG_DATA ConfigData;  ///<  TCP4 configuration data
  EFI_TCP4_OPTION Option;           ///<  TCP4 port options
  BOOLEAN bConfigured;              ///<  TRUE if configuration was successful

  //
  //  Tokens
  //
  EFI_TCP4_LISTEN_TOKEN ListenToken;  ///<  Listen control
  EFI_TCP4_CONNECTION_TOKEN ConnectToken; ///<  Connection control
  EFI_TCP4_CLOSE_TOKEN CloseToken;    ///<  Close control

  //
  //  Receive data management
  //
  EFI_TCP4_IO_TOKEN RxToken;        ///<  Receive token
  DT_PACKET * pReceivePending;      ///<  Receive operation in progress

  //
  //  Transmit data management
  //
  EFI_TCP4_IO_TOKEN TxOobToken;     ///<  Urgent data token
  DT_PACKET * pTxOobPacket;         ///<  Urgent data in progress

  EFI_TCP4_IO_TOKEN TxToken;        ///<  Normal data token
  DT_PACKET * pTxPacket;            ///<  Normal transmit in progress
} DT_TCP4_CONTEXT;

/**
  UDP4 context structure

  The driver uses this structure to manage the UDP4 connections.
**/
typedef struct {
  //
  //  UDP4 context
  //
  EFI_HANDLE Handle;                ///<  UDP4 port handle
  EFI_UDP4_PROTOCOL * pProtocol;    ///<  UDP4 protocol pointer
  EFI_UDP4_CONFIG_DATA ConfigData;  ///<  UDP4 configuration data
  BOOLEAN bConfigured;              ///<  TRUE if configuration was successful

  //
  //  Receive data management
  //
  EFI_UDP4_COMPLETION_TOKEN RxToken;///<  Receive token
  DT_PACKET * pReceivePending;      ///<  Receive operation in progress

  //
  //  Transmit data management
  //
  EFI_UDP4_COMPLETION_TOKEN TxToken;///<  Transmit token
  DT_PACKET * pTxPacket;            ///<  Transmit in progress
} DT_UDP4_CONTEXT;


/**
  Port control structure

  The driver uses this structure to manager the socket's connection
  with the network driver.
**/
typedef struct _DT_PORT {
  UINTN Signature;              ///<  Structure identification

  //
  //  List links
  //
  DT_PORT * pLinkService;       ///<  Link in service port list
  DT_PORT * pLinkSocket;        ///<  Link in socket port list

  //
  //  Structures
  //
  DT_SERVICE * pService;        ///<  Service for this port
  DT_SOCKET * pSocket;          ///<  Socket for this port
//  PFN_CLOSE_PORT pfnClosePort;  ///<  Routine to immediately close the port
  PFN_PORT_CLOSE_START * pfnCloseStart; ///<  Routine to start closing the port

  //
  //  Protocol specific management data
  //
  PORT_STATE State;             ///<  State of the port
  UINTN DebugFlags;             ///<  Debug flags used to close the port
  BOOLEAN bCloseNow;            ///<  TRUE = Close the port immediately
  
  union {
    DT_TCP4_CONTEXT Tcp4;       ///<  TCPv4 management data
    DT_UDP4_CONTEXT Udp4;       ///<  UDPv4 management data
  } Context;
}GCC_DT_PORT;

/**
  Socket control structure

  The driver uses this structure to manage the socket.
**/
typedef struct _DT_SOCKET {
  UINTN Signature;          ///<  Structure identification

  //
  //  Protocol binding
  //
  EFI_SOCKET_PROTOCOL SocketProtocol; ///<  Socket protocol declaration

  //
  //  Socket management
  //
  DT_SOCKET * pNext;            ///<  Next socket in the list of sockets
  int errno;                    ///<  Error information for this socket
  EFI_STATUS Status;            ///<  Asyncronous error information for this socket
  SOCKET_STATE State;           ///<  Socket state

  //
  //  Socket data
  //
  int Domain;                   ///<  Specifies family of protocols
  int Type;                     ///<  Specifies how to make network connection
  int Protocol;                 ///<  Specifies lower layer protocol to use
  BOOLEAN bConfigured;          ///<  Set after the socket is configured

  BOOLEAN bRxDisable;           ///<  Receive disabled via shutdown
  size_t RxBytes;               ///<  Total Rx bytes
  size_t RxOobBytes;            ///<  Urgent Rx bytes
  EFI_STATUS RxError;           ///<  Error during receive

  BOOLEAN bTxDisable;           ///<  Transmit disabled via shutdown
  size_t TxBytes;               ///<  Normal Tx bytes
  size_t TxOobBytes;            ///<  Urgent Tx bytes
  EFI_STATUS TxError;           ///<  Error during transmit

  //
  //  Pending connection data
  //
  BOOLEAN bConnected;           ///<  Set when connected, cleared by poll
  EFI_STATUS ConnectStatus;     ///<  Connection status
  UINTN MaxFifoDepth;           ///<  Maximum FIFO depth
  UINTN FifoDepth;              ///<  Number of sockets in the FIFO
  DT_SOCKET * pFifoHead;        ///<  Head of the FIFO
  DT_SOCKET * pFifoTail;        ///<  Tail of the FIFO
  DT_SOCKET * pNextConnection;  ///<  Link in the FIFO

  //
  //  Network use
  //
  DT_PORT * pPortList;          ///<  List of ports managed by this socket
  EFI_EVENT WaitAccept;         ///<  Wait for accept completion

  //
  //  Receive data management
  //
  UINT32 MaxRxBuf;                  ///<  Maximum size of the receive buffer
  struct timeval RxTimeout;         ///<  Receive timeout
  DT_PACKET * pRxFree;              ///<  Free packet list
  DT_PACKET * pRxOobPacketListHead; ///<  Urgent data list head
  DT_PACKET * pRxOobPacketListTail; ///<  Urgent data list tail
  DT_PACKET * pRxPacketListHead;    ///<  Normal data list head
  DT_PACKET * pRxPacketListTail;    ///<  Normal data list tail

  //
  //  Transmit data management
  //
  UINT32 MaxTxBuf;                  ///<  Maximum size of the transmit buffer
  DT_PACKET * pTxOobPacketListHead; ///<  Urgent data list head
  DT_PACKET * pTxOobPacketListTail; ///<  Urgent data list tail
  DT_PACKET * pTxPacketListHead;    ///<  Normal data list head
  DT_PACKET * pTxPacketListTail;    ///<  Normal data list tail
}GCC_DT_SOCKET;

#define SOCKET_FROM_PROTOCOL(a)  CR(a, DT_SOCKET, SocketProtocol, SOCKET_SIGNATURE) ///< Locate DT_SOCKET from protocol

/**
  Socket layer control structure

  The driver uses this structure to manage the driver.
**/
typedef struct {
  UINTN Signature;              ///<  Structure identification

  //
  //  Service binding interface
  //
  EFI_SERVICE_BINDING_PROTOCOL ServiceBinding;///<  Driver's binding

  //
  //  Image data
  //
  EFI_HANDLE ImageHandle;       ///<  Image handle

  //
  //  Network services
  //
  DT_SERVICE * pTcp4List;       ///<  List of Tcp4 services
  DT_SERVICE * pUdp4List;       ///<  List of Udp4 services

  //
  //  Socket management
  //
  DT_SOCKET * pSocketList;      ///<  List of sockets
  
  //
  //  TCP4 service
  //
  UINTN TcpCloseMax4;           ///<  Number of entries in the ring buffer
  UINTN TcpCloseIn4;            ///<  Offset into TcpClose4 ring buffer - Close request
  UINTN TcpCloseOut4;           ///<  Offset into TcpClose4 ring buffer - Close operation
  EFI_TCP4_PROTOCOL ** ppTcpClose4; ///<  Ring buffer to close TCP4 ports
} DT_LAYER;

#define LAYER_FROM_SERVICE(a) CR(a, DT_LAYER, ServiceBinding, LAYER_SIGNATURE) ///< Locate DT_LAYER from service binding

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------

extern DT_LAYER mEslLayer;

//------------------------------------------------------------------------------
// Socket Support Routines
//------------------------------------------------------------------------------

/**
  Allocate and initialize a DT_SOCKET structure.
  
  The ::SocketAllocate() function allocates a DT_SOCKET structure
  and installs a protocol on ChildHandle.  If pChildHandle is a
  pointer to NULL, then a new handle is created and returned in
  pChildHandle.  If pChildHandle is not a pointer to NULL, then
  the protocol installs on the existing pChildHandle.

  @param [in, out] pChildHandle Pointer to the handle of the child to create.
                                If it is NULL, then a new handle is created.
                                If it is a pointer to an existing UEFI handle, 
                                then the protocol is added to the existing UEFI
                                handle.
  @param [in] DebugFlags        Flags for debug messages
  @param [in, out] ppSocket     The buffer to receive the DT_SOCKET structure address.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child
  @retval other                 The child handle was not created
  
**/
EFI_STATUS
EFIAPI
EslSocketAllocate (
  IN OUT EFI_HANDLE * pChildHandle,
  IN     UINTN DebugFlags,
  IN OUT DT_SOCKET ** ppSocket
  );

/**
  Allocate a packet for a receive or transmit operation

  @param [in] ppPacket      Address to receive the DT_PACKET structure
  @param [in] LengthInBytes Length of the packet structure
  @param [in] DebugFlags    Flags for debug messages

  @retval EFI_SUCCESS - The packet was allocated successfully

 **/
EFI_STATUS
EslSocketPacketAllocate (
  IN DT_PACKET ** ppPacket,
  IN size_t LengthInBytes,
  IN UINTN DebugFlags
  );

/**
  Free a packet used for receive or transmit operation

  @param [in] pPacket     Address of the DT_PACKET structure
  @param [in] DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS - The packet was allocated successfully

 **/
EFI_STATUS
EslSocketPacketFree (
  IN DT_PACKET * pPacket,
  IN UINTN DebugFlags
  );

//------------------------------------------------------------------------------
// Tcp4 Routines
//------------------------------------------------------------------------------

/**
  Accept a network connection.

  The SocketAccept routine waits for a network connection to the socket.
  It is able to return the remote network address to the caller if
  requested.

  @param [in] pSocket   Address of the socket structure.

  @param [in] pSockAddr       Address of a buffer to receive the remote
                              network address.

  @param [in, out] pSockAddrLength  Length in bytes of the address buffer.
                                    On output specifies the length of the
                                    remote network address.

  @retval EFI_SUCCESS   Remote address is available
  @retval Others        Remote address not available

 **/
EFI_STATUS
EslTcpAccept4 (
  IN DT_SOCKET * pSocket,
  IN struct sockaddr * pSockAddr,
  IN OUT socklen_t * pSockAddrLength
  );

/**
  Bind a name to a socket.

  The ::TcpBind4 routine connects a name to A TCP4 stack on the local machine.

  @param [in] pSocket   Address of the socket structure.

  @param [in] pSockAddr Address of a sockaddr structure that contains the
                        connection point on the local machine.  An IPv4 address
                        of INADDR_ANY specifies that the connection is made to
                        all of the network stacks on the platform.  Specifying a
                        specific IPv4 address restricts the connection to the
                        network stack supporting that address.  Specifying zero
                        for the port causes the network layer to assign a port
                        number from the dynamic range.  Specifying a specific
                        port number causes the network layer to use that port.

  @param [in] SockAddrLen   Specifies the length in bytes of the sockaddr structure.

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslTcpBind4 (
  IN DT_SOCKET * pSocket,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  );

/**
  Poll for completion of the connection attempt.

  The ::TcpConnectPoll4 routine determines when the connection
  attempt transitions from being in process to being complete.

  @param [in] pSocket         Address of the socket structure.

  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslTcpConnectPoll4 (
  IN DT_SOCKET * pSocket
  );

/**
  Connect to a remote system via the network.

  The ::TcpConnectStart4= routine starts the connection processing
  for a TCP4 port.

  @param [in] pSocket         Address of the socket structure.

  @param [in] pSockAddr       Network address of the remote system.
    
  @param [in] SockAddrLength  Length in bytes of the network address.
  
  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslTcpConnectStart4 (
  IN DT_SOCKET * pSocket,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  );

/**
  Initialize the TCP4 service.

  This routine initializes the TCP4 service after its service binding
  protocol was located on a controller.

  @param [in] pService             DT_SERVICE structure address

  @retval EFI_SUCCESS          The service was properly initialized
  @retval other                A failure occurred during the service initialization

**/
EFI_STATUS
EFIAPI
EslTcpInitialize4 (
  IN DT_SERVICE * pService
  );

/**
  Get the local socket address

  @param [in] pSocket             Address of the socket structure.

  @param [out] pAddress           Network address to receive the local system address

  @param [in,out] pAddressLength  Length of the local network address structure

  @retval EFI_SUCCESS - Address available
  @retval Other - Failed to get the address

**/
EFI_STATUS
EslTcpGetLocalAddress4 (
  IN DT_SOCKET * pSocket,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  );

/**
  Get the remote socket address

  @param [in] pSocket             Address of the socket structure.

  @param [out] pAddress           Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @retval EFI_SUCCESS - Address available
  @retval Other - Failed to get the address

**/
EFI_STATUS
EslTcpGetRemoteAddress4 (
  IN DT_SOCKET * pSocket,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  );

/**
  Establish the known port to listen for network connections.

  The ::Tcp4Listen routine places the port into a state that enables connection
  attempts.  Connections are placed into FIFO order in a queue to be serviced
  by the application.  The application calls the ::Tcp4Accept routine to remove
  the next connection from the queue and get the associated socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html">POSIX</a>
  documentation for the listen routine is available online for reference.

  @param [in] pSocket     Address of the socket structure.
  
  @retval EFI_SUCCESS - Socket successfully created
  @retval Other - Failed to enable the socket for listen

**/
EFI_STATUS
EslTcpListen4 (
  IN DT_SOCKET * pSocket
  );

/**
  Process the connection attempt

  A system has initiated a connection attempt with a socket in the
  listen state.  Attempt to complete the connection.

  @param  Event         The listeen completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpListenComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Allocate and initialize a DT_PORT structure.

  @param [in] pSocket     Address of the socket structure.
  @param [in] pService    Address of the DT_SERVICE structure.
  @param [in] ChildHandle TCP4 child handle
  @param [in] pIpAddress  Buffer containing IP4 network address of the local host
  @param [in] PortNumber  Tcp4 port number
  @param [in] DebugFlags  Flags for debug messages
  @param [out] ppPort     Buffer to receive new DT_PORT structure address

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslTcpPortAllocate4 (
  IN DT_SOCKET * pSocket,
  IN DT_SERVICE * pService,
  IN EFI_HANDLE ChildHandle,
  IN CONST UINT8 *pIpAddress,
  IN UINT16 PortNumber,
  IN UINTN DebugFlags,
  OUT DT_PORT ** ppPort
  );

/**
  Close a TCP4 port.

  This routine releases the resources allocated by
  ::TcpPortAllocate4().
  
  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS     The port is closed
  @retval other           Port close error

**/
EFI_STATUS
EslTcpPortClose4 (
  IN DT_PORT * pPort
  );

/**
  Process the port close completion

  @param  Event         The close completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpPortCloseComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Port close state 3

  Continue the close operation after the receive is complete.

  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS         The port is closed
  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslTcpPortCloseRxDone4 (
  IN DT_PORT * pPort
  );

/**
  Start the close operation on a TCP4 port.

  @param [in] pPort       Address of the port structure.
  @param [in] bAbort      Set TRUE to abort active transfers
  @param [in] DebugFlags  Flags for debug messages

**/
EFI_STATUS
EslTcpPortCloseStart4 (
  IN DT_PORT * pPort,
  IN BOOLEAN bAbort,
  IN UINTN DebugFlags
  );

/**
  Port close state 2

  Continue the close operation after the transmission is complete.

  @param [in] pPort       Address of the port structure.

  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslTcpPortCloseTxDone4 (
  IN DT_PORT * pPort
  );

/**
  Receive data from a network connection.


  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslTcpReceive4 (
  IN DT_SOCKET * pSocket,
  IN INT32 Flags,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  );

/**
  Cancel the receive operations

  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @retval EFI_SUCCESS - The cancel was successful

 **/
EFI_STATUS
EslTcpRxCancel4 (
  IN DT_SOCKET * pSocket
  );

/**
  Process the receive completion

  Buffer the data that was just received.

  @param  Event         The receive completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpRxComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Start a receive operation

  @param [in] pPort       Address of the DT_PORT structure.

 **/
VOID
EslTcpRxStart4 (
  IN DT_PORT * pPort
  );

/**
  Shutdown the TCP4 service.

  This routine undoes the work performed by ::TcpInitialize4.

  @param [in] pService             DT_SERVICE structure address

**/
VOID
EFIAPI
EslTcpShutdown4 (
  IN DT_SERVICE * pService
  );

/**
  Determine if the socket is configured.


  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @retval EFI_SUCCESS - The port is connected
  @retval EFI_NOT_STARTED - The port is not connected

 **/
 EFI_STATUS
 EslTcpSocketIsConfigured4 (
  IN DT_SOCKET * pSocket
  );

/**
  Buffer data for transmission over a network connection.

  This routine is called by the socket layer API to buffer
  data for transmission.  When necessary, this routine will
  start the transmit engine that performs the data transmission
  on the network connection.

  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @retval EFI_SUCCESS - Socket data successfully buffered

 **/
EFI_STATUS
EslTcpTxBuffer4 (
  IN DT_SOCKET * pSocket,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength
  );

/**
  Process the normal data transmit completion

  @param  Event         The normal transmit completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpTxComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Process the urgent data transmit completion

  @param  Event         The urgent transmit completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslTcpTxOobComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Transmit data using a network connection.


  @param [in] pPort           Address of a DT_PORT structure
  @param [in] pToken          Address of either the OOB or normal transmit token
  @param [in] ppQueueHead     Transmit queue head address
  @param [in] ppQueueTail     Transmit queue tail address
  @param [in] ppPacket        Active transmit packet address

 **/
VOID
EslTcpTxStart4 (
  IN DT_PORT * pPort,
  IN EFI_TCP4_IO_TOKEN * pToken,
  IN DT_PACKET ** ppQueueHead,
  IN DT_PACKET ** ppQueueTail,
  IN DT_PACKET ** ppPacket
  );

//------------------------------------------------------------------------------
// Udp4 Routines
//------------------------------------------------------------------------------

/**
  Bind a name to a socket.

  The ::UdpBind4 routine connects a name to a UDP4 stack on the local machine.

  @param [in] pSocket   Address of the socket structure.

  @param [in] pSockAddr Address of a sockaddr structure that contains the
                        connection point on the local machine.  An IPv4 address
                        of INADDR_ANY specifies that the connection is made to
                        all of the network stacks on the platform.  Specifying a
                        specific IPv4 address restricts the connection to the
                        network stack supporting that address.  Specifying zero
                        for the port causes the network layer to assign a port
                        number from the dynamic range.  Specifying a specific
                        port number causes the network layer to use that port.

  @param [in] SockAddrLen   Specifies the length in bytes of the sockaddr structure.

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslUdpBind4 (
  IN DT_SOCKET * pSocket,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  );

/**
  Initialize the UDP4 service.

  This routine initializes the UDP4 service after its service binding
  protocol was located on a controller.

  @param [in] pService        DT_SERVICE structure address

  @retval EFI_SUCCESS         The service was properly initialized
  @retval other               A failure occurred during the service initialization

**/
EFI_STATUS
EFIAPI
EslUdpInitialize4 (
  IN DT_SERVICE * pService
  );

/**
  Allocate and initialize a DT_PORT structure.

  @param [in] pSocket     Address of the socket structure.
  @param [in] pService    Address of the DT_SERVICE structure.
  @param [in] ChildHandle Udp4 child handle
  @param [in] pIpAddress  Buffer containing IP4 network address of the local host
  @param [in] PortNumber  Udp4 port number
  @param [in] DebugFlags  Flags for debug messages
  @param [out] ppPort     Buffer to receive new DT_PORT structure address

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslUdpPortAllocate4 (
  IN DT_SOCKET * pSocket,
  IN DT_SERVICE * pService,
  IN EFI_HANDLE ChildHandle,
  IN CONST UINT8 * pIpAddress,
  IN UINT16 PortNumber,
  IN UINTN DebugFlags,
  OUT DT_PORT ** ppPort
  );

/**
  Close a UDP4 port.

  This routine releases the resources allocated by
  ::UdpPortAllocate4().
  
  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS     The port is closed
  @retval other           Port close error

**/
EFI_STATUS
EslUdpPortClose4 (
  IN DT_PORT * pPort
  );

/**
  Start the close operation on a UDP4 port, state 1.

  Closing a port goes through the following states:
  1. Port close starting - Mark the port as closing and wait for transmission to complete
  2. Port TX close done - Transmissions complete, close the port and abort the receives
  3. Port RX close done - Receive operations complete, close the port
  4. Port closed - Release the port resources
  
  @param [in] pPort       Address of the port structure.
  @param [in] bCloseNow   Set TRUE to abort active transfers
  @param [in] DebugFlags  Flags for debug messages

  @retval EFI_SUCCESS         The port is closed, not normally returned
  @retval EFI_NOT_READY       The port has started the closing process
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslUdpPortCloseStart4 (
  IN DT_PORT * pPort,
  IN BOOLEAN bCloseNow,
  IN UINTN DebugFlags
  );

/**
  Port close state 2

  Continue the close operation after the transmission is complete.

  @param [in] pPort       Address of the port structure.

  @retval EFI_SUCCESS         The port is closed, not normally returned
  @retval EFI_NOT_READY       The port is still closing
  @retval EFI_ALREADY_STARTED Error, the port is in the wrong state,
                              most likely the routine was called already.

**/
EFI_STATUS
EslUdpPortCloseTxDone4 (
  IN DT_PORT * pPort
  );

/**
  Connect to a remote system via the network.

  The ::UdpConnectStart4= routine sets the remote address for the connection.

  @param [in] pSocket         Address of the socket structure.

  @param [in] pSockAddr       Network address of the remote system.
    
  @param [in] SockAddrLength  Length in bytes of the network address.
  
  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslUdpConnect4 (
  IN DT_SOCKET * pSocket,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength
  );

/**
  Get the local socket address

  @param [in] pSocket             Address of the socket structure.

  @param [out] pAddress           Network address to receive the local system address

  @param [in,out] pAddressLength  Length of the local network address structure

  @retval EFI_SUCCESS - Address available
  @retval Other - Failed to get the address

**/
EFI_STATUS
EslUdpGetLocalAddress4 (
  IN DT_SOCKET * pSocket,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  );

/**
  Get the remote socket address

  @param [in] pSocket             Address of the socket structure.

  @param [out] pAddress           Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @retval EFI_SUCCESS - Address available
  @retval Other - Failed to get the address

**/
EFI_STATUS
EslUdpGetRemoteAddress4 (
  IN DT_SOCKET * pSocket,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  );

/**
  Receive data from a network connection.

  To minimize the number of buffer copies, the ::UdpRxComplete4
  routine queues the UDP4 driver's buffer to a list of datagrams
  waiting to be received.  The socket driver holds on to the
  buffers from the UDP4 driver until the application layer requests
  the data or the socket is closed.

  The application calls this routine in the socket layer to
  receive datagrams from one or more remote systems. This routine
  removes the next available datagram from the list of datagrams
  and copies the data from the UDP4 driver's buffer into the
  application's buffer.  The UDP4 driver's buffer is then returned.

  @param [in] pSocket         Address of a DT_SOCKET structure

  @param [in] Flags           Message control flags

  @param [in] BufferLength    Length of the the buffer

  @param [in] pBuffer         Address of a buffer to receive the data.

  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @retval EFI_SUCCESS - Socket data successfully received

**/
EFI_STATUS
EslUdpReceive4 (
  IN DT_SOCKET * pSocket,
  IN INT32 Flags,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength
  );

/**
  Cancel the receive operations

  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @retval EFI_SUCCESS - The cancel was successful

 **/
EFI_STATUS
EslUdpRxCancel4 (
  IN DT_SOCKET * pSocket
  );

/**
  Process the receive completion

  Keep the UDP4 driver's buffer and append it to the list of
  datagrams for the application to receive.  The UDP4 driver's
  buffer will be returned by either ::UdpReceive4 or
  ::UdpPortCloseTxDone4.

  @param  Event         The receive completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslUdpRxComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Start a receive operation

  @param [in] pPort       Address of the DT_PORT structure.

 **/
VOID
EslUdpRxStart4 (
  IN DT_PORT * pPort
  );

/**
  Determine if the socket is configured.


  @param [in] pSocket         Address of a DT_SOCKET structure
  
  @retval EFI_SUCCESS - The port is connected
  @retval EFI_NOT_STARTED - The port is not connected

 **/
 EFI_STATUS
 EslUdpSocketIsConfigured4 (
  IN DT_SOCKET * pSocket
  );

/**
  Process the transmit completion

  @param  Event         The normal transmit completion event

  @param  pPort         The DT_PORT structure address

**/
VOID
EslUdpTxComplete4 (
  IN EFI_EVENT Event,
  IN DT_PORT * pPort
  );

/**
  Shutdown the UDP4 service.

  This routine undoes the work performed by ::UdpInitialize4.

  @param [in] pService        DT_SERVICE structure address

**/
VOID
EFIAPI
EslUdpShutdown4 (
  IN DT_SERVICE * pService
  );

/**
  Buffer data for transmission over a network connection.

  This routine is called by the socket layer API to buffer
  data for transmission.  The data is copied into a local buffer
  freeing the application buffer for reuse upon return.  When
  necessary, this routine will start the transmit engine that
  performs the data transmission on the network connection.  The
  transmit engine transmits the data a packet at a time over the
  network connection.

  Transmission errors are returned during the next transmission or
  during the close operation.  Only buffering errors are returned
  during the current transmission attempt.

  @param [in] pSocket         Address of a DT_SOCKET structure

  @param [in] Flags           Message control flags

  @param [in] BufferLength    Length of the the buffer

  @param [in] pBuffer         Address of a buffer to receive the data.

  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [in] pAddress        Network address of the remote system address

  @param [in] AddressLength   Length of the remote network address structure

  @retval EFI_SUCCESS - Socket data successfully buffered

**/
EFI_STATUS
EslUdpTxBuffer4 (
  IN DT_SOCKET * pSocket,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength,
  IN const struct sockaddr * pAddress,
  IN socklen_t AddressLength
  );

/**
  Transmit data using a network connection.

  @param [in] pPort           Address of a DT_PORT structure

 **/
VOID
EslUdpTxStart4 (
  IN DT_PORT * pPort
  );

//------------------------------------------------------------------------------

#endif  //  _SOCKET_H_
