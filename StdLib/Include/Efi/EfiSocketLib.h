/** @file
  Definitions for the EFI Socket layer library.

  Copyright (c) 2011 - 2015, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_SOCKET_LIB_H_
#define _EFI_SOCKET_LIB_H_

#include <Uefi.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EfiSocket.h>
#include <Protocol/Ip4Config2.h>
#include <Protocol/Ip6Config.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Tcp6.h>
#include <Protocol/Udp4.h>
#include <Protocol/Udp6.h>

#include <sys/time.h>

//------------------------------------------------------------------------------
//  Constants
//------------------------------------------------------------------------------

#define DEBUG_TPL           0x40000000  ///<  Display TPL change messages

#define TPL_SOCKETS     TPL_CALLBACK    ///<  TPL for routine synchronization

//------------------------------------------------------------------------------
//  Macros
//------------------------------------------------------------------------------

#if defined(_MSC_VER)           /* Handle Microsoft VC++ compiler specifics. */
#define DBG_ENTER()             DEBUG (( DEBUG_INFO, "Entering " __FUNCTION__ "\n" )) ///<  Display routine entry
#define DBG_EXIT()              DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ "\n" ))  ///<  Display routine exit
#define DBG_EXIT_DEC(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %d\n", Status ))      ///<  Display routine exit with decimal value
#define DBG_EXIT_HEX(Status)    DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: 0x%08x\n", Status ))  ///<  Display routine exit with hex value
#define DBG_EXIT_STATUS(Status) DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", Status: %r\n", Status ))      ///<  Display routine exit with status value
#define DBG_EXIT_TF(Status)     DEBUG (( DEBUG_INFO, "Exiting " __FUNCTION__ ", returning %s\n", (FALSE == Status) ? L"FALSE" : L"TRUE" ))  ///<  Display routine with TRUE/FALSE value
#else   //  _MSC_VER
#define DBG_ENTER()               ///<  Display routine entry
#define DBG_EXIT()                ///<  Display routine exit
#define DBG_EXIT_DEC(Status)      ///<  Display routine exit with decimal value
#define DBG_EXIT_HEX(Status)      ///<  Display routine exit with hex value
#define DBG_EXIT_STATUS(Status)   ///<  Display routine exit with status value
#define DBG_EXIT_TF(Status)       ///<  Display routine with TRUE/FALSE value
#endif  //  _MSC_VER

#define DIM(x)    ( sizeof ( x ) / sizeof ( x[0] ))   ///<  Compute the number of entries in an array

/**
  Verify new TPL value

  This macro which is enabled when debug is enabled verifies that
  the new TPL value is >= the current TPL value.
**/
#ifdef VERIFY_TPL
#undef VERIFY_TPL
#endif  //  VERIFY_TPL

#if !defined(MDEPKG_NDEBUG)

/**
  Verify that the TPL is at the correct level
**/
#define VERIFY_AT_TPL(tpl)                           \
{                                                 \
  EFI_TPL PreviousTpl;                            \
                                                  \
  PreviousTpl = EfiGetCurrentTpl ( );             \
  if ( PreviousTpl != tpl ) {                     \
    DEBUG (( DEBUG_ERROR | DEBUG_TPL,             \
              "Current TPL: %d, New TPL: %d\r\n", \
              PreviousTpl, tpl ));                \
    ASSERT ( PreviousTpl == tpl );                \
  }                                               \
}

#define VERIFY_TPL(tpl)                           \
{                                                 \
  EFI_TPL PreviousTpl;                            \
                                                  \
  PreviousTpl = EfiGetCurrentTpl ( );             \
  if ( PreviousTpl > tpl ) {                      \
    DEBUG (( DEBUG_ERROR | DEBUG_TPL,             \
              "Current TPL: %d, New TPL: %d\r\n", \
              PreviousTpl, tpl ));                \
    ASSERT ( PreviousTpl <= tpl );                \
  }                                               \
}

#else   //  MDEPKG_NDEBUG

#define VERIFY_AT_TPL(tpl)    ///<  Verify that the TPL is at the correct level
#define VERIFY_TPL(tpl)       ///<  Verify that the TPL is at the correct level

#endif  //  MDEPKG_NDEBUG

/**
  Raise TPL to the specified level
**/
#define RAISE_TPL(PreviousTpl, tpl)     \
  VERIFY_TPL ( tpl );                   \
  PreviousTpl = gBS->RaiseTPL ( tpl );

/**
  Restore the TPL to the previous value
**/
#define RESTORE_TPL(tpl)            \
  gBS->RestoreTPL ( tpl )

//------------------------------------------------------------------------------
// Data Types
//------------------------------------------------------------------------------

typedef struct _ESL_SERVICE ESL_SERVICE;  ///<  Forward delcaration

