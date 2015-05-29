/** @file
  Support ResetSystem Runtime call using PSCI calls

  Note: A similar library is implemented in
  ArmPkg/Library/ArmPsciResetSystemLib. Similar issues might
  exist in this implementation too.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/EfiResetSystemLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmHvcLib.h>

#include <IndustryStandard/ArmStdSmc.h>

STATIC UINT32 mArmPsciMethod;

RETURN_STATUS
EFIAPI
ArmPsciResetSystemLibConstructor (
  VOID
  )
{
  mArmPsciMethod = PcdGet32 (PcdArmPsciMethod);
  return RETURN_SUCCESS;
}

/**
  Resets the entire platform.

  @param  ResetType             The type of reset to perform.
  @param  ResetStatus           The status code for the reset.
  @param  DataSize              The size, in bytes, of WatchdogData.
  @param  ResetData             For a ResetType of EfiResetCold, EfiResetWarm, or
                                EfiResetShutdown the data buffer starts with a Null-terminated
                                Unicode string, optionally followed by additional binary data.

**/
EFI_STATUS
EFIAPI
LibResetSystem (
  IN EFI_RESET_TYPE   ResetType,
  IN EFI_STATUS       ResetStatus,
  IN UINTN            DataSize,
  IN CHAR16           *ResetData OPTIONAL
  )
{
  ARM_SMC_ARGS ArmSmcArgs;
  ARM_HVC_ARGS ArmHvcArgs;

  switch (ResetType) {

  case EfiResetPlatformSpecific:
    // Map the platform specific reset as reboot
  case EfiResetWarm:
    // Map a warm reset into a cold reset
  case EfiResetCold:
    // Send a PSCI 0.2 SYSTEM_RESET command
    ArmSmcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_RESET;
    ArmHvcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_RESET;
    break;
  case EfiResetShutdown:
    // Send a PSCI 0.2 SYSTEM_OFF command
    ArmSmcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_OFF;
    ArmHvcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_OFF;
    break;
  default:
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  switch (mArmPsciMethod) {
  case 1:
    ArmCallHvc (&ArmHvcArgs);
    break;

  case 2:
    ArmCallSmc (&ArmSmcArgs);
    break;

  default:
    DEBUG ((EFI_D_ERROR, "%a: no PSCI method defined\n", __FUNCTION__));
    return EFI_UNSUPPORTED;
  }

  // We should never be here
  DEBUG ((EFI_D_ERROR, "%a: PSCI Reset failed\n", __FUNCTION__));
  CpuDeadLoop ();
  return EFI_UNSUPPORTED;
}

/**
  Initialize any infrastructure required for LibResetSystem () to function.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
LibInitializeResetSystem (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
