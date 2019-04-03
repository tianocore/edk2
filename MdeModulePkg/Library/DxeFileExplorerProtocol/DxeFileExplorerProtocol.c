/** @file
  Instance of file explorer Library based on gEfiFileExplorerProtocolGuid.

  Implement the file explorer library instance by wrap the interface
  provided in the file explorer protocol. This protocol is defined as the internal
  protocol related to this implementation, not in the public spec. So, this
  library instance is only for this code base.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Base.h>
#include <Protocol/FileExplorer.h>

#include <Library/FileExplorerLib.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

EFI_FILE_EXPLORER_PROTOCOL *mProtocol = NULL;

/**
  The constructor function caches the pointer to file explorer protocol.

  The constructor function locates Print2 protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
FileExplorerConstructor (
  IN EFI_HANDLE                ImageHandle,
  IN EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_STATUS                   Status;

  Status = SystemTable->BootServices->LocateProtocol (
                                        &gEfiFileExplorerProtocolGuid,
                                        NULL,
                                        (VOID**) &mProtocol
                                        );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mProtocol != NULL);

  return Status;
}

/**
  Choose a file in the specified directory.

  If user input NULL for the RootDirectory, will choose file in the system.

  If user input *File != NULL, function will return the allocate device path
  info for the choosed file, caller has to free the memory after use it.

  @param  RootDirectory    Pointer to the root directory.
  @param  FileType         The file type need to choose.
  @param  ChooseHandler    Function pointer to the extra task need to do
                           after choose one file.
  @param  File             Return the device path for the last time chosed file.

  @retval EFI_SUCESS             Choose file success.
  @retval EFI_INVALID_PARAMETER  Both ChooseHandler and return device path are NULL
                                 One of them must not NULL.
  @retval Other errors           Choose file failed.
**/
EFI_STATUS
EFIAPI
ChooseFile (
  IN  EFI_DEVICE_PATH_PROTOCOL  *RootDirectory,
  IN  CHAR16                    *FileType,  OPTIONAL
  IN  CHOOSE_HANDLER            ChooseHandler,  OPTIONAL
  OUT EFI_DEVICE_PATH_PROTOCOL  **File  OPTIONAL
  )
{
  return mProtocol->ChooseFile (RootDirectory, FileType, ChooseHandler, File);
}

