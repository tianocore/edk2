#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>

.text
.align 2
GCC_ASM_EXPORT(Cp15IdCode)
GCC_ASM_EXPORT(Cp15CacheInfo)
GCC_ASM_EXPORT(ArmEnableInterrupts)
GCC_ASM_EXPORT(ArmDisableInterrupts)
GCC_ASM_EXPORT(ArmGetInterruptState)
GCC_ASM_EXPORT(ArmEnableFiq)
GCC_ASM_EXPORT(ArmDisableFiq)
GCC_ASM_EXPORT(ArmGetFiqState)
GCC_ASM_EXPORT(ArmInvalidateTlb)
GCC_ASM_EXPORT(ArmSetTTBR0)
GCC_ASM_EXPORT(ArmGetTTBR0BaseAddress)
GCC_ASM_EXPORT(ArmSetDomainAccessControl)
GCC_ASM_EXPORT(CPSRMaskInsert)
GCC_ASM_EXPORT(CPSRRead)

#------------------------------------------------------------------------------

ASM_PFX(Cp15IdCode):
  mrc     p15,0,R0,c0,c0,0
  bx      LR

ASM_PFX(Cp15CacheInfo):
  mrc     p15,0,R0,c0,c0,1
  bx      LR

ASM_PFX(ArmEnableInterrupts):
\s\smrs     R0,CPSR
\s\sbic     R0,R0,#0x80\s\s\s\s@Enable IRQ interrupts
\s\smsr     CPSR_c,R0
\s\sbx      LR

ASM_PFX(ArmDisableInterrupts):
\s\smrs     R0,CPSR
\s\sorr     R1,R0,#0x80\s\s\s\s@Disable IRQ interrupts
\s\smsr     CPSR_c,R1
  tst     R0,#0x80
  moveq   R0,#1
  movne   R0,#0
\s\sbx      LR

ASM_PFX(ArmGetInterruptState):
\s\smrs     R0,CPSR
\s\stst     R0,#0x80\s\s    @Check if IRQ is enabled.
\s\smoveq   R0,#1
\s\smovne   R0,#0
\s\sbx      LR

ASM_PFX(ArmEnableFiq):
\s\smrs     R0,CPSR
\s\sbic     R0,R0,#0x40\s\s\s\s@Enable FIQ interrupts
\s\smsr     CPSR_c,R0
\s\sbx      LR

ASM_PFX(ArmDisableFiq):
\s\smrs     R0,CPSR
\s\sorr     R1,R0,#0x40\s\s\s\s@Disable FIQ interrupts
\s\smsr     CPSR_c,R1
  tst     R0,#0x80
  moveq   R0,#1
  movne   R0,#0
\s\sbx      LR

ASM_PFX(ArmGetFiqState):
\s\smrs     R0,CPSR
\s\stst     R0,#0x80\s\s    @Check if FIQ is enabled.
\s\smoveq   R0,#1
\s\smovne   R0,#0
\s\sbx      LR

ASM_PFX(ArmInvalidateTlb):
  mov     r0,#0
  mcr     p15,0,r0,c8,c7,0
  bx      lr

ASM_PFX(ArmSetTTBR0):
  mcr     p15,0,r0,c2,c0,0
  bx      lr

ASM_PFX(ArmGetTTBR0BaseAddress):
  mrc     p15,0,r0,c2,c0,0
  LoadConstantToReg(0xFFFFC000, r1)   @  and     r0, r0, #0xFFFFC000
  and     r0, r0, r1
  bx      lr


ASM_PFX(ArmSetDomainAccessControl):
  mcr     p15,0,r0,c3,c0,0
  bx      lr

ASM_PFX(CPSRMaskInsert):    @ on entry, r0 is the mask and r1 is the field to insert
  stmfd   sp!, {r4-r12, lr} @ save all the banked registers
  mov     r3, sp            @ copy the stack pointer into a non-banked register
  mrs     r2, cpsr          @ read the cpsr
  bic     r2, r2, r0        @ clear mask in the cpsr
  and     r1, r1, r0        @ clear bits outside the mask in the input
  orr     r2, r2, r1        @ set field
  msr     cpsr_cxsf, r2     @ write back cpsr (may have caused a mode switch)
  mov     sp, r3            @ restore stack pointer
  ldmfd   sp!, {r4-r12, lr} @ restore registers
  bx      lr                @ return (hopefully thumb-safe!)

ASM_PFX(CPSRRead):
  mrs     r0, cpsr
  bx      lr
  
ASM_FUNCTION_REMOVE_IF_UNREFERENCED
