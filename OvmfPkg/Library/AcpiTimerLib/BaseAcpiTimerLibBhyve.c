/** @file
  Provide InternalAcpiGetTimerTick for the bhyve instance of the
  Base ACPI Timer Library

  Copyright (C) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/IoLib.h>
#include <OvmfPlatforms.h>
#include "AcpiTimerLib.h"
#include "TscTimerLib.h"

/**
  Internal function to read the current tick counter of ACPI.

  Read the current ACPI tick counter using the counter address cached
  by this instance's constructor.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  )
{
  //
  // Return the current ACPI timer value.
  //
  return IoRead32 (BHYVE_ACPI_TIMER_IO_ADDR);
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return IsTscTimerFrequencyAvailable () ? TscGetPerformanceCounter () : AcpiGetPerformanceCounter ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64  *StartValue   OPTIONAL,
  OUT      UINT64  *EndValue     OPTIONAL
  )
{
  return IsTscTimerFrequencyAvailable () ? TscGetPerformanceCounterProperties (StartValue, EndValue) : AcpiGetPerformanceCounterProperties (StartValue, EndValue);
}
