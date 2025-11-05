/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>

extern GUID  mFrequencyHobGuid;

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
  UINT64             *PerformanceCounterFrequency;
  EFI_HOB_GUID_TYPE  *GuidHob;

  PerformanceCounterFrequency = NULL;
  GuidHob                     = GetFirstGuidHob (&mFrequencyHobGuid);
  if (GuidHob == NULL) {
    PerformanceCounterFrequency = (UINT64 *)BuildGuidHob (&mFrequencyHobGuid, sizeof (*PerformanceCounterFrequency));
    ASSERT (PerformanceCounterFrequency != NULL);
    *PerformanceCounterFrequency = InternalCalculateTscFrequency ();
  } else {
    PerformanceCounterFrequency = (UINT64 *)GET_GUID_HOB_DATA (GuidHob);
  }

  return *PerformanceCounterFrequency;
}
