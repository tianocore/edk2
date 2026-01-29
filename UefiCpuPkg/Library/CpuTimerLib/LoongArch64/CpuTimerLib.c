/** @file
  CPUCFG 0x4 and 0x5 for Stable Counter frequency instance of Timer Library.

  Copyright (c) 2024, Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/SafeIntLib.h>
#include <Library/TimerLib.h>
#include <Register/LoongArch64/Cpucfg.h>

/**
  Calculate clock frequency using CPUCFG 0x4 and 0x5 registers.

  @param  VOID.

  @return The frequency in Hz.

**/
STATIC
UINT64
CalcConstFreq (
  VOID
  )
{
  UINT32                 BaseFreq;
  UINT64                 ClockMultiplier;
  UINT32                 ClockDivide;
  CPUCFG_REG4_INFO_DATA  CcFreq;
  CPUCFG_REG5_INFO_DATA  CpucfgReg5Data;
  UINT64                 StableTimerFreq;

  //
  // Get the the crystal frequency corresponding to the constant
  // frequency timer and the clock used by the timer.
  //
  AsmCpucfg (CPUCFG_REG4_INFO, &CcFreq.Uint32);

  //
  // Get the multiplication factor and frequency division factor
  // corresponding to the constant frequency timer and the clock
  // used by the timer.
  //
  AsmCpucfg (CPUCFG_REG5_INFO, &CpucfgReg5Data.Uint32);

  BaseFreq        = CcFreq.Bits.CC_FREQ;
  ClockMultiplier = CpucfgReg5Data.Bits.CC_MUL & 0xFFFF;
  ClockDivide     = CpucfgReg5Data.Bits.CC_DIV & 0xFFFF;

  if ((BaseFreq == 0x0) || (ClockMultiplier == 0x0) || (ClockDivide == 0x0)) {
    DEBUG ((
      DEBUG_ERROR,
      "LoongArch Stable Timer is not available in the CPU, hence this library cannot be used.\n"
      ));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }

  StableTimerFreq = ((ClockMultiplier * BaseFreq) / ClockDivide);

  ASSERT (StableTimerFreq != 0);

  return StableTimerFreq;
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN UINTN  MicroSeconds
  )
{
  UINT64         CurrentTicks, ExceptedTicks, Remaining;
  RETURN_STATUS  Status;

  Status = SafeUint64Mult (MicroSeconds, CalcConstFreq (), &Remaining);
  ASSERT_RETURN_ERROR (Status);

  ExceptedTicks  = DivU64x32 (Remaining, 1000000U);
  CurrentTicks   = AsmReadStableCounter ();
  ExceptedTicks += CurrentTicks;

  do {
    CurrentTicks = AsmReadStableCounter ();
  } while (CurrentTicks < ExceptedTicks);

  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN UINTN  NanoSeconds
  )
{
  UINTN  MicroSeconds;

  // Round up to 1us Tick Number
  MicroSeconds  = NanoSeconds / 1000;
  MicroSeconds += ((NanoSeconds % 1000) == 0) ? 0 : 1;

  MicroSecondDelay (MicroSeconds);

  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running Stable Counter.

  The LoongArch defines a constant frequency timer, whose main body is a
  64-bit counter called StableCounter. StableCounter is set to 0 after
  reset, and then increments by 1 every counting clock cycle. When the
  count reaches all 1s, it automatically wraps around to 0 and continues
  to increment.
  The properties of the Stable Counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the Stable Counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  //
  // Just return the value of Stable Counter.
  //
  return AsmReadStableCounter ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of Stable Counter
  values.

  If StartValue is not NULL, then the value that the stbale counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the stable counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the system frequency in Hz is always returned.

  @param  StartValue  The value the stable counter starts with when it
                      rolls over.
  @param  EndValue    The value that the stable counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT UINT64  *StartValue   OPTIONAL,
  OUT UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xFFFFFFFFFFFFFFFFULL;
  }

  return CalcConstFreq ();
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
  IN UINT64  Ticks
  )
{
  UINT64         Frequency;
  UINT64         NanoSeconds;
  UINT64         Remainder;
  INTN           Shift;
  RETURN_STATUS  Status;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  Status = SafeUint64Mult (
             DivU64x64Remainder (Ticks, Frequency, &Remainder),
             1000000000u,
             &NanoSeconds
             );
  ASSERT_RETURN_ERROR (Status);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift     = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN)Shift);
  Frequency = RShiftU64 (Frequency, (UINTN)Shift);

  Status = SafeUint64Add (
             NanoSeconds,
             DivU64x64Remainder (
               MultU64x32 (Remainder, 1000000000u),
               Frequency,
               NULL
               ),
             &NanoSeconds
             );
  ASSERT_RETURN_ERROR (Status);

  return NanoSeconds;
}
