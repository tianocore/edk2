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
  IN WINDOWS_FILE_IO_CONTEXT *Context
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

  Context->CreateFilePtr = (Win_CreateFile *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "CreateFileA");
  if (Context->CreateFilePtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->CreateFilePtr == NULL\n");
  }

  Context->FlushFileBuffersPtr = (Win_FlushFileBuffers *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "FlushFileBuffers");
  if (Context->FlushFileBuffersPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->FlushFileBuffersPtr == NULL\n");
  }

  Context->CloseHandlePtr = (Win_CloseHandle *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "CloseHandle");
  if (Context->CloseHandlePtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->CloseHandlePtr == NULL\n");
  }

  Context->ReadFilePtr = (Win_ReadFile *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "ReadFile");
  if (Context->ReadFilePtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->ReadFilePtr == NULL\n");
  }

  Context->WriteFilePtr = (Win_WriteFile *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "WriteFile");
  if (Context->WriteFilePtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->WriteFilePtr == NULL\n");
  }

  Context->SetFilePointerExPtr = (Win_SetFilePointerEx *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "SetFilePointerEx");
  if (Context->SetFilePointerExPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->SetFilePointerExPtr == NULL\n");
  }

  Context->GetLastErrorPtr = (Win_GetLastError *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "GetLastError");
  if (Context->GetLastErrorPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->GetLastErrorPtr == NULL\n");
  }

  Context->GetCurrentDirectoryPtr = (Win_GetCurrentDirectory *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "GetCurrentDirectoryA");
  if (Context->GetCurrentDirectoryPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->GetCurrentDirectoryPtr == NULL\n");
  }

  Context->SetCurrentDirectoryPtr = (Win_SetCurrentDirectory *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "SetCurrentDirectoryA");
  if (Context->SetCurrentDirectoryPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->SetCurrentDirectoryPtr == NULL\n");
  }

  Context->CreateDirectoryPtr = (Win_CreateDirectory *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "CreateDirectoryA");
  if (Context->CreateDirectoryPtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->CreateDirectoryPtr == NULL\n");
  }
  
  Context->CopyFilePtr = (Win_CopyFile *)(UINTN)NtThunkProtocol->GetProcAddress (Context->KernelHandle, "CopyFileA");
  if (Context->CopyFilePtr == NULL) {
    CHECK_NULL_RETURN_OR_ASSERT (Context->KernelHandle, EFI_NOT_FOUND, "Context->CopyFilePtr == NULL\n");
  }
  
  TRACE_EXIT ();

  return EFI_SUCCESS;

} // AssignWindowsFunctionPointers

EFI_STATUS
EFIAPI
PeiWindowsFileIoEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS Status;
  PEI_NT_THUNK_PPI *NtThunkPpi;
  WINDOWS_FILE_IO_CONTEXT *Context;
  HOST_OS_FILE_IO_PPI *HostOsFileIoPpi;

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

  Status = PeiServicesAllocatePool (sizeof (WINDOWS_FILE_IO_CONTEXT), &Context);
  if (EFI_ERROR(Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesAllocatePool failed, Status = %r\n", Status);
  }

  //
  // Initialize our context signature.
  //

  Context->Signature = WINDOWS_FILE_IO_CONTEXT_SIGNATURE;

  //
  // Load the DLL and assign the function pointers. We do this once
  // at startup for performance reasons.
  // 

  Status = AssignWindowsFunctionPointers (Context);
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "AssignWindowsFunctionPointers failed, Status = %r\n", Status);
  }

  HostOsFileIoPpi = &Context->HostOsFileIoPpi;

  //
  // Assign the protocol members
  //

  HostOsFileIoPpi->Revision                   = HOST_OS_FILE_IO_PPI_REVISION;
  HostOsFileIoPpi->HostOsOpenFile             = WindowsOpenFile;
  HostOsFileIoPpi->HostOsFlushFile            = WindowsFlushFile;
  HostOsFileIoPpi->HostOsCloseFile            = WindowsCloseFile;
  HostOsFileIoPpi->HostOsReadFile             = WindowsReadFile;
  HostOsFileIoPpi->HostOsWriteFile            = WindowsWriteFile;
  HostOsFileIoPpi->HostOsSetFilePointer       = WindowsSetFilePointer;
  HostOsFileIoPpi->HostOsGetCurrentDirectory  = WindowsGetCurrentDirectory;
  HostOsFileIoPpi->HostOsSetCurrentDirectory  = WindowsSetCurrentDirectory;
  HostOsFileIoPpi->HostOsCreateDirectory      = WindowsCreateDirectory;
  HostOsFileIoPpi->HostOsCopyFile             = WindowsCopyFile;

  //
  // Fill in the PPI Descriptor.
  //

  Context->PpiList.Flags = EFI_PEI_PPI_DESCRIPTOR_PPI  | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST;
  Context->PpiList.Guid  = &gHostOsFileIoPpiGuid;
  Context->PpiList.Ppi   = &Context->HostOsFileIoPpi;

  //
  // Install the PPI.
  //

  Status = PeiServicesInstallPpi (&(Context->PpiList));
  if (EFI_ERROR (Status)) {
    EFI_ERROR_RETURN_OR_ASSERT (Status, "PeiServicesInstallPpi failed, Status = %r\n", Status);
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // PeiWindowsFileIoEntry


