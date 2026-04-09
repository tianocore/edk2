/** @file
  CPUID Leaf 0x15 for Core Crystal Clock frequency instance of Timer Library.

  Copyright (c) 2019 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Register/Cpuid.h>

GUID  mCpuCrystalFrequencyHobGuid = {
  0xe1ec5ad0, 0x8569, 0x46bd, { 0x8d, 0xcd, 0x3b, 0x9f, 0x6f, 0x45, 0x82, 0x7a }
};

/**
  Internal function to retrieves the 64-bit frequency in Hz.

  Internal function to retrieves the 64-bit frequency in Hz.

  @return The frequency in Hz.

**/
UINT64
InternalGetPerformanceCounterFrequency (
  VOID
  );

/**
  CPUID Leaf 0x15 for Core Crystal Clock Frequency.

  The TSC counting frequency is determined by using CPUID leaf 0x15. Frequency in MHz = Core XTAL frequency * EBX/EAX.
  In newer flavors of the CPU, core xtal frequency is returned in ECX or 0 if not supported.
  @return The number of TSC counts per second.

**/
UINT64
CpuidCoreClockCalculateTscFrequency (
  VOID
  )
{
  UINT64  TscFrequency;
  UINT64  CoreXtalFrequency;
  UINT32  RegEax;
  UINT32  RegEbx;
  UINT32  RegEcx;

  //
  // Use CPUID leaf 0x15 Time Stamp Counter and Nominal Core Crystal Clock Information
  // EBX returns 0 if not supported. ECX, if non zero, provides Core Xtal Frequency in hertz.
  // TSC frequency = (ECX, Core Xtal Frequency) * EBX/EAX.
  //
  AsmCpuid (CPUID_TIME_STAMP_COUNTER, &RegEax, &RegEbx, &RegEcx, NULL);

  //
  // If EAX or EBX returns 0, the XTAL ratio is not enumerated.
  //
  if ((RegEax == 0) || (RegEbx == 0)) {
    ASSERT (RegEax != 0);
    ASSERT (RegEbx != 0);
    return 0;
  }

  //
  // If ECX returns 0, the XTAL frequency is not enumerated.
  // And PcdCpuCoreCrystalClockFrequency defined should base on processor series.
  //
  if (RegEcx == 0) {
    CoreXtalFrequency = PcdGet64 (PcdCpuCoreCrystalClockFrequency);
  } else {
    CoreXtalFrequency = (UINT64)RegEcx;
  }

  //
  // Calculate TSC frequency = (ECX, Core Xtal Frequency) * EBX/EAX
  //
  TscFrequency = DivU64x32 (MultU64x32 (CoreXtalFrequency, RegEbx) + (UINT64)(RegEax >> 1), RegEax);

  return TscFrequency;
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
VOID
InternalCpuDelay (
  IN UINT64  Delay
  )
{
  UINT64  Ticks;

  //
  // The target timer count is calculated here
  //
  Ticks = AsmReadTsc () + Delay;

  //
  // Wait until time out
  // Timer wrap-arounds are NOT handled correctly by this function.
  // Thus, this function must be called within 10 years of reset since
  // Intel guarantees a minimum of 10 years before the TSC wraps.
  //
  while (AsmReadTsc () <= Ticks) {
    CpuPause ();
  }
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param[in]  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN UINTN  MicroSeconds
  )
{
  InternalCpuDelay (
    DivU64x32 (
      MultU64x64 (
        MicroSeconds,
        InternalGetPerformanceCounterFrequency ()
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
  IN UINTN  NanoSeconds
  )
{
  InternalCpuDelay (
    DivU64x32 (
      MultU64x64 (
        NanoSeconds,
        InternalGetPerformanceCounterFrequency ()
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
  return AsmReadTsc ();
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
  OUT UINT64  *StartValue   OPTIONAL,
  OUT UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xffffffffffffffffULL;
  }

  return InternalGetPerformanceCounterFrequency ();
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
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift        = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder    = RShiftU64 (Remainder, (UINTN)Shift);
  Frequency    = RShiftU64 (Frequency, (UINTN)Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
