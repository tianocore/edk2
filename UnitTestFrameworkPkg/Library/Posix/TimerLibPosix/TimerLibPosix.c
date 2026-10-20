/** @file
  Instance of Timer Library based on the host's standard C library.

  Uses the C11 timespec_get() API to provide a free running performance
  counter and calibrated delays for host-based unit tests.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <time.h>

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>

///
/// The host performance counter provided by this library counts in
/// nanoseconds, so its frequency is one billion ticks per second.
///
#define TIMER_LIB_POSIX_NANO_SECONDS_PER_SECOND        1000000000ULL
#define TIMER_LIB_POSIX_NANO_SECONDS_PER_MICRO_SECOND  1000ULL

/**
  Reads the host's real time clock and returns the current time in nanoseconds.

  @return The current host time in nanoseconds.

**/
STATIC
UINT64
GetHostTimeInNanoSeconds (
  VOID
  )
{
  struct timespec  Time;

  if (timespec_get (&Time, TIME_UTC) == 0) {
    //
    // The host clock could not be read. This is not expected on any
    // supported host, so assert to surface the failure to the developer.
    //
    ASSERT (FALSE);
    return 0;
  }

  return ((UINT64)Time.tv_sec * TIMER_LIB_POSIX_NANO_SECONDS_PER_SECOND) +
         (UINT64)Time.tv_nsec;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds inputted.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN  NanoSeconds
  )
{
  UINT64  Start;

  Start = GetHostTimeInNanoSeconds ();
  while ((GetHostTimeInNanoSeconds () - Start) < (UINT64)NanoSeconds) {
    //
    // Busy wait until the host clock has advanced by the requested delay.
    //
  }

  return NanoSeconds;
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds inputted.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN  MicroSeconds
  )
{
  UINT64  Start;
  UINT64  DelayInNanoSeconds;

  DelayInNanoSeconds = (UINT64)MicroSeconds * TIMER_LIB_POSIX_NANO_SECONDS_PER_MICRO_SECOND;
  Start              = GetHostTimeInNanoSeconds ();
  while ((GetHostTimeInNanoSeconds () - Start) < DelayInNanoSeconds) {
    //
    // Busy wait until the host clock has advanced by the requested delay.
    //
  }

  return MicroSeconds;
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
  return GetHostTimeInNanoSeconds ();
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
  OUT      UINT64  *StartValue   OPTIONAL,
  OUT      UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = MAX_UINT64;
  }

  return TIMER_LIB_POSIX_NANO_SECONDS_PER_SECOND;
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
  IN      UINT64  Ticks
  )
{
  //
  // The performance counter ticks are already in nanoseconds
  // (frequency is TIMER_LIB_POSIX_NANO_SECONDS_PER_SECOND), so the elapsed
  // ticks are equal to the elapsed time in nanoseconds.
  //
  return Ticks;
}
