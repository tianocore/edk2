/** @file
  Null Debug Agent timer.

  The debug agent uses the timer so the debugger can break into running programs.
  If you link against this library you will not be able to break into a running
  program with the debugger.

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


/**
  Setup all the hardware needed for the debug agents timer.

  This function is used to set up debug enviroment. It may enable interrupts.

**/
VOID
EFIAPI
DebugAgentTimerIntialize (
  VOID
  )
{
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
}

