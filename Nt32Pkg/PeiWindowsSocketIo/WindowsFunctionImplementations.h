/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

HostOsFunctions.h

Abstract:

Host OS function typedefs

* Other names and brands may be claimed as the property of others.

**/

#ifndef _HOST_OS_FUNCTIONS_H_
#define _HOST_OS_FUNCTIONS_H_

#include "Meta.h"

//
// Typedefs for funtion pointers to Windows functions.
//

typedef int (PASCAL FAR Win_WSAStartup) (
  _In_  WORD      wVersionRequested,
  _Out_ LPWSADATA lpWSAData
  );

typedef int (PASCAL FAR Win_WSACleanup) (
  void
  );

typedef int (PASCAL FAR Win_WSAGetLastError) (
  void
  );

typedef SOCKET (PASCAL FAR Win_socket) (
  _In_ int af,
  _In_ int type,
  _In_ int protocol
  );

typedef int (PASCAL FAR Win_connect) (
  _In_ SOCKET                s,
  _In_ const struct sockaddr *name,
  _In_ int                   namelen
  );

typedef int (PASCAL FAR Win_setsockopt) (
  _In_       SOCKET s,
  _In_       int    level,
  _In_       int    optname,
  _In_ const char   *optval,
  _In_       int    optlen
);

typedef int (PASCAL FAR Win_closesocket) (
  _In_ SOCKET s
  );

typedef int (PASCAL FAR Win_send) (
  _In_       SOCKET s,
  _In_ const char   *buf,
  _In_       int    len,
  _In_       int    flags
  );

typedef int (PASCAL FAR Win_recv) (
  _In_  SOCKET s,
  _Out_ char   *buf,
  _In_  int    len,
  _In_  int    flags
  );

typedef unsigned long (PASCAL FAR Win_inet_addr) (
  _In_ const char *cp
  );

typedef u_short (PASCAL FAR Win_htons) (
  _In_ u_short hostshort
  );

typedef u_long (PASCAL FAR Win_htonl) (
  _In_ u_long hostlong
  );

typedef u_short (PASCAL FAR Win_ntohs) (
  _In_ u_short netshort
  );

typedef u_long (PASCAL FAR Win_ntohl) (
  _In_ u_long netlong
  );

typedef struct hostent* (PASCAL FAR Win_gethostbyname) (
  _In_ const char *name
  );

typedef int (PASCAL FAR Win_gethostname) (
  _Out_ char *name,
  _In_  int  namelen
  );

typedef char* (PASCAL FAR Win_inet_ntoa) (
  _In_ struct   in_addr in
  );

//
// Function Declarations
//

EFI_STATUS
EFIAPI 
WindowsWSAStartup (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI      *This
  );

EFI_STATUS
EFIAPI 
WindowsWSACleanup (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI      *This
  );

EFI_STATUS
EFIAPI 
WindowsWSAGetLastError (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT INT32                   *LastError
  );

EFI_STATUS
EFIAPI
WindowsSocket (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET_ADDRESS_FAMILY AddressFamily,
  IN  HOST_OS_SOCKET_TYPE           SocketType,
  IN  HOST_OS_SOCKET_PROTOCOL       Protocol,
  OUT HOST_OS_SOCKET                *Socket
  );

EFI_STATUS
EFIAPI 
WindowsConnect (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET                Socket,
  IN  Sockaddr_in                   *SocketName,
  IN  UINT16                        SocketNameLength
  );

EFI_STATUS
EFIAPI
WindowsCloseSocket (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket
  );

EFI_STATUS
EFIAPI
WindowsSend (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket,
  IN CONST UINT8              *Buffer,
  IN UINT32                   *BufferLength
  );

EFI_STATUS
EFIAPI 
WindowsRecv (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This,
  IN  HOST_OS_SOCKET            Socket,
  IN  UINT8                     *Buffer,
  IN  UINT32                    *BufferLength,
  IN  HOST_OS_SOCKET_RECV_FLAGS Flags
  );

EFI_STATUS
EFIAPI 
WindowsInetAddr (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CONST CHAR8             *AddressString,
  OUT UINT32                  *Address
  );

EFI_STATUS
EFIAPI
WindowsHtons (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressNetworkByteOrder
  );

EFI_STATUS
EFIAPI
WindowsHtonl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressNetworkByteOrder
  );

EFI_STATUS
EFIAPI
WindowsNtohs (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressHostByteOrder
  );

EFI_STATUS
EFIAPI
WindowsNtohl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressHostByteOrder
  );

EFI_STATUS
EFIAPI
WindowsGetHostByName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CHAR8                   *Name,
  OUT Hostent                 *HostInformation
  );

EFI_STATUS
EFIAPI
WindowsGetHostName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT CHAR8                   *Name,
  IN  UINT16                  NameLength
  );

EFI_STATUS
EFIAPI
WindowsInet_Ntoa (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN In_addr InAddress,
  OUT CHAR8 *ConvertedAddress
  );

#endif // _HOST_OS_FUNCTIONS_H_

