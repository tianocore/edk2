#========================================================================================
#  Copyright (c) 2011-2017, ARM Limited. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
#=======================================================================================

#include <AsmMacroLib.h>
#include <AArch64/AArch64.h>

// Setup EL1 while in EL1
ASM_FUNC(SetupExceptionLevel1)
   mov  x5, x30                   // Save LR

   mov  x0, #CPACR_DEFAULT
   bl   ASM_PFX(ArmWriteCpacr)    // Enable architectural features

   ret  x5

// Setup EL2 while in EL2
ASM_FUNC(SetupExceptionLevel2)
   msr     sctlr_el2, xzr
   mrs     x0, hcr_el2            // Read EL2 Hypervisor configuration Register

   // Send all interrupts to their respective Exception levels for EL2
   orr     x0, x0, #(1 << 3)      // Enable EL2 FIQ
   orr     x0, x0, #(1 << 4)      // Enable EL2 IRQ
   orr     x0, x0, #(1 << 5)      // Enable EL2 SError and Abort
   msr     hcr_el2, x0            // Write back our settings

   // NB: We assume that we have not been entered on VHE systems with
   // HCR_EL2.E2H set.
   mov     x0, #AARCH64_CPTR_DEFAULT
   msr     cptr_el2, x0           // Enable architectural features

   // Enable Timer access for non-secure EL1 and EL0
   // The cnthctl_el2 register bits are architecturally
   // UNKNOWN on reset.
   // Disable event stream as it is not in use at this stage
   mov     x0, #(CNTHCTL_EL2_EL1PCTEN | CNTHCTL_EL2_EL1PCEN)
   msr     cnthctl_el2, x0

   ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
