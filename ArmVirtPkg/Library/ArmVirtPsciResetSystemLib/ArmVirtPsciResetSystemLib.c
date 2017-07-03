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
#include <Library/ResetSystemLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmHvcLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/ArmStdSmc.h>

#include <Protocol/FdtClient.h>

STATIC UINT32 mArmPsciMethod;

RETURN_STATUS
EFIAPI
ArmPsciResetSystemLibConstructor (
  VOID
  )
{
  EFI_STATUS            Status;
  FDT_CLIENT_PROTOCOL   *FdtClient;
  CONST VOID            *Prop;

  Status = gBS->LocateProtocol (&gFdtClientProtocolGuid, NULL,
                  (VOID **)&FdtClient);
  ASSERT_EFI_ERROR (Status);

  Status = FdtClient->FindCompatibleNodeProperty (FdtClient, "arm,psci-0.2",
                        "method", &Prop, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (AsciiStrnCmp (Prop, "hvc", 3) == 0) {
    mArmPsciMethod = 1;
  } else if (AsciiStrnCmp (Prop, "smc", 3) == 0) {
    mArmPsciMethod = 2;
  } else {
    DEBUG ((EFI_D_ERROR, "%a: Unknown PSCI method \"%a\"\n", __FUNCTION__,
      Prop));
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

/**
  This function causes a system-wide reset (cold reset), in which
  all circuitry within the system returns to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  If this function returns, it means that the system does not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  ARM_SMC_ARGS ArmSmcArgs;
  ARM_HVC_ARGS ArmHvcArgs;

  // Send a PSCI 0.2 SYSTEM_RESET command
  ArmSmcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_RESET;
  ArmHvcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_RESET;

  switch (mArmPsciMethod) {
  case 1:
    ArmCallHvc (&ArmHvcArgs);
    break;

  case 2:
    ArmCallSmc (&ArmSmcArgs);
    break;

  default:
    DEBUG ((EFI_D_ERROR, "%a: no PSCI method defined\n", __FUNCTION__));
  }
}

/**
  This function causes a system-wide initialization (warm reset), in which all processors
  are set to their initial state. Pending cycles are not corrupted.

  If this function returns, it means that the system does not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  // Map a warm reset into a cold reset
  ResetCold ();
}

/**
  This function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  If this function returns, it means that the system does not support shutdown reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  ARM_SMC_ARGS ArmSmcArgs;
  ARM_HVC_ARGS ArmHvcArgs;

  // Send a PSCI 0.2 SYSTEM_OFF command
  ArmSmcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_OFF;
  ArmHvcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_OFF;

  switch (mArmPsciMethod) {
  case 1:
    ArmCallHvc (&ArmHvcArgs);
    break;

  case 2:
    ArmCallSmc (&ArmSmcArgs);
    break;

  default:
    DEBUG ((EFI_D_ERROR, "%a: no PSCI method defined\n", __FUNCTION__));
  }
}

/**
  This function causes the system to enter S3 and then wake up immediately.

  If this function returns, it means that the system does not support S3 feature.
**/
VOID
EFIAPI
EnterS3WithImmediateWake (
  VOID
  )
{
  // not implemented
}

/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string passed
  into ResetData. If the platform does not recognize the EFI_GUID in ResetData
  the platform must pick a supported reset type to perform.The platform may
  optionally log the parameters from any non-normal reset that occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  )
{
  // Map the platform specific reset as reboot
  ResetCold ();
}
