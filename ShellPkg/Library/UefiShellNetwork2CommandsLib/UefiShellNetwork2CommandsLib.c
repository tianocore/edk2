/** @file
  Main file for NULL named library for network2 shell command functions.

  Copyright (c) 2016, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "UefiShellNetwork2CommandsLib.h"

CONST CHAR16    gShellNetwork2FileName[] = L"ShellCommands";
EFI_HII_HANDLE  gShellNetwork2HiiHandle  = NULL;

/**
  return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
CONST CHAR16 *
EFIAPI
ShellCommandGetManFileNameNetwork2 (
  VOID
  )
{
  return (gShellNetwork2FileName);
}

/**
  Constructor for the Shell Network2 Commands library.

  Install the handlers for Network2 UEFI Shell 2.0 profile commands.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The shell command handlers were installed sucessfully.
  @retval EFI_UNSUPPORTED       The shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellNetwork2CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellNetwork2HiiHandle = NULL;

  //
  // check our bit of the profiles mask
  //
  if ((PcdGet8 (PcdShellProfileMask) & BIT4) == 0) {
    return (EFI_SUCCESS);
  }

  gShellNetwork2HiiHandle = HiiAddPackages (&gShellNetwork2HiiGuid, gImageHandle, UefiShellNetwork2CommandsLibStrings, NULL);
  if (gShellNetwork2HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers
  //
  ShellCommandRegisterCommandName (L"ping6", ShellCommandRunPing6, ShellCommandGetManFileNameNetwork2, 0, L"network2", TRUE, gShellNetwork2HiiHandle, STRING_TOKEN (STR_GET_HELP_PING6));
  ShellCommandRegisterCommandName (L"ifconfig6", ShellCommandRunIfconfig6, ShellCommandGetManFileNameNetwork2, 0, L"network2", TRUE, gShellNetwork2HiiHandle, STRING_TOKEN (STR_GET_HELP_IFCONFIG6));

  return EFI_SUCCESS;
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellNetwork2CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellNetwork2HiiHandle != NULL) {
    HiiRemovePackages (gShellNetwork2HiiHandle);
  }

  return EFI_SUCCESS;
}
