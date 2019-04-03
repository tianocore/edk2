/** @file
  MicroSecondDelay implementation of ACPI Timer.

  Copyright (c) 2006 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __I2C_DELAY_PEI__

#define __I2C_DELAY_PEI__
#include "PiPei.h"

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
EFI_STATUS
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  );

#endif
