/** @file
  Main file for NULL named library for install1 shell command functions.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiDpLib.h"

STATIC CONST CHAR16 mFileName[] = L"ShellCommands";
EFI_HANDLE gDpHiiHandle = NULL;

#define DP_HII_GUID \
  { \
  0xeb832fd9, 0x9089, 0x4898, { 0x83, 0xc9, 0x41, 0x61, 0x8f, 0x5c, 0x48, 0xb9 } \
  }

EFI_GUID gDpHiiGuid = DP_HII_GUID;

/**
  Function to get the filename with help context if HII will not be used.

  @return   The filename with help text in it.
**/
CONST CHAR16*
EFIAPI
UefiDpLibGetManFileName (
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
UefiDpLibConstructor (
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

  //
  // 3rd parameter 'HII strings array' must be name of .uni strings file followed by 'Strings', e.g. mycommands.uni must be
  // specified as 'mycommandsStrings' because the build Autogen process defines this as a string array for the strings in your
  // .uni file.  Examine your Build folder under your package's DEBUG folder and you will find it defined in a xxxStrDefs.h file.
  //
  gDpHiiHandle = HiiAddPackages (&gDpHiiGuid, gImageHandle, UefiDpLibStrings, NULL);
  if (gDpHiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers that are always installed
  //
  ShellCommandRegisterCommandName(L"dp", ShellCommandRunDp , UefiDpLibGetManFileName, 0, L"", FALSE, gDpHiiHandle, STRING_TOKEN(STR_GET_HELP_DP));

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
UefiDpLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gDpHiiHandle != NULL) {
    HiiRemovePackages(gDpHiiHandle);
  }
  return (EFI_SUCCESS);
}
