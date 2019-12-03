/** @file
  Header file for 'acpiview' Shell command functions.

  Copyright (c) 2016 - 2017, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef UEFI_SHELL_ACPIVIEW_COMMAND_LIB_H_
#define UEFI_SHELL_ACPIVIEW_COMMAND_LIB_H_

extern EFI_HII_HANDLE gShellAcpiViewHiiHandle;

/**
  Function for 'acpiview' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunAcpiView (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif // UEFI_SHELL_ACPIVIEW_COMMAND_LIB_H_
