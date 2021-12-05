/** @file
  Header file for debug timer to support debug agent library implementation.

  Copyright (c) 2010 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DEBUG_TIMER_H_
#define _DEBUG_TIMER_H_

/**
  Initialize CPU local APIC timer.

  @param[out] TimerFrequency  Local APIC timer frequency returned.
  @param[in]  DumpFlag        If TRUE, dump Local APIC timer's parameter.

  @return   32-bit Local APIC timer init count.
**/
UINT32
InitializeDebugTimer (
  OUT UINT32   *TimerFrequency,
  IN  BOOLEAN  DumpFlag
  );

/**
  Check if the timer is time out.

  @param[in] TimerCycle             Timer initial count.
  @param[in] Timer                  The start timer from the begin.
  @param[in] TimeoutTicker          Ticker number need time out.

  @return TRUE  Timer time out occurs.
  @retval FALSE Timer does not time out.

**/
BOOLEAN
IsDebugTimerTimeout (
  IN UINT32  TimerCycle,
  IN UINT32  Timer,
  IN UINT32  TimeoutTicker
  );

#endif
