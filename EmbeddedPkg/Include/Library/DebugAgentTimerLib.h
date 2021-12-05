/** @file
  Platform specific Debug Agent abstraction for timer used by the agent.

  The timer is used by the debugger to break into a running program.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GDB_TIMER_LIB__
#define __GDB_TIMER_LIB__

/**
  Setup all the hardware needed for the debug agents timer.

  This function is used to set up debug environment. It may enable interrupts.

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
