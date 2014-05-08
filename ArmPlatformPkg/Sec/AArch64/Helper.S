#========================================================================================
#  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

GCC_ASM_EXPORT(SwitchToNSExceptionLevel1)
GCC_ASM_EXPORT(enter_monitor_mode)
GCC_ASM_EXPORT(return_from_exception)
GCC_ASM_EXPORT(copy_cpsr_into_spsr)
GCC_ASM_EXPORT(set_non_secure_mode)

// Switch from EL3 to NS-EL1
ASM_PFX(SwitchToNSExceptionLevel1):
   // Now setup our EL1. Controlled by EL2 config on Model
   mrs     x0, hcr_el2            // Read EL2 Hypervisor configuration Register
   orr     x0, x0, #(1 << 31)     // Set EL1 to be 64bit

   // Send all interrupts to their respective Exception levels for EL2
   and     x0, x0, #~(ARM_HCR_FMO | ARM_HCR_IMO | ARM_HCR_AMO) // Disable virtual FIQ, IRQ, SError and Abort
   msr     hcr_el2, x0            // Write back our settings

   msr     cptr_el2, xzr          // Disable copro traps to EL2

   msr     sctlr_el2, xzr

   // Enable architected timer access
   mrs     x0, cnthctl_el2
   orr     x0, x0, #3             // Enable EL1 access to timers
   msr     cnthctl_el2, x0

   mrs     x0, cntkctl_el1
   orr     x0, x0, #3             // EL0 access to counters
   msr     cntkctl_el1, x0

   // Set ID regs
   mrs     x0, midr_el1
   mrs     x1, mpidr_el1
   msr     vpidr_el2, x0
   msr     vmpidr_el2, x1

   ret


// EL3 on AArch64 is Secure/monitor so this funtion is reduced vs ARMv7
// we don't need a mode switch, just setup the Arguments and jump.
// x0: Monitor World EntryPoint
// x1: MpId
// x2: SecBootMode
// x3: Secure Monitor mode stack
ASM_PFX(enter_monitor_mode):
   mov     x4, x0                 // Swap EntryPoint and MpId registers
   mov     x0, x1
   mov     x1, x2
   mov     x2, x3
   br      x4

// Put the address in correct ELR_ELx and do a eret.
// We may need to do some config before we change to another Mode.
ASM_PFX(return_from_exception):
   msr     elr_el3, x0
   eret

// For AArch64 we need to construct the spsr we want from individual bits and pieces.
ASM_PFX(copy_cpsr_into_spsr):
   mrs     x0, CurrentEl  // Get the current exception level we  are running at.
   mrs     x1, SPSel      // Which Stack are we using
   orr     x0, x0, x1
   mrs     x1, daif       // Which interrupts are enabled
   orr     x0, x0, x1
   msr     spsr_el3, x0   // Write to spsr
   ret

// Get this from platform file.
ASM_PFX(set_non_secure_mode):
   msr     spsr_el3, x0
   ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
