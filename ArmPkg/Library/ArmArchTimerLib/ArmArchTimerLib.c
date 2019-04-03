/** @file
  Generic ARM implementation of TimerLib.h

  Copyright (c) 2011-2016, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmGenericTimerCounterLib.h>

#define TICKS_PER_MICRO_SEC     (PcdGet32 (PcdArmArchTimerFreqInHz)/1000000U)

// Select appropriate multiply function for platform architecture.
#ifdef MDE_CPU_ARM
#define MultU64xN MultU64x32
#else
#define MultU64xN MultU64x64
#endif


RETURN_STATUS
EFIAPI
TimerConstructor (
  VOID
  )
{
  //
  // Check if the ARM Generic Timer Extension is implemented.
  //
  if (ArmIsArchTimerImplemented ()) {

    //
    // Check if Architectural Timer frequency is pre-determined by the platform
    // (ie. nonzero).
    //
    if (PcdGet32 (PcdArmArchTimerFreqInHz) != 0) {
      //
      // Check if ticks/uS is not 0. The Architectural timer runs at constant
      // frequency, irrespective of CPU frequency. According to Generic Timer
      // Ref manual, lower bound of the frequency is in the range of 1-10MHz.
      //
      ASSERT (TICKS_PER_MICRO_SEC);

#ifdef MDE_CPU_ARM
      //
      // Only set the frequency for ARMv7. We expect the secure firmware to
      // have already done it.
      // If the security extension is not implemented, set Timer Frequency
      // here.
      //
      if ((ArmReadIdPfr1 () & ARM_PFR1_SEC) == 0x0) {
        ArmGenericTimerSetTimerFreq (PcdGet32 (PcdArmArchTimerFreqInHz));
      }
#endif
    }

    //
    // Architectural Timer Frequency must be set in Secure privileged
    // mode (if secure extension is supported).
    // If the reset value (0) is returned, just ASSERT.
    //
    ASSERT (ArmGenericTimerGetTimerFreq () != 0);

  } else {
    DEBUG ((EFI_D_ERROR, "ARM Architectural Timer is not available in the CPU, hence this library cannot be used.\n"));
    ASSERT (0);
  }

  return RETURN_SUCCESS;
}

/**
  A local utility function that returns the PCD value, if specified.
  Otherwise it defaults to ArmGenericTimerGetTimerFreq.

  @return The timer frequency.

**/
STATIC
UINTN
EFIAPI
GetPlatformTimerFreq (
  )
{
  UINTN TimerFreq;

  TimerFreq = PcdGet32 (PcdArmArchTimerFreqInHz);
  if (TimerFreq == 0) {
    TimerFreq = ArmGenericTimerGetTimerFreq ();
  }
  return TimerFreq;
}


/**
  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds input.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  UINT64 TimerTicks64;
  UINT64 SystemCounterVal;

  // Calculate counter ticks that represent requested delay:
  //  = MicroSeconds x TICKS_PER_MICRO_SEC
  //  = MicroSeconds x Frequency.10^-6
  TimerTicks64 = DivU64x32 (
                   MultU64xN (
                     MicroSeconds,
                     GetPlatformTimerFreq ()
                     ),
                   1000000U
                   );

  // Read System Counter value
  SystemCounterVal = ArmGenericTimerGetSystemCount ();

  TimerTicks64 += SystemCounterVal;

  // Wait until delay count expires.
  while (SystemCounterVal < TimerTicks64) {
    SystemCounterVal = ArmGenericTimerGetSystemCount ();
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
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    // Timer starts at 0
    *StartValue = (UINT64)0ULL ;
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
  IN      UINT64                     Ticks
  )
{
  UINT64  NanoSeconds;
  UINT32  Remainder;
  UINT32  TimerFreq;

  TimerFreq = GetPlatformTimerFreq ();
  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64xN (
                  DivU64x32Remainder (
                    Ticks,
                    TimerFreq,
                    &Remainder),
                  1000000000U
                  );

  //
  // Frequency < 0x100000000, so Remainder < 0x100000000, then (Remainder * 1,000,000,000)
  // will not overflow 64-bit.
  //
  NanoSeconds += DivU64x32 (
                   MultU64xN (
                     (UINT64) Remainder,
                     1000000000U),
                   TimerFreq
                   );

  return NanoSeconds;
}
