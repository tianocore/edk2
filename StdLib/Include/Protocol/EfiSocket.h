/** @file
  Definitions for the EFI Socket protocol.

  Copyright (c) 2011, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_SOCKET_H_
#define _EFI_SOCKET_H_

#include <errno.h>
#include <Uefi.h>

#include <netinet/in.h>

#include <sys/poll.h>
#include <sys/socket.h>

//------------------------------------------------------------------------------
//  Data Types
//------------------------------------------------------------------------------

typedef struct _EFI_SOCKET_PROTOCOL EFI_SOCKET_PROTOCOL;

/**
  Constructor/Destructor

  @retval EFI_SUCCESS       The operation was successful

 **/
typedef
EFI_STATUS
(* PFN_ESL_xSTRUCTOR) (
  VOID
  );

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------

extern PFN_ESL_xSTRUCTOR mpfnEslConstructor;
extern PFN_ESL_xSTRUCTOR mpfnEslDestructor;

extern EFI_GUID  gEfiSocketProtocolGuid;
extern EFI_GUID  gEfiSocketServiceBindingProtocolGuid;

//------------------------------------------------------------------------------
//  Socket API
//------------------------------------------------------------------------------

/**
  Accept a network connection.

  The SocketAccept routine waits for a network connection to the socket.
  It is able to return the remote network address to the caller if
  requested.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] pSockAddr       Address of a buffer to receive the remote
                              network address.

  @param [in, out] pSockAddrLength  Length in bytes of the address buffer.
                                    On output specifies the length of the
                                    remote network address.

  @param [out] ppSocketProtocol Address of a buffer to receive the socket protocol
                                instance associated with the new socket.

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS   New connection successfully created
  @retval EFI_NOT_READY No connection is available

 **/
typedef
EFI_STATUS
(* PFN_ACCEPT) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN struct sockaddr * pSockAddr,
  IN OUT socklen_t * pSockAddrLength,
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol,
  IN int * pErrno
  );

/**
  Bind a name to a socket.

  The ::SocketBind routine connects a name to a socket on the local machine.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/bind.html">POSIX</a>
  documentation for the bind routine is available online for reference.

  @param [in] pSocketProtocol Address of the socket protocol structure.

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

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created

 **/
typedef
EFI_STATUS
(* PFN_BIND) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength,
  OUT int * pErrno
  );

/**
  Determine if the socket is closed

  Reverses the operations of the ::SocketAllocate() routine.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket

**/
typedef
EFI_STATUS
(* PFN_CLOSE_POLL) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  );

/**
  Start the close operation on the socket

  Start closing the socket by closing all of the ports.  Upon
  completion, the ::pfnClosePoll() routine finishes closing the
  socket.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] bCloseNow       Boolean to control close behavior
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket

**/
typedef
EFI_STATUS
(* PFN_CLOSE_START) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN BOOLEAN bCloseNow,
  IN int * pErrno
  );

/**
  Connect to a remote system via the network.

  The ::Connect routine attempts to establish a connection to a
  socket on the local or remote system using the specified address.
  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html">POSIX</a>
  documentation is available online.

  There are three states associated with a connection:
  <ul>
    <li>Not connected</li>
    <li>Connection in progress</li>
    <li>Connected</li>
  </ul>
  In the "Not connected" state, calls to ::connect start the connection
  processing and update the state to "Connection in progress".  During
  the "Connection in progress" state, connect polls for connection completion
  and moves the state to "Connected" after the connection is established.
  Note that these states are only visible when the file descriptor is marked
  with O_NONBLOCK.  Also, the POLL_WRITE bit is set when the connection
  completes and may be used by poll or select as an indicator to call
  connect again.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] pSockAddr       Network address of the remote system.
    
  @param [in] SockAddrLength  Length in bytes of the network address.
  
  @param [out] pErrno   Address to receive the errno value upon completion.
  
  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
typedef
EFI_STATUS
(* PFN_CONNECT) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN const struct sockaddr * pSockAddr,
  IN socklen_t SockAddrLength,
  IN int * pErrno
  );

/**
  Get the local address.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [out] pAddress       Network address to receive the local system address

  @param [in,out] pAddressLength  Length of the local network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Local address successfully returned

 **/
