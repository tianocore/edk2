/** @file
  Code for debug timer to support debug agent library implementation.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DebugAgent.h"

/**
  Initialize CPU local APIC timer.

**/
VOID
InitializeDebugTimer (
  VOID
  )
{
  UINTN       ApicTimerDivisor;
  UINT32      InitialCount;

  GetApicTimerState (&ApicTimerDivisor, NULL, NULL);

  //
  // Cpu Local Apic timer interrupt frequency, it is set to 0.1s
  //
  InitialCount = (UINT32)DivU64x32 (
                   MultU64x64 (
                     PcdGet32(PcdFSBClock) / (UINT32)ApicTimerDivisor,
                     100
                     ),
                   1000
                   );

  InitializeApicTimer (ApicTimerDivisor, InitialCount, TRUE, DEBUG_TIMER_VECTOR);

  if (MultiProcessorDebugSupport) {
    mDebugMpContext.DebugTimerInitCount = InitialCount;
  }
}

/**
  Enable/Disable the interrupt of debug timer and return the interrupt state
  prior to the operation.

  If EnableStatus is TRUE, enable the interrupt of debug timer.
  If EnableStatus is FALSE, disable the interrupt of debug timer.

  @param[in] EnableStatus    Enable/Disable.

  @retval TRUE  Debug timer interrupt were enabled on entry to this call.
  @retval FALSE Debug timer interrupt were disabled on entry to this call.

**/
BOOLEAN
EFIAPI
SaveAndSetDebugTimerInterrupt (
  IN BOOLEAN                EnableStatus
  )
{
  BOOLEAN     OldInterruptState;
  BOOLEAN     OldDebugTimerInterruptState;

  OldInterruptState = SaveAndDisableInterrupts ();
  OldDebugTimerInterruptState = GetApicTimerInterruptState ();
  
  if (EnableStatus) {
    EnableApicTimerInterrupt ();
  } else {
    DisableApicTimerInterrupt ();
  }

  SetInterruptState (OldInterruptState);
  return OldDebugTimerInterruptState;
}

