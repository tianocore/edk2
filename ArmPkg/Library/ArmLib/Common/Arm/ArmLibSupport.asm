//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
// Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
//
// This program and the accompanying materials
// are licensed and made available under the terms and conditions of the BSD License
// which accompanies this distribution.  The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>

    INCLUDE AsmMacroIoLib.inc

#ifdef ARM_CPU_ARMv6
// No memory barriers for ARMv6
#define isb
#define dsb
#endif

    EXPORT ArmReadMidr
    EXPORT ArmCacheInfo
    EXPORT ArmGetInterruptState
    EXPORT ArmGetFiqState
    EXPORT ArmGetTTBR0BaseAddress
    EXPORT ArmSetTTBR0
    EXPORT ArmSetDomainAccessControl
    EXPORT CPSRMaskInsert
    EXPORT CPSRRead
    EXPORT ArmReadCpacr
    EXPORT ArmWriteCpacr
    EXPORT ArmWriteAuxCr
    EXPORT ArmReadAuxCr
    EXPORT ArmInvalidateTlb
    EXPORT ArmUpdateTranslationTableEntry
    EXPORT ArmReadScr
    EXPORT ArmWriteScr
    EXPORT ArmReadMVBar
    EXPORT ArmWriteMVBar
    EXPORT ArmReadHVBar
    EXPORT ArmWriteHVBar
    EXPORT ArmCallWFE
    EXPORT ArmCallSEV
    EXPORT ArmReadSctlr
    EXPORT ArmReadCpuActlr
    EXPORT ArmWriteCpuActlr

    AREA ArmLibSupport, CODE, READONLY

ArmReadMidr
  mrc     p15,0,R0,c0,c0,0
  bx      LR

ArmCacheInfo
  mrc     p15,0,R0,c0,c0,1
  bx      LR

ArmGetInterruptState
  mrs     R0,CPSR
  tst     R0,#0x80      // Check if IRQ is enabled.
  moveq   R0,#1
  movne   R0,#0
  bx      LR

ArmGetFiqState
  mrs     R0,CPSR
  tst     R0,#0x40      // Check if FIQ is enabled.
  moveq   R0,#1
  movne   R0,#0
  bx      LR

ArmSetDomainAccessControl
  mcr     p15,0,r0,c3,c0,0
  bx      lr

CPSRMaskInsert    // on entry, r0 is the mask and r1 is the field to insert
  stmfd   sp!, {r4-r12, lr} // save all the banked registers
  mov     r3, sp            // copy the stack pointer into a non-banked register
  mrs     r2, cpsr          // read the cpsr
  bic     r2, r2, r0        // clear mask in the cpsr
  and     r1, r1, r0        // clear bits outside the mask in the input
  orr     r2, r2, r1        // set field
  msr     cpsr_cxsf, r2     // write back cpsr (may have caused a mode switch)
  isb
  mov     sp, r3            // restore stack pointer
  ldmfd   sp!, {r4-r12, lr} // restore registers
  bx      lr                // return (hopefully thumb-safe!)             // return (hopefully thumb-safe!)

CPSRRead
  mrs     r0, cpsr
  bx      lr

ArmReadCpacr
  mrc     p15, 0, r0, c1, c0, 2
  bx      lr

ArmWriteCpacr
  mcr     p15, 0, r0, c1, c0, 2
  isb
  bx      lr

ArmWriteAuxCr
  mcr     p15, 0, r0, c1, c0, 1
  bx      lr

ArmReadAuxCr
  mrc     p15, 0, r0, c1, c0, 1
  bx      lr

ArmSetTTBR0
  mcr     p15,0,r0,c2,c0,0
  isb
  bx      lr

ArmGetTTBR0BaseAddress
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
ArmUpdateTranslationTableEntry
  mcr     p15,0,R0,c7,c14,1     // DCCIMVAC Clean data cache by MVA
  dsb
  mcr     p15,0,R1,c8,c7,1      // TLBIMVA TLB Invalidate MVA
  mcr     p15,0,R9,c7,c5,6      // BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

ArmInvalidateTlb
  mov     r0,#0
  mcr     p15,0,r0,c8,c7,0
  mcr     p15,0,R9,c7,c5,6      // BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

ArmReadScr
  mrc     p15, 0, r0, c1, c1, 0
  bx      lr

ArmWriteScr
  mcr     p15, 0, r0, c1, c1, 0
  bx      lr

ArmReadHVBar
  mrc     p15, 4, r0, c12, c0, 0
  bx      lr

ArmWriteHVBar
  mcr     p15, 4, r0, c12, c0, 0
  bx      lr

ArmReadMVBar
  mrc     p15, 0, r0, c12, c0, 1
  bx      lr

ArmWriteMVBar
  mcr     p15, 0, r0, c12, c0, 1
  bx      lr

ArmCallWFE
  wfe
  bx      lr

ArmCallSEV
  sev
  bx      lr

ArmReadSctlr
  mrc     p15, 0, r0, c1, c0, 0      // Read SCTLR into R0 (Read control register configuration data)
  bx      lr


ArmReadCpuActlr
  mrc     p15, 0, r0, c1, c0, 1
  bx      lr

ArmWriteCpuActlr
  mcr     p15, 0, r0, c1, c0, 1
  dsb
  isb
  bx      lr

  END
