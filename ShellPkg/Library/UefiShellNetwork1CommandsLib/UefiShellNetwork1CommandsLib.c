/** @file
  Main file for NULL named library for network1 shell command functions.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "UefiShellNetwork1CommandsLib.h"

CONST CHAR16    gShellNetwork1FileName[] = L"ShellCommands";
EFI_HII_HANDLE  gShellNetwork1HiiHandle  = NULL;

/**
  return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
CONST CHAR16 *
EFIAPI
ShellCommandGetManFileNameNetwork1 (
  VOID
  )
{
  return (gShellNetwork1FileName);
}

/**
  Constructor for the Shell Network1 Commands library.

  Install the handlers for Network1 UEFI Shell 2.0 profile commands.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The shell command handlers were installed successfully.
  @retval EFI_UNSUPPORTED       The shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellNetwork1CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellNetwork1HiiHandle = NULL;

  //
  // check our bit of the profiles mask
  //
  if ((PcdGet8 (PcdShellProfileMask) & BIT3) == 0) {
    return (EFI_SUCCESS);
  }

  gShellNetwork1HiiHandle = HiiAddPackages (&gShellNetwork1HiiGuid, gImageHandle, UefiShellNetwork1CommandsLibStrings, NULL);
  if (gShellNetwork1HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers
  //
  ShellCommandRegisterCommandName (L"ping", ShellCommandRunPing, ShellCommandGetManFileNameNetwork1, 0, L"network1", TRUE, gShellNetwork1HiiHandle, STRING_TOKEN (STR_GET_HELP_PING));
  ShellCommandRegisterCommandName (L"ifconfig", ShellCommandRunIfconfig, ShellCommandGetManFileNameNetwork1, 0, L"network1", TRUE, gShellNetwork1HiiHandle, STRING_TOKEN (STR_GET_HELP_IFCONFIG));

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellNetwork1CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellNetwork1HiiHandle != NULL) {
    HiiRemovePackages (gShellNetwork1HiiHandle);
  }

  return (EFI_SUCCESS);
}
