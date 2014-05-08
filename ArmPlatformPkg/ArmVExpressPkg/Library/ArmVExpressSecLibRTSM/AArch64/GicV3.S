//
//  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <AsmMacroIoLibV8.h>
#include <Chipset/AArch64.h>

#ifndef __clang__
// Register definitions used by GCC for GICv3 access.
// These are defined by ARMCC, so keep them in the GCC specific code for now.
#define ICC_SRE_EL2     S3_4_C12_C9_5
#define ICC_SRE_EL3     S3_6_C12_C12_5
#define ICC_CTLR_EL1    S3_0_C12_C12_4
#define ICC_CTLR_EL3    S3_6_C12_C12_4
#define ICC_PMR_EL1     S3_0_C4_C6_0
#endif

.text
.align 3

GCC_ASM_EXPORT(InitializeGicV3)

/* Initialize GICv3 to expose it as a GICv2 as UEFI does not support GICv3 yet */
ASM_PFX(InitializeGicV3):
  // We have a GICv3. UEFI still uses the GICv2 mode. We must do enough setup
  // to allow Linux to use GICv3 if it chooses.

  // In order to setup NS side we need to enable it first.
  mrs     x0, scr_el3
  orr     x0, x0, #1
  msr     scr_el3, x0

  // Enable SRE at EL3 and ICC_SRE_EL2 access
  mov     x0, #((1 << 3) | (1 << 0))      // Enable | SRE
  mrs     x1, ICC_SRE_EL3
  orr     x1, x1, x0
  msr     ICC_SRE_EL3, x1
  isb

  // Enable SRE at EL2 and ICC_SRE_EL1 access..
  mrs     x1, ICC_SRE_EL2
  orr     x1, x1, x0
  msr     ICC_SRE_EL2, x1
  isb

  // Configure CPU interface
  msr     ICC_CTLR_EL3, xzr
  isb
  msr     ICC_CTLR_EL1, xzr
  isb

  // The MemoryMap view and Register view may not be consistent, So Set PMR again.
  mov     w1, #1 << 7                        // allow NS access to GICC_PMR
  msr     ICC_PMR_EL1, x1
  isb

  // Remove the SCR.NS bit
  mrs     x0, scr_el3
  and     x0, x0, #~SCR_NS
  msr     scr_el3, x0
  ret
