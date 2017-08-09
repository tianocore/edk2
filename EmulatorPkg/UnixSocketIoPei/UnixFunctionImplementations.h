/**@file

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
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

#include "UnixSocketIoPei.h"

//
// Typedefs for funtion pointers to Unix functions.
//

typedef
int
UnixFn_socket (
  int socket_family, 
  int socket_type, 
  int protocol
  );

typedef
int 
UnixFn_connect (
  int sockfd, 
  const struct sockaddr *addr, 
  socklen_t addrlen
  );

typedef
int 
UnixFn_close (
  int fildes
  );

typedef
ssize_t 
UnixFn_send (
  int sockfd, 
  const void *buf, 
  size_t len, 
  int flags
  );

typedef
ssize_t 
UnixFn_recv (
  int sockfd, 
  void *buf, 
  size_t len, 
  int flags
  );

typedef
in_addr_t 
UnixFn_inet_addr (
  const char *cp
  );

typedef
uint16_t 
UnixFn_htons (
  uint16_t hostshort
  );

typedef
uint32_t 
UnixFn_htonl (
  uint32_t hostlong
  );

typedef
uint16_t 
UnixFn_ntohs (
  uint16_t netshort
  );

typedef
uint32_t 
UnixFn_ntohl (
  uint32_t netlong
  );

typedef
struct hostent *
UnixFn_gethostbyname (
  const char *name
  );

typedef
int 
UnixFn_gethostname (
  char *name, 
  size_t len
  );


//
// Function Declarations
//


EFI_STATUS
EFIAPI
NetworkInit (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This
  );

EFI_STATUS
EFIAPI
NetworkTeardown (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This
  );

EFI_STATUS
EFIAPI
GetLastError (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT INT32                   *LastError
  );

EFI_STATUS
EFIAPI 
CreateSocket (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN HOST_OS_SOCKET_IO_PPI   *This,
  IN INT16 SocketFamily,
  IN INT16 SocketType,
  IN INT16 Protocol,
  OUT HOST_OS_SOCKET *Socket
  );

EFI_STATUS
EFIAPI 
Connect (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET                Socket,
  IN  Sockaddr_in                   *SocketName,
  IN  UINT16                        SocketNameLength
  );

EFI_STATUS
EFIAPI 
ConnectUnix (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI         *This,
  IN  HOST_OS_SOCKET                Socket,
  IN  CHAR8                         *SocketName
  );

EFI_STATUS
EFIAPI 
Close (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket
  );

EFI_STATUS
EFIAPI 
Send (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  HOST_OS_SOCKET          Socket,
  IN CONST UINT8              *Buffer,
  IN OUT UINT32               *BufferLength
  );

EFI_STATUS
EFIAPI 
Recv (
  IN  CONST EFI_PEI_SERVICES    **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI     *This,
  IN  HOST_OS_SOCKET            Socket,
  IN  UINT8                     *Buffer,
  IN  UINT32                    *BufferLength,
  IN  HOST_OS_SOCKET_RECV_FLAGS Flags
  );

EFI_STATUS
EFIAPI 
InetAddr (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CONST CHAR8             *AddressString,
  OUT UINT32                  *Address
  );

EFI_STATUS
EFIAPI 
Htons (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressNetworkByteOrder
  );

EFI_STATUS
EFIAPI 
Htonl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressNetworkByteOrder
  );

EFI_STATUS
EFIAPI 
Ntohs (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT16                  Address,
  OUT UINT16                  *AddressHostByteOrder
  );

EFI_STATUS
EFIAPI 
Ntohl (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  UINT32                  Address,
  OUT UINT32                  *AddressHostByteOrder
  );

EFI_STATUS
EFIAPI 
GetHostByName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  IN  CHAR8                   *Name,
  OUT Hostent                 *HostInformation
  );

EFI_STATUS
EFIAPI 
GetHostName (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_SOCKET_IO_PPI   *This,
  OUT CHAR8                   *Name,
  IN  UINT16                  NameLength
  );


#endif // _HOST_OS_FUNCTIONS_H_

