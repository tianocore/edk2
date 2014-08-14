/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (c) 2013 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>

//
// Cached performance counter frequency
//
UINT64  mPerformanceCounterFrequency = 0;

/**
  Internal function to retrieves the 64-bit frequency in Hz.

  Internal function to retrieves the 64-bit frequency in Hz.

  @return The frequency in Hz.

**/
UINT64
InternalGetPerformanceCounterFrequency (
  VOID
  ) 
{
  BOOLEAN  InterruptState;
  UINT64   Count;

  if (mPerformanceCounterFrequency == 0) {
    InterruptState = SaveAndDisableInterrupts ();
    Count = GetPerformanceCounter ();
    MicroSecondDelay (100);
    mPerformanceCounterFrequency = MultU64x32 (GetPerformanceCounter () - Count, 10000);
    SetInterruptState (InterruptState);
  }
  return  mPerformanceCounterFrequency;
}
