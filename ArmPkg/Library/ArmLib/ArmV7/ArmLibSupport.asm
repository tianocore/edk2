//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
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


    EXPORT  Cp15IdCode
    EXPORT  Cp15CacheInfo
    EXPORT  ArmIsMPCore
    EXPORT  ArmEnableAsynchronousAbort
    EXPORT  ArmDisableAsynchronousAbort
    EXPORT  ArmEnableIrq
    EXPORT  ArmDisableIrq
    EXPORT  ArmGetInterruptState
    EXPORT  ArmEnableFiq
    EXPORT  ArmDisableFiq
    EXPORT  ArmEnableInterrupts
    EXPORT  ArmDisableInterrupts
    EXPORT  ArmGetFiqState
    EXPORT  ArmInvalidateTlb
    EXPORT  ArmSetTTBR0
    EXPORT  ArmGetTTBR0BaseAddress
    EXPORT  ArmSetDomainAccessControl
    EXPORT  ArmUpdateTranslationTableEntry
    EXPORT  CPSRMaskInsert
    EXPORT  CPSRRead
    EXPORT  ReadCCSIDR
    EXPORT  ReadCLIDR
    
    AREA ArmLibSupport, CODE, READONLY


//------------------------------------------------------------------------------

Cp15IdCode
  mrc     p15,0,R0,c0,c0,0
  bx      LR

Cp15CacheInfo
  mrc     p15,0,R0,c0,c0,1
  bx      LR

ArmIsMPCore
  mrc     p15,0,R0,c0,c0,5
  // Get Multiprocessing extension (bit31) & U bit (bit30)
  and     R0, R0, #0xC0000000
  // if bit30 == 0 then the processor is part of a multiprocessor system)
  and     R0, R0, #0x80000000
  bx      LR

ArmEnableAsynchronousAbort
  cpsie   a
  isb
  bx      LR

ArmDisableAsynchronousAbort
  cpsid   a
  isb
  bx      LR

ArmEnableIrq
  cpsie   i
  isb
	bx      LR

ArmDisableIrq
  cpsid   i
  isb
	bx      LR

ArmEnableFiq
  cpsie   f
  isb
	bx      LR

ArmDisableFiq
  cpsid   f
  isb
  bx      LR

ArmEnableInterrupts
  cpsie   if
  isb
  bx      LR

ArmDisableInterrupts
  cpsid   if
  isb
  bx      LR

ArmGetInterruptState
  mrs     R0,CPSR
  tst     R0,#0x80	    ;Check if IRQ is enabled.
  moveq   R0,#1
  movne   R0,#0
	bx      LR

ArmGetFiqState
	mrs     R0,CPSR
	tst     R0,#0x40	    ;Check if FIQ is enabled.
	moveq   R0,#1
	movne   R0,#0
	bx      LR
  
ArmInvalidateTlb
  mov     r0,#0
  mcr     p15,0,r0,c8,c7,0
  mcr     p15,0,R9,c7,c5,6      ; BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

ArmSetTTBR0
  mcr     p15,0,r0,c2,c0,0
  isb
  bx      lr

ArmGetTTBR0BaseAddress
  mrc     p15,0,r0,c2,c0,0
  ldr	  r1, = 0xFFFFC000
  and     r0, r0, r1
  isb
  bx      lr


ArmSetDomainAccessControl
  mcr     p15,0,r0,c3,c0,0
  isb
  bx      lr

//
//VOID
//ArmUpdateTranslationTableEntry (
//  IN VOID  *TranslationTableEntry  // R0
//  IN VOID  *MVA                    // R1
//  );
ArmUpdateTranslationTableEntry
  mcr     p15,0,R0,c7,c14,1     ; DCCIMVAC Clean data cache by MVA
  dsb
  mcr     p15,0,R1,c8,c7,1      ; TLBIMVA TLB Invalidate MVA  
  mcr     p15,0,R9,c7,c5,6      ; BPIALL Invalidate Branch predictor array. R9 == NoOp
  dsb
  isb
  bx      lr

CPSRMaskInsert              ; on entry, r0 is the mask and r1 is the field to insert
  stmfd   sp!, {r4-r12, lr} ; save all the banked registers
  mov     r3, sp            ; copy the stack pointer into a non-banked register
  mrs     r2, cpsr          ; read the cpsr
  bic     r2, r2, r0        ; clear mask in the cpsr
  and     r1, r1, r0        ; clear bits outside the mask in the input
  orr     r2, r2, r1        ; set field
  msr     cpsr_cxsf, r2     ; write back cpsr (may have caused a mode switch)
  isb
  mov     sp, r3            ; restore stack pointer
  ldmfd   sp!, {r4-r12, lr} ; restore registers
  bx      lr                ; return (hopefully thumb-safe!)

CPSRRead
  mrs     r0, cpsr
  bx      lr
  

// UINT32 
// ReadCCSIDR (
//   IN UINT32 CSSELR
//   )  
ReadCCSIDR
  mcr p15,2,r0,c0,c0,0   ; Write Cache Size Selection Register (CSSELR)
  isb
  mrc p15,1,r0,c0,c0,0 ; Read current CP15 Cache Size ID Register (CCSIDR)
  bx  lr
  

// UINT32 
// ReadCLIDR (
//   IN UINT32 CSSELR
//   )  
ReadCLIDR
  mrc p15,1,r0,c0,c0,1 ; Read CP15 Cache Level ID Register
  bx  lr
  
END
