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

ASM_GLOBAL ASM_PFX(SetupExceptionLevel3)
ASM_GLOBAL ASM_PFX(SwitchToNSExceptionLevel1)
ASM_GLOBAL ASM_PFX(enter_monitor_mode)
ASM_GLOBAL ASM_PFX(return_from_exception)
ASM_GLOBAL ASM_PFX(copy_cpsr_into_spsr)
ASM_GLOBAL ASM_PFX(set_non_secure_mode)

ASM_PFX(SetupExceptionLevel3):
   mrs    x0, scr_el3            // Read EL3 Secure Configuration Register
   orr    x0, x0, #1             // EL0 an EL1 cannot access secure memory

   // Send all interrupts to their respective Exception levels for EL3
   bic    x0, x0, #(1 << 1)      // IRQ
   bic    x0, x0, #(1 << 2)      // FIQ
   bic    x0, x0, #(1 << 3)      // Serror and Abort
   orr    x0, x0, #(1 << 8)      // Enable HVC
   orr    x0, x0, #(1 << 10)     // Make next level down 64Bit. This is EL2 in the case of the Model.
                                 // We need a nice way to detect this.
   msr    scr_el3, x0            // Write back our settings

   msr    cptr_el3, xzr          // Disable copro traps to EL3

   // Check for the primary CPU to avoid a race on the distributor registers.
   mrs     x0, mpidr_el1
   tst     x0, #15
   b.ne    1f                              // secondary CPU

   LoadConstantToReg (FixedPcdGet32(PcdGicInterruptInterfaceBase), x1)
   mov     w0, #3                          // EnableGrp0 | EnableGrp1
   str     w0, [x1]

1: LoadConstantToReg (FixedPcdGet32(PcdGicDistributorBase), x1)
   add     x1, x1, #0x80
   mov     w0, #~0                         // Grp1 interrupts
   str     w0, [x1], #4
   b.ne    2f                              // Only local interrupts for secondary CPUs
   str     w0, [x1], #4
   str     w0, [x1], #4

2: LoadConstantToReg (FixedPcdGet32(PcdGicInterruptInterfaceBase), x1)
   ldr     w0, [x1]
   mov     w0, #3                          // EnableGrp0 | EnableGrp1
   str     w0, [x1]

   mov     w0, #1 << 7                     // allow NS access to GICC_PMR
   str     w0, [x1, #4]                    // GICC_PMR

   ret

// Switch from EL3 to NS-EL1
ASM_PFX(SwitchToNSExceptionLevel1):
   // Now setup our EL1. Controlled by EL2 config on Model
   mrs     x0, hcr_el2            // Read EL2 Hypervisor configuration Register
   orr     x0, x0, #(1 << 31)     // Set EL1 to be 64bit

   // Send all interrupts to their respective Exception levels for EL2
   bic     x0, x0, #(1 << 3)      // Disable virtual FIQ
   bic     x0, x0, #(1 << 4)      // Disable virtual IRQ
   bic     x0, x0, #(1 << 5)      // Disable virtual SError and Abort
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

   mrs     x7, spsr_el3
   ands    w7, w7, #0xC
   cmp     w7, #0xC       // EL3?
   b.eq    3f
   bl      ASM_PFX(SetupExceptionLevel3)
   cmp     w7, #0x8       // EL2?
   b.eq    2f
   cmp     w7, #0x4       // EL1?
   b.eq    1f
   b       dead           // We should never get here.

1: bl      ASM_PFX(SwitchToNSExceptionLevel1)
2: // EL2: No more setup required.
3: // EL3: Not sure why we would do this.
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

dead:
   b       dead

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
