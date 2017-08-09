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

typedef HANDLE (WINAPI PASCAL FAR Win_CreateFile) (
  _In_     LPCTSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile
  );

typedef BOOL (WINAPI PASCAL FAR Win_FlushFileBuffers) (
  _In_ HANDLE hFile
  );

typedef BOOL (WINAPI PASCAL FAR Win_CloseHandle) (
  _In_ HANDLE hObject
  );

typedef BOOL (WINAPI PASCAL FAR Win_ReadFile) (
  _In_        HANDLE       hFile,
  _Out_       LPVOID       lpBuffer,
  _In_        DWORD        nNumberOfBytesToRead,
  _Out_opt_   LPDWORD      lpNumberOfBytesRead,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
  );

typedef BOOL (WINAPI PASCAL FAR Win_WriteFile) (
  _In_        HANDLE       hFile,
  _In_        LPCVOID      lpBuffer,
  _In_        DWORD        nNumberOfBytesToWrite,
  _Out_opt_   LPDWORD      lpNumberOfBytesWritten,
  _Inout_opt_ LPOVERLAPPED lpOverlapped
  );

typedef BOOL (WINAPI PASCAL FAR Win_SetFilePointerEx) (
  _In_      HANDLE         hFile,
  _In_      LARGE_INTEGER  liDistanceToMove,
  _Out_opt_ PLARGE_INTEGER lpNewFilePointer,
  _In_      DWORD          dwMoveMethod
  );

typedef DWORD (WINAPI PASCAL FAR Win_GetLastError) (
  void
  );

typedef DWORD (WINAPI PASCAL FAR Win_GetCurrentDirectory) (
  _In_  DWORD  nBufferLength,
  _Out_ LPTSTR lpBuffer
  );

typedef BOOL (WINAPI PASCAL FAR Win_SetCurrentDirectory) (
  _In_ LPCTSTR lpPathName
  );

typedef BOOL (WINAPI PASCAL FAR Win_CreateDirectory) (
  _In_     LPCTSTR               lpPathName,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
  );

typedef BOOL (WINAPI PASCAL FAR Win_CopyFile) (
  _In_ LPCTSTR lpExistingFileName,
  _In_ LPCTSTR lpNewFileName,
  _In_ BOOL    bFailIfExists
  );

//
// Function Declarations
//

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
  );

EFI_STATUS
EFIAPI
WindowsFlushFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle
  );

EFI_STATUS
EFIAPI
WindowsCloseFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle
  );

EFI_STATUS
EFIAPI
WindowsReadFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle,
  OUT VOID                    *Buffer,
  IN OUT UINT32               *Bytes
  );

EFI_STATUS
EFIAPI
WindowsWriteFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  HOST_OS_FILE_HANDLE     Handle,
  IN  VOID                    *Buffer,
  IN OUT UINT32               *Bytes
  );

EFI_STATUS
EFIAPI 
WindowsSetFilePointer (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  HOST_OS_FILE_IO_PPI           *This,
  IN  HOST_OS_FILE_HANDLE           Handle,
  IN  INT64                         Offset,
  IN  HOST_OS_FILE_POINTER_LOCATION StartingPoint
  );

EFI_STATUS
EFIAPI 
WindowsGetCurrentDirectory (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  HOST_OS_FILE_IO_PPI         *This,
  OUT CHAR8                       *Buffer,
  IN  UINT16                      BufferLength
  );

EFI_STATUS
EFIAPI 
WindowsSetCurrentDirectory (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  CHAR8                   *Directory
  );

EFI_STATUS
EFIAPI 
WindowsCreateDirectory (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  CHAR8                   *Directory
  );

EFI_STATUS
EFIAPI 
WindowsCopyFile (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  HOST_OS_FILE_IO_PPI     *This,
  IN  CHAR8                   *OldFileName,
  IN  CHAR8                   *NewFileName,
  IN  BOOLEAN                 FailIfExists
  );

#endif // _HOST_OS_FUNCTIONS_H_

