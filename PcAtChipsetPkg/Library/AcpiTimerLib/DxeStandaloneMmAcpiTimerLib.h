/** @file
  Header file internal to ACPI TimerLib.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef _DXE_STANDALONE_MM_ACPI_TIMER_LIB_H_
#define _DXE_STANDALONE_MM_ACPI_TIMER_LIB_H_

/**
  The constructor function enables ACPI IO space, and caches PerformanceCounterFrequency.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
EFI_STATUS
CommonAcpiTimerLibConstructor (
  VOID
  );

#endif
