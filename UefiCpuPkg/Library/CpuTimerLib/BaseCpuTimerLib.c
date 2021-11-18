/** @file
  CPUID Leaf 0x15 for Core Crystal Clock frequency instance as Base Timer Library.

  Copyright (c) 2019 Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>

/**
  CPUID Leaf 0x15 for Core Crystal Clock Frequency.

  The TSC counting frequency is determined by using CPUID leaf 0x15. Frequency in MHz = Core XTAL frequency * EBX/EAX.
  In newer flavors of the CPU, core xtal frequency is returned in ECX or 0 if not supported.
  @return The number of TSC counts per second.

**/
UINT64
CpuidCoreClockCalculateTscFrequency (
  VOID
  );

/**
  Internal function to retrieves the 64-bit frequency in Hz.

  Internal function to retrieves the 64-bit frequency in Hz.

  @return The frequency in Hz.

**/
UINT64
InternalGetPerformanceCounterFrequency (
  VOID
  )
{
  return CpuidCoreClockCalculateTscFrequency ();
}
