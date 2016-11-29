/** @file

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Edb.h"

/**
  Read a file.

  @param  Vol             - File System Volume
  @param  FileName        - The file to be read.
  @param  BufferSize      - The file buffer size
  @param  Buffer          - The file buffer

  @retval EFI_SUCCESS    - read file successfully
  @retval EFI_NOT_FOUND  - file not found

**/
EFI_STATUS
EFIAPI
ReadFileFromVol (
  IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Vol,
  IN  CHAR16                      *FileName,
  OUT UINTN                       *BufferSize,
  OUT VOID                        **Buffer
  )
{
  EFI_STATUS                        Status;
  EFI_FILE_HANDLE                   RootDir;
  EFI_FILE_HANDLE                   Handle;
  UINTN                             FileInfoSize;
  EFI_FILE_INFO                     *FileInfo;
  UINTN                             TempBufferSize;
  VOID                              *TempBuffer;

  //
  // Open the root directory
  //
  Status = Vol->OpenVolume (Vol, &RootDir);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the file
  //
  Status = RootDir->Open (
                      RootDir,
                      &Handle,
                      FileName,
                      EFI_FILE_MODE_READ,
                      0
                      );
  if (EFI_ERROR (Status)) {
    RootDir->Close (RootDir);
    return Status;
  }

  RootDir->Close (RootDir);

  //
  // Get the file information
  //
  FileInfoSize = sizeof(EFI_FILE_INFO) + 1024;

  FileInfo = AllocateZeroPool (FileInfoSize);
  if (FileInfo == NULL) {
    Handle->Close (Handle);
    return Status;
  }

  Status = Handle->GetInfo (
                     Handle,
                     &gEfiFileInfoGuid,
                     &FileInfoSize,
                     FileInfo
                     );
  if (EFI_ERROR (Status)) {
    Handle->Close (Handle);
    gBS->FreePool (FileInfo);
    return Status;
  }

  //
  // Allocate buffer for the file data. The last CHAR16 is for L'\0'
  //
  TempBufferSize = (UINTN) FileInfo->FileSize + sizeof(CHAR16);
  TempBuffer = AllocateZeroPool (TempBufferSize);
  if (TempBuffer == NULL) {
    Handle->Close (Handle);
    gBS->FreePool (FileInfo);
    return Status;
  }

  gBS->FreePool (FileInfo);

  //
  // Read the file data to the buffer
  //
  Status = Handle->Read (
                     Handle,
                     &TempBufferSize,
                     TempBuffer
                     );
  if (EFI_ERROR (Status)) {
    Handle->Close (Handle);
    gBS->FreePool (TempBuffer);
    return Status;
  }

  Handle->Close (Handle);

  *BufferSize = TempBufferSize;
  *Buffer     = TempBuffer;
  return EFI_SUCCESS;
}

