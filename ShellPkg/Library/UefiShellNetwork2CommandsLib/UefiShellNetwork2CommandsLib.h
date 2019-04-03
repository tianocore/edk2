/** @file
  Main file for NULL named library for network2 shell command functions.

  Copyright (c) 2016, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UEFI_SHELL_NETWORK2_COMMANDS_LIB_H_
#define _UEFI_SHELL_NETWORK2_COMMANDS_LIB_H_

#include <Protocol/Cpu.h>
#include <Protocol/ServiceBinding.h>
#include <Protocol/Ip6.h>
#include <Protocol/Ip6Config.h>

#include <Guid/ShellLibHiiGuid.h>

#include <Library/ShellLib.h>
#include <Library/ShellCommandLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HiiLib.h>
#include <Library/NetLib.h>

extern EFI_HANDLE gShellNetwork2HiiHandle;

/**
  Function for 'ping6' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @retval SHELL_SUCCESS  The ping6 processed successfullly.
  @retval others         The ping6 processed unsuccessfully.

**/
SHELL_STATUS
EFIAPI
ShellCommandRunPing6 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Function for 'ifconfig6' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).

  @retval SHELL_SUCCESS   The ifconfig6 command processed successfully.
  @retval others          The ifconfig6 command process failed.

**/
SHELL_STATUS
EFIAPI
ShellCommandRunIfconfig6 (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

#endif

