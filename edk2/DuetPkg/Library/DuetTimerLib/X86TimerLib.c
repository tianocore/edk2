/** @file
  Timer Library functions built upon local APIC on IA32/x64.

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Metronome.h>

/**
EFI_METRONOME_ARCH_PROTOCOL *gDuetMetronome = NULL;

EFI_METRONOME_ARCH_PROTOCOL*
GetMetronomeArchProtocol (
    VOID
    )
{
  if (gDuetMetronome == NULL) {
     gBS->LocateProtocol (&gEfiMetronomeArchProtocolGuid, NULL, (VOID**) &gDuetMetronome);
  }
  
  return gDuetMetronome;
}    
**/

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  gBS->Stall (MicroSeconds);
/**
  EFI_METRONOME_ARCH_PROTOCOL       *mMetronome;
  UINT32                            Counter;
  UINTN                             Remainder;  
  
  if ((mMetronome = GetMetronomeArchProtocol()) == NULL) {
    return MicroSeconds;
  }
  
  //
  // Calculate the number of ticks by dividing the number of microseconds by
  // the TickPeriod.
  // Calculation is based on 100ns unit.
  //
  Counter = (UINT32) DivU64x32Remainder (
                       MicroSeconds * 10,
                       mMetronome->TickPeriod,
                       &Remainder
                       );
  //
  // Call WaitForTick for Counter + 1 ticks to try to guarantee Counter tick
  // periods, thus attempting to ensure Microseconds of stall time.
  //
  if (Remainder != 0) {
    Counter++;
  }

  mMetronome->WaitForTick (mMetronome, Counter);
**/
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
  IN      UINTN                     NanoSeconds
  )
{
  //
  // Duet platform need *not* this interface.
  //
  //ASSERT (FALSE);
  return 0;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
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
  //
  // Duet platform need *not* this interface.
  //
  //ASSERT (FALSE);
  return 0;
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
  //
  // Duet platform need *not* this interface.
  //
  //ASSERT (FALSE);
  return 0;
}
