/**@file

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WindowsFunctionImplementations.c

Abstract:

  Windows Socket IO PPI implementation

  * Other names and brands may be claimed as the property of others.

**/


#include "UnixSocketIoPei.h"

/**
  Initialize network services, does nothing on Unix.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.

  @retval  EFI_SUCCESS      Always returns success.

**/

EFI_STATUS
EFIAPI
NetworkInit (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This
  )
{

  TRACE_ENTER ();

  //
  // Not used in Unix
  //

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // NetworkInit

/**
  Tear down network services, does nothing on Unix.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.

  @retval  EFI_SUCCESS      Always returns success.

**/

EFI_STATUS
EFIAPI
NetworkTeardown (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This
  )
{

  TRACE_ENTER ();

  //
  // Not used in Unix
  //

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // NetworkTeardown

/**
  Tear down network services, does nothing on Unix.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.
  @param [out] LastError    Ptr to the last error as reported by the host OS.

  @retval  EFI_SUCCESS      Always returns success.

**/

EFI_STATUS
EFIAPI
GetLastError (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT INT32                   *LastError
  )
{

  TRACE_ENTER ();

  //
  // TODO: figure out how to handle this on Unix, need to get errno.
  //

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // GetLastError

/**
  Create a socket.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.
  @param [in] SocketFamily  Socket family as defined in HostOsSocketDefs.h
  @param [in] SocketType    Socket family as defined in HostOsSocketDefs.h
  @param [in] Protocol      Protocol as defined in HostOsSocketDefs.h
  @param [out] Socket       Ptr to the newly created socket.

  @retval  EFI_SUCCESS      New socket created successfully.
  @retval  !EFI_SUCCESS     Socket creation failed.

**/

EFI_STATUS
EFIAPI 
CreateSocket (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN HOST_OS_SOCKET_IO_PPI   *This,
  IN INT16 SocketFamily,
  IN INT16 SocketType,
  IN INT16 Protocol,
  OUT HOST_OS_SOCKET *Socket
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  int Result;
  int Domain, Type, Prot;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);
  
  switch (SocketFamily) {

  case HOST_OS_SOCKET_ADDRESS_FAMILY_UNSPEC:

    Domain = AF_UNSPEC;
    break;

  case HOST_OS_SOCKET_ADDRESS_FAMILY_INET:

    Domain = AF_INET;
    break;

  case HOST_OS_SOCKET_ADDRESS_FAMILY_UNIX:

    Domain = AF_UNIX;
    break;

  default:
    
    DPRINTF_ERROR ("Passed an unsupported Address Family Type 0x%x\n", SocketFamily);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Call the Unix function
  //

  Result = Context->SocketFn (Domain, SOCK_STREAM, 0); // Always SOCK_STREAM?
  if (Result == -1) {
    DPRINTF_ERROR ("Context->SocketFn failed, return value = 0x%x\n", Result);
    return EFI_DEVICE_ERROR;
  }
  
  *Socket = (HOST_OS_SOCKET)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // CreateSocket

/**
  Blocking call to connect to a server.

  @param [in] PeiServices       An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This              Ptr to the Socket PPI.
  @param [in] Socket            Ptr to the socket.
  @param [in] SocketName        Ptr to a Sockaddr_in structure that describes the server.
  @param [in] SocketNameLength  Length in bytes of SocketName

  @retval  EFI_SUCCESS      Connection succeeded.
  @retval  !EFI_SUCCESS     Connection failed.

**/

EFI_STATUS
EFIAPI 
Connect (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET                Socket,
  IN  Sockaddr_in                   *SocketName,
  IN  UINT16                        SocketNameLength
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  int Result;
  int *ErrorCode;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  switch (SocketName->sin_family) {

  case HOST_OS_SOCKET_ADDRESS_FAMILY_UNSPEC:

    SocketName->sin_family = AF_UNSPEC;
    break;

  case HOST_OS_SOCKET_ADDRESS_FAMILY_INET:

    SocketName->sin_family = AF_INET;
    break;

  default:
    
    DPRINTF_ERROR ("Passed an unsupported Address Family Type 0x%x\n", SocketName->sin_family);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Fix up the port
  //

  SocketName->sin_port = Context->HtonsFn ((u_short)SocketName->sin_port);

  //
  // Call the Unix function
  //

  Result = Context->ConnectFn ((int)Socket, (const struct sockaddr *)SocketName, (socklen_t)SocketNameLength);    
  if (Result != 0) {
    DPRINTF_ERROR ("Context->ConnectFn failed, return value = 0x%x, errno = %d\n", Result, *(Context->errno));
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Connect

/**
  Blocking call to connect to a server.

  @param [in] PeiServices       An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This              Ptr to the Socket PPI.
  @param [in] Socket            Ptr to the socket.
  @param [in] SocketName        Ptr to a Sockaddr_in structure that describes the server.
  @param [in] SocketNameLength  Length in bytes of SocketName

  @retval  EFI_SUCCESS      Connection succeeded.
  @retval  !EFI_SUCCESS     Connection failed.

**/

EFI_STATUS
EFIAPI 
ConnectUnix (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET                Socket,
  IN  CHAR8                         *SocketName
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  int Result;
  Sockaddr_un Address;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  ZeroMem (&Address, sizeof (Address));

  Address.sun_family = AF_UNIX;

  CopyMem (&Address.sun_path, SocketName, AsciiStrLen(SocketName));

  //
  // Call the Unix function
  //

  Result = Context->ConnectFn ((int)Socket, (const struct sockaddr *)&Address, (socklen_t)sizeof (Sockaddr_un));    
  if (Result != 0) {
    DPRINTF_ERROR ("Context->ConnectFn failed, return value = 0x%x, errno = %d\n", Result, *(Context->errno));
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // ConnectUnix

/**
  Close a connection.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.
  @param [in] Socket        Ptr to connected socket.

  @retval  EFI_SUCCESS      Close succeeded.
  @retval  !EFI_SUCCESS     Close failed.

**/

EFI_STATUS
EFIAPI 
Close (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Unix function
  //

  Result = Context->CloseFn ((int)Socket);    
  if (Result != 0) {
    DPRINTF_ERROR ("Context->ConnectFn failed, return value = 0x%x\n", Result);
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Close

/**
  Send data over a connection.

  @param [in] PeiServices       An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This              Ptr to the Socket PPI.
  @param [in] Socket            Ptr to connected socket.
  @param [in] Buffer            Buffer containing the data to send.
  @param [in out] BufferLength  Length of the buffer, updated to reflect the bytes send on return.

  @retval  EFI_SUCCESS      Send succeeded.
  @retval  !EFI_SUCCESS     Send failed.

**/

EFI_STATUS
EFIAPI 
Send (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket,
  IN CONST UINT8              *Buffer,
  IN OUT UINT32               *BufferLength
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  ssize_t Result = 0;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL) ||
      (Buffer == NULL) ||
      (BufferLength == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  Result = Context->SendFn ((int)Socket, (const void *)Buffer, (size_t)*BufferLength, 0);
  if (Result == -1) {
    DPRINTF_ERROR ("Context->SendFn failed, return value = 0x%x\n", Result);
    return EFI_DEVICE_ERROR;
  }

  *BufferLength = Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Send

/**
  Recieve data from a connection.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.
  @param [in] Socket        Ptr to connected socket.
  @param [in] Buffer        Buffer to store the recieved data.
  @param [in] BufferLength  Length of the recieve buffer, updated to reflect the actual bytes recieved on return.
  @param [in] Flags         Recieve flags as defined in HostOsSocketDefs.h

  @retval  EFI_SUCCESS      Recieve succeeded.
  @retval  !EFI_SUCCESS     Recieve failed.

**/

EFI_STATUS
EFIAPI 
Recv (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This,
  IN  HOST_OS_SOCKET            Socket,
  IN  UINT8                     *Buffer,
  IN  UINT32                    *BufferLength,
  IN  HOST_OS_SOCKET_RECV_FLAGS Flags
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  ssize_t Result = 0;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  Result = Context->RecvFn ((int)Socket, (const void *)Buffer, (size_t)*BufferLength, 0);
  if (Result == -1) {
    DPRINTF_ERROR ("Context->RecvFn failed, return value = 0x%x\n", Result);
    return EFI_DEVICE_ERROR;
  }
  
  DPRINTF_INIT ("Result = 0x%x\n", Result);

  // update length on failure?

  *BufferLength = Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Recv

/**
  Convert an IP Address string to the corresponding UINT32.

  @param [in] PeiServices   An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This          Ptr to the Socket PPI.
  @param [in] AddressString String to convert.
  @param [out] Address      Converted address.

  @retval  EFI_SUCCESS      Conversion succeeded.
  @retval  !EFI_SUCCESS     Conversion failed.

**/

EFI_STATUS
EFIAPI 
InetAddr (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CONST CHAR8             *AddressString,
  OUT UINT32                  *Address
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  in_addr_t Result;

  TRACE_ENTER ();
  
  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->InetAddrFn ((const char *)AddressString);

  *Address = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // InetAddr

/**
  Convert a UINT16 (short) from host to network byte order.

  @param [in] PeiServices               An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This                      Ptr to the Socket PPI.
  @param [in] Address                   Address to convert.
  @param [out] AddressNetworkByteOrder  Converted address.

  @retval  EFI_SUCCESS      Conversion succeeded.
  @retval  !EFI_SUCCESS     Conversion failed.

**/

EFI_STATUS
EFIAPI 
Htons (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressNetworkByteOrder
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  uint16_t Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->HtonsFn ((uint16_t)Address);

  *AddressNetworkByteOrder = (UINT16)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Htons

/**
  Convert a UINT32 (long) from host to network byte order.

  @param [in] PeiServices               An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This                      Ptr to the Socket PPI.
  @param [in] Address                   Address to convert.
  @param [out] AddressNetworkByteOrder  Converted address.

  @retval  EFI_SUCCESS      Conversion succeeded.
  @retval  !EFI_SUCCESS     Conversion failed.

**/

EFI_STATUS
EFIAPI 
Htonl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressNetworkByteOrder
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  uint32_t Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->HtonlFn ((uint32_t)Address);

  *AddressNetworkByteOrder = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Htonl
 
/**
  Convert a UINT16 (short) from network to host byte order.

  @param [in] PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This                  Ptr to the Socket PPI.
  @param [in] Address               Address to convert.
  @param [out] AddressHostByteOrder Converted address.

  @retval  EFI_SUCCESS      Conversion succeeded.
  @retval  !EFI_SUCCESS     Conversion failed.

**/

EFI_STATUS
EFIAPI 
Ntohs (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressHostByteOrder
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  uint16_t Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->NtohsFn ((uint16_t)Address);

  *AddressHostByteOrder = (UINT16)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Ntohs

/**
  Convert a UINT32 (long) from network to host byte order.

  @param [in] PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This                  Ptr to the Socket PPI.
  @param [in] Address               Address to convert.
  @param [out] AddressHostByteOrder Converted address.

  @retval  EFI_SUCCESS      Conversion succeeded.
  @retval  !EFI_SUCCESS     Conversion failed.

**/

EFI_STATUS
EFIAPI 
Ntohl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressHostByteOrder
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  uint32_t Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->NtohsFn ((uint32_t)Address);

  *AddressHostByteOrder = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // Ntohl

/**
  Get a Hostent structure for a given host name or IP address.

  @param [in] PeiServices           An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This                  Ptr to the Socket PPI.
  @param [in] Name                  Host name or IP address.
  @param [out] HostInformation      Host information.

  @retval  EFI_SUCCESS      Retrieval succeeded.
  @retval  !EFI_SUCCESS     Retrieval failed.

**/

EFI_STATUS
EFIAPI 
GetHostByName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CHAR8                   *Name,
  OUT Hostent                 *HostInformation
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  struct hostent *Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->GethostbynameFn ((const char *)Name);
  if (Result == NULL) {
    DPRINTF_ERROR ("gethostbyname failed\n");
    return EFI_DEVICE_ERROR;
  }

  CopyMem (HostInformation, Result, sizeof (Hostent));

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // GetHostByName

/**
  Get the host name of the machine this code is executing on. 

  @param [in] PeiServices      An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param [in] This             Ptr to the Socket PPI.
  @param [out] Name            Ptr to the buffer to recieve the host name.
  @param [in] HostInformation  Length of the buffer.

  @retval  EFI_SUCCESS      Retrieval succeeded.
  @retval  !EFI_SUCCESS     Retrieval failed.

**/

EFI_STATUS
EFIAPI 
GetHostName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT CHAR8                   *Name,
  IN  UINT16                  NameLength
  )
{
  UNIX_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = UNIX_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Apparently the Unix function will never fail
  //

  Result = Context->GethostnameFn ((char *)Name, (size_t)NameLength);
  if (Result == -1) {
    DPRINTF_ERROR ("GethostnameFn failed\n");
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // GetHostName


