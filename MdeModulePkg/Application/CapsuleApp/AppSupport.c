/** @file
  A shell application that triggers capsule update process.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CapsuleApp.h"

UINTN               Argc;
CHAR16              **Argv;
EFI_SHELL_PROTOCOL  *mShellProtocol = NULL;

/**

  This function parse application ARG.

  @return Status
**/
EFI_STATUS
GetArg (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters;

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellParameters
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Argc = ShellParameters->Argc;
  Argv = ShellParameters->Argv;
  return EFI_SUCCESS;
}

/**
  Get shell protocol.

  @return Pointer to shell protocol.
**/
EFI_SHELL_PROTOCOL *
GetShellProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mShellProtocol == NULL) {
    Status = gBS->LocateProtocol (
                    &gEfiShellProtocolGuid,
                    NULL,
                    (VOID **)&mShellProtocol
                    );
    if (EFI_ERROR (Status)) {
      mShellProtocol = NULL;
    }
  }

  return mShellProtocol;
}

/**
  Read a file.

  @param[in]  FileName        The file to be read.
  @param[out] BufferSize      The file buffer size
  @param[out] Buffer          The file buffer

  @retval EFI_SUCCESS    Read file successfully
  @retval EFI_NOT_FOUND  Shell protocol or file not found
  @retval others         Read file failed
**/
EFI_STATUS
ReadFileToBuffer (
  IN  CHAR16  *FileName,
  OUT UINTN   *BufferSize,
  OUT VOID    **Buffer
  )
{
  EFI_STATUS          Status;
  EFI_SHELL_PROTOCOL  *ShellProtocol;
  SHELL_FILE_HANDLE   Handle;
  UINT64              FileSize;
  UINTN               TempBufferSize;
  VOID                *TempBuffer;

  ShellProtocol = GetShellProtocol ();
  if (ShellProtocol == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Open file by FileName.
  //
  Status = ShellProtocol->OpenFileByName (
                            FileName,
                            &Handle,
                            EFI_FILE_MODE_READ
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the file size.
  //
  Status = ShellProtocol->GetFileSize (Handle, &FileSize);
  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (Handle);
    return Status;
  }

  TempBufferSize = (UINTN)FileSize;
  TempBuffer     = AllocateZeroPool (TempBufferSize);
  if (TempBuffer == NULL) {
    ShellProtocol->CloseFile (Handle);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Read the file data to the buffer
  //
  Status = ShellProtocol->ReadFile (
                            Handle,
                            &TempBufferSize,
                            TempBuffer
                            );
  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (Handle);
    return Status;
  }

  ShellProtocol->CloseFile (Handle);

  *BufferSize = TempBufferSize;
  *Buffer     = TempBuffer;
  return EFI_SUCCESS;
}

/**
  Write a file.

  @param[in] FileName        The file to be written.
  @param[in] BufferSize      The file buffer size
  @param[in] Buffer          The file buffer

  @retval EFI_SUCCESS    Write file successfully
  @retval EFI_NOT_FOUND  Shell protocol not found
  @retval others         Write file failed
**/
EFI_STATUS
WriteFileFromBuffer (
  IN  CHAR16  *FileName,
  IN  UINTN   BufferSize,
  IN  VOID    *Buffer
  )
{
  EFI_STATUS          Status;
  EFI_SHELL_PROTOCOL  *ShellProtocol;
  SHELL_FILE_HANDLE   Handle;
  EFI_FILE_INFO       *FileInfo;
  UINTN               TempBufferSize;

  ShellProtocol = GetShellProtocol ();
  if (ShellProtocol == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Open file by FileName.
  //
  Status = ShellProtocol->OpenFileByName (
                            FileName,
                            &Handle,
                            EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE
                            );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Empty the file contents.
  //
  FileInfo = ShellProtocol->GetFileInfo (Handle);
  if (FileInfo == NULL) {
    ShellProtocol->CloseFile (Handle);
    return EFI_DEVICE_ERROR;
  }

  //
  // If the file size is already 0, then it has been empty.
  //
  if (FileInfo->FileSize != 0) {
    //
    // Set the file size to 0.
    //
    FileInfo->FileSize = 0;
    Status             = ShellProtocol->SetFileInfo (Handle, FileInfo);
    if (EFI_ERROR (Status)) {
      FreePool (FileInfo);
      ShellProtocol->CloseFile (Handle);
      return Status;
    }
  }

  FreePool (FileInfo);

  //
  // Write the file data from the buffer
  //
  TempBufferSize = BufferSize;
  Status         = ShellProtocol->WriteFile (
                                    Handle,
                                    &TempBufferSize,
                                    Buffer
                                    );
  if (EFI_ERROR (Status)) {
    ShellProtocol->CloseFile (Handle);
    return Status;
  }

  ShellProtocol->CloseFile (Handle);

  return EFI_SUCCESS;
}
