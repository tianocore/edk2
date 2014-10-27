//------------------------------------------------------------------------------
//
// Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
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

    EXPORT  ArmInvalidateInstructionCache
    EXPORT  ArmInvalidateDataCacheEntryByMVA
    EXPORT  ArmCleanDataCacheEntryByMVA
    EXPORT  ArmCleanInvalidateDataCacheEntryByMVA
    EXPORT  ArmInvalidateDataCacheEntryBySetWay
    EXPORT  ArmCleanDataCacheEntryBySetWay
    EXPORT  ArmCleanInvalidateDataCacheEntryBySetWay
    EXPORT  ArmDrainWriteBuffer
    EXPORT  ArmEnableMmu
    EXPORT  ArmDisableMmu
    EXPORT  ArmDisableCachesAndMmu
    EXPORT  ArmMmuEnabled
    EXPORT  ArmEnableDataCache
    EXPORT  ArmDisableDataCache
    EXPORT  ArmEnableInstructionCache
    EXPORT  ArmDisableInstructionCache
    EXPORT  ArmEnableSWPInstruction
    EXPORT  ArmEnableBranchPrediction
    EXPORT  ArmDisableBranchPrediction
    EXPORT  ArmSetLowVectors
    EXPORT  ArmSetHighVectors
    EXPORT  ArmV7AllDataCachesOperation
    EXPORT  ArmV7PerformPoUDataCacheOperation
    EXPORT  ArmDataMemoryBarrier
    EXPORT  ArmDataSyncronizationBarrier
    EXPORT  ArmInstructionSynchronizationBarrier
    EXPORT  ArmReadVBar
    EXPORT  ArmWriteVBar
    EXPORT  ArmEnableVFP
    EXPORT  ArmCallWFI
    EXPORT  ArmReadCbar
    EXPORT  ArmReadMpidr
    EXPORT  ArmReadTpidrurw
    EXPORT  ArmWriteTpidrurw
    EXPORT  ArmIsArchTimerImplemented
    EXPORT  ArmReadIdPfr1

    AREA    ArmV7Support, CODE, READONLY
    PRESERVE8

DC_ON           EQU     ( 0x1:SHL:2 )
IC_ON           EQU     ( 0x1:SHL:12 )
CTRL_M_BIT      EQU     (1 << 0)
CTRL_C_BIT      EQU     (1 << 2)
CTRL_B_BIT      EQU     (1 << 7)
CTRL_I_BIT      EQU     (1 << 12)


ArmInvalidateDataCacheEntryByMVA
  mcr     p15, 0, r0, c7, c6, 1   ; invalidate single data cache line
  dsb
  isb
  bx      lr

ArmCleanDataCacheEntryByMVA
  mcr     p15, 0, r0, c7, c10, 1  ; clean single data cache line
  dsb
  isb
  bx      lr


ArmCleanInvalidateDataCacheEntryByMVA
  mcr     p15, 0, r0, c7, c14, 1  ; clean and invalidate single data cache line
  dsb
  isb
  bx      lr


ArmInvalidateDataCacheEntryBySetWay
  mcr     p15, 0, r0, c7, c6, 2        ; Invalidate this line
  dsb
  isb
  bx      lr


ArmCleanInvalidateDataCacheEntryBySetWay
  mcr     p15, 0, r0, c7, c14, 2       ; Clean and Invalidate this line
  dsb
  isb
  bx      lr


ArmCleanDataCacheEntryBySetWay
  mcr     p15, 0, r0, c7, c10, 2       ; Clean this line
  dsb
  isb
  bx      lr


ArmInvalidateInstructionCache
  mcr     p15,0,R0,c7,c5,0      ;Invalidate entire instruction cache
  isb
  bx      LR

ArmEnableMmu
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  orr     R0,R0,#1              ; Set SCTLR.M bit : Enable MMU
  mcr     p15,0,R0,c1,c0,0      ; Write R0 into SCTLR (Write control register configuration data)
  dsb
  isb
  bx      LR

ArmDisableMmu
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  bic     R0,R0,#1              ; Clear SCTLR.M bit : Disable MMU
  mcr     p15,0,R0,c1,c0,0      ; Write R0 into SCTLR (Write control register configuration data)

  mcr     p15,0,R0,c8,c7,0      ; TLBIALL : Invalidate unified TLB
  mcr     p15,0,R0,c7,c5,6      ; BPIALL  : Invalidate entire branch predictor array
  dsb
  isb
  bx      LR

ArmDisableCachesAndMmu
  mrc   p15, 0, r0, c1, c0, 0           ; Get control register
  bic   r0, r0, #CTRL_M_BIT             ; Disable MMU
  bic   r0, r0, #CTRL_C_BIT             ; Disable D Cache
  bic   r0, r0, #CTRL_I_BIT             ; Disable I Cache
  mcr   p15, 0, r0, c1, c0, 0           ; Write control register
  dsb
  isb
  bx      LR

ArmMmuEnabled
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  and     R0,R0,#1
  bx      LR

