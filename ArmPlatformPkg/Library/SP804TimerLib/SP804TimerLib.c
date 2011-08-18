/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2011, ARM Limited. All rights reserved.
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Drivers/SP804Timer.h>

#define SP804_TIMER_METRONOME_BASE    ((UINTN)PcdGet32 (PcdSP804TimerMetronomeBase))
#define SP804_TIMER_PERFORMANCE_BASE  ((UINTN)PcdGet32 (PcdSP804TimerPerformanceBase))

// Setup SP810's Timer2 for managing delay functions. And Timer3 for Performance counter
// Note: ArmVE's Timer0 and Timer1 are used by TimerDxe.
RETURN_STATUS
EFIAPI
TimerConstructor (
  VOID
  )
{
  // Check if the Metronome Timer is already initialized
  if (MmioRead32(SP804_TIMER_METRONOME_BASE + SP804_TIMER_CONTROL_REG) & SP804_TIMER_CTRL_ENABLE) {
    return RETURN_SUCCESS;
  } else {
    // Configure the Metronome Timer for free running operation, 32 bits, no prescaler, and interrupt disabled
    MmioWrite32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_32BIT | SP804_PRESCALE_DIV_1);

    // Start the Metronome Timer ticking
    MmioOr32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ENABLE);
  }

  // Check if the Performance Timer is already initialized
  if (MmioRead32(SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CONTROL_REG) & SP804_TIMER_CTRL_ENABLE) {
    return RETURN_SUCCESS;
  } else {
    // Configure the Performance timer for free running operation, 32 bits, no prescaler, interrupt disabled
    MmioWrite32 (SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_32BIT | SP804_PRESCALE_DIV_1);

    // Start the Performance Timer ticking
    MmioOr32 (SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ENABLE);
  }

  return RETURN_SUCCESS;
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.
  The hardware timer is 32 bits.
  The maximum possible delay is (0xFFFFFFFF / TimerFrequencyMHz), i.e. ([32bits] / FreqInMHz)
  For example:
  +----------------+------------+----------+----------+
  | TimerFrequency |  MaxDelay  | MaxDelay | MaxDelay |
  |     (MHz)      |    (us)    |   (s)    |  (min)   |
  +----------------+------------+----------+----------+
  |        1       | 0xFFFFFFFF |   4294   |   71.5   |
  |        5       | 0x33333333 |    859   |   14.3   |
  |       10       | 0x19999999 |    429   |    7.2   |
  |       50       | 0x051EB851 |     86   |    1.4   |
  +----------------+------------+----------+----------+
  If it becomes necessary to support higher delays, then consider using the
  real time clock.

  During this delay, the cpu is not yielded to any other process, with one exception:
  events that are triggered off a timer and which execute at a higher TPL than
  this function. These events may call MicroSecondDelay (or NanoSecondDelay) to
  fulfil their own needs.
  Therefore, this function must be re-entrant, as it may be interrupted and re-started.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds inputted.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN  UINTN MicroSeconds
  )
{
  UINT64    DelayTicks64;         // Convert from microseconds to timer ticks, more bits to detect over-range conditions.
  UINTN     DelayTicks;           // Convert from microseconds to timer ticks, native size for general calculations.
  UINTN     StartTicks;           // Timer value snapshot at the start of the delay
  UINTN     TargetTicks;          // Timer value to signal the end of the delay
  UINTN     CurrentTicks;         // Current value of the 64-bit timer value at any given moment

  // If we snapshot the timer at the start of the delay function then we minimise unaccounted overheads.
  StartTicks = MmioRead32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CURRENT_REG);

  // We are operating at the limit of 32bits. For the range checking work in 64 bits to avoid overflows.
  DelayTicks64 = MultU64x32((UINT64)MicroSeconds, PcdGet32(PcdSP804TimerFrequencyInMHz));

  // We are limited to 32 bits.
  // If the specified delay is exactly equal to the max range of the timer,
  // then the start will be equal to the stop plus one timer overflow (wrap-around).
  // To avoid having to check for that, reduce the maximum acceptable range by 1 tick,
  // i.e. reject delays equal or greater than the max range of the timer.
  if (DelayTicks64 >= (UINT64)SP804_MAX_TICKS) {
    DEBUG((EFI_D_ERROR,"MicroSecondDelay: ERROR: MicroSeconds=%d exceed SP804 count range. Max MicroSeconds=%d\n",
      MicroSeconds,
      ((UINTN)SP804_MAX_TICKS/PcdGet32(PcdSP804TimerFrequencyInMHz))));
  }
  ASSERT(DelayTicks64 < (UINT64)SP804_MAX_TICKS);

  // From now on do calculations only in native bit size.
  DelayTicks = (UINTN)DelayTicks64;

  // Calculate the target value of the timer.

  //Note: SP804 timer is counting down
  if (StartTicks >= DelayTicks) {
    // In this case we do not expect a wrap-around of the timer to occur.
    // CurrentTicks must be less than StartTicks and higher than TargetTicks.
    // If this is not the case, then the delay has been reached and may even have been exceeded if this
    // function was suspended by a higher priority interrupt.

    TargetTicks = StartTicks - DelayTicks;

    do {
      CurrentTicks = MmioRead32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CURRENT_REG);
    } while ((CurrentTicks > TargetTicks) && (CurrentTicks <= StartTicks));

  } else {
    // In this case TargetTicks is larger than StartTicks.
    // This means we expect a wrap-around of the timer to occur and we must wait for it.
    // Before the wrap-around, CurrentTicks must be less than StartTicks and less than TargetTicks.
    // After the wrap-around, CurrentTicks must be larger than StartTicks and larger than TargetTicks.
    // If this is not the case, then the delay has been reached and may even have been exceeded if this
    // function was suspended by a higher priority interrupt.

    // The order of operations is essential to avoid arithmetic overflow problems
    TargetTicks = ((UINTN)SP804_MAX_TICKS - DelayTicks) + StartTicks;

    // First wait for the wrap-around to occur
    do {
      CurrentTicks = MmioRead32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CURRENT_REG);
    } while (CurrentTicks <= StartTicks);

    // Then wait for the target
    do {
      CurrentTicks = MmioRead32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CURRENT_REG);
    } while (CurrentTicks > TargetTicks);
  }

  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  When the timer frequency is 1MHz, each tick corresponds to 1 microsecond.
  Therefore, the nanosecond delay will be rounded up to the nearest 1 microsecond.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds inputted.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN  UINTN NanoSeconds
  )
{
  UINTN  MicroSeconds;

  // Round up to 1us Tick Number
  MicroSeconds = NanoSeconds / 1000;
  MicroSeconds += ((NanoSeconds % 1000) == 0) ? 0 : 1;

  MicroSecondDelay (MicroSeconds);

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
  // Free running 64-bit/32-bit counter is needed here.
  // Don't think we need this to boot, just to do performance profile
  UINT64 Value;
  Value = MmioRead32 (SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CURRENT_REG);
  return Value;
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
  OUT UINT64  *StartValue,  OPTIONAL
  OUT UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    // Timer starts with the reload value
    *StartValue = 0xFFFFFFFF;
  }
  
  if (EndValue != NULL) {
    // Timer counts down to 0x0
    *EndValue = (UINT64)0ULL;
  }
  
  return PcdGet64 (PcdEmbeddedPerformanceCounterFrequencyInHz);
}
