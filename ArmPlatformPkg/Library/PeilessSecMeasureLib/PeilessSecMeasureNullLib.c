/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Pi/PiMultiPhase.h>

/**
  Measurement for PeilessSec.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid firmware volume information.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasurePeilessSec (
  VOID
  )
{
  /*
   * Nothing to measure.
   */
  return EFI_SUCCESS;
}
