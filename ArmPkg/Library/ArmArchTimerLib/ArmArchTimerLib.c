/** @file
  Generic ARM implementation of TimerLib.h

  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmArchTimerLib.h>

#define TICKS_PER_MICRO_SEC     (PcdGet32 (PcdArmArchTimerFreqInHz)/1000000U)

RETURN_STATUS
EFIAPI
TimerConstructor (
  VOID
  )
{
  // Check if the ARM Generic Timer Extension is implemented
  if (ArmIsArchTimerImplemented ()) {

    UINTN TimerFreq;

    // Check if Architectural Timer frequency is valid number (should not be 0)
    ASSERT (PcdGet32 (PcdArmArchTimerFreqInHz));

    // Check if ticks/uS is not 0. The Architectural timer runs at constant
    // frequency irrespective of CPU frequency. According to General Timer Ref
    // manual lower bound of the frequency is in the range of 1-10MHz
    ASSERT (TICKS_PER_MICRO_SEC);

#ifdef MDE_CPU_ARM
    // Only set the frequency for ARMv7. We expect the secure firmware to have already do it
    // If the security extensions are not implemented set Timer Frequency
    if ((ArmReadIdPfr1 () & 0xF0) == 0x0) {
      ArmArchTimerSetTimerFreq (PcdGet32 (PcdArmArchTimerFreqInHz));
    }
#endif

    // Architectural Timer Frequency must be set in the Secure privileged(if secure extensions are supported) mode.
    // If the reset value (0) is returned just ASSERT.
    TimerFreq = ArmArchTimerGetTimerFreq ();
    ASSERT (TimerFreq != 0);

  } else {
    DEBUG ((EFI_D_ERROR, "ARM Architectural Timer is not available in the CPU, hence this library can not be used.\n"));
    ASSERT (0);
  }

  return RETURN_SUCCESS;
}


/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds inputted.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  UINT64 TimerTicks64;
  UINT64 SystemCounterVal;

  // Calculate counter ticks that can represent requested delay
  TimerTicks64 = MultU64x32 (MicroSeconds, TICKS_PER_MICRO_SEC);

  // Read System Counter value
  SystemCounterVal = ArmArchTimerGetSystemCount ();

  TimerTicks64 += SystemCounterVal;

  // Wait until delay count is expired.
  while (SystemCounterVal < TimerTicks64) {
    SystemCounterVal = ArmArchTimerGetSystemCount ();
  }

  return MicroSeconds;
}


/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  When the timer frequency is 1MHz, each tick corresponds to 1 microsecond.
  Therefore, the nanosecond delay will be rounded up to the nearest 1 microsecond.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds inputed.

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
  // Just return the value of system count
  return ArmArchTimerGetSystemCount ();
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
  if (StartValue != NULL) {
    // Timer starts with the reload value
    *StartValue = (UINT64)0ULL ;
  }

  if (EndValue != NULL) {
    // Timer counts down to 0x0
    *EndValue = 0xFFFFFFFFFFFFFFFFUL;
  }

  return (UINT64)ArmArchTimerGetTimerFreq ();
}
