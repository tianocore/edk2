/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include "DxeStandaloneMmAcpiTimerLib.h"

/**
  The constructor function enables ACPI IO space, and caches PerformanceCounterFrequency.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeAcpiTimerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return CommonAcpiTimerLibConstructor ();
}
