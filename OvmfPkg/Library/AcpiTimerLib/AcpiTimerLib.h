/** @file
  Internal definitions for ACPI Timer Library

  Copyright (C) 2014, Gabriel L. Somlo <somlo@cmu.edu>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

/**
  Internal function to read the current tick counter of ACPI.

  @return The tick counter read.

**/
UINT32
InternalAcpiGetTimerTick (
  VOID
  );
