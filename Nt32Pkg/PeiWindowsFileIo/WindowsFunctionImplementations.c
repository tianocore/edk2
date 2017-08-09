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
WindowsOpenFile (
  IN  CONST EFI_PEI_SERVICES     **PeiServices,
  IN  HOST_OS_FILE_IO_PPI        *This,
  IN  CHAR8                      *FileName,
  IN  HOST_OS_FILE_ACCESS_MODE    AccessMode,
  IN  HOST_OS_FILE_SHARING_MODE  SharingMode,
  IN  HOST_OS_FILE_CREATION_MODE CreationMode,
  OUT HOST_OS_FILE_HANDLE        *Handle
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  DWORD dwAccessMode, dwShareMode, dwCreationDisposition;
  HANDLE FileHandle;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)      ||
      (FileName == NULL)  ||
      (Handle == NULL))   {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  switch (AccessMode) {
  
  case HOST_OS_FILE_READ:

    dwAccessMode =  GENERIC_READ;
    break;

  case HOST_OS_FILE_WRITE:

    dwAccessMode =  GENERIC_WRITE;
    break;

  case HOST_OS_FILE_READ_WRITE:

    dwAccessMode =  GENERIC_READ | GENERIC_WRITE;
    break;

  default:

    return EFI_INVALID_PARAMETER;

  }

  //
  // Translate from HOST_OS_FILE_SHARING_MODE to 
  // Windows version.
  //

  switch (SharingMode) {

  case HOST_OS_FILE_SHARE_EXCLUSIVE:

    dwShareMode = 0;
    break;
      
  case HOST_OS_FILE_SHARE_DELETE:

    dwShareMode = FILE_SHARE_DELETE;
    break;
  
  case HOST_OS_FILE_SHARE_READ:

    dwShareMode = FILE_SHARE_READ;
    break;
  
  case HOST_OS_FILE_SHARE_WRITE:

    dwShareMode = FILE_SHARE_WRITE;
    break;
  
  default:

    return EFI_INVALID_PARAMETER;

  }

  //
  // Translate from HOST_OS_FILE_CREATION_MODE to 
  // Windows version.
  //

  switch (CreationMode) {

  case HOST_OS_FILE_CREATE_ALWAYS:

    dwCreationDisposition = CREATE_ALWAYS;
    break;

  case HOST_OS_FILE_CREATE_NEW:

    dwCreationDisposition = CREATE_NEW;
    break;

  case HOST_OS_FILE_OPEN_ALWAYS:

    dwCreationDisposition = OPEN_ALWAYS;
    break;

  case HOST_OS_FILE_OPEN_EXISTING:

    dwCreationDisposition = OPEN_EXISTING;
    break;

  case HOST_OS_FILE_TRUNCATE_EXISTING:

    dwCreationDisposition = TRUNCATE_EXISTING;
    break;

  default:

    return EFI_INVALID_PARAMETER;

  }

  //
  // Call the Windows function
  //

  FileHandle = Context->CreateFilePtr ((LPCTSTR)FileName,
                                          dwAccessMode,
                                          dwShareMode,
                                          NULL,
                                          dwCreationDisposition,
                                          FILE_ATTRIBUTE_NORMAL,
                                          NULL);
  if (FileHandle == INVALID_HANDLE_VALUE) {
    DPRINTF_ERROR ("Windows CreateFile failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  

    //
    // TODO - Need to load GetLastError() from the kernel and call it here,
    //  Map from common Windows error codes to EFI_STATUS codes
    //

    return EFI_DEVICE_ERROR;
  }

  *Handle = (HOST_OS_FILE_HANDLE)FileHandle;

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsOpenFile

EFI_STATUS
EFIAPI
WindowsFlushFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->FlushFileBuffersPtr ((HANDLE)Handle);
  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows FlushFileBuffers failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsFlushFile

EFI_STATUS
EFIAPI
WindowsCloseFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);


  //
  // Call the Windows function
  //

  Result = Context->CloseHandlePtr ((HANDLE)Handle);
  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows CloseHandle failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsCloseFile

EFI_STATUS
EFIAPI
WindowsReadFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle,
  OUT VOID                    *Buffer,
  IN OUT UINT32               *Bytes
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;
  DWORD BytesToRead, BytesRead;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)      ||
      (Buffer == NULL)  ||
      (Bytes == NULL))   {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  BytesToRead = *Bytes;

  Result = Context->ReadFilePtr ((HANDLE)Handle, 
                                  Buffer, 
                                  BytesToRead, 
                                  &BytesRead, 
                                  NULL);

  //
  // Update bytes read regardless of success or failure.
  //

  *Bytes = BytesRead;

  //
  // Now check result.
  //

  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows ReadFile failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsReadFile

EFI_STATUS
EFIAPI
WindowsWriteFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle,
  IN  VOID                    *Buffer,
  IN OUT UINT32               *Bytes
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;
  DWORD BytesToWrite, BytesWritten;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)      ||
      (Buffer == NULL)  ||
      (Bytes == NULL))   {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  BytesToWrite = *Bytes;

  Result = Context->WriteFilePtr ((HANDLE)Handle, 
                                    Buffer, 
                                    BytesToWrite, 
                                    &BytesWritten, 
                                    NULL);

  //
  // Update bytes read regardless of success or failure.
  //

  *Bytes = BytesWritten;

  //
  // Now check result.
  //

  if (Result != TRUE) {

    DWORD LastError = Context->GetLastErrorPtr();

    DPRINTF_ERROR ("Windows WriteFile failed, GetLastError returns 0x%x\n", LastError);  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsWriteFile

EFI_STATUS
EFIAPI 
WindowsSetFilePointer (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_FILE_IO_PPI           *This,
  IN  HOST_OS_FILE_HANDLE           Handle,
  IN  INT64                         Offset,
  IN  HOST_OS_FILE_POINTER_LOCATION StartingPoint
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;
  DWORD MoveMethod;
  LARGE_INTEGER LocalOffset;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if (This == NULL) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  switch (StartingPoint) {

  case HOST_OS_FILE_BEGIN:

    MoveMethod = FILE_BEGIN;
    break;

  case HOST_OS_FILE_CURRENT:

    MoveMethod = FILE_CURRENT;
    break;

  case HOST_OS_FILE_END:

    MoveMethod = FILE_END;
    break;

  default:

    return EFI_INVALID_PARAMETER;

  }

  LocalOffset.QuadPart = Offset;

  Result = Context->SetFilePointerExPtr ((HANDLE)Handle, LocalOffset, NULL, MoveMethod);
  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows SetFilePointerEx failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsSetFilePointer

EFI_STATUS
EFIAPI 
WindowsGetCurrentDirectory (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  HOST_OS_FILE_IO_PPI         *This,
  OUT CHAR8                       *Buffer,
  IN  UINT16                      BufferLength
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  DWORD Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL) ||
      (Buffer == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->GetCurrentDirectoryPtr ((DWORD)BufferLength, Buffer);
  if (Result == 0) {
    DPRINTF_ERROR ("Windows GetCurrentDirectory failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsGetCurrentDirectory

EFI_STATUS
EFIAPI 
WindowsSetCurrentDirectory (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  CHAR8                   *Directory
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL) ||
      (Directory == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->SetCurrentDirectoryPtr (Directory);
  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows SetCurrentDirectory failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsSetCurrentDirectory

EFI_STATUS
EFIAPI 
WindowsCreateDirectory (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  CHAR8                   *Directory
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL) ||
      (Directory == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->CreateDirectoryPtr (Directory, NULL);
  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows CreateDirectory failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsCreateDirectory

EFI_STATUS
EFIAPI 
WindowsCopyFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  CHAR8                   *OldFileName,
  IN  CHAR8                   *NewFileName,
  IN  BOOLEAN                 FailIfExists
  )
{
  WINDOWS_FILE_IO_CONTEXT *Context;
  BOOL Result;

  TRACE_ENTER ();

  //
  // Check parameters for sanity.
  //

  if ((This == NULL)         ||
      (OldFileName == NULL)  ||
      (NewFileName == NULL)) {
    DPRINTF_ERROR ("Passed a bad parameter\n");
    return EFI_INVALID_PARAMETER;
  }

  Context = WINDOWS_FILE_IO_CONTEXT_FROM_THIS (This);

  //
  // Call the Windows function
  //

  Result = Context->CopyFilePtr (OldFileName, NewFileName, FailIfExists);
  if (Result != TRUE) {
    DPRINTF_ERROR ("Windows CloseHandle failed, GetLastError returns 0x%x\n", Context->GetLastErrorPtr());  
    return EFI_DEVICE_ERROR;
  }

  TRACE_EXIT ();

  return EFI_SUCCESS;

} // WindowsCopyFile
