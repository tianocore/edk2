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

/**
EfiSocketLib (SocketDxe) interface 
**/
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

extern PFN_ESL_xSTRUCTOR mpfnEslConstructor;  ///<  Constructor address for EslSocketLib
extern PFN_ESL_xSTRUCTOR mpfnEslDestructor;   ///<  Destructor address for EslSocketLib

extern EFI_GUID  gEfiSocketProtocolGuid;      ///<  Socket protocol GUID
extern EFI_GUID  gEfiSocketServiceBindingProtocolGuid;  ///<  Socket layer service binding protocol GUID

//------------------------------------------------------------------------------
//  Socket API
//------------------------------------------------------------------------------

/**
  Accept a network connection.

  This routine calls the network specific layer to remove the next
  connection from the FIFO.

  The ::accept calls this routine to poll for a network
  connection to the socket.  When a connection is available
  this routine returns the ::EFI_SOCKET_PROTOCOL structure address
  associated with the new socket and the remote network address
  if requested.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.

  @param [in] pSockAddr       Address of a buffer to receive the remote
                              network address.

  @param [in, out] pSockAddrLength  Length in bytes of the address buffer.
                                    On output specifies the length of the
                                    remote network address.

  @param [out] ppSocketProtocol Address of a buffer to receive the
                                ::EFI_SOCKET_PROTOCOL instance
                                associated with the new socket.

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

  This routine calls the network specific layer to save the network
  address of the local connection point.

  The ::bind routine calls this routine to connect a name
  (network address and port) to a socket on the local machine.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.

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

  This routine checks the state of the socket to determine if
  the network specific layer has completed the close operation.

  The ::close routine polls this routine to determine when the
  close operation is complete.  The close operation needs to
  reverse the operations of the ::EslSocketAllocate routine.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
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

  This routine calls the network specific layer to initiate the
  close state machine.  This routine then calls the network
  specific layer to determine if the close state machine has gone
  to completion.  The result from this poll is returned to the
  caller.

  The ::close routine calls this routine to start the close
  operation which reverses the operations of the
  ::EslSocketAllocate routine.  The close routine then polls
  the ::EslSocketClosePoll routine to determine when the
  socket is closed.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
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

  This routine calls the network specific layer to establish
  the remote system address and establish the connection to
  the remote system.

  The ::connect routine calls this routine to establish a
  connection with the specified remote system.  This routine
  is designed to be polled by the connect routine for completion
  of the network connection.
  
  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.

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

  This routine calls the network specific layer to get the network
  address of the local host connection point.

  The ::getsockname routine calls this routine to obtain the network
  address associated with the local host connection point.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
  
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

  This routine calls the network specific layer to get the remote
  system connection point.

  The ::getpeername routine calls this routine to obtain the network
  address of the remote connection point.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
  
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

  This routine calls into the network protocol layer to establish
  a handler that is called upon connection completion.  The handler
  is responsible for inserting the connection into the FIFO.

  The ::listen routine indirectly calls this routine to place the
  socket into a state that enables connection attempts.  Connections
  are placed in a FIFO that is serviced by the application.  The
  application calls the ::accept (::EslSocketAccept) routine to
  remove the next connection from the FIFO and get the associated
  socket and address.

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

  This routine handles the socket level options and passes the
  others to the network specific layer.

  The ::getsockopt routine calls this routine to retrieve the
  socket options one at a time by name.

  @param [in] pSocketProtocol   Address of the ::EFI_SOCKET_PROTOCOL structure.
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

  This routine handles the socket level options and passes the
  others to the network specific layer.

  The ::setsockopt routine calls this routine to adjust the socket
  options one at a time by name.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
  @param [in] level           Option protocol level
  @param [in] OptionName      Name of the option
  @param [in] pOptionValue    Buffer containing the option value
  @param [in] OptionLength    Length of the buffer in bytes
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Option successfully set

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

  This routine builds a detected event mask which is returned to
  the caller in the buffer provided.

  The ::poll routine calls this routine to determine if the socket
  needs to be serviced as a result of connection, error, receive or
  transmit activity.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.

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

  This routine calls the network specific routine to remove the
  next portion of data from the receive queue and return it to the
  caller.

  The ::recvfrom routine calls this routine to determine if any data
  is received from the remote system.  Note that the other routines
  ::recv and ::read are layered on top of ::recvfrom.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
  
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
  Shutdown the socket receive and transmit operations

  This routine sets a flag to stop future transmissions and calls
  the network specific layer to cancel the pending receive operation.

  The ::shutdown routine calls this routine to stop receive and transmit
  operations on the socket.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
  
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

  This routine initializes the communication endpoint.

  The ::socket routine calls this routine indirectly to create
  the communication endpoint.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] domain    Select the family of protocols for the client or server
                        application.  See the ::socket documentation for values.
  @param [in] type      Specifies how to make the network connection.
                        See the ::socket documentation for values.
  @param [in] protocol  Specifies the lower layer protocol to use.
                        See the ::socket documentation for values.
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

/**
  Send data using a network connection.

  This routine calls the network specific layer to queue the data
  for transmission.  Eventually the buffer will reach the head of
  the queue and will get transmitted over the network.  For datagram
  sockets there is no guarantee that the data reaches the application
  running on the remote system.

  The ::sendto routine calls this routine to send data to the remote
  system.  Note that ::send and ::write are layered on top of ::sendto.

  @param [in] pSocketProtocol Address of the ::EFI_SOCKET_PROTOCOL structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer containing the data to send
  
  @param [in] pDataLength     Address to receive the number of data bytes sent

  @param [in] pAddress        Network address of the remote system address

  @param [in] AddressLength   Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully queued for transmit

 **/
