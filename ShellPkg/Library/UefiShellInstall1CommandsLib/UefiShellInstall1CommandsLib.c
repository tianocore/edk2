/** @file
  Main file for NULL named library for install1 shell command functions.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BcfgCommandLib.h>

/**
  Constructor for the Shell Level 1 Commands library.

  Install the handlers for level 1 UEFI Shell 2.0 commands.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed successfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellInstall1CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // check our bit of the profiles mask
  //
  if ((PcdGet8 (PcdShellProfileMask) & BIT2) == 0) {
    return (EFI_SUCCESS);
  }

  return (BcfgLibraryRegisterBcfgCommand (ImageHandle, SystemTable, L"Install1"));
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellInstall1CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return (BcfgLibraryUnregisterBcfgCommand (ImageHandle, SystemTable));
}
