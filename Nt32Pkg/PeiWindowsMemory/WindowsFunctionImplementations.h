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

typedef LPVOID (WINAPI PASCAL FAR Win_HeapAlloc) (
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ SIZE_T dwBytes
  );

typedef BOOL (WINAPI PASCAL FAR Win_HeapFree) (
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ LPVOID lpMem
  );

typedef LPVOID (WINAPI PASCAL FAR Win_HeapReAlloc) (
  _In_ HANDLE hHeap,
  _In_ DWORD  dwFlags,
  _In_ LPVOID lpMem,
  _In_ SIZE_T dwBytes
  );

typedef HANDLE (WINAPI PASCAL FAR Win_GetProcessHeap) (
  void
  );



//
// Function Declarations
//

EFI_STATUS
EFIAPI 
WindowsMalloc (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_MEMORY_PPI      *This,
  IN  UINT32                  Size,
  OUT VOID                    **AllocatedMemory
  );

EFI_STATUS
EFIAPI 
WindowsFree (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_MEMORY_PPI      *This,
  OUT VOID                    *AllocatedMemory
  );

EFI_STATUS
EFIAPI 
WindowsRealloc (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_MEMORY_PPI      *This,
  IN  OUT VOID                **AllocatedMemory,
  IN  UINT32                  Size
  );



#endif // _HOST_OS_FUNCTIONS_H_

