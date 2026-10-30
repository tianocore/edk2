/** @file
  RISC-V instance of the Timer Library for the Universal Payload.

  The time CSR is free-running. Its frequency comes from
  PcdRiscVTimeBaseFrequency so the FIT entry can delay before any HOB exists.

  Copyright 2026 NewFW Technology

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>

/**
  Return the time CSR frequency in Hz.

  @return The frequency in Hz.

**/
STATIC
UINT64
RiscVTimeBaseFrequency (
  VOID
  )
{
  UINT64  Frequency;

  Frequency = FixedPcdGet32 (PcdRiscVTimeBaseFrequency);
  ASSERT (Frequency != 0);
  return Frequency;
}

/**
  Stall the CPU for at least the given number of timer ticks.

  @param  Delay  The number of ticks to stall.

**/
STATIC
VOID
InternalRiscVTimerDelay (
  IN UINT64  Delay
  )
{
  UINT64  Ticks;

  Ticks = RiscVReadTimer () + Delay;

  while (RiscVReadTimer () <= Ticks) {
    CpuPause ();
  }
}

/**
  Stalls the CPU for at least the given number of microseconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN UINTN  MicroSeconds
  )
{
  InternalRiscVTimerDelay (
    DivU64x32 (
      MultU64x32 (
        MicroSeconds,
        (UINT32)RiscVTimeBaseFrequency ()
        ),
      1000000u
      )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  @param  NanoSeconds  The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN UINTN  NanoSeconds
  )
{
  InternalRiscVTimerDelay (
    DivU64x32 (
      MultU64x32 (
        NanoSeconds,
        (UINT32)RiscVTimeBaseFrequency ()
        ),
      1000000000u
      )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of the 64-bit free-running time CSR.

  @return The current value of the time CSR.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return RiscVReadTimer ();
}

/**
  Retrieves the frequency and rollover range of the time CSR.

  @param  StartValue  The value the counter starts with after rollover.
  @param  EndValue    The value the counter ends with before rollover.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT UINT64  *StartValue, OPTIONAL
  OUT UINT64  *EndValue    OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = MAX_UINT64;
  }

  return RiscVTimeBaseFrequency ();
}

/**
  Converts elapsed timer ticks to nanoseconds.

  @param  Ticks  The number of elapsed ticks.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN UINT64  Ticks
  )
{
  UINT64  NanoSeconds;
  UINT32  Remainder;
  UINT64  Frequency;

  Frequency = RiscVTimeBaseFrequency ();

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x32Remainder (Ticks, (UINT32)Frequency, &Remainder), 1000000000u);
  NanoSeconds += DivU64x32 (MultU64x32 ((UINT64)Remainder, 1000000000u), (UINT32)Frequency);

  return NanoSeconds;
}
