/** @file
  Timer Library functions built upon local APIC on IA32/x64.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
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
#include <Library/IoLib.h>
#include <Library/PcdLib.h>


//
// The following array is used in calculating the frequency of local APIC
// timer. Refer to IA-32 developers' manual for more details.
//
GLOBAL_REMOVE_IF_UNREFERENCED
CONST UINT8                           mTimerLibLocalApicDivisor[] = {
  0x02, 0x04, 0x08, 0x10,
  0x02, 0x04, 0x08, 0x10,
  0x20, 0x40, 0x80, 0x01,
  0x20, 0x40, 0x80, 0x01
};

/**
  Internal function to retrieve the base address of local APIC.

  @return The base address of local APIC

**/
UINTN
EFIAPI
InternalX86GetApicBase (
  VOID
  )
{
  return (UINTN)AsmMsrBitFieldRead64 (27, 12, 35) << 12;
}

/**
  Internal function to return the frequency of the local APIC timer.

  @param  ApicBase  The base address of memory mapped registers of local APIC.

  @return The frequency of the timer in Hz.

**/
UINT32
EFIAPI
InternalX86GetTimerFrequency (
  IN      UINTN                     ApicBase
  )
{
  return
    PcdGet32(PcdFSBClock) /
    mTimerLibLocalApicDivisor[MmioBitFieldRead32 (ApicBase + 0x3e0, 0, 3)];
}

/**
  Internal function to read the current tick counter of local APIC.

  @param  ApicBase  The base address of memory mapped registers of local APIC.

  @return The tick counter read.

**/
INT32
EFIAPI
InternalX86GetTimerTick (
  IN      UINTN                     ApicBase
  )
{
  return MmioRead32 (ApicBase + 0x390);
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  ApicBase  The base address of memory mapped registers of local APIC.
  @param  Delay     A period of time to delay in ticks.

**/
VOID
EFIAPI
InternalX86Delay (
  IN      UINTN                     ApicBase,
  IN      UINT32                    Delay
  )
{
  INT32                             Ticks;

  //
  // The target timer count is calculated here
  //
  Ticks = InternalX86GetTimerTick (ApicBase) - Delay;

  //
  // Wait until time out
  // Delay > 2^31 could not be handled by this function
  // Timer wrap-arounds are handled correctly by this function
  //
  while (InternalX86GetTimerTick (ApicBase) - Ticks >= 0);
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
  IN      UINTN                     MicroSeconds
  )
{
  UINTN                             ApicBase;

  ApicBase = InternalX86GetApicBase ();
  InternalX86Delay (
    ApicBase,
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (ApicBase),
                MicroSeconds
                ),
              1000000u
              )
    );
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
  UINTN                             ApicBase;

  ApicBase = InternalX86GetApicBase ();
  InternalX86Delay (
    ApicBase,
    (UINT32)DivU64x32 (
              MultU64x64 (
                InternalX86GetTimerFrequency (ApicBase),
                NanoSeconds
                ),
              1000000000u
              )
    );
  return NanoSeconds;
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
  return (UINT64)(UINT32)InternalX86GetTimerTick (InternalX86GetApicBase ());
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
  UINTN                             ApicBase;

  ApicBase = InternalX86GetApicBase ();

  if (StartValue != NULL) {
    *StartValue = MmioRead32 (ApicBase + 0x380);
  }

  if (EndValue != NULL) {
    *EndValue = 0;
  }

  return (UINT64) InternalX86GetTimerFrequency (ApicBase);;
}
