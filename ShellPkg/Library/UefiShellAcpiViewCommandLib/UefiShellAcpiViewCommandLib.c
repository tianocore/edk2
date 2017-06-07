/** @file
  Main file for 'acpiview' Shell command function.

  Copyright (c) 2016, ARM Limited. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Guid/ShellLibHiiGuid.h>
#include <Library/ShellCommandLib.h>
#include <Library/HiiLib.h>
#include "UefiShellAcpiViewCommandLib.h"
#include "AcpiView.h"

CONST CHAR16 gShellAcpiViewFileName[] = L"ShellCommand";


/**
  Return the file name of the help text file if not using HII.

  @return The string pointer to the file name.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameAcpiView (
  VOID
  )
{
  return gShellAcpiViewFileName;
}

/**
  Constructor for the Shell AcpiView Command library.

  Install the handlers for acpiview UEFI Shell command.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.

  @retval EFI_SUCCESS           The Shell command handlers were installed
                                successfully.
  @retval EFI_DEVICE_ERROR      Hii package failed to install.
**/
EFI_STATUS
EFIAPI
ShellAcpiViewCommandLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  gShellAcpiViewHiiHandle = NULL;

  // Check Shell Profile Debug1 bit of the profiles mask
  if ((FixedPcdGet8 (PcdShellProfileMask) & BIT1) == 0) {
    return EFI_SUCCESS;
  }

  gShellAcpiViewHiiHandle = HiiAddPackages (
                              &gShellAcpiViewHiiGuid,
                              gImageHandle,
                              UefiShellAcpiViewCommandLibStrings,
                              NULL
                              );
  if (gShellAcpiViewHiiHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }
  // Install our Shell command handler
  ShellCommandRegisterCommandName (
    L"acpiview",
    ShellCommandRunAcpiView,
    ShellCommandGetManFileNameAcpiView,
    0,
    L"acpiview",
    TRUE,
    gShellAcpiViewHiiHandle,
    STRING_TOKEN (STR_GET_HELP_ACPIVIEW)
    );

  return EFI_SUCCESS;
}

/**
  Destructor for the library. free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
ShellAcpiViewCommandLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellAcpiViewHiiHandle != NULL) {
    HiiRemovePackages (gShellAcpiViewHiiHandle);
  }
  return EFI_SUCCESS;
}
