/** @file

  Copyright (c) 2011-2017, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PeilessSec.h"

#include <AArch64/AArch64.h>

/**
  Architecture specific initialization routine.
**/
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

    /* Enable Timer access for non-secure EL1 and EL0
       The cnthctl_el2 register bits are architecturally
       UNKNOWN on reset.
       Disable event stream as it is not in use at this stage
    */
    ArmWriteCntHctl (CNTHCTL_EL2_EL1PCTEN | CNTHCTL_EL2_EL1PCEN);
  }
}
