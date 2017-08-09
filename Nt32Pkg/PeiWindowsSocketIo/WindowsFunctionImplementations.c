/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
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


#include "Meta.h"

EFI_STATUS
EFIAPI 
WindowsWSAStartup (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  WORD VersionRequested;
  WSADATA Data;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  VersionRequested = MAKEWORD(2, 2);

  //
  // Call the Windows function
  //

  Result = Context->WSAStartupFunctionPtr (VersionRequested, &Data);
  if (Result != 0) {
    DPRINTF_ERROR ("Windows WSAStartup failed, return value = 0x%x\n", Result);
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsWSAStartup

EFI_STATUS
EFIAPI 
WindowsWSACleanup (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->WSACleanupFunctionPtr ();
  if (Result != 0) {
    DPRINTF_ERROR ("Windows WSACleanup failed, return value = 0x%x\n, WSAgetLastError returns 0x%x\n", Result, Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsWSACleanup

EFI_STATUS
EFIAPI 
WindowsWSAGetLastError (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT INT32                   *LastError
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL) ||
      (LastError == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  *LastError = Context->WSAGetLastErrorFunctionPtr ();

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsWSAGetLastError

EFI_STATUS
EFIAPI
WindowsSocket (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET_ADDRESS_FAMILY AddressFamily,
  IN  HOST_OS_SOCKET_TYPE           SocketType,
  IN  HOST_OS_SOCKET_PROTOCOL       Protocol,
  OUT HOST_OS_SOCKET                *Socket
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int WinAddressFamily, WinSocketType, WinProtocol, Result;
  SOCKET WinSocket;
  BOOL KeepAlive;
  int OptionLength;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)    ||
      (Socket == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Translate from HOST_OS_SOCKET_ADDRESS_FAMILY to 
  // Windows version.
  //

  switch (AddressFamily) {

  case HOST_OS_SOCKET_ADDRESS_FAMILY_UNSPEC:

    WinAddressFamily = AF_UNSPEC;
    break;

  case HOST_OS_SOCKET_ADDRESS_FAMILY_INET:

    WinAddressFamily = AF_INET;
    break;

  //TODO, missing definitions for these Windows symbols: AF_INET6, AF_IRDA, AF_BTH

  default:
    
    DPRINTF_ERROR ("Passed an unsupported Address Family Type 0x%x\n", AddressFamily);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Translate from HOST_OS_SOCKET_TYPE to 
  // Windows version.
  //

  switch (SocketType) {

  case HOST_OS_SOCKET_TYPE_SOCK_STREAM:

    WinSocketType = SOCK_STREAM;
    break;

  case HOST_OS_SOCKET_TYPE_SOCK_DGRAM:

    WinSocketType = SOCK_DGRAM;
    break;

  case HOST_OS_SOCKET_TYPE_SOCK_RAW:

    WinSocketType = SOCK_RAW;
    break;

  case HOST_OS_SOCKET_TYPE_SOCK_RDM:

    WinSocketType = SOCK_RDM;
    break;

  case HOST_OS_SOCKET_TYPE_SOCK_SEQPACKET:

    WinSocketType = SOCK_SEQPACKET;
    break;

  default:

    DPRINTF_ERROR ("Passed an unsupported Socket Type 0x%x\n", SocketType);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Translate from HOST_OS_SOCKET_TYPE to 
  // Windows version.
  //

  switch (Protocol) {

  case HOST_OS_SOCKET_PROTOCOL_ICMP:

    WinProtocol = IPPROTO_ICMP;
    break;

  case HOST_OS_SOCKET_PROTOCOL_IGMP:

    WinProtocol = IPPROTO_IGMP;
    break;

  case HOST_OS_SOCKET_PROTOCOL_TCP:

    WinProtocol = IPPROTO_TCP;
    break;

  case HOST_OS_SOCKET_PROTOCOL_UDP:

    WinProtocol = IPPROTO_UDP;
    break;

  //TODO, missing definitions for these Windows symbols: BTHPROTO_RFCOMM, IPPROTO_ICMPV6, IPPROTO_RM

  default:

    DPRINTF_ERROR ("Passed an unsupported Protocol Type 0x%x\n", Protocol);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Call the Windows function
  //

  WinSocket = Context->socketFunctionPtr (WinAddressFamily, WinSocketType, WinProtocol);
  if (WinSocket == INVALID_SOCKET) {
    DPRINTF_ERROR ("Windows Socket Failed, WSAGetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  //
  // Turn on keepalive behavior
  //

  KeepAlive = TRUE;
  OptionLength = sizeof (BOOL);

  Result = Context->setsockoptFunctionPtr (WinSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&KeepAlive, OptionLength);
  if (Result == SOCKET_ERROR) {
    DPRINTF_ERROR ("Windows setsockopt Failed, WSAGetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  *Socket = (HOST_OS_SOCKET)WinSocket;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsSocket

EFI_STATUS
EFIAPI 
WindowsConnect (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket,
  IN  Sockaddr_in             *SocketName,
  IN  UINT16                  SocketNameLength
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)            ||
      (SocketName == NULL)      ||
      (SocketNameLength == 0)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Translate the HostOS family to the Windows family
  //

  switch (SocketName->sin_family) {

  case HOST_OS_SOCKET_ADDRESS_FAMILY_UNSPEC:

    SocketName->sin_family = AF_UNSPEC;
    break;

  case HOST_OS_SOCKET_ADDRESS_FAMILY_INET:

    SocketName->sin_family = AF_INET;
    break;

  //TODO, missing definitions for these Windows symbols: AF_INET6, AF_IRDA, AF_BTH

  default:
    
    DPRINTF_ERROR ("Passed an unsupported Address Family Type 0x%x\n", SocketName->sin_family);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Fix up the port
  //

  SocketName->sin_port = Context->htonsFunctionPtr ((u_short)SocketName->sin_port);

  //
  // Call the Windows function
  //

  Result = Context->connectFunctionPtr ((SOCKET)Socket, (struct sockaddr *)SocketName, (int)SocketNameLength);
  if (Result != 0) {
    DPRINTF_ERROR ("Windows connect failed, return value = 0x%x\n, WSAgetLastError returns 0x%x\n", Result, Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsConnect
 
EFI_STATUS
EFIAPI
WindowsCloseSocket (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->closesocketFunctionPtr ((SOCKET)Socket);
  if (Result != 0) {
    DPRINTF_ERROR ("Windows closesocket failed, return value = 0x%x\n, WSAgetLastError returns 0x%x\n", Result, Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsCloseSocket

EFI_STATUS
EFIAPI
WindowsSend (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket,
  IN CONST UINT8              *Buffer,
  IN UINT32                   *BufferLength
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->sendFunctionPtr ((SOCKET)Socket, (const char *)Buffer, (int)*BufferLength, 0);
  if (Result == SOCKET_ERROR) {
    DPRINTF_ERROR ("Windows send failed, WSAgetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  *BufferLength= Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsCloseSocket

EFI_STATUS
EFIAPI 
WindowsRecv (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This,
  IN  HOST_OS_SOCKET            Socket,
  IN  UINT8                     *Buffer,
  IN  UINT32                    *BufferLength,
  IN  HOST_OS_SOCKET_RECV_FLAGS Flags
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int Result, WinFlags;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)    ||
      (Buffer == NULL)  ||
      (BufferLength == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Translate from HOST_OS_SOCKET_RECV_FLAGS to 
  // Windows version.
  //

  // TODO - Should check bitfields since flags can be or'ed.

  switch (Flags) {

  case HOST_OS_SOCKET_RECV_FLAGS_NONE:

    WinFlags = 0;
    break;

  case HOST_OS_SOCKET_RECV_FLAGS_MSG_OOB:
  
    WinFlags = MSG_OOB;
    break;
  
  case HOST_OS_SOCKET_RECV_FLAGS_MSG_PEEK:

    WinFlags = MSG_PEEK;
    break;

  default:

    DPRINTF_ERROR ("Passed an unsupported Flags Type 0x%x\n", Flags);
    return EFI_INVALID_PARAMETER;

  }

  //
  // Call the Windows function
  //

  Result = Context->recvFunctionPtr ((SOCKET)Socket, (char *)Buffer, (int)*BufferLength, WinFlags);
  if (Result == SOCKET_ERROR) {
    DPRINTF_ERROR ("Windows recv failed, WSAgetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  *BufferLength = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsRecv

EFI_STATUS
EFIAPI 
WindowsInetAddr (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CONST CHAR8             *AddressString,
  OUT UINT32                  *Address
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  unsigned long Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)    ||
      (AddressString == NULL)  ||
      (Address == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->inet_addrFunctionPtr ((const char *)AddressString);
  if (Result == INADDR_NONE) {
    DPRINTF_ERROR ("Windows inet_addr failed\n");
    return EFI_DEVICE_ERROR;
  }

  *Address = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsInetAddr

EFI_STATUS
EFIAPI
WindowsHtons (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressNetworkByteOrder
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  u_short Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)                      ||
      (AddressNetworkByteOrder == NULL))  {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->htonsFunctionPtr ((u_short)Address);  // Will not ever return error according to MSDN

  *AddressNetworkByteOrder = (UINT16)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsHtons

EFI_STATUS
EFIAPI
WindowsHtonl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressNetworkByteOrder
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  u_long Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)                      ||
      (AddressNetworkByteOrder == NULL))  {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->htonlFunctionPtr ((u_long)Address);  // Will not ever return error according to MSDN

  *AddressNetworkByteOrder = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsHtonl

EFI_STATUS
EFIAPI
WindowsNtohs (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressHostByteOrder
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  u_short Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)                    ||
      (AddressHostByteOrder == NULL))   {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->ntohsFunctionPtr ((u_short)Address);  // Will not ever return error according to MSDN

  *AddressHostByteOrder = (UINT16)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsNtohs

EFI_STATUS
EFIAPI
WindowsNtohl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressHostByteOrder
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  u_long Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)                    ||
      (AddressHostByteOrder == NULL))   {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->ntohlFunctionPtr ((u_long)Address);  // Will not ever return error according to MSDN

  *AddressHostByteOrder = (UINT32)Result;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsNtohl

EFI_STATUS
EFIAPI
WindowsGetHostByName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CHAR8                   *Name,
  OUT Hostent                 *HostInformation
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  struct hostent *Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)              ||
      (Name == NULL)              ||
      (HostInformation == NULL))  {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->gethostbynameFunctionPtr ((const char *)Name);
  if (Result == NULL) {
    DPRINTF_ERROR ("Windows gethostbyname failed, WSAgetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  //
  // Hostent structure follows *nix convention, where h_addrtype
  // and h_length are 32bits, Windows hostent (not the same as Hostent)
  // uses 16 bits for these fields, so we cannot do a straight copy.
  //

  HostInformation->h_name = Result->h_name;
  HostInformation->h_aliases = Result->h_aliases;

  HostInformation->h_addrtype = (INT32)Result->h_addrtype;
  HostInformation->h_length = (INT32)Result->h_length;

  HostInformation->h_addr_list = Result->h_addr_list;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsGetHostByName

EFI_STATUS
EFIAPI
WindowsGetHostName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT CHAR8                   *Name,
  IN  UINT16                  NameLength
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  int Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)  ||
      (Name == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->gethostnameFunctionPtr ((char *)Name, (int)NameLength);
  if (Result != 0) {
    DPRINTF_ERROR ("Windows gethostname failed, WSAgetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsGetHostByName

EFI_STATUS
EFIAPI
WindowsInet_Ntoa (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN In_addr InAddress,
  OUT CHAR8 *ConvertedAddress
  )
{
  WINDOWS_SOCKET_IO_CONTEXT *Context;
  char *Result;
  struct in_addr *LocalPtr;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)  ||
      (ConvertedAddress == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  LocalPtr = (struct in_addr *)&InAddress;

  Result = Context->inet_ntoaFunctionPtr (*LocalPtr);
  if (Result == NULL) {
    DPRINTF_ERROR ("Windows inet_ntoa failed, WSAgetLastError returns 0x%x\n", Context->WSAGetLastErrorFunctionPtr());
    return EFI_DEVICE_ERROR;
  }

  AsciiStrCpy (ConvertedAddress, Result);

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsInet_Ntoa