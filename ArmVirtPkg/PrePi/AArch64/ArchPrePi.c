/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