typedef
EFI_STATUS
(* PFN_TRANSMIT) (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int Flags,
  IN size_t BufferLength,
  IN CONST UINT8 * pBuffer,
  OUT size_t * pDataLength,
  IN const struct sockaddr * pAddress,
  IN socklen_t AddressLength,
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
  PFN_OPTION_GET pfnOptionGet;    ///<  Get socket options
  PFN_OPTION_SET pfnOptionSet;    ///<  Set socket options
  PFN_POLL pfnPoll;               ///<  Poll for socket activity
  PFN_RECEIVE pfnReceive;         ///<  Receive data from a socket
  PFN_SHUTDOWN pfnShutdown;       ///<  Shutdown receive and transmit operations
  PFN_SOCKET pfnSocket;           ///<  Initialize the socket
  PFN_TRANSMIT pfnTransmit;       ///<  Transmit data using the socket
} GCC_EFI_SOCKET_PROTOCOL;

//------------------------------------------------------------------------------
//  Non-blocking routines
//------------------------------------------------------------------------------

/**
  Non blocking version of ::accept.

  @param [in] s         Socket file descriptor returned from ::socket.

  @param [in] address   Address of a buffer to receive the remote network address.

  @param [in, out] address_len  Address of a buffer containing the Length in bytes
                                of the remote network address buffer.  Upon return,
                                contains the length of the remote network address.

  @return     This routine returns zero if successful and -1 when an error occurs.
              In the case of an error, ::errno contains more details.

 **/
int
AcceptNB (
  int s,
  struct sockaddr * address,
  socklen_t * address_len
  );

/**
  Free the socket resources

  This releases the socket resources allocated by calling
  EslServiceGetProtocol.

  This routine is called from the ::close routine in BsdSocketLib
  to release the socket resources.

  @param [in] pSocketProtocol   Address of an ::EFI_SOCKET_PROTOCOL
                                structure

  @return       Value for ::errno, zero (0) indicates success.

 **/
int
EslServiceFreeProtocol (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol
  );

/**
  Connect to the EFI socket library

  @param [in] ppSocketProtocol  Address to receive the socket protocol address

  @return       Value for ::errno, zero (0) indicates success.

 **/
int
EslServiceGetProtocol (
  IN EFI_SOCKET_PROTOCOL ** ppSocketProtocol
  );

//------------------------------------------------------------------------------

#endif  //  _EFI_SOCKET_H_
