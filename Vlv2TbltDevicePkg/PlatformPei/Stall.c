/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  Stall.c

Abstract:

  Produce Stall Ppi.

--*/


#include "CommonHeader.h"
#include "PlatformBaseAddresses.h"
#include "PchRegs.h"

/**
  Waits for at least the given number of microseconds.

  @param PeiServices     General purpose services available to every PEIM.
  @param This            PPI instance structure.
  @param Microseconds    Desired length of time to wait.

  @retval EFI_SUCCESS    If the desired amount of time was passed.

**/
EFI_STATUS
EFIAPI
Stall (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN CONST EFI_PEI_STALL_PPI      *This,
  IN UINTN              Microseconds
  )
{
  UINTN   Ticks;
  UINTN   Counts;
  UINT32  CurrentTick;
  UINT32  OriginalTick;
  UINT32  RemainingTick;

  if (Microseconds == 0) {
    return EFI_SUCCESS;
  }

  OriginalTick = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_TMR);
  OriginalTick &= (V_PCH_ACPI_PM1_TMR_MAX_VAL - 1);
  CurrentTick = OriginalTick;

  //
  // The timer frequency is 3.579545MHz, so 1 ms corresponds to 3.58 clocks
  //
  Ticks = Microseconds * 358 / 100 + OriginalTick + 1;

  //
  // The loops needed for timer overflow
  //
  Counts = (UINTN)RShiftU64((UINT64)Ticks, 24);

  //
  // Remaining clocks within one loop
  //
  RemainingTick = Ticks & 0xFFFFFF;

  //
  // Do not intend to use TMROF_STS bit of register PM1_STS, because this add extra
  // one I/O operation, and may generate SMI
  //
  while (Counts != 0) {
    CurrentTick = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_TMR) & B_PCH_ACPI_PM1_TMR_VAL;
    if (CurrentTick <= OriginalTick) {
      Counts--;
    }
    OriginalTick = CurrentTick;
  }

  while ((RemainingTick > CurrentTick) && (OriginalTick <= CurrentTick)) {
    OriginalTick  = CurrentTick;
    CurrentTick   = IoRead32 (ACPI_BASE_ADDRESS + R_PCH_ACPI_PM1_TMR) & B_PCH_ACPI_PM1_TMR_VAL;
  }

  return EFI_SUCCESS;
}
