/** @file
  Template for ArmEb DebugAgentLib.

  For ARM we reserve FIQ for the Debug Agent Timer. We don't care about
  laytency as we only really need the timer to run a few times a second
  (how fast can some one type a ctrl-c?), but it works much better if
  the interrupt we are using to break into the debugger is not being
  used, and masked, by the system.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/DebugAgentTimerLib.h>



/**
  Setup all the hardware needed for the debug agents timer.

  This function is used to set up debug enviroment.

**/
VOID
EFIAPI
DebugAgentTimerIntialize (
  VOID
  )
{
  // Map Timer to FIQ
}


/**
  Set the period for the debug agent timer. Zero means disable the timer.

  @param[in] TimerPeriodMilliseconds    Frequency of the debug agent timer.

**/
VOID
EFIAPI
DebugAgentTimerSetPeriod (
  IN  UINT32  TimerPeriodMilliseconds
  )
{
  if (TimerPeriodMilliseconds == 0) {
    // Disable timer and Disable FIQ
    return;
  }

  // Set timer period and unmask FIQ
}


/**
  Perform End Of Interrupt for the debug agent timer. This is called in the
  interrupt handler after the interrupt has been processed.

**/
VOID
EFIAPI
DebugAgentTimerEndOfInterrupt (
  VOID
  )
{
  // EOI Timer interrupt for FIQ
}
