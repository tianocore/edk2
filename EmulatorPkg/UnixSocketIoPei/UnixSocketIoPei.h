/**@file

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnixSocketIoPei.h

Abstract:

  Unix socket IO protocol implementation

* Other names and brands may be claimed as the property of others.

**/

#ifndef _UNIX_SOCKET_IO_PEI_H_
#define _UNIX_SOCKET_IO_PEI_H_

//
// The package level header files this module uses
//

#include <Uefi.h>

//
// The protocols, PPI and GUID defintions for this module
//

#include <Ppi/HostOsSocketIoPpi.h>
#include <Ppi/EmuDynamicLoad.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>

//
// Host OS Specific include files
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <netdb.h>

//
// Driver specifc include files
//

#include "UnixFunctionImplementations.h"
#include "UnixSocketIoPei.h"
#include "Config.h"
#include "Debug.h"

//
// Driver context
//

typedef struct _UNIX_SOCKET_IO_CONTEXT {
  UINTN                   Signature;
  HOST_OS_SOCKET_IO_PPI   HostOsSocketIoPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;

  VOID                    *LibraryHandle;

  //
  // Pointers to the Unix functions
  //

  UnixFn_socket           *SocketFn;
  UnixFn_connect          *ConnectFn;
  UnixFn_close            *CloseFn;
  UnixFn_send             *SendFn;
  UnixFn_recv             *RecvFn;
  UnixFn_inet_addr        *InetAddrFn;
  UnixFn_htons            *HtonsFn;
  UnixFn_htonl            *HtonlFn;
  UnixFn_ntohs            *NtohsFn;
  UnixFn_ntohl            *NtohlFn;
  UnixFn_gethostbyname    *GethostbynameFn;
  UnixFn_gethostname      *GethostnameFn;
  INT32			              *errno;

} UNIX_SOCKET_IO_CONTEXT;

//
// Context signature
//

#define UNIX_SOCKET_IO_CONTEXT_SIGNATURE  SIGNATURE_32  ('u', 'n', 's', 'o')

//
// Macro to get the context structure from the Ppi.
//

#define UNIX_SOCKET_IO_CONTEXT_FROM_THIS(a) CR (a, UNIX_SOCKET_IO_CONTEXT, HostOsSocketIoPpi, UNIX_SOCKET_IO_CONTEXT_SIGNATURE)


EFI_STATUS
EFIAPI
PeiUnixSocketIoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

#endif // _UNIX_SOCKET_IO_PEI_H_

