/** @file
  UEFI Miscellaneous boot Services Stall service implementation

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Include statements
//

#include "DxeMain.h"

/**
  Internal worker function to call the Metronome Architectural Protocol for
  the number of ticks specified by the UINT64 Counter value.  WaitForTick()
  service of the Metronome Architectural Protocol uses a UINT32 for the number
  of ticks to wait, so this function loops when Counter is larger than 0xffffffff.

  @param  Counter           Number of ticks to wait.

**/
VOID
CoreInternalWaitForTick (
  IN UINT64  Counter
  )
{
  while (RShiftU64 (Counter, 32) > 0) {
    gMetronome->WaitForTick (gMetronome, 0xffffffff);
    Counter -= 0xffffffff;
  }
  gMetronome->WaitForTick (gMetronome, (UINT32)Counter);
}

/**
  Introduces a fine-grained stall.

  @param  Microseconds           The number of microseconds to stall execution.

  @retval EFI_SUCCESS            Execution was stalled for at least the requested
                                 amount of microseconds.
  @retval EFI_NOT_AVAILABLE_YET  gMetronome is not available yet

**/
EFI_STATUS
EFIAPI
CoreStall (
  IN UINTN            Microseconds
  )
{
  UINT64  Counter;
  UINT32  Remainder;
  UINTN   Index;

  if (gMetronome == NULL) {
    return EFI_NOT_AVAILABLE_YET;
  }

  //
  // Counter = Microseconds * 10 / gMetronome->TickPeriod
  // 0x1999999999999999 = (2^64 - 1) / 10
  //
  if ((UINT64) Microseconds > 0x1999999999999999ULL) {
    //
    // Microseconds is too large to multiple by 10 first.  Perform the divide
    // operation first and loop 10 times to avoid 64-bit math overflow.
    //
    Counter = DivU64x32Remainder (
                Microseconds,
                gMetronome->TickPeriod,
                &Remainder
                );
    for (Index = 0; Index < 10; Index++) {
      CoreInternalWaitForTick (Counter);
    }

    if (Remainder != 0) {
      //
      // If Remainder was not zero, then normally, Counter would be rounded
      // up by 1 tick.  In this case, since a loop for 10 counts was used
      // to emulate the multiply by 10 operation, Counter needs to be rounded
      // up by 10 counts.
      //
      CoreInternalWaitForTick (10);
    }
  } else {
    //
    // Calculate the number of ticks by dividing the number of microseconds by
    // the TickPeriod.  Calculation is based on 100ns unit.
    //
    Counter = DivU64x32Remainder (
                MultU64x32 (Microseconds, 10),
                gMetronome->TickPeriod,
                &Remainder
                );
    if (Remainder != 0) {
      //
      // If Remainder is not zero, then round Counter up by one tick.
      //
      Counter++;
    }
    CoreInternalWaitForTick (Counter);
  }

  return EFI_SUCCESS;
}
