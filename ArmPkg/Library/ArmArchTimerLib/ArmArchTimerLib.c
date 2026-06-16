/** @file
  Generic ARM implementation of TimerLib.h

  Copyright (c) 2011 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmGenericTimerCounterLib.h>

/** Get the timer frequency.

  @return The timer frequency.

**/
STATIC
UINTN
EFIAPI
GetPlatformTimerFreq (
  VOID
  )
{
  UINTN  TimerFreq;

  TimerFreq = ArmGenericTimerGetTimerFreq ();

  ASSERT (TimerFreq != 0);

  return TimerFreq;
}

/** Compute (MultA * MultB) / Div

  The function:
  - avoids intermediate overflow
  - rounds up/down the result

  @param[in]  MultA     First multiplicand.
  @param[in]  MultB     Second multiplicand.
  @param[in]  Div       Divisor.
  @param[in]  RoundUp    Whether to round the result up when a remainder is present.

  @return The converted value.

**/
UINTN
EFIAPI
MulDivWithRounding (
  IN UINTN    MultA,
  IN UINTN    MultB,
  IN UINTN    Div,
  IN BOOLEAN  RoundUp
  )
{
  UINT64  Result;
  UINT64  Remainder;

  ASSERT (Div != 0);

  Result = MultU64x64 (
             DivU64x64Remainder (MultA, Div, &Remainder),
             MultB
             );
  Result += DivU64x64Remainder (
              MultU64x64 (
                Remainder,
                MultB
                ),
              Div,
              &Remainder
              );

  if (RoundUp && (Remainder != 0)) {
    Result += 1;
  }

  return (UINTN)Result;
}

/**
  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds  The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds input.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN  NanoSeconds
  )
{
  UINTN   TimerTicks64;
  UINT64  SystemCounterVal;
  UINT64  PreviousSystemCounterVal;
  UINT64  DeltaCounterVal;

  //
  //           Time
  // Ticks = --------- x Frequency
  //           10e9
  //
  TimerTicks64 = MulDivWithRounding (NanoSeconds, GetPlatformTimerFreq (), 1000000000U, TRUE);

  // Read System Counter value
  PreviousSystemCounterVal = ArmGenericTimerGetSystemCount ();

  // Wait until delay count expires.
  while (TimerTicks64 > 0) {
    SystemCounterVal = ArmGenericTimerGetSystemCount ();
    // Get how much we advanced this tick. Wrap around still has delta correct
    DeltaCounterVal = (SystemCounterVal - PreviousSystemCounterVal)
                      & (MAX_UINT64 >> 8); // Account for a lesser (minimum) size
    // Never wrap back around below zero by choosing the min and thus stop at 0
    TimerTicks64            -= MIN (TimerTicks64, DeltaCounterVal);
    PreviousSystemCounterVal = SystemCounterVal;
  }

  return NanoSeconds;
}

/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds input.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN  MicroSeconds
  )
{
  UINTN  InputMicroSeconds;

  InputMicroSeconds = MicroSeconds;

  // Arbitrary chunks of 1s
  while (MicroSeconds >= 1000000U) {
    MicroSeconds -= 1000000U;
    NanoSecondDelay (1000000000U);
  }

  if (MicroSeconds != 0) {
    NanoSecondDelay (MicroSeconds * 1000U);
  }

  return InputMicroSeconds;
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
  // Just return the value of system count
  return ArmGenericTimerGetSystemCount ();
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
    // Timer starts at 0
    *StartValue = (UINT64)0ULL;
  }

  if (EndValue != NULL) {
    // Timer counts up.
    *EndValue = 0xFFFFFFFFFFFFFFFFUL;
  }

  return (UINT64)ArmGenericTimerGetTimerFreq ();
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
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  return MulDivWithRounding (Ticks, 1000000000U, GetPlatformTimerFreq (), FALSE);
}
