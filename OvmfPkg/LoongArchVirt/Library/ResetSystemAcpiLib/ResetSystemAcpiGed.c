/** @file
  ResetSystem library implementation.

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>        // CpuDeadLoop()
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/ResetSystemLib.h> // ResetCold()
#include "ResetSystemAcpiGed.h"

POWER_MANAGER  mPowerManager;

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
STATIC VOID
AcpiGedReset (
  VOID
  )
{
  MmioWrite8 (
    (UINTN)mPowerManager.ResetRegAddr,
    mPowerManager.ResetValue
    );

  CpuDeadLoop ();
}

/**
  This function causes the system to enter a power state equivalent
    to the ACPI S5 states.

 * */
STATIC VOID
AcpiGedShutdown (
  VOID
  )
{
  MmioWrite8 (
    (UINTN)mPowerManager.SleepControlRegAddr,
    (1 << 5) /* enable bit */ |
    (5 << 2) /* typ == S5  */
    );

  CpuDeadLoop ();
}

/**
  This function causes a system-wide reset (cold reset), in which
  all circuitry within the system returns to its initial state. This type of
  reset is asynchronous to system operation and operates without regard to
  cycle boundaries.

  If this function returns, it means that the system does not support cold
  reset.
**/
VOID EFIAPI
ResetCold (
  VOID
  )
{
  AcpiGedReset ();
}

/**
  This function causes a system-wide initialization (warm reset), in which all
  processors are set to their initial state. Pending cycles are not corrupted.

  If this function returns, it means that the system does not support warm
  reset.
**/
VOID EFIAPI
ResetWarm (
  VOID
  )
{
  AcpiGedReset ();
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
  IN UINTN  DataSize,
  IN VOID   *ResetData
  )
{
  AcpiGedReset ();
}

/**
  This function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  If this function returns, it means that the system does not support shut down
  reset.
**/
VOID EFIAPI
ResetShutdown (
  VOID
  )
{
  AcpiGedShutdown ();
}