ArmEnableDataCache
  ldr     R1,=DC_ON             ; Specify SCTLR.C bit : (Data) Cache enable bit
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  orr     R0,R0,R1              ; Set SCTLR.C bit : Data and unified caches enabled
  mcr     p15,0,R0,c1,c0,0      ; Write R0 into SCTLR (Write control register configuration data)
  dsb
  isb
  bx      LR

ArmDisableDataCache
  ldr     R1,=DC_ON             ; Specify SCTLR.C bit : (Data) Cache enable bit
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  bic     R0,R0,R1              ; Clear SCTLR.C bit : Data and unified caches disabled
  mcr     p15,0,R0,c1,c0,0      ; Write R0 into SCTLR (Write control register configuration data)
  dsb
  isb
  bx      LR

ArmEnableInstructionCache
  ldr     R1,=IC_ON             ; Specify SCTLR.I bit : Instruction cache enable bit
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  orr     R0,R0,R1              ; Set SCTLR.I bit : Instruction caches enabled
  mcr     p15,0,R0,c1,c0,0      ; Write R0 into SCTLR (Write control register configuration data)
  dsb
  isb
  bx      LR

ArmDisableInstructionCache
  ldr     R1,=IC_ON             ; Specify SCTLR.I bit : Instruction cache enable bit
  mrc     p15,0,R0,c1,c0,0      ; Read SCTLR into R0 (Read control register configuration data)
  BIC     R0,R0,R1              ; Clear SCTLR.I bit : Instruction caches disabled
  mcr     p15,0,R0,c1,c0,0      ; Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      LR

ArmEnableSWPInstruction
  mrc     p15, 0, r0, c1, c0, 0
  orr     r0, r0, #0x00000400
  mcr     p15, 0, r0, c1, c0, 0
  isb
  bx      LR

ArmEnableBranchPrediction
  mrc     p15, 0, r0, c1, c0, 0 ; Read SCTLR into R0 (Read control register configuration data)
  orr     r0, r0, #0x00000800   ;
  mcr     p15, 0, r0, c1, c0, 0 ; Write R0 into SCTLR (Write control register configuration data)
  dsb
  isb
  bx      LR

ArmDisableBranchPrediction
  mrc     p15, 0, r0, c1, c0, 0 ; Read SCTLR into R0 (Read control register configuration data)
  bic     r0, r0, #0x00000800   ;
  mcr     p15, 0, r0, c1, c0, 0 ; Write R0 into SCTLR (Write control register configuration data)
  dsb
  isb
  bx      LR

ArmSetLowVectors
  mrc     p15, 0, r0, c1, c0, 0 ; Read SCTLR into R0 (Read control register configuration data)
  bic     r0, r0, #0x00002000   ; clear V bit
  mcr     p15, 0, r0, c1, c0, 0 ; Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      LR

ArmSetHighVectors
  mrc     p15, 0, r0, c1, c0, 0 ; Read SCTLR into R0 (Read control register configuration data)
  orr     r0, r0, #0x00002000   ; Set V bit
  mcr     p15, 0, r0, c1, c0, 0 ; Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      LR

ArmV7AllDataCachesOperation
  stmfd SP!,{r4-r12, LR}
  mov   R1, R0                ; Save Function call in R1
  mrc   p15, 1, R6, c0, c0, 1 ; Read CLIDR
  ands  R3, R6, #&7000000     ; Mask out all but Level of Coherency (LoC)
  mov   R3, R3, LSR #23       ; Cache level value (naturally aligned)
  beq   Finished
  mov   R10, #0

Loop1
  add   R2, R10, R10, LSR #1    ; Work out 3xcachelevel
  mov   R12, R6, LSR R2         ; bottom 3 bits are the Cache type for this level
  and   R12, R12, #7            ; get those 3 bits alone
  cmp   R12, #2
  blt   Skip                    ; no cache or only instruction cache at this level
  mcr   p15, 2, R10, c0, c0, 0  ; write the Cache Size selection register (CSSELR) // OR in 1 for Instruction
  isb                           ; isb to sync the change to the CacheSizeID reg
  mrc   p15, 1, R12, c0, c0, 0  ; reads current Cache Size ID register (CCSIDR)
  and   R2, R12, #&7            ; extract the line length field
  add   R2, R2, #4              ; add 4 for the line length offset (log2 16 bytes)
  ldr   R4, =0x3FF
  ands  R4, R4, R12, LSR #3     ; R4 is the max number on the way size (right aligned)
  clz   R5, R4                  ; R5 is the bit position of the way size increment
  ldr   R7, =0x00007FFF
  ands  R7, R7, R12, LSR #13    ; R7 is the max number of the index size (right aligned)

Loop2
  mov   R9, R4                  ; R9 working copy of the max way size (right aligned)

Loop3
  orr   R0, R10, R9, LSL R5     ; factor in the way number and cache number into R11
  orr   R0, R0, R7, LSL R2      ; factor in the index number

  blx   R1

  subs  R9, R9, #1              ; decrement the way number
  bge   Loop3
  subs  R7, R7, #1              ; decrement the index
  bge   Loop2
