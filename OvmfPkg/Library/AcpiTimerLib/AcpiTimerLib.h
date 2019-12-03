/** @file
  Internal definitions for ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _ACPI_TIMER_LIB_INTERNAL_H_
#define _ACPI_TIMER_LIB_INTERNAL_H_

/**
  Internal function to read the current tick counter of ACPI.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  );

#endif // _ACPI_TIMER_LIB_INTERNAL_H_
