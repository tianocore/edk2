/** @file

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  
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

#define SP804_TIMER_METRONOME_BASE    (UINTN)PcdGet32 (PcdSP804TimerPerformanceBase)
#define SP804_TIMER_PERFORMANCE_BASE  (UINTN)PcdGet32 (PcdSP804TimerMetronomeBase)

// Setup SP810's Timer2 for managing delay functions. And Timer3 for Performance counter
// Note: ArmVE's Timer0 and Timer1 are used by TimerDxe.
RETURN_STATUS
EFIAPI
TimerConstructor (
  VOID
  )
{
  // Check if Timer 2 is already initialized
  if (MmioRead32(SP804_TIMER_METRONOME_BASE + SP804_TIMER_CONTROL_REG) & SP804_TIMER_CTRL_ENABLE) {
    return RETURN_SUCCESS;
  } else {
    // Configure timer 2 for one shot operation, 32 bits, no prescaler, and interrupt disabled
    MmioOr32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ONESHOT | SP804_TIMER_CTRL_32BIT | SP804_PRESCALE_DIV_1);

    // Preload the timer count register
    MmioWrite32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_LOAD_REG, 1);

    // Enable the timer
    MmioOr32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ENABLE);
  }

  // Check if Timer 3 is already initialized
  if (MmioRead32(SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CONTROL_REG) & SP804_TIMER_CTRL_ENABLE) {
    return RETURN_SUCCESS;
  } else {
    // Configure timer 3 for free running operation, 32 bits, no prescaler, interrupt disabled
    MmioOr32 (SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_32BIT | SP804_PRESCALE_DIV_1);

    // Enable the timer
    MmioOr32 (SP804_TIMER_PERFORMANCE_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ENABLE);
  }

  return RETURN_SUCCESS;
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
  IN  UINTN MicroSeconds
  )
{
  UINTN Index;

  // Reload the counter for each 1Mhz to avoid an overflow in the load value
  for (Index = 0; Index < (UINTN)PcdGet32(PcdSP804TimerFrequencyInMHz); Index++) {
    // load the timer count register
    MmioWrite32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_LOAD_REG, MicroSeconds);

    while (MmioRead32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CURRENT_REG) > 0) {
      ;
    }
  }

  return MicroSeconds;
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
  IN  UINTN NanoSeconds
  )
{
  UINTN   Index;
  UINT32  MicroSeconds;

  // Round up to 1us Tick Number
  MicroSeconds =   (UINT32)NanoSeconds / 1000;
  MicroSeconds += ((UINT32)NanoSeconds % 1000) == 0 ? 0 : 1;

  // Reload the counter for each 1Mhz to avoid an overflow in the load value
  for (Index = 0; Index < (UINTN)PcdGet32(PcdSP804TimerFrequencyInMHz); Index++) {
    // load the timer count register
    MmioWrite32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_LOAD_REG, MicroSeconds);

    while (MmioRead32 (SP804_TIMER_METRONOME_BASE + SP804_TIMER_CURRENT_REG) > 0) {
      ;
    }
  }
 
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
  ASSERT(Value > 0);
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
    *StartValue = (UINT64)0ULL;
  }
  
  if (EndValue != NULL) {
    // Timer counts up to 0xFFFFFFFF
    *EndValue = 0xFFFFFFFF;
  }
  
  return PcdGet64 (PcdEmbeddedPerformanceCounterFrequencyInHz);
}
