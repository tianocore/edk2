/** @file
SMM Timer feature support

Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PiSmmCpuDxeSmm.h"

UINT64  mTimeoutTicker = 0;
//
//  Number of counts in a roll-over cycle of the performance counter.
//
UINT64  mCycle = 0;
//
// Flag to indicate the performance counter is count-up or count-down.
//
BOOLEAN  mCountDown;

/**
  Initialize Timer for SMM AP Sync.

**/
VOID
InitializeSmmTimer (
  VOID
  )
{
  UINT64  TimerFrequency;
  UINT64  Start;
  UINT64  End;

  TimerFrequency = GetPerformanceCounterProperties (&Start, &End);
  mTimeoutTicker = DivU64x32 (
                     MultU64x64 (TimerFrequency, PcdGet64 (PcdCpuSmmApSyncTimeout)),
                     1000 * 1000
                     );
  if (End < Start) {
    mCountDown = TRUE;
    mCycle     = Start - End;
  } else {
    mCountDown = FALSE;
    mCycle     = End - Start;
  }
}

/**
  Start Timer for SMM AP Sync.

**/
UINT64
EFIAPI
StartSyncTimer (
  VOID
  )
{
  return GetPerformanceCounter ();
}

/**
  Check if the SMM AP Sync timer is timeout.

  @param Timer  The start timer from the begin.

**/
BOOLEAN
EFIAPI
IsSyncTimerTimeout (
  IN      UINT64  Timer
  )
{
  UINT64  CurrentTimer;
  UINT64  Delta;

  CurrentTimer = GetPerformanceCounter ();
  //
  // We need to consider the case that CurrentTimer is equal to Timer
  // when some timer runs too slow and CPU runs fast. We think roll over
  // condition does not happen on this case.
  //
  if (mCountDown) {
    //
    // The performance counter counts down.  Check for roll over condition.
    //
    if (CurrentTimer <= Timer) {
      Delta = Timer - CurrentTimer;
    } else {
      //
      // Handle one roll-over.
      //
      Delta = mCycle - (CurrentTimer - Timer) + 1;
    }
  } else {
    //
    // The performance counter counts up.  Check for roll over condition.
    //
    if (CurrentTimer >= Timer) {
      Delta = CurrentTimer - Timer;
    } else {
      //
      // Handle one roll-over.
      //
      Delta = mCycle - (Timer - CurrentTimer) + 1;
    }
  }

  return (BOOLEAN)(Delta >= mTimeoutTicker);
}