typedef
EFI_STATUS
(* PFN_GET_LOCAL) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  );

/**
  Get the peer address.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Remote address successfully returned

 **/
typedef
EFI_STATUS
(* PFN_GET_PEER) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  );

/**
  Establish the known port to listen for network connections.

  The ::SocketAisten routine places the port into a state that enables connection
  attempts.  Connections are placed into FIFO order in a queue to be serviced
  by the application.  The application calls the ::SocketAccept routine to remove
  the next connection from the queue and get the associated socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/listen.html">POSIX</a>
  documentation for the bind routine is available online for reference.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] Backlog         Backlog specifies the maximum FIFO depth for
                              the connections waiting for the application
                              to call accept.  Connection attempts received
                              while the queue is full are refused.

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval Other - Failed to enable the socket for listen

**/
typedef
EFI_STATUS
(* PFN_LISTEN) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Backlog,
  OUT int * pErrno
  );

/**
  Get the socket options

  Retrieve the socket options one at a time by name.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol   Address of the socket protocol structure.
  @param [in] level             Option protocol level
  @param [in] OptionName        Name of the option
  @param [out] pOptionValue     Buffer to receive the option value
  @param [in,out] pOptionLength Length of the buffer in bytes,
                                upon return length of the option value in bytes
  @param [out] pErrno           Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
typedef
EFI_STATUS
(* PFN_OPTION_GET) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int OptionName,
  OUT void * __restrict pOptionValue,
  IN OUT socklen_t * __restrict pOptionLength,
  IN int * pErrno
  );

/**
  Set the socket options

  Adjust the socket options one at a time by name.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/setsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] level           Option protocol level
  @param [in] OptionName      Name of the option
  @param [in] pOptionValue    Buffer containing the option value
  @param [in] OptionLength    Length of the buffer in bytes
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
typedef
EFI_STATUS
(* PFN_OPTION_SET) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int OptionName,
  IN CONST void * pOptionValue,
  IN socklen_t OptionLength,
  IN int * pErrno
  );

/**
  Poll a socket for pending activity.

  The SocketPoll routine checks a socket for pending activity associated
  with the event mask.  Activity is returned in the detected event buffer.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] Events    Events of interest for this socket

  @param [in] pEvents   Address to receive the detected events

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully polled
  @retval EFI_INVALID_PARAMETER - When pEvents is NULL

 **/
typedef
EFI_STATUS
(* PFN_POLL) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN short Events,
  IN short * pEvents,
  IN int * pErrno
  );

/**
  Receive data from a network connection.

  The ::recv routine waits for receive data from a remote network
  connection.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/recv.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
typedef
EFI_STATUS
(* PFN_RECEIVE) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int Flags,
  IN size_t BufferLength,
  IN UINT8 * pBuffer,
  OUT size_t * pDataLength,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  );

/**
  Send data using a network connection.

  The SocketTransmit routine queues the data for transmission to the
  remote network connection.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer containing the data to send
  
  @param [in] pDataLength     Address to receive the number of data bytes sent

  @param [in] pAddress        Network address of the remote system address

  @param [in] AddressLength   Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully queued for transmission

 **/
typedef
EFI_STATUS
(* PFN_SEND) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength,
  IN const struct sockaddr * pAddress,
  IN socklen_t AddressLength,
  IN int * pErrno
  );

/**
  Shutdown the socket receive and transmit operations

  The SocketShutdown routine stops the socket receive and transmit
  operations.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  
  @param [in] How             Which operations to stop
  
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket operations successfully shutdown

 **/
typedef
EFI_STATUS
(* PFN_SHUTDOWN) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int How,
  IN int * pErrno
  );

