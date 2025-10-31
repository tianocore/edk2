/** @file
  Base Library CPU Functions for all architectures.

  Copyright (c) 2006 - 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/PvPanicLib.h>

static volatile UINTN  mDeadLoopComparator = 0;

/**
  Executes an infinite loop.

  Forces the CPU to execute an infinite loop. A debugger may be used to skip
  past the loop and the code that follows the loop must execute properly. This
  implies that the infinite loop must not cause the code that follow it to be
  optimized away.

**/
VOID
EFIAPI
CpuDeadLoop (
  VOID
  )
{
  volatile UINTN  Index;

  PvPanicLibSendEventGuestPanicked ();

  for (Index = mDeadLoopComparator; Index == mDeadLoopComparator;) {
    CpuPause ();
  }
}
