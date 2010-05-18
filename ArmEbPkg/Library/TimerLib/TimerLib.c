/** @file
  TimerLib for ARM EB. Hardcoded to 100ns period

  This library assume the following initialization, usually done in SEC. 

  // configure SP810 to use 1MHz clock and disable
  MmioAndThenOr32 (EB_SP810_CTRL_BASE + SP810_SYS_CTRL_REG, ~SP810_SYS_CTRL_TIMER2_EN, SP810_SYS_CTRL_TIMER2_TIMCLK);
  // Enable
  MmioOr32 (EB_SP810_CTRL_BASE + SP810_SYS_CTRL_REG, SP810_SYS_CTRL_TIMER2_EN);

  // configure timer 2 for one shot operation, 32 bits, no prescaler, and interrupt disabled
  MmioOr32 (EB_SP804_TIMER2_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ONESHOT | SP804_TIMER_CTRL_32BIT | SP804_PRESCALE_DIV_1);

  // preload the timer count register
  MmioWrite32 (EB_SP804_TIMER2_BASE + SP804_TIMER_LOAD_REG, 1);

  // enable the timer
  MmioOr32 (EB_SP804_TIMER2_BASE + SP804_TIMER_CONTROL_REG, SP804_TIMER_CTRL_ENABLE);


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

#include <ArmEb/ArmEb.h>


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
  UINT64  NanoSeconds;
  
  NanoSeconds = MultU64x32 (MicroSeconds, 1000);

  while (NanoSeconds > (UINTN)-1) { 
    NanoSecondDelay((UINTN)-1);
    NanoSeconds -= (UINTN)-1;
  }

  NanoSecondDelay (NanoSeconds);

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
  UINT32  TickNumber;

  if (NanoSeconds == 0) {
    return NanoSeconds;
  }

  // Round up to 100ns Tick Number
  TickNumber =   (UINT32)NanoSeconds / 100;
  TickNumber += ((UINT32)NanoSeconds % 100) == 0 ? 0 : 1;

  // load the timer count register
  MmioWrite32 (EB_SP804_TIMER2_BASE + SP804_TIMER_LOAD_REG, TickNumber);

  while (MmioRead32 (EB_SP804_TIMER2_BASE + SP804_TIMER_CURRENT_REG) > 0) {
    ;
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
  ASSERT (FALSE);
  return (UINT64)0ULL;
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
  
  return 100;
}


