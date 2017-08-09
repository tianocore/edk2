/**@file

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiWindowsFileIo.c

Abstract:

  Windows File IO protocol implementation

  * Other names and brands may be claimed as the property of others.

**/


#include "Meta.h"

STATIC
EFI_STATUS
AssignWindowsFunctionPointers (
  IN WINDOWS_MEMORY_CONTEXT *Context
  )
{
  EFI_STATUS                Status;
  PEI_NT_THUNK_PPI          *NtThunkPpi;
  EFI_WIN_NT_THUNK_PROTOCOL *NtThunkProtocol;

  TRACE_ENTER ();

  //
  // Check parameters for sanity
  //

  CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_INVALID_PARAMETER, "Passed a bad parameter\n");
  
  //
  // Get the thunk PPI
  //

  Status = PeiServicesLocatePpi (&gPeiNtThunkPpiGuid,
                                  0,
                                  NULL,
                                  (VOID**)&NtThunkPpi);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesLocatePpi failed, Status = %r\n", Status);
  }

  //
  // NtThunk is defined as a protocol, not a PPI so cast to a
  // protocol. This is fine as NtThunk is published by SecMain.exe and
  // is available in both PEI and DXE.
  //

  NtThunkProtocol = (EFI_WIN_NT_THUNK_PROTOCOL*)NtThunkPpi->NtThunk();


  //
  // Load the DLL
  //
  
  Context->KernelHandle = NtThunkProtocol->LoadLibraryEx (L"Kernel32.dll", NULL, 0);
  if (Context->KernelHandle == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->KernelHandle == NULL\n");
  }

  //
  // Set our function pointers
  //

  Context->MallocPtr = (Win_HeapAlloc *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "HeapAlloc");
  if (Context->MallocPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->MallocPtr == NULL\n");
  }

  Context->FreePtr = (Win_HeapFree *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "HeapFree");
  if (Context->FreePtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->FreePtr == NULL\n");
  }

  Context->ReallocPtr = (Win_HeapReAlloc *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "HeapReAlloc");
  if (Context->ReallocPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->ReallocPtr == NULL\n");
  }

  Context->GetProcHeapPtr = (Win_GetProcessHeap *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "GetProcessHeap");
  if (Context->GetProcHeapPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->GetProcHeapPtr == NULL\n");
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // AssignWindowsFunctionPointers

EFI_STATUS
EFIAPI
PeiWindowsMemoryEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;
  PEI_NT_THUNK_PPI *NtThunkPpi;
  WINDOWS_MEMORY_CONTEXT *Context;
  HOST_OS_MEMORY_PPI *HostOsMemoryPpi;

  TRACE_ENTER ();

  //
  // Get the thunk ppi
  //

  Status = PeiServicesLocatePpi (&gPeiNtThunkPpiGuid,
                                  0,
                                  NULL,
                                  (VOID **)&NtThunkPpi);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesLocatePpi failed, Status = %r\n", Status);
  }

  //
  // Allocate memory for our context.
  //

  Status = PeiServicesAllocatePool (sizeof (WINDOWS_MEMORY_CONTEXT), &Context);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesAllocatePool failed, Status = %r\n", Status);
  }

  //
  // Initialize our context signature.
  //

  Context->Signature = WINDOWS_MEMORY_CONTEXT_SIGNATURE;

  //
  // Load the DLL and assign the function pointers. We do this once
  // at startup for performance reasons.
  // 

  Status = AssignWindowsFunctionPointers (Context);
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "AssignWindowsFunctionPointers failed, Status = %r\n", Status);
  }

  HostOsMemoryPpi = &Context->HostOsMemoryPpi;

  //
  // Assign the protocol members
  //

  HostOsMemoryPpi->Revision       = HOST_OS_MEMORY_PPI_REVISION;
  HostOsMemoryPpi->HostOsMalloc   = WindowsMalloc;
  HostOsMemoryPpi->HostOsFree     = WindowsFree;
  HostOsMemoryPpi->HostOsRealloc  = WindowsRealloc;

  //
  // Fill in the PPI Descriptor.
  //

  Context->PpiList.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI  | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  Context->PpiList.Guid  = &gHostOsMemoryPpiGuid;
  Context->PpiList.Ppi   = &Context->HostOsMemoryPpi;

  //
  // Install the PPI.
  //

  Status = PeiServicesInstallPpi (&(Context->PpiList));
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesInstallPpi failed, Status = %r\n", Status);
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // PeiWindowsMemoryEntry


