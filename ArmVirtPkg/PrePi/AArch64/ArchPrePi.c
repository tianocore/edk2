/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include "PrePi.h"

#include <Chipset/AArch64.h>

VOID
ArchInitialize (
  VOID
  )
{
  // Enable Floating Point
  if (FixedPcdGet32 (PcdVFPEnabled)) {
    ArmEnableVFP ();
  }

  if (ArmReadCurrentEL () == AARCH64_EL2) {
    // Trap General Exceptions. All exceptions that would be routed to EL1 are routed to EL2
    ArmWriteHcr (ARM_HCR_TGE);
  }
}
