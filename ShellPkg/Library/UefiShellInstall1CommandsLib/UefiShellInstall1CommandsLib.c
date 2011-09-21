/** @file
  Main file for NULL named library for install1 shell command functions.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellInstall1CommandsLib.h"

STATIC CONST CHAR16 mFileName[] = L"ShellCommands";
EFI_HANDLE gShellInstall1HiiHandle = NULL;

/**
  Function to get the filename with help context if HII will not be used.

  @return   The filename with help text in it.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameInstall1 (
  VOID
  )
{
  return (mFileName);
}

/**
  Constructor for the Shell Level 1 Commands library.

  Install the handlers for level 1 UEFI Shell 2.0 commands.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
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
  if ((PcdGet8(PcdShellProfileMask) & BIT2) == 0) {
    return (EFI_SUCCESS);
  }

  gShellInstall1HiiHandle = HiiAddPackages (&gShellInstall1HiiGuid, gImageHandle, UefiShellInstall1CommandsLibStrings, NULL);
  if (gShellInstall1HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers that are always installed
  //
  ShellCommandRegisterCommandName(L"bcfg", ShellCommandRunBcfgInstall , ShellCommandGetManFileNameInstall1, 0, L"Install", FALSE, gShellInstall1HiiHandle, STRING_TOKEN(STR_GET_HELP_BCFG));

  return (EFI_SUCCESS);
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
  if (gShellInstall1HiiHandle != NULL) {
    HiiRemovePackages(gShellInstall1HiiHandle);
  }
  return (EFI_SUCCESS);
}
