/** @file
  A semi-functional instance of the Timer Library.

  Copyright (c) Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/**
  Stalls the CPU for at least the given number of microseconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.
  @return The value of MicroSeconds inputted.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  // Since this is a host based test, we don't actually want to stall
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.
  @return The value of NanoSeconds inputted.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  // Since this is a host based test, we don't actually want to stall
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  The counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  // Return the current number of nanoseconds on the clock
  return ts.tv_nsec;
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if ( EndValue != NULL ) {
    *EndValue = MAX_UINT64;
  }
  if ( StartValue != NULL ) {
    *StartValue = 0;
  }
  // Return the number of nanoseconds in a second
  return 1000000000;
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  return Ticks;
}