/**
  Protocol binding and installation control structure

  The driver uses this structure to simplify the driver binding processing.
**/
typedef struct {
  CHAR16 * pName;                 ///<  Protocol name
  EFI_GUID * pNetworkBinding;     ///<  Network service binding protocol for socket support
  EFI_GUID * pNetworkProtocolGuid;///<  Network protocol GUID
  CONST EFI_GUID * pTagGuid;      ///<  Tag to mark protocol in use
  UINTN ServiceListOffset;        ///<  Offset in ::ESL_LAYER for the list of services
  UINTN RxIo;                     ///<  Number of receive ESL_IO_MGMT structures for data
  UINTN TxIoNormal;               ///<  Number of transmit ESL_IO_MGMT structures for normal data
  UINTN TxIoUrgent;               ///<  Number of transmit ESL_IO_MGMT structures for urgent data
} ESL_SOCKET_BINDING;

//------------------------------------------------------------------------------
// GUIDs
//------------------------------------------------------------------------------

extern CONST EFI_GUID mEslIp4ServiceGuid;   ///<  Tag GUID for the IPv4 layer
extern CONST EFI_GUID mEslIp6ServiceGuid;   ///<  Tag GUID for the IPv6 layer
extern CONST EFI_GUID mEslTcp4ServiceGuid;  ///<  Tag GUID for the TCPv4 layer
extern CONST EFI_GUID mEslTcp6ServiceGuid;  ///<  Tag GUID for the TCPv6 layer
extern CONST EFI_GUID mEslUdp4ServiceGuid;  ///<  Tag GUID for the UDPv4 layer
extern CONST EFI_GUID mEslUdp6ServiceGuid;  ///<  Tag GUID for the UDPv6 layer

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------

extern CONST ESL_SOCKET_BINDING cEslSocketBinding[];///<  List of network service bindings
extern CONST UINTN cEslSocketBindingEntries;        ///<  Number of network service bindings

//------------------------------------------------------------------------------
// DXE Support Routines
//------------------------------------------------------------------------------

/**
  Creates a child handle and installs a protocol.

  When the socket application is linked against UseSocketDxe, the ::socket
  routine indirectly calls this routine in SocketDxe to create a child
  handle if necessary and install the socket protocol on the handle.
  Upon return, EslServiceGetProtocol in UseSocketLib returns the
  ::EFI_SOCKET_PROTOCOL address to the socket routine.

  @param [in] pThis        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param [in] pChildHandle Pointer to the handle of the child to create. If it is NULL,
                           then a new handle is created. If it is a pointer to an existing UEFI handle, 
                           then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
EslDxeCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN OUT EFI_HANDLE * pChildHandle
  );

/**
  Destroys a child handle with a protocol installed on it.
  
  When the socket application is linked against UseSocketDxe, the ::close
  routine indirectly calls this routine in SocketDxe to undo the operations
  done by the ::EslDxeCreateChild routine.  This routine removes the socket
  protocol from the handle and then destroys the child handle if there are
  no other protocols attached.

  @param [in] pThis       Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param [in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is not a valid UEFI Handle.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
EslDxeDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN EFI_HANDLE ChildHandle
  );

/**
Install the socket service

SocketDxe uses this routine to announce the socket interface to
the rest of EFI.

@param [in] pImageHandle      Address of the image handle

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslDxeInstall (
  IN EFI_HANDLE * pImageHandle
  );

/**
Uninstall the socket service

SocketDxe uses this routine to notify EFI that the socket layer
is no longer available.

@param [in] ImageHandle       Handle for the image.

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslDxeUninstall (
  IN EFI_HANDLE ImageHandle
  );

//------------------------------------------------------------------------------
// Service Support Routines
//------------------------------------------------------------------------------

/**
  Connect to the network service bindings

  Walk the network service protocols on the controller handle and
  locate any that are not in use.  Create ::ESL_SERVICE structures to
  manage the network layer interfaces for the socket driver.  Tag
  each of the network interfaces that are being used.  Finally, this
  routine calls ESL_SOCKET_BINDING::pfnInitialize to prepare the network
  interface for use by the socket layer.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller       Handle of device to work with.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
EslServiceConnect (
  IN EFI_HANDLE BindingHandle,
  IN EFI_HANDLE Controller
  );

/**
  Shutdown the connections to the network layer by locating the
  tags on the network interfaces established by ::EslServiceConnect.
  This routine calls ESL_SOCKET_BINDING::pfnShutdown to shutdown the any
  activity on the network interface and then free the ::ESL_SERVICE
  structures.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller       Handle of device to stop driver on.

  @retval EFI_SUCCESS          This driver is removed Controller.
  @retval EFI_DEVICE_ERROR     The device could not be stopped due to a device error.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
EslServiceDisconnect (
  IN  EFI_HANDLE BindingHandle,
  IN  EFI_HANDLE Controller
  );

/**
Initialize the service layer

@param [in] ImageHandle       Handle for the image.

**/
VOID
EFIAPI
EslServiceLoad (
  IN EFI_HANDLE ImageHandle
  );

/**
  Shutdown the service layer

**/
VOID
EFIAPI
EslServiceUnload (
  VOID
  );

//------------------------------------------------------------------------------
// Socket Protocol Routines
//------------------------------------------------------------------------------

