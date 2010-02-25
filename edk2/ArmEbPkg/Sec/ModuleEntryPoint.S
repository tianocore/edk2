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

  //
  // Set stack based on PCD values. Need to do it this way to make C code work 
  // when it runs from FLASH. 
  //  
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackBase) ,r2)    /* stack base arg2  */
  LoadConstantToReg (FixedPcdGet32(PcdPrePiStackSize) ,r3)    /* stack size arg3  */
  add     r4, r2, r3

  //Enter SVC mode and set up SVC stack pointer
  mov     r0,#0x13|0x80|0x40
  msr     CPSR_c,r0
  sub     r4, r4, #0x400
  mov     r13,r4

  // Call C entry point
  LoadConstantToReg (FixedPcdGet32(PcdMemorySize) ,r1)    /* memory size arg1          */
  LoadConstantToReg (FixedPcdGet32(PcdMemoryBase) ,r0)    /* memory size arg0         */

  blx      ASM_PFX(CEntryPoint) /* Assume C code is ARM      */

ShouldNeverGetHere:
  /* _CEntryPoint should never return */
  b       ShouldNeverGetHere

  