/**
  Initialize an endpoint for network communication.

  The ::Socket routine initializes the communication endpoint by providing
  the support for the socket library function ::socket.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/socket.html">POSIX</a>
  documentation for the socket routine is available online for reference.

  @param [in] pSocketProtocol Address of the socket protocol structure.

  @param [in] domain    Select the family of protocols for the client or server
                        application.

  @param [in] type      Specifies how to make the network connection.  The following values
                        are supported:
                        <ul>
                          <li>
                            SOCK_STREAM - Connect to TCP, provides a byte stream
                            that is manipluated by read, recv, send and write.
                          </li>
                          <li>
                            SOCK_SEQPACKET - Connect to TCP, provides sequenced packet stream
                            that is manipulated by read, recv, send and write.
                          </li>
                          <li>
                            SOCK_DGRAM - Connect to UDP, provides a datagram service that is
                            manipulated by recvfrom and sendto.
                          </li>
                        </ul>

  @param [in] protocol  Specifies the lower layer protocol to use.  The following
                        values are supported:
                        <ul>
                          <li>IPPROTO_TCP</li> - This value must be combined with SOCK_STREAM.</li>
                          <li>IPPROTO_UDP</li> - This value must be combined with SOCK_DGRAM.</li>
                        </ul>

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval EFI_INVALID_PARAMETER - Invalid domain value, errno = EAFNOSUPPORT
  @retval EFI_INVALID_PARAMETER - Invalid type value, errno = EINVAL
  @retval EFI_INVALID_PARAMETER - Invalid protocol value, errno = EINVAL

 **/
typedef
EFI_STATUS
(*PFN_SOCKET) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int domain,
  IN int type,
  IN int protocol,
  IN int * pErrno
  );

//------------------------------------------------------------------------------
//  Socket Protocol
//------------------------------------------------------------------------------

/**
  Socket protocol declaration
**/
typedef struct _EFI_SOCKET_PROTOCOL {
  EFI_HANDLE SocketHandle;        ///<  Handle for the socket
  PFN_ACCEPT pfnAccept;           ///<  Accept a network connection
  PFN_BIND pfnBind;               ///<  Bind a local address to the socket
  PFN_CLOSE_POLL pfnClosePoll;    ///<  Determine if the socket is closed
  PFN_CLOSE_START pfnCloseStart;  ///<  Start the close operation
  PFN_CONNECT pfnConnect;         ///<  Connect to a remote system
  PFN_GET_LOCAL pfnGetLocal;      ///<  Get local address
  PFN_GET_PEER pfnGetPeer;        ///<  Get peer address
  PFN_LISTEN pfnListen;           ///<  Enable connection attempts on known port
  PFN_POLL pfnPoll;               ///<  Poll for socket activity
  PFN_OPTION_GET pfnOptionGet;    ///<  Get socket options
  PFN_OPTION_SET pfnOptionSet;    ///<  Set socket options
  PFN_RECEIVE pfnReceive;         ///<  Receive data from a socket
  PFN_SEND pfnSend;               ///<  Transmit data using the socket
  PFN_SHUTDOWN pfnShutdown;       ///<  Shutdown receive and transmit operations
  PFN_SOCKET pfnSocket;           ///<  Initialize the socket
} GCC_EFI_SOCKET_PROTOCOL;

//------------------------------------------------------------------------------
//  Non-blocking routines
//------------------------------------------------------------------------------

/**
  Non blocking version of accept.

  See ::accept

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] address   Address of a buffer to receive the remote network address.

  @param [in, out] address_len  Address of a buffer containing the Length in bytes
                                of the remote network address buffer.  Upon return,
                                contains the length of the remote network address.

  @returns    This routine returns zero if successful and -1 when an error occurs.
              In the case of an error, errno contains more details.

 **/
int
AcceptNB (
  int s,
  struct sockaddr * address,
  socklen_t * address_len
  );

/**
  Connect to the socket driver

  @param [in] ppSocketProtocol  Address to receive the socket protocol address

  @retval 0             Successfully returned the socket protocol
  @retval other         Value for errno
 **/
int
EslServiceGetProtocol (
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol
  );

//------------------------------------------------------------------------------

#endif  //  _EFI_SOCKET_H_