Skip
  add   R10, R10, #2            ; increment the cache number
  cmp   R3, R10
  bgt   Loop1

Finished
  dsb
  ldmfd SP!, {r4-r12, lr}
  bx    LR

ArmV7PerformPoUDataCacheOperation
  stmfd SP!,{r4-r12, LR}
  mov   R1, R0                ; Save Function call in R1
  mrc   p15, 1, R6, c0, c0, 1 ; Read CLIDR
  ands  R3, R6, #&38000000    ; Mask out all but Level of Unification (LoU)
  mov   R3, R3, LSR #26       ; Cache level value (naturally aligned)
  beq   Finished2
  mov   R10, #0

Loop4
  add   R2, R10, R10, LSR #1    ; Work out 3xcachelevel
  mov   R12, R6, LSR R2         ; bottom 3 bits are the Cache type for this level
  and   R12, R12, #7            ; get those 3 bits alone
  cmp   R12, #2
  blt   Skip2                   ; no cache or only instruction cache at this level
  mcr   p15, 2, R10, c0, c0, 0  ; write the Cache Size selection register (CSSELR) // OR in 1 for Instruction
  isb                           ; isb to sync the change to the CacheSizeID reg
  mrc   p15, 1, R12, c0, c0, 0  ; reads current Cache Size ID register (CCSIDR)
  and   R2, R12, #&7            ; extract the line length field
  add   R2, R2, #4              ; add 4 for the line length offset (log2 16 bytes)
  ldr   R4, =0x3FF
  ands  R4, R4, R12, LSR #3     ; R4 is the max number on the way size (right aligned)
  clz   R5, R4                  ; R5 is the bit position of the way size increment
  ldr   R7, =0x00007FFF
  ands  R7, R7, R12, LSR #13    ; R7 is the max number of the index size (right aligned)

Loop5
  mov   R9, R4                  ; R9 working copy of the max way size (right aligned)

Loop6
  orr   R0, R10, R9, LSL R5     ; factor in the way number and cache number into R11
  orr   R0, R0, R7, LSL R2      ; factor in the index number

  blx   R1

  subs  R9, R9, #1              ; decrement the way number
  bge   Loop6
  subs  R7, R7, #1              ; decrement the index
  bge   Loop5
Skip2
  add   R10, R10, #2            ; increment the cache number
  cmp   R3, R10
  bgt   Loop4

Finished2
  dsb
  ldmfd SP!, {r4-r12, lr}
  bx    LR

ArmDataMemoryBarrier
  dmb
  bx      LR

ArmDataSyncronizationBarrier
ArmDrainWriteBuffer
  dsb
  bx      LR

ArmInstructionSynchronizationBarrier
  isb
  bx      LR

ArmReadVBar
  // Set the Address of the Vector Table in the VBAR register
  mrc     p15, 0, r0, c12, c0, 0
  bx      lr

ArmWriteVBar
  // Set the Address of the Vector Table in the VBAR register
  mcr     p15, 0, r0, c12, c0, 0
  // Ensure the SCTLR.V bit is clear
  mrc     p15, 0, r0, c1, c0, 0 ; Read SCTLR into R0 (Read control register configuration data)
  bic     r0, r0, #0x00002000   ; clear V bit
  mcr     p15, 0, r0, c1, c0, 0 ; Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      lr

ArmEnableVFP
  // Read CPACR (Coprocessor Access Control Register)
  mrc     p15, 0, r0, c1, c0, 2
  // Enable VPF access (Full Access to CP10, CP11) (V* instructions)
  orr     r0, r0, #0x00f00000
  // Write back CPACR (Coprocessor Access Control Register)
  mcr     p15, 0, r0, c1, c0, 2
  isb
  // Set EN bit in FPEXC. The Advanced SIMD and VFP extensions are enabled and operate normally.
  mov     r0, #0x40000000
  mcr     p10,#0x7,r0,c8,c0,#0
  bx      lr

ArmCallWFI
  wfi
  bx      lr

//Note: Return 0 in Uniprocessor implementation
ArmReadCbar
  mrc     p15, 4, r0, c15, c0, 0  //Read Configuration Base Address Register
  bx      lr

ArmReadMpidr
  mrc     p15, 0, r0, c0, c0, 5     ; read MPIDR
  bx      lr

ArmReadTpidrurw
  mrc     p15, 0, r0, c13, c0, 2    ; read TPIDRURW
  bx      lr

ArmWriteTpidrurw
  mcr     p15, 0, r0, c13, c0, 2   ; write TPIDRURW
  bx      lr

ArmIsArchTimerImplemented
  mrc    p15, 0, r0, c0, c1, 1     ; Read ID_PFR1
  and    r0, r0, #0x000F0000
  bx     lr

ArmReadIdPfr1
  mrc    p15, 0, r0, c0, c1, 1     ; Read ID_PFR1 Register
  bx     lr

 END
