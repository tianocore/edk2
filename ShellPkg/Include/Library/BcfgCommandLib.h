/** @file
  Header file for BCFG command library.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

