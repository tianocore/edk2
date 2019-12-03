//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
// Copyright (c) 2011 - 2016, ARM Limited. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

    INCLUDE AsmMacroIoLib.inc


    INCLUDE AsmMacroExport.inc

 RVCT_ASM_EXPORT ArmReadMidr
  mrc     p15,0,R0,c0,c0,0
  bx      LR

 RVCT_ASM_EXPORT ArmCacheInfo
  mrc     p15,0,R0,c0,c0,1
  bx      LR

 RVCT_ASM_EXPORT ArmGetInterruptState
  mrs     R0,CPSR
  tst     R0,#0x80      // Check if IRQ is enabled.
  moveq   R0,#1
  movne   R0,#0
  bx      LR

 RVCT_ASM_EXPORT ArmGetFiqState
  mrs     R0,CPSR
  tst     R0,#0x40      // Check if FIQ is enabled.
  moveq   R0,#1
  movne   R0,#0
  bx      LR

 RVCT_ASM_EXPORT ArmSetDomainAccessControl
  mcr     p15,0,r0,c3,c0,0
  bx      lr

 RVCT_ASM_EXPORT CPSRMaskInsert
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

 RVCT_ASM_EXPORT CPSRRead
  mrs     r0, cpsr
  bx      lr

 RVCT_ASM_EXPORT ArmReadCpacr
  mrc     p15, 0, r0, c1, c0, 2
  bx      lr

 RVCT_ASM_EXPORT ArmWriteCpacr
  mcr     p15, 0, r0, c1, c0, 2
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmWriteAuxCr
  mcr     p15, 0, r0, c1, c0, 1
  bx      lr

 RVCT_ASM_EXPORT ArmReadAuxCr
  mrc     p15, 0, r0, c1, c0, 1
  bx      lr

 RVCT_ASM_EXPORT ArmSetTTBR0
  mcr     p15,0,r0,c2,c0,0
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmSetTTBCR
  mcr     p15, 0, r0, c2, c0, 2
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmGetTTBR0BaseAddress
  mrc     p15,0,r0,c2,c0,0
  MOV32   r1, 0xFFFFC000
  and     r0, r0, r1
  isb
  bx      lr

//
//VOID
//ArmUpdateTranslationTableEntry (
//  IN VOID  *TranslationTableEntry  // R0
//  IN VOID  *MVA                    // R1
//  );
 RVCT_ASM_EXPORT ArmUpdateTranslationTableEntry
  mcr     p15,0,R0,c7,c14,1     // DCCIMVAC Clean data cache by MVA
  dsb
  mcr     p15,0,R1,c8,c7,1      // TLBIMVA TLB Invalidate MVA
  mcr     p15,0,R9,c7,c5,6      // BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmInvalidateTlb
  mov     r0,#0
  mcr     p15,0,r0,c8,c7,0
  mcr     p15,0,R9,c7,c5,6      // BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmReadScr
  mrc     p15, 0, r0, c1, c1, 0
  bx      lr

 RVCT_ASM_EXPORT ArmWriteScr
  mcr     p15, 0, r0, c1, c1, 0
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmReadHVBar
  mrc     p15, 4, r0, c12, c0, 0
  bx      lr

 RVCT_ASM_EXPORT ArmWriteHVBar
  mcr     p15, 4, r0, c12, c0, 0
  bx      lr

 RVCT_ASM_EXPORT ArmReadMVBar
  mrc     p15, 0, r0, c12, c0, 1
  bx      lr

 RVCT_ASM_EXPORT ArmWriteMVBar
  mcr     p15, 0, r0, c12, c0, 1
  bx      lr

 RVCT_ASM_EXPORT ArmCallWFE
  wfe
  bx      lr

 RVCT_ASM_EXPORT ArmCallSEV
  sev
  bx      lr

 RVCT_ASM_EXPORT ArmReadSctlr
  mrc     p15, 0, r0, c1, c0, 0      // Read SCTLR into R0 (Read control register configuration data)
  bx      lr

 RVCT_ASM_EXPORT ArmWriteSctlr
  mcr     p15, 0, r0, c1, c0, 0
  bx      lr

 RVCT_ASM_EXPORT ArmReadCpuActlr
  mrc     p15, 0, r0, c1, c0, 1
  bx      lr

 RVCT_ASM_EXPORT ArmWriteCpuActlr
  mcr     p15, 0, r0, c1, c0, 1
  dsb
  isb
  bx      lr

 RVCT_ASM_EXPORT ArmGetPhysicalAddressBits
  mrc     p15, 0, r0, c0, c1, 4   ; MMFR0
  and     r0, r0, #0xf            ; VMSA [3:0]
  cmp     r0, #5                  ; >= 5 implies LPAE support
  movlt   r0, #32                 ; 32 bits if no LPAE
  movge   r0, #40                 ; 40 bits if LPAE
  bx      lr

  END
