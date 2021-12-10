/** @file
  Header file for BCFG command library.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BCFG_COMMAND_LIB_H_
#define _BCFG_COMMAND_LIB_H_

/**
  "Constructor" for the library.

  This will register the handler for the bcfg command.

  @param[in] ImageHandle    the image handle of the process
  @param[in] SystemTable    the EFI System Table pointer
  @param[in] Name           the profile name to use

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
BcfgLibraryRegisterBcfgCommand (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable,
  IN CONST CHAR16      *Name
  );

/**
  "Destructor" for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
**/
EFI_STATUS
EFIAPI
BcfgLibraryUnregisterBcfgCommand (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif
