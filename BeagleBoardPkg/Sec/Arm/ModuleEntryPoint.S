#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008-2009 Apple Inc. All rights reserved.
#
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>
#include <Library/PcdLib.h>

.text
.align 3

.globl ASM_PFX(CEntryPoint)
.globl ASM_PFX(_ModuleEntryPoint)

ASM_PFX(_ModuleEntryPoint):

  //Disable L2 cache
  mrc     p15, 0, r0, c1, c0, 1   // read Auxiliary Control Register
  bic     r0, r0, #0x00000002     // disable L2 cache
  mcr     p15, 0, r0, c1, c0, 1   // store Auxiliary Control Register
  
  //Enable Strict alignment checking & Instruction cache
  mrc     p15, 0, r0, c1, c0, 0
  bic     r0, r0, #0x00002300     /* clear bits 13, 9:8 (--V- --RS) */
  bic     r0, r0, #0x00000005     /* clear bits 0, 2 (---- -C-M) */
  orr     r0, r0, #0x00000002     /* set bit 1 (A) Align */
  orr     r0, r0, #0x00001000     /* set bit 12 (I) enable I-Cache */
  mcr     p15, 0, r0, c1, c0, 0

  // Set CPU vectors to start of DRAM
  mov     r0, #0x80000000
  mcr     p15, 0, r0, c12, c0, 0

  /* before we call C code, lets setup the stack pointer */
stack_pointer_setup:

  //
  // Set stack based on PCD values. Need to do it this way to make C code work 
  // when it runs from FLASH. 
  //  
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackBase) ,r2)    /* stack base arg2  */
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackSize) ,r3)    /* stack size arg3  */
  add     r4, r2, r3

  //Enter IRQ mode and set up IRQ stack pointer
  mov     r0,#0x12|0x80|0x40
  msr     CPSR_c,r0
  mov     r13,r4

  //Enter Abort mode and set up Abort stack pointer
  mov     r0,#0x17|0x80|0x40
  msr     CPSR_c,r0
  sub     r4, r4, #0x400
  mov     r13,r4

  //Enter Undefined mode and set up Undefined stack pointer
  mov     r0,#0x1b|0x80|0x40
  msr     CPSR_c,r0
  sub     r4, r4, #0x400
  mov     r13,r4

  //Enter SVC mode and set up SVC stack pointer
  mov     r0,#0x13|0x80|0x40
  msr     CPSR_c,r0
  sub     r4, r4, #0x400
  mov     r13,r4

  //Enter System mode and set up System stack pointer
  mov     r0,#0x1f|0x80|0x40
  msr     CPSR_c,r0
  sub     r4, r4, #0x400
  mov     r13,r4

  // Call C entry point
  mov     r0, #0x80000000   /* memory base arg0          */
  mov     r1, #0x10000000   /* memory size arg1          */

  bl      ASM_PFX(CEntryPoint) /* Assume C code is ARM      */

ShouldNeverGetHere:
  /* _CEntryPoint should never return */
  b       ShouldNeverGetHere

  
