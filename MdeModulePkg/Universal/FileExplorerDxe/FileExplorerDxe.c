/** @file
  This driver produces file explorer protocol layered on top of the FileExplorerLib from the MdeModulePkg.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/FileExplorer.h>
#include <Library/FileExplorerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>

EFI_HANDLE  mFileExplorerThunkHandle = NULL;

CONST EFI_FILE_EXPLORER_PROTOCOL  mFileExplorerProtocol = {
  ChooseFile
};

/**
  The user Entry Point for File explorer module.

  This is the entry point for Print DXE Driver. It installs the file explorer Protocol.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval Others            Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
FileExplorerEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mFileExplorerThunkHandle,
                  &gEfiFileExplorerProtocolGuid,
                  &mFileExplorerProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
