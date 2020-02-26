/** @file
  Reset System lib using PSCI hypervisor or secure monitor calls

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2013, ARM Ltd. All rights reserved.<BR>
  Copyright (c) 2014-2020, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <libfdt.h>
#include <Library/ArmHvcLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/ResetSystemLib.h>

#include <IndustryStandard/ArmStdSmc.h>

typedef enum {
  PsciMethodUnknown,
  PsciMethodSmc,
  PsciMethodHvc,
} PSCI_METHOD;

STATIC
PSCI_METHOD
DiscoverPsciMethod (
  VOID
  )
{
  VOID                            *DeviceTreeBase;
  INT32                           Node, Prev;
  INT32                           Len;
  CONST CHAR8                     *Compatible;
  CONST CHAR8                     *CompatibleItem;
  CONST VOID                      *Prop;

  DeviceTreeBase = (VOID*)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (fdt_check_header (DeviceTreeBase) == 0);

  //
  // Enumerate all FDT nodes looking for the PSCI node and capture the method
  //
  for (Prev = 0;; Prev = Node) {
    Node = fdt_next_node (DeviceTreeBase, Prev, NULL);
    if (Node < 0) {
      break;
    }

    Compatible = fdt_getprop (DeviceTreeBase, Node, "compatible", &Len);
    if (Compatible == NULL) {
      continue;
    }

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompatibleItem = Compatible; CompatibleItem < Compatible + Len;
      CompatibleItem += 1 + AsciiStrLen (CompatibleItem)) {

      if (AsciiStrCmp (CompatibleItem, "arm,psci-0.2") != 0) {
        continue;
      }

      Prop = fdt_getprop (DeviceTreeBase, Node, "method", NULL);
      if (!Prop) {
        DEBUG ((DEBUG_ERROR, "%a: Missing PSCI method property\n",
          __FUNCTION__));
        return PsciMethodUnknown;
      }

      if (AsciiStrnCmp (Prop, "hvc", 3) == 0) {
        return PsciMethodHvc;
      } else if (AsciiStrnCmp (Prop, "smc", 3) == 0) {
        return PsciMethodSmc;
      } else {
        DEBUG ((DEBUG_ERROR, "%a: Unknown PSCI method \"%a\"\n", __FUNCTION__,
          Prop));
        return PsciMethodUnknown;
      }
    }
  }
  return PsciMethodUnknown;
}

STATIC
VOID
PerformPsciAction (
  IN  UINTN     Arg0
  )
{
  ARM_SMC_ARGS ArmSmcArgs;
  ARM_HVC_ARGS ArmHvcArgs;

  ArmSmcArgs.Arg0 = Arg0;
  ArmHvcArgs.Arg0 = Arg0;

  switch (DiscoverPsciMethod ()) {
  case PsciMethodHvc:
    ArmCallHvc (&ArmHvcArgs);
    break;

  case PsciMethodSmc:
    ArmCallSmc (&ArmSmcArgs);
    break;

  default:
    DEBUG ((DEBUG_ERROR, "%a: no PSCI method defined\n", __FUNCTION__));
    ASSERT (FALSE);
  }
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
  // Send a PSCI 0.2 SYSTEM_RESET command
  PerformPsciAction (ARM_SMC_ID_PSCI_SYSTEM_RESET);
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
  // Send a PSCI 0.2 SYSTEM_OFF command
  PerformPsciAction (ARM_SMC_ID_PSCI_SYSTEM_OFF);
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
