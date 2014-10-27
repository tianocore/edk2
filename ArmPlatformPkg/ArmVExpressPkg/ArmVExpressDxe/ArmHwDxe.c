/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ArmShellCmdLib.h>

EFI_STATUS
EFIAPI
ArmHwInitialise (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  // Install dynamic Shell command to run baremetal binaries.
  Status = ShellDynCmdRunAxfInstall (ImageHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArmHwDxe: Failed to install ShellDynCmdRunAxf\n"));
  }

  return Status;
}
