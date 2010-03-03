//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008-2009 Apple Inc. All rights reserved.
//
// All rights reserved. This program and the accompanying materials
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
    EXPORT  ArmMmuEnabled
    EXPORT  ArmEnableDataCache
    EXPORT  ArmDisableDataCache
    EXPORT  ArmEnableInstructionCache
    EXPORT  ArmDisableInstructionCache
    EXPORT  ArmEnableBranchPrediction
    EXPORT  ArmDisableBranchPrediction
    EXPORT  ArmV7AllDataCachesOperation
    EXPORT  ArmDataMemoryBarrier
    EXPORT  ArmDataSyncronizationBarrier
    EXPORT  ArmInstructionSynchronizationBarrier

    AREA    ArmCacheLib, CODE, READONLY
    PRESERVE8

DC_ON       EQU     ( 0x1:SHL:2 )
IC_ON       EQU     ( 0x1:SHL:12 )



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


ArmDrainWriteBuffer
  mcr     p15, 0, r0, c7, c10, 4       ; Drain write buffer for sync
  dsb
  isb
  bx      lr


ArmInvalidateInstructionCache
  mov     R0,#0
  mcr     p15,0,R0,c7,c5,0      ;Invalidate entire instruction cache
  mov     R0,#0
  dsb
  isb
  bx      LR

ArmEnableMmu
  mrc     p15,0,R0,c1,c0,0
  orr     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  dsb
  isb
  bx      LR

ArmMmuEnabled
  mrc     p15,0,R0,c1,c0,0
  and     R0,R0,#1
  isb
  bx      LR

ArmDisableMmu
  mov     R0,#0
  mcr     p15,0,R0,c13,c0,0     ;FCSE PID register must be cleared before disabling MMU
  mrc     p15,0,R0,c1,c0,0
  bic     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0      ;Disable MMU
  dsb
  isb
  bx      LR

ArmEnableDataCache
  ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      ;Read control register configuration data
  orr     R0,R0,R1              ;Set C bit
  mcr     p15,0,r0,c1,c0,0      ;Write control register configuration data
  dsb
  isb
  bx      LR
    
ArmDisableDataCache
  ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      ;Read control register configuration data
  bic     R0,R0,R1              ;Clear C bit
  mcr     p15,0,r0,c1,c0,0      ;Write control register configuration data
  isb
  bx      LR

ArmEnableInstructionCache
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0      ;Read control register configuration data
  orr     R0,R0,R1              ;Set I bit
  mcr     p15,0,r0,c1,c0,0      ;Write control register configuration data
  dsb
  isb
  bx      LR
  
ArmDisableInstructionCache
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0     ;Read control register configuration data
  BIC     R0,R0,R1             ;Clear I bit.
  mcr     p15,0,r0,c1,c0,0     ;Write control register configuration data
  isb
  bx      LR

ArmEnableBranchPrediction
  mrc     p15, 0, r0, c1, c0, 0
  orr     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  isb
  bx      LR

ArmDisableBranchPrediction
  mrc     p15, 0, r0, c1, c0, 0
  bic     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
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
  add   R2, R10, R10, LSR #1  ; Work out 3xcachelevel
  mov   R12, R6, LSR R2       ; bottom 3 bits are the Cache type for this level
  and   R12, R12, #7          ; get those 3 bits alone
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
  ldmfd SP!, {r4-r12, lr}
  bx    LR


ArmDataMemoryBarrier
  dmb
  bx      LR
  
ArmDataSyncronizationBarrier
  dsb
  bx      LR
  
ArmInstructionSynchronizationBarrier
  isb
  bx      LR

    END
