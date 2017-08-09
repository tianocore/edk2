/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

PeiWindowsFileIo.h

Abstract:

Windows IO protocol implementation

* Other names and brands may be claimed as the property of others.

**/

#ifndef _PEI_WINDOWS_SOCKET_IO_H_
#define _PEI_WINDOWS_SOCKET_IO_H_

#include "Meta.h"

//
// Driver context
//

typedef struct _WINDOWS_SOCKET_IO_CONTEXT {
  UINTN                   Signature;
  HOST_OS_SOCKET_IO_PPI   HostOsSocketIoPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;
  HMODULE                 WinsockHandle;

  //
  // Pointers to the Windows functions
  //

  Win_WSAStartup          *WSAStartupFunctionPtr;
  Win_WSACleanup          *WSACleanupFunctionPtr;
  Win_WSAGetLastError     *WSAGetLastErrorFunctionPtr;
  Win_socket              *socketFunctionPtr;
  Win_connect             *connectFunctionPtr;
  Win_setsockopt          *setsockoptFunctionPtr;
  Win_closesocket         *closesocketFunctionPtr;
  Win_send                *sendFunctionPtr;
  Win_recv                *recvFunctionPtr;
  Win_inet_addr           *inet_addrFunctionPtr;
  Win_htons               *htonsFunctionPtr;
  Win_htonl               *htonlFunctionPtr;
  Win_ntohs               *ntohsFunctionPtr;
  Win_ntohl               *ntohlFunctionPtr;
  Win_gethostbyname       *gethostbynameFunctionPtr;
  Win_gethostname         *gethostnameFunctionPtr;
  Win_inet_ntoa           *inet_ntoaFunctionPtr;
} WINDOWS_SOCKET_IO_CONTEXT;

//
// Context signature
//

#define WINDOWS_SOCKET_IO_CONTEXT_SIGNATURE  SIGNATURE_32  ('w', 'i', 's', 'o')

//
// Macro to get the context structure from the Ppi.
//

#define WINDOWS_SOCKET_IO_CONTEXT_FROM_THIS(a) CR (a, WINDOWS_SOCKET_IO_CONTEXT, HostOsSocketIoPpi, WINDOWS_SOCKET_IO_CONTEXT_SIGNATURE)


EFI_STATUS
EFIAPI
PeiWindowsSocketIoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

#endif // _PEI_WINDOWS_SOCKET_IO_H_

