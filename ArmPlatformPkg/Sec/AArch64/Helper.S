#========================================================================================
#  Copyright (c) 2011-2017, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#=======================================================================================

#include <AsmMacroLib.h>
#include <AArch64/AArch64.h>

.set CTRL_M_BIT,      (1 << 0)

// Setup EL1 while in EL1
ASM_FUNC(SetupExceptionLevel1)
   mov  x0, #CPACR_DEFAULT
   b    ASM_PFX(ArmWriteCpacr)    // Tail call

// Setup EL2 while in EL2
ASM_FUNC(SetupExceptionLevel2)
   // Check MMU state before clearing SCTLR_EL2.
   // ArmPlatformPeiBootAction() runs before us and may enable the MMU.
   mrs     x0, sctlr_el2
   tst     x0, #CTRL_M_BIT
   b.ne    0f
   msr     sctlr_el2, xzr

0: mrs     x0, hcr_el2            // Read EL2 Hypervisor configuration Register

   // Send all interrupts to their respective Exception levels for EL2
   orr     x0, x0, #(1 << 3)      // Enable EL2 FIQ
   orr     x0, x0, #(1 << 4)      // Enable EL2 IRQ
   orr     x0, x0, #(1 << 5)      // Enable EL2 SError and Abort
   msr     hcr_el2, x0            // Write back our settings

   // Check whether we have been entered with HCR_EL2.E2H set, which is
   // permitted to be RES1. In this case, CPACR_EL1 is aliased to CPTR_EL2.
   tst     x0, #ARM_HCR_E2H
   b.ne    SetupExceptionLevel1

   mov     x0, #AARCH64_CPTR_DEFAULT
   msr     cptr_el2, x0           // Enable architectural features
   ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
