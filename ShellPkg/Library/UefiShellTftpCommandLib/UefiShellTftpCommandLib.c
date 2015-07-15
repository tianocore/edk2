/** @file
  Main file for NULL named library for 'tftp' Shell command functions.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "UefiShellTftpCommandLib.h"

CONST CHAR16 gShellTftpFileName[] = L"ShellCommand";
EFI_HANDLE gShellTftpHiiHandle = NULL;

/**
  Return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameTftp (
  VOID
  )
{
  return gShellTftpFileName;
}

/**
  Constructor for the Shell Tftp Command library.

  Install the handlers for Tftp UEFI Shell command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The Shell command handlers were installed sucessfully.
  @retval EFI_UNSUPPORTED       The Shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellTftpCommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellTftpHiiHandle = NULL;

  //
  // check our bit of the profiles mask
  //
  if ((PcdGet8 (PcdShellProfileMask) & BIT3) == 0) {
    return EFI_SUCCESS;
  }

  gShellTftpHiiHandle = HiiAddPackages (
                          &gShellTftpHiiGuid, gImageHandle,
                          UefiShellTftpCommandLibStrings, NULL
                          );
  if (gShellTftpHiiHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }
  //
  // Install our Shell command handler
  //
  ShellCommandRegisterCommandName (
     L"tftp", ShellCommandRunTftp, ShellCommandGetManFileNameTftp, 0,
     L"tftp", TRUE , gShellTftpHiiHandle, STRING_TOKEN (STR_GET_HELP_TFTP)
     );

  return EFI_SUCCESS;
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellTftpCommandLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellTftpHiiHandle != NULL) {
    HiiRemovePackages (gShellTftpHiiHandle);
  }
  return EFI_SUCCESS;
}
