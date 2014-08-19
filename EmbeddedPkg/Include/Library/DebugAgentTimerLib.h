/** @file
  Platform specific Debug Agent abstraction for timer used by the agent.

  The timer is used by the debugger to break into a running program.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __GDB_TIMER_LIB__
#define __GDB_TIMER_LIB__



/**
  Setup all the hardware needed for the debug agents timer.

  This function is used to set up debug enviroment. It may enable interrupts.

**/
VOID
EFIAPI
DebugAgentTimerIntialize (
  VOID
  );


/**
  Set the period for the debug agent timer. Zero means disable the timer.

  @param[in] TimerPeriodMilliseconds    Frequency of the debug agent timer.

**/
VOID
EFIAPI
DebugAgentTimerSetPeriod (
  IN  UINT32  TimerPeriodMilliseconds
  );


/**
  Perform End Of Interrupt for the debug agent timer. This is called in the
  interrupt handler after the interrupt has been processed.

**/
VOID
EFIAPI
DebugAgentTimerEndOfInterrupt (
  VOID
  );

#endif


