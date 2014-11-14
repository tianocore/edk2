/** @file
  Internal definitions for ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
