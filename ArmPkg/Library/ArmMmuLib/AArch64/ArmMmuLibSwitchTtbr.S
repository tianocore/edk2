#------------------------------------------------------------------------------
#
# Copyright (c) 2024, Google LLC. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroLib.h>

  .set SCTLR_M_BIT,     (1 << 0)
  .set HCR_E2H_BIT_POS, 34

//VOID
//ArmSwitchTtbr (
//  IN  UINT64  *RootTable,
//  IN  UINTN   Asid
//  )
ASM_FUNC(ArmSwitchTtbr)

  EL1_OR_EL2(x2)
1:orr   x0, x0, x1, lsl #48
  msr   ttbr0_el1, x0
  isb
  ret

  // With VHE enabled, EL2 can use the EL1 fast path above.  Otherwise, the MMU
  // needs to be disabled and re-enabled in order to switch TTBR0 safely.
2:mrs   x2, hcr_el2
  tbnz  x2, #HCR_E2H_BIT_POS, 1b

  // disable the MMU
  mrs   x2, sctlr_el2
  bic   x3, x2, #SCTLR_M_BIT
  msr   sctlr_el2, x3
  isb

  // invalidate all cached translations
  tlbi  alle2
  dsb   nsh
  isb

  msr   ttbr0_el2, x0

  // re-enable the MMU
  msr   sctlr_el2, x2
  isb
  ret
ASM_GLOBAL ASM_PFX(ArmSwitchTtbrSize)

ASM_PFX(ArmSwitchTtbrSize):
  .long   . - ArmSwitchTtbr