/**
  Bind a name to a socket.

  This routine calls the network specific layer to save the network
  address of the local connection point.

  The ::bind routine calls this routine to connect a name
  (network address and port) to a socket on the local machine.

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.

  @param [in] pSockAddr Address of a sockaddr structure that contains the
                        connection point on the local machine.  An IPv4 address
                        of INADDR_ANY specifies that the connection is made to
                        all of the network stacks on the platform.  Specifying a
                        specific IPv4 address restricts the connection to the
                        network stack supporting that address.  Specifying zero
                        for the port causes the network layer to assign a port
                        number from the dynamic range.  Specifying a specific
                        port number causes the network layer to use that port.

  @param [in] SockAddrLength  Specifies the length in bytes of the sockaddr structure.

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created

 **/
EFI_STATUS
EslSocketBind (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN CONST struct sockaddr * pSockAddr,
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket

**/
EFI_STATUS
EslSocketClosePoll (
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param [in] bCloseNow       Boolean to control close behavior
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS     Socket successfully closed
  @retval EFI_NOT_READY   Close still in progress
  @retval EFI_ALREADY     Close operation already in progress
  @retval Other           Failed to close the socket

**/
EFI_STATUS
EslSocketCloseStart (
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
  
  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.

  @param [in] pSockAddr       Network address of the remote system.
    
  @param [in] SockAddrLength  Length in bytes of the network address.
  
  @param [out] pErrno   Address to receive the errno value upon completion.
  
  @retval EFI_SUCCESS   The connection was successfully established.
  @retval EFI_NOT_READY The connection is in progress, call this routine again.
  @retval Others        The connection attempt failed.

 **/
EFI_STATUS
EslSocketConnect (
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  
  @param [out] pAddress       Network address to receive the local system address

  @param [in,out] pAddressLength  Length of the local network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Local address successfully returned

 **/
EFI_STATUS
EslSocketGetLocalAddress (
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  
  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Remote address successfully returned

 **/
EFI_STATUS
EslSocketGetPeerAddress (
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.

  @param [in] Backlog         Backlog specifies the maximum FIFO depth for
                              the connections waiting for the application
                              to call accept.  Connection attempts received
                              while the queue is full are refused.

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully created
  @retval Other - Failed to enable the socket for listen

**/
EFI_STATUS
EslSocketListen (
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

  @param [in] pSocketProtocol   Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param [in] level             Option protocol level
  @param [in] OptionName        Name of the option
  @param [out] pOptionValue     Buffer to receive the option value
  @param [in,out] pOptionLength Length of the buffer in bytes,
                                upon return length of the option value in bytes
  @param [out] pErrno           Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslSocketOptionGet (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int option_name,
  OUT void * __restrict option_value,
  IN OUT socklen_t * __restrict option_len,
  IN int * pErrno
  );

/**
  Set the socket options

  This routine handles the socket level options and passes the
  others to the network specific layer.

  The ::setsockopt routine calls this routine to adjust the socket
  options one at a time by name.

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  @param [in] level           Option protocol level
  @param [in] OptionName      Name of the option
  @param [in] pOptionValue    Buffer containing the option value
  @param [in] OptionLength    Length of the buffer in bytes
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Option successfully set

 **/
EFI_STATUS
EslSocketOptionSet (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int level,
  IN int option_name,
  IN CONST void * option_value,
  IN socklen_t option_len,
  IN int * pErrno
  );

/**
  Poll a socket for pending activity.

  This routine builds a detected event mask which is returned to
  the caller in the buffer provided.

  The ::poll routine calls this routine to determine if the socket
  needs to be serviced as a result of connection, error, receive or
  transmit activity.

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.

  @param [in] Events    Events of interest for this socket

  @param [in] pEvents   Address to receive the detected events

  @param [out] pErrno   Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket successfully polled
  @retval EFI_INVALID_PARAMETER - When pEvents is NULL

 **/
EFI_STATUS
EslSocketPoll (
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer to receive the data.
  
  @param [in] pDataLength     Number of received data bytes in the buffer.

  @param [out] pAddress       Network address to receive the remote system address

  @param [in,out] pAddressLength  Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

 **/
EFI_STATUS
EslSocketReceive (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Flags,
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  
  @param [in] How             Which operations to stop
  
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket operations successfully shutdown

 **/
EFI_STATUS
EslSocketShutdown (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int How,
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

  @param [in] pSocketProtocol Address of an ::EFI_SOCKET_PROTOCOL structure.
  
  @param [in] Flags           Message control flags
  
  @param [in] BufferLength    Length of the the buffer
  
  @param [in] pBuffer         Address of a buffer containing the data to send
  
  @param [in] pDataLength     Address to receive the number of data bytes sent

  @param [in] pAddress        Network address of the remote system address

  @param [in] AddressLength   Length of the remote network address structure

  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully queued for transmit

 **/
EFI_STATUS
EslSocketTransmit (
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

#endif  //  _EFI_SOCKET_LIB_H_
