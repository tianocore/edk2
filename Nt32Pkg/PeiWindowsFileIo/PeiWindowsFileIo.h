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

#ifndef _PEI_WINDOWS_FILE_IO_H_
#define _PEI_WINDOWS_FILE_IO_H_

#include "Meta.h"

//
// Driver context
//

typedef struct _WINDOWS_FILE_IO_CONTEXT {
  UINTN                   Signature;
  HOST_OS_FILE_IO_PPI     HostOsFileIoPpi;
  EFI_PEI_PPI_DESCRIPTOR  PpiList;
  HMODULE                 KernelHandle;

  //
  // Pointers to the Windows functions
  //

  Win_CreateFile          *CreateFilePtr;
  Win_FlushFileBuffers    *FlushFileBuffersPtr;
  Win_CloseHandle         *CloseHandlePtr;
  Win_ReadFile            *ReadFilePtr;
  Win_WriteFile           *WriteFilePtr;
  Win_SetFilePointerEx    *SetFilePointerExPtr;
  Win_GetLastError        *GetLastErrorPtr;
  Win_GetCurrentDirectory *GetCurrentDirectoryPtr;
  Win_SetCurrentDirectory *SetCurrentDirectoryPtr;
  Win_CreateDirectory     *CreateDirectoryPtr;
  Win_CopyFile            *CopyFilePtr;

} WINDOWS_FILE_IO_CONTEXT;

//
// Context signature
//

#define WINDOWS_FILE_IO_CONTEXT_SIGNATURE  SIGNATURE_32  ('w', 'i', 'i', 'o')

//
// Macro to get the context structure from the Ppi.
//

#define WINDOWS_FILE_IO_CONTEXT_FROM_THIS(a) CR (a, WINDOWS_FILE_IO_CONTEXT, HostOsFileIoPpi, WINDOWS_FILE_IO_CONTEXT_SIGNATURE)


EFI_STATUS
EFIAPI
PeiWindowsFileIoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  );

#endif // _PEI_WINDOWS_FILE_IO_H_

