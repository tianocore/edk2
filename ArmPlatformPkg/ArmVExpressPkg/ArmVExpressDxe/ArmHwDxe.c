/** @file

  Copyright (c) 2013-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmVExpressInternal.h"
#include <Library/ArmShellCmdLib.h>

//
// Description of the four hardware platforms :
// just the platform id for the time being.
// Platform ids are defined in ArmVExpressInternal.h for
// all "ArmVExpress-like" platforms (AARCH64 or ARM architecture,
// model or hardware platforms).
//
CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[] = {
  { ARM_HW_A9x4 },
  { ARM_HW_A15x2_A7x3 },
  { ARM_HW_A15 },
  { ARM_HW_A5 },
  { ARM_FVP_VEXPRESS_UNKNOWN }
};

/**
  Get information about the VExpress platform the firmware is running on.

  @param[out]  Platform   Address where the pointer to the platform information
                          (type ARM_VEXPRESS_PLATFORM*) should be stored.
                          The returned pointer does not point to an allocated
                          memory area. Not used here.

  @retval  EFI_NOT_FOUND  The platform was not recognised.

**/
EFI_STATUS
ArmVExpressGetPlatform (
  OUT CONST ARM_VEXPRESS_PLATFORM** Platform
  )
{
  return EFI_NOT_FOUND;
}

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
