#========================================================================================
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http:#opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#=======================================================================================

#include <AsmMacroIoLibV8.h>
#include <Chipset/AArch64.h>

#start of the code section
.text
.align 3

GCC_ASM_EXPORT(SetupExceptionLevel1)
GCC_ASM_EXPORT(SetupExceptionLevel2)

// Setup EL1 while in EL1
ASM_PFX(SetupExceptionLevel1):
   mov  x5, x30                   // Save LR

   mov  x0, #CPACR_CP_FULL_ACCESS
   bl   ASM_PFX(ArmWriteCpacr)    // Disable copro traps to EL1

   ret  x5

// Setup EL2 while in EL2
ASM_PFX(SetupExceptionLevel2):
   msr     sctlr_el2, xzr
   mrs     x0, hcr_el2            // Read EL2 Hypervisor configuration Register

   // Send all interrupts to their respective Exception levels for EL2
   orr     x0, x0, #(1 << 3)      // Enable EL2 FIQ
   orr     x0, x0, #(1 << 4)      // Enable EL2 IRQ
   orr     x0, x0, #(1 << 5)      // Enable EL2 SError and Abort
   msr     hcr_el2, x0            // Write back our settings

   msr     cptr_el2, xzr          // Disable copro traps to EL2

   ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
