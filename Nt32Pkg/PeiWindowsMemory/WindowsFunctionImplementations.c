/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WindowsFunctionImpelmentations.c

Abstract:

  Windows File IO protocol implementation

  * Other names and brands may be claimed as the property of others.

**/


#include "Meta.h"

EFI_STATUS
EFIAPI 
WindowsMalloc (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_MEMORY_PPI      *This,
  IN  UINT32                  Size,
  OUT VOID                    **AllocatedMemory
  )
{
  WINDOWS_MEMORY_CONTEXT *Context;
  VOID *LocalPtr;
  HANDLE HeapHandle;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)  ||
      (Size == 0))    {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_MEMORY_CONTEXT_FROM_THIS (This);

  HeapHandle = Context->GetProcHeapPtr ();
  if (HeapHandle == NULL) {
    DPRINTF_ERROR ("GetProcHeapPtr failed\n");
    return EFI_DEVICE_ERROR;
  }

  //
  // Call the Windows function
  //

  LocalPtr = Context->MallocPtr (HeapHandle, 0, Size);
  if (LocalPtr == NULL) {
    DPRINTF_ERROR ("Windows malloc failed\n");  

    return EFI_DEVICE_ERROR;
  }

  *AllocatedMemory = LocalPtr;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsMalloc

EFI_STATUS
EFIAPI 
WindowsFree (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_MEMORY_PPI      *This,
  OUT VOID                    *AllocatedMemory
  )
{
  WINDOWS_MEMORY_CONTEXT *Context;
  HANDLE HeapHandle;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_MEMORY_CONTEXT_FROM_THIS (This);

  HeapHandle = Context->GetProcHeapPtr ();
  if (HeapHandle == NULL) {
    DPRINTF_ERROR ("GetProcHeapPtr failed\n");
    return EFI_DEVICE_ERROR;
  }

  //
  // Call the Windows function, does not return anything
  //

  if (!Context->FreePtr (HeapHandle, 0, AllocatedMemory)) {
    DPRINTF_ERROR ("HeapFree failed\n");
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsFree

EFI_STATUS
EFIAPI 
WindowsRealloc (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_MEMORY_PPI      *This,
  IN  OUT VOID                **AllocatedMemory,
  IN  UINT32                  Size
  )
{
  WINDOWS_MEMORY_CONTEXT *Context;
  VOID *LocalPtr;
  HANDLE HeapHandle;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_MEMORY_CONTEXT_FROM_THIS (This);

  HeapHandle = Context->GetProcHeapPtr ();
  if (HeapHandle == NULL) {
    DPRINTF_ERROR ("GetProcHeapPtr failed\n");
    return EFI_DEVICE_ERROR;
  }

  //
  // Call the Windows function, does not return anything
  //

  LocalPtr = *AllocatedMemory;

  *AllocatedMemory = Context->ReallocPtr (HeapHandle, 0, LocalPtr, Size);
  if (*AllocatedMemory == NULL) {
    DPRINTF_ERROR ("Windows realloc failed\n");  

    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsRealloc


