/** @file
  ResetSystemLib implementation using PSCI calls

  Copyright (c) 2017 - 2018, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/ArmMmuLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <IndustryStandard/ArmStdSmc.h>

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

  // Send a PSCI 0.2 SYSTEM_RESET command
  ArmSmcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_RESET;
  ArmCallSmc (&ArmSmcArgs);
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

  // Send a PSCI 0.2 SYSTEM_OFF command
  ArmSmcArgs.Arg0 = ARM_SMC_ID_PSCI_SYSTEM_OFF;
  ArmCallSmc (&ArmSmcArgs);
}

VOID DisableMmuAndReenterPei (VOID);

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
  EFI_PHYSICAL_ADDRESS        Alloc;
  EFI_MEMORY_DESCRIPTOR       *MemMap;
  UINTN                       MemMapSize;
  UINTN                       MapKey, DescriptorSize;
  UINT32                      DescriptorVersion;
  EFI_STATUS                  Status;

  if (FeaturePcdGet (PcdArmReenterPeiForCapsuleWarmReboot) &&
      !EfiAtRuntime ()) {
    //
    // At boot time, we are the only core running, so we can implement the
    // immediate wake (which is used by capsule update) by disabling the MMU
    // and interrupts, and jumping to the PEI entry point.
    //

    //
    // Obtain the size of the memory map
    //
    MemMapSize = 0;
    MemMap = NULL;
    Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize,
                    &DescriptorVersion);
    ASSERT (Status == EFI_BUFFER_TOO_SMALL);

    //
    // Add some slack to the allocation to cater for changes in the memory
    // map if ExitBootServices () fails the first time around.
    //
    MemMapSize += SIZE_4KB;
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData,
                    EFI_SIZE_TO_PAGES (MemMapSize), &Alloc);
    ASSERT_EFI_ERROR (Status);

    MemMap = (EFI_MEMORY_DESCRIPTOR *)(UINTN)Alloc;

    Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize,
                    &DescriptorVersion);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->ExitBootServices (gImageHandle, MapKey);
    if (EFI_ERROR (Status)) {
      //
      // ExitBootServices () may fail the first time around if an event fired
      // right after the call to GetMemoryMap() which allocated or freed memory.
      // Since that first call to ExitBootServices () will disarm the timer,
      // this is guaranteed not to happen again, so one additional attempt
      // should suffice.
      //
      Status = gBS->GetMemoryMap (&MemMapSize, MemMap, &MapKey, &DescriptorSize,
                      &DescriptorVersion);
      ASSERT_EFI_ERROR (Status);

      Status = gBS->ExitBootServices (gImageHandle, MapKey);
      ASSERT_EFI_ERROR (Status);
    }

    DisableMmuAndReenterPei ();
  }
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

/**
  The ResetSystem function resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset.
**/
VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN VOID                         *ResetData OPTIONAL
  )
{
  switch (ResetType) {
  case EfiResetWarm:
    ResetWarm ();
    break;

  case EfiResetCold:
    ResetCold ();
    break;

  case EfiResetShutdown:
    ResetShutdown ();
    return;

  case EfiResetPlatformSpecific:
    ResetPlatformSpecific (DataSize, ResetData);
    return;

  default:
    return;
  }
}
