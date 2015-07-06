//
//  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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


/* Secondary core pens for AArch64 Linux booting.

   This code it placed in Linux kernel memory and marked reserved. UEFI ensures
   that the secondary cores get to this pen and the kernel can then start the
   cores from here.
   NOTE: This code must be self-contained.
*/

#include <Library/ArmLib.h>

.text
.align 3

GCC_ASM_EXPORT(SecondariesPenStart)
ASM_GLOBAL SecondariesPenEnd

ASM_PFX(SecondariesPenStart):
   // Registers x0-x3 are reserved for future use and should be set to zero.
   mov x0, xzr
   mov x1, xzr
   mov x2, xzr
   mov x3, xzr

   // Get core position. Taken from ArmPlatformGetCorePosition().
   // Assumes max 4 cores per cluster.
   mrs x4, mpidr_el1             // Get MPCore register.
   and x5, x4, #ARM_CORE_MASK    // Get core number.
   and x4, x4, #ARM_CLUSTER_MASK // Get cluster number.
   add x4, x5, x4, LSR #6        // Add scaled cluster number to core number.
   lsl x4, x4, 3            // Get mailbox offset for this core.
   ldr x5, AsmMailboxbase   // Get mailbox addr relative to pc (36 bytes ahead).
   add x4, x4, x5           // Add core mailbox offset to base of mailbox.
1: ldr x5, [x4]             // Load value from mailbox.
   cmp xzr, x5              // Has the mailbox been set?
   b.ne 2f                  // If so break out of loop.
   wfe                      // If not, wait a bit.
   b 1b                     // Wait over, check if mailbox set again.
2: br x5                    // Jump to mailbox value.

.align 3 // Make sure the variable below is 8 byte aligned.
                .global     AsmMailboxbase
AsmMailboxbase: .xword      0xdeaddeadbeefbeef

SecondariesPenEnd:
