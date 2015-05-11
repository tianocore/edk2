/** @file

  Copyright (c) 2013-2015, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ArmVExpressInternal.h"
#include <Library/ArmShellCmdLib.h>

CONST EFI_GUID ArmHwA9x4Guid = { 0x2fd21cf6, 0xe6e8, 0x4ff2, { 0xa9, 0xca, 0x3b, 0x9f, 0x00, 0xe9, 0x28, 0x89 } };
CONST EFI_GUID ArmHwA15x2A7x3Guid = { 0xd5e606eb, 0x83df, 0x4e90, { 0x81, 0xe8, 0xc3, 0xdb, 0x2f, 0x77, 0x17, 0x9a } };
CONST EFI_GUID ArmHwA15Guid = { 0x6b8947c2, 0x4287, 0x4d91, { 0x8f, 0xe0, 0xa3, 0x81, 0xea, 0x5b, 0x56, 0x8f } };
CONST EFI_GUID ArmHwA5Guid = { 0xa2cc7663, 0x4d7c, 0x448a, { 0xaa, 0xb5, 0x4c, 0x03, 0x4b, 0x6f, 0xda, 0xb7 } };
CONST EFI_GUID NullGuid = { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

//
// Description of the four hardware platforms :
// just the platform id for the time being.
// Platform ids are defined in ArmVExpressInternal.h for
// all "ArmVExpress-like" platforms (AARCH64 or ARM architecture,
// model or hardware platforms).
//
CONST ARM_VEXPRESS_PLATFORM ArmVExpressPlatforms[] = {
  { ARM_HW_A9x4, &ArmHwA9x4Guid, L"vexpress-v2p-ca9.dtb" },
  { ARM_HW_A15x2_A7x3, &ArmHwA15x2A7x3Guid, L"vexpress-v2p-ca15_a7.dtb" },
  { ARM_HW_A15, &ArmHwA15Guid, L"vexpress-v2p-ca15-tc1.dtb" },
  { ARM_HW_A5, &ArmHwA5Guid, L"vexpress-v2p-ca5s.dtb" },
  { ARM_FVP_VEXPRESS_UNKNOWN, &NullGuid, NULL }
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

/**
 * Generic UEFI Entrypoint for 'ArmHwDxe' driver
 * See UEFI specification for the details of the parameters
 */
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
