#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
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

#ifdef ARM_CPU_ARMv6
// No memory barriers for ARMv6
#define isb
#define dsb
#endif

.text
.align 2
GCC_ASM_EXPORT(ArmReadMidr)
GCC_ASM_EXPORT(ArmCacheInfo)
GCC_ASM_EXPORT(ArmGetInterruptState)
GCC_ASM_EXPORT(ArmGetFiqState)
GCC_ASM_EXPORT(ArmGetTTBR0BaseAddress)
GCC_ASM_EXPORT(ArmSetTTBR0)
GCC_ASM_EXPORT(ArmSetDomainAccessControl)
GCC_ASM_EXPORT(CPSRMaskInsert)
GCC_ASM_EXPORT(CPSRRead)
GCC_ASM_EXPORT(ArmReadCpacr)
GCC_ASM_EXPORT(ArmWriteCpacr)
GCC_ASM_EXPORT(ArmWriteAuxCr)
GCC_ASM_EXPORT(ArmReadAuxCr)
GCC_ASM_EXPORT(ArmInvalidateTlb)
GCC_ASM_EXPORT(ArmUpdateTranslationTableEntry)
GCC_ASM_EXPORT(ArmReadScr)
GCC_ASM_EXPORT(ArmWriteScr)
GCC_ASM_EXPORT(ArmReadMVBar)
GCC_ASM_EXPORT(ArmWriteMVBar)
GCC_ASM_EXPORT(ArmReadHVBar)
GCC_ASM_EXPORT(ArmWriteHVBar)
GCC_ASM_EXPORT(ArmCallWFE)
GCC_ASM_EXPORT(ArmCallSEV)
GCC_ASM_EXPORT(ArmReadSctlr)
GCC_ASM_EXPORT(ArmReadCpuActlr)
GCC_ASM_EXPORT(ArmWriteCpuActlr)

#------------------------------------------------------------------------------

ASM_PFX(ArmReadMidr):
  mrc     p15,0,R0,c0,c0,0
  bx      LR

ASM_PFX(ArmCacheInfo):
  mrc     p15,0,R0,c0,c0,1
  bx      LR

ASM_PFX(ArmGetInterruptState):
  mrs     R0,CPSR
  tst     R0,#0x80      @Check if IRQ is enabled.
  moveq   R0,#1
  movne   R0,#0
  bx      LR

ASM_PFX(ArmGetFiqState):
  mrs     R0,CPSR
  tst     R0,#0x40      @Check if FIQ is enabled.
  moveq   R0,#1
  movne   R0,#0
  bx      LR

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
  isb
  mov     sp, r3            @ restore stack pointer
  ldmfd   sp!, {r4-r12, lr} @ restore registers
  bx      lr                @ return (hopefully thumb-safe!)

ASM_PFX(CPSRRead):
  mrs     r0, cpsr
  bx      lr

ASM_PFX(ArmReadCpacr):
  mrc     p15, 0, r0, c1, c0, 2
  bx      lr

ASM_PFX(ArmWriteCpacr):
  mcr     p15, 0, r0, c1, c0, 2
  isb
  bx      lr

ASM_PFX(ArmWriteAuxCr):
  mcr     p15, 0, r0, c1, c0, 1
  bx      lr

ASM_PFX(ArmReadAuxCr):
  mrc     p15, 0, r0, c1, c0, 1
  bx      lr

ASM_PFX(ArmSetTTBR0):
  mcr     p15,0,r0,c2,c0,0
  isb
  bx      lr

ASM_PFX(ArmGetTTBR0BaseAddress):
  mrc     p15,0,r0,c2,c0,0
  LoadConstantToReg(0xFFFFC000, r1)
  and     r0, r0, r1
  isb
  bx      lr

//
//VOID
//ArmUpdateTranslationTableEntry (
//  IN VOID  *TranslationTableEntry  // R0
//  IN VOID  *MVA                    // R1
//  );
ASM_PFX(ArmUpdateTranslationTableEntry):
  mcr     p15,0,R0,c7,c14,1     @ DCCIMVAC Clean data cache by MVA
  dsb
  mcr     p15,0,R1,c8,c7,1      @ TLBIMVA TLB Invalidate MVA
  mcr     p15,0,R9,c7,c5,6      @ BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

ASM_PFX(ArmInvalidateTlb):
  mov     r0,#0
  mcr     p15,0,r0,c8,c7,0
  mcr     p15,0,R9,c7,c5,6      @ BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

ASM_PFX(ArmReadScr):
  mrc     p15, 0, r0, c1, c1, 0
  bx      lr

ASM_PFX(ArmWriteScr):
  mcr     p15, 0, r0, c1, c1, 0
  bx      lr

ASM_PFX(ArmReadHVBar):
  mrc     p15, 4, r0, c12, c0, 0
  bx      lr

ASM_PFX(ArmWriteHVBar):
  mcr     p15, 4, r0, c12, c0, 0
  bx      lr

ASM_PFX(ArmReadMVBar):
  mrc     p15, 0, r0, c12, c0, 1
  bx      lr

ASM_PFX(ArmWriteMVBar):
  mcr     p15, 0, r0, c12, c0, 1
  bx      lr

ASM_PFX(ArmCallWFE):
  wfe
  bx      lr

ASM_PFX(ArmCallSEV):
  sev
  bx      lr

ASM_PFX(ArmReadSctlr):
  mrc     p15, 0, r0, c1, c0, 0      @ Read SCTLR into R0 (Read control register configuration data)
  bx      lr

ASM_PFX(ArmReadCpuActlr):
  mrc     p15, 0, r0, c1, c0, 1
  bx      lr

ASM_PFX(ArmWriteCpuActlr):
  mcr     p15, 0, r0, c1, c0, 1
  dsb
  isb
  bx      lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
