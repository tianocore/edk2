/** @file
  Base Reset System Library Shutdown API implementation for bhyve.

  Copyright (C) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (C) 2020, Red Hat, Inc.
  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>                   // BIT13

#include <IndustryStandard/Bhyve.h> // BHYVE_PM_REG
#include <Library/BaseLib.h>        // CpuDeadLoop()
#include <Library/IoLib.h>          // IoOr16()
#include <Library/ResetSystemLib.h> // ResetShutdown()

/**
  Calling this function causes the system to enter a power state equivalent
  to the ACPI G2/S5 or G3 states.

  System shutdown should not return, if it returns, it means the system does
  not support shut down reset.
**/
VOID
EFIAPI
ResetShutdown (
  VOID
  )
{
  IoBitFieldWrite16 (BHYVE_PM_REG, 10, 13, 5);
  IoOr16 (BHYVE_PM_REG, BIT13);
  CpuDeadLoop ();
}
