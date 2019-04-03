/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>

/**
  Calculate TSC frequency.

  The TSC counting frequency is determined by comparing how far it counts
  during a 101.4 us period as determined by the ACPI timer.
  The ACPI timer is used because it counts at a known frequency.
  The TSC is sampled, followed by waiting 363 counts of the ACPI timer,
  or 101.4 us. The TSC is then sampled again. The difference multiplied by
  9861 is the TSC frequency. There will be a small error because of the
  overhead of reading the ACPI timer. An attempt is made to determine and
  compensate for this error.

  @return The number of TSC counts per second.

**/
UINT64
InternalCalculateTscFrequency (
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
  return InternalCalculateTscFrequency ();
}
