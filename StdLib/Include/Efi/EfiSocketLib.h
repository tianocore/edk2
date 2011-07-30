/** @file
  Definitions for the EFI Socket layer library.

  Copyright (c) 2011, Intel Corporation
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
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/EfiSocket.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Tcp4.h>
#include <Protocol/Udp4.h>

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
#define DBG_ENTER()
#define DBG_EXIT()
#define DBG_EXIT_DEC(Status)
#define DBG_EXIT_HEX(Status)
#define DBG_EXIT_STATUS(Status)
#define DBG_EXIT_TF(Status)
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

#define VERIFY_TPL(tpl)                           \
{                                                 \
  EFI_TPL PreviousTpl;                            \
                                                  \
  PreviousTpl = gBS->RaiseTPL ( TPL_HIGH_LEVEL ); \
  gBS->RestoreTPL ( PreviousTpl );                \
  if ( PreviousTpl > tpl ) {                      \
    DEBUG (( DEBUG_ERROR | DEBUG_TPL,             \
              "Current TPL: %d, New TPL: %d\r\n", \
              PreviousTpl, tpl ));                \
    ASSERT ( PreviousTpl <= tpl );                \
  }                                               \
}

#else   //  MDEPKG_NDEBUG

#define VERIFY_TPL(tpl)

#endif  //  MDEPKG_NDEBUG

#define RAISE_TPL(PreviousTpl, tpl)     \
  VERIFY_TPL ( tpl );                   \
  PreviousTpl = gBS->RaiseTPL ( tpl );  \
  DEBUG (( DEBUG_TPL | DEBUG_TPL,       \
          "%d: TPL\r\n",                \
          tpl ))

#define RESTORE_TPL(tpl)            \
  gBS->RestoreTPL ( tpl );          \
  DEBUG (( DEBUG_TPL | DEBUG_TPL,   \
          "%d: TPL\r\n",            \
          tpl ))

//------------------------------------------------------------------------------
// Data Types
//------------------------------------------------------------------------------

typedef struct _DT_SERVICE DT_SERVICE;  ///<  Forward delcaration

typedef
EFI_STATUS
(* PFN_SB_INITIALIZE) (
    DT_SERVICE * pService
    );

typedef
VOID
(* PFN_SB_SHUTDOWN) (
    DT_SERVICE * pService
    );

/**
  Protocol binding and installation control structure

  The driver uses this structure to simplify the driver binding processing.
**/
typedef struct {
  CHAR16 * pName;                 ///<  Protocol name
  EFI_GUID * pNetworkBinding;     ///<  Network service binding protocol for socket support
  CONST EFI_GUID * pTagGuid;      ///<  Tag to mark protocol in use
  PFN_SB_INITIALIZE pfnInitialize;///<  Routine to initialize the service
  PFN_SB_SHUTDOWN pfnShutdown;    ///<  Routine to shutdown the service
} DT_SOCKET_BINDING;

//------------------------------------------------------------------------------
// GUIDs
//------------------------------------------------------------------------------

extern CONST EFI_GUID mEslRawServiceGuid;
extern CONST EFI_GUID mEslTcp4ServiceGuid;
extern CONST EFI_GUID mEslUdp4ServiceGuid;

//------------------------------------------------------------------------------
// Data
//------------------------------------------------------------------------------

extern CONST DT_SOCKET_BINDING cEslSocketBinding [];
extern CONST UINTN cEslSocketBindingEntries;

//------------------------------------------------------------------------------
// Service Support Routines
//------------------------------------------------------------------------------

/**
  Connect to the network service bindings

  Walk the network service protocols on the controller handle and
  locate any that are not in use.  Create service structures to
  manage the service binding for the socket driver.

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
  Shutdown the network connections to this controller by removing
  NetworkInterfaceIdentifier protocol and closing the DevicePath
  and PciIo protocols on Controller.

  @param [in] BindingHandle    Handle for protocol binding.
  @param [in] Controller           Handle of device to stop driver on.

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
Install the socket service

@param [in] pImageHandle      Address of the image handle

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslServiceInstall (
  IN EFI_HANDLE * pImageHandle
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
Uninstall the socket service

@param [in] ImageHandle       Handle for the image.

@retval EFI_SUCCESS     Service installed successfully
**/
EFI_STATUS
EFIAPI
EslServiceUninstall (
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
// Socket Service Binding Protocol Routines
//------------------------------------------------------------------------------

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If pChildHandle is a pointer to NULL, then a new handle is created and returned in pChildHandle.
  If pChildHandle is not a pointer to NULL, then the protocol installs on the existing pChildHandle.

  @param [in] pThis        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param [in] pChildHandle Pointer to the handle of the child to create. If it is NULL,
                           then a new handle is created. If it is a pointer to an existing UEFI handle,
                           then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
EslSocketCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN OUT EFI_HANDLE * pChildHandle
  );

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param [in] pThis       Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param [in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is not a valid UEFI Handle.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
EslSocketDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL * pThis,
  IN EFI_HANDLE ChildHandle
  );

//------------------------------------------------------------------------------
// Socket Protocol Routines
//------------------------------------------------------------------------------

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
EFI_STATUS
EslSocketBind (
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
EFI_STATUS
EslSocketClosePoll (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN int * pErrno
  );

/**
  Start the close operation on the socket

  Start closing the socket by closing all of the ports.  Upon
  completion, the ::SocketPoll() routine finishes closing the
  socket.

  @param [in] pSocketProtocol Address of the socket protocol structure.
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

  The ::SocketConnect routine attempts to establish a connection to a
  socket on the local or remote system using the specified address.
  The POSIX
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/connect.html">connect</a>
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
EFI_STATUS
EslSocketConnect (
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
EFI_STATUS
EslSocketGetLocalAddress (
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
EFI_STATUS
EslSocketGetPeerAddress (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  OUT struct sockaddr * pAddress,
  IN OUT socklen_t * pAddressLength,
  IN int * pErrno
  );

/**
  Establish the known port to listen for network connections.

  The ::SocketListen routine places the port into a state that enables connection
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
EFI_STATUS
EslSocketListen (
  IN EFI_SOCKET_PROTOCOL * pSocketProtocol,
  IN INT32 Backlog,
  OUT int * pErrno
  );

/**
  Get the socket options

  Retrieve the socket options one at a time by name.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/getsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] level           Option protocol level
  @param [in] option_name     Name of the option
  @param [out] option_value   Buffer to receive the option value
  @param [in,out] option_len  Length of the buffer in bytes,
                              upon return length of the option value in bytes
  @param [out] pErrno         Address to receive the errno value upon completion.

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

  Adjust the socket options one at a time by name.  The
  <a href="http://pubs.opengroup.org/onlinepubs/9699919799/functions/setsockopt.html">POSIX</a>
  documentation is available online.

  @param [in] pSocketProtocol Address of the socket protocol structure.
  @param [in] level           Option protocol level
  @param [in] option_name     Name of the option
  @param [in] option_value    Buffer containing the option value
  @param [in] option_len      Length of the buffer in bytes
  @param [out] pErrno         Address to receive the errno value upon completion.

  @retval EFI_SUCCESS - Socket data successfully received

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

  The SocketPoll routine checks a socket for pending activity associated
  with the event mask.  Activity is returned in the detected event buffer.

  @param [in] pSocketProtocol Address of the socket protocol structure.

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

  The SocketShutdown routine stops the socket receive and transmit
  operations.

  @param [in] pSocketProtocol Address of the socket protocol structure.

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

  @retval EFI_SUCCESS - Socket data successfully received

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