/**

  Read a file.
  If ScanFs is FLASE, it will use DebuggerPrivate->Vol as default Fs.
  If ScanFs is TRUE, it will scan all FS and check the file.
  If there is only one file match the name, it will be read.
  If there is more than one file match the name, it will return Error.

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  FileName        - The file to be read.
  @param  BufferSize      - The file buffer size
  @param  Buffer          - The file buffer
  @param  ScanFs          - Need Scan all FS

  @retval EFI_SUCCESS    - read file successfully
  @retval EFI_NOT_FOUND  - file not found
  @retval EFI_NO_MAPPING - there is duplicated files found

**/
EFI_STATUS
EFIAPI
ReadFileToBuffer (
  IN  EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN  CHAR16                      *FileName,
  OUT UINTN                       *BufferSize,
  OUT VOID                        **Buffer,
  IN  BOOLEAN                     ScanFs
  )
{
  EFI_STATUS                        Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Vol;
  UINTN                             TempBufferSize;
  VOID                              *TempBuffer;
  UINTN                             NoHandles;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             Index;

  //
  // Check parameters
  //
  if ((FileName == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // not scan fs
  //
  if (!ScanFs) {
    if (DebuggerPrivate->Vol == NULL) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Read file directly from Vol
    //
    return ReadFileFromVol (DebuggerPrivate->Vol, FileName, BufferSize, Buffer);
  }

  //
  // need scan fs
  //

  //
  // Get all Vol handle
  //
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiSimpleFileSystemProtocolGuid,
                   NULL,
                   &NoHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status) && (NoHandles == 0)) {
    return EFI_NOT_FOUND;
  }

  //
  // Walk through each Vol
  //
  DebuggerPrivate->Vol = NULL;
  *BufferSize = 0;
  *Buffer     = NULL;
  for (Index = 0; Index < NoHandles; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiSimpleFileSystemProtocolGuid,
                    (VOID**) &Vol
                    );
    if (EFI_ERROR(Status)) {
      continue;
    }

    Status = ReadFileFromVol (Vol, FileName, &TempBufferSize, &TempBuffer);
    if (!EFI_ERROR (Status)) {
      //
      // Read file OK, check duplication
      //
      if (DebuggerPrivate->Vol != NULL) {
        //
        // Find the duplicated file
        //
        gBS->FreePool (TempBuffer);
        gBS->FreePool (*Buffer);
        EDBPrint (L"Duplicated FileName found!\n");
        return EFI_NO_MAPPING;
      } else {
        //
        // Record value
        //
        DebuggerPrivate->Vol = Vol;
        *BufferSize = TempBufferSize;
        *Buffer     = TempBuffer;
      }
    }
  }

  //
  // Scan Fs done
  //
  if (DebuggerPrivate->Vol == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**

  Get file name under this dir with index

  @param  DebuggerPrivate - EBC Debugger private data structure
  @param  DirName         - The dir to be read.
  @param  FileName        - The file name pattern under this dir
  @param  Index           - The file index under this dir

  @return File Name which match the pattern and index.

**/
CHAR16 *
EFIAPI
GetFileNameUnderDir (
  IN  EFI_DEBUGGER_PRIVATE_DATA   *DebuggerPrivate,
  IN  CHAR16                      *DirName,
  IN  CHAR16                      *FileName,
  IN OUT UINTN                    *Index
  )
{
  EFI_STATUS                        Status;
  EFI_FILE_HANDLE                   RootDir;
  EFI_FILE_HANDLE                   Handle;
  UINTN                             FileInfoSize;
  EFI_FILE_INFO                     *FileInfo;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL   *Vol;
  VOID                              *TempName;
  UINTN                             FileIndex;

  if (DebuggerPrivate->Vol == NULL) {
    Status = gBS->LocateProtocol (
                    &gEfiSimpleFileSystemProtocolGuid,
                    NULL,
                    (VOID**) &DebuggerPrivate->Vol
                    );
    if (EFI_ERROR(Status)) {
      return NULL;
    }
  }
  Vol = DebuggerPrivate->Vol;

  //
  // Open the root directory
  //
  Status = Vol->OpenVolume (Vol, &RootDir);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Open the file
  //
  Status = RootDir->Open (
                      RootDir,
                      &Handle,
                      DirName,
                      EFI_FILE_MODE_READ,
                      EFI_FILE_DIRECTORY
                      );
  if (EFI_ERROR (Status)) {
    RootDir->Close (RootDir);
    return NULL;
  }
  RootDir->Close (RootDir);

  //
  // Set Dir Position
  //
  Status = Handle->SetPosition (Handle, 0);
  if (EFI_ERROR (Status)) {
    Handle->Close (Handle);
    return NULL;
  }

  //
  // Get the file information
  //
  FileInfoSize = sizeof(EFI_FILE_INFO) + 1024;

  FileInfo = AllocateZeroPool (FileInfoSize);
  if (FileInfo == NULL) {
    Handle->Close (Handle);
    return NULL;
  }

  //
  // Walk through each file in the directory
  //
  FileIndex = 0;
  TempName = NULL;
  while (TRUE) {
    //
    // Read a file entry
    //
    FileInfoSize = sizeof(EFI_FILE_INFO) + 1024;

    Status = Handle->Read (
                       Handle,
                       &FileInfoSize,
                       FileInfo
                       );
    if (EFI_ERROR (Status) || (FileInfoSize == 0)) {
      break;
    }

    if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) == 0) {
      //
      // This is a file
      //

      //
      // Only deal with the EFI key file
      //
      if (!StrEndWith (FileInfo->FileName, FileName)) {
        continue;
      }

      if (FileIndex == *Index) {
        TempName = StrDuplicate (FileInfo->FileName);
        *Index = *Index + 1;
        break;
      }
      FileIndex ++;
    }
  }

  //
  // Free resources
  //
  gBS->FreePool (FileInfo);
  Handle->Close (Handle);

  return TempName;
}
