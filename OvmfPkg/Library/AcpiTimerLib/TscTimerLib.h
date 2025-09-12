/** @file
  Internal definitions for TSC Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _TSC_TIMER_LIB_INTERNAL_H_
#define _TSC_TIMER_LIB_INTERNAL_H_

/**
  Get TSC-based performance counter value.

  @return The current value of the free running performance counter.

**/
UINT64
TscGetPerformanceCounter (
  VOID
  );

/**
  Get TSC-based performance counter properties.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
TscGetPerformanceCounterProperties (
  OUT      UINT64  *StartValue   OPTIONAL,
  OUT      UINT64  *EndValue     OPTIONAL
  );

#endif // _TSC_TIMER_LIB_INTERNAL_H_
