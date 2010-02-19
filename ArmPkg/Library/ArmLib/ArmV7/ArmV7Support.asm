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

DC_ON       EQU     ( 0x1:SHL:2 )
IC_ON       EQU     ( 0x1:SHL:12 )


    AREA    ArmCacheLib, CODE, READONLY
    PRESERVE8


ArmInvalidateDataCacheEntryByMVA
  MCR     p15, 0, r0, c7, c6, 1   ; invalidate single data cache line       
  DSB
  ISB
  BX      lr


ArmCleanDataCacheEntryByMVA
  MCR     p15, 0, r0, c7, c10, 1  ; clean single data cache line     
  DSB
  ISB
  BX      lr


ArmCleanInvalidateDataCacheEntryByMVA
  MCR     p15, 0, r0, c7, c14, 1  ; clean and invalidate single data cache line
  DSB
  ISB
  BX      lr


ArmInvalidateDataCacheEntryBySetWay
  mcr     p15, 0, r0, c7, c6, 2        ; Invalidate this line		
  DSB
  ISB
  bx      lr


ArmCleanInvalidateDataCacheEntryBySetWay
  mcr     p15, 0, r0, c7, c14, 2       ; Clean and Invalidate this line		
  DSB
  ISB
  bx      lr


ArmCleanDataCacheEntryBySetWay
  mcr     p15, 0, r0, c7, c10, 2       ; Clean this line		
  DSB
  ISB
  bx      lr


ArmDrainWriteBuffer
  mcr     p15, 0, r0, c7, c10, 4       ; Drain write buffer for sync
  DSB
  ISB
  bx      lr


ArmInvalidateInstructionCache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c5,0      ;Invalidate entire instruction cache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c5,4      ;Instruction synchronization barrier
  DSB
  ISB
  BX      LR

ArmEnableMmu
  mrc     p15,0,R0,c1,c0,0
  orr     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  DSB
  ISB
  bx      LR

ArmMmuEnabled
  mrc     p15,0,R0,c1,c0,0
  and     R0,R0,#1
  ISB
  bx      LR

ArmDisableMmu
  mov     R0,#0
  mcr     p15,0,R0,c13,c0,0     ;FCSE PID register must be cleared before disabling MMU
  mrc     p15,0,R0,c1,c0,0
  bic     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0      ;Disable MMU
  DSB
  ISB
  bx      LR

ArmEnableDataCache
  LDR     R1,=DC_ON
  MRC     p15,0,R0,c1,c0,0      ;Read control register configuration data
  ORR     R0,R0,R1              ;Set C bit
  MCR     p15,0,r0,c1,c0,0      ;Write control register configuration data
  DSB
  ISB
  BX      LR
    
ArmDisableDataCache
  LDR     R1,=DC_ON
  MRC     p15,0,R0,c1,c0,0      ;Read control register configuration data
  BIC     R0,R0,R1              ;Clear C bit
  MCR     p15,0,r0,c1,c0,0      ;Write control register configuration data
  ISB
  BX      LR

ArmEnableInstructionCache
  LDR     R1,=IC_ON
  MRC     p15,0,R0,c1,c0,0      ;Read control register configuration data
  ORR     R0,R0,R1              ;Set I bit
  MCR     p15,0,r0,c1,c0,0      ;Write control register configuration data
  ISB
  BX      LR
  
ArmDisableInstructionCache
  LDR     R1,=IC_ON
  MRC     p15,0,R0,c1,c0,0     ;Read control register configuration data
  BIC     R0,R0,R1             ;Clear I bit.
  MCR     p15,0,r0,c1,c0,0     ;Write control register configuration data
  ISB
  BX      LR

ArmEnableBranchPrediction
  mrc     p15, 0, r0, c1, c0, 0
  orr     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  ISB
  bx      LR

ArmDisableBranchPrediction
  mrc     p15, 0, r0, c1, c0, 0
  bic     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  ISB
  bx      LR


ArmV7AllDataCachesOperation
  STMFD SP!,{r4-r12, LR}
  MOV   R1, R0                ; Save Function call in R1
  MRC   p15, 1, R6, c0, c0, 1 ; Read CLIDR
  ANDS  R3, R6, #&7000000     ; Mask out all but Level of Coherency (LoC)
  MOV   R3, R3, LSR #23       ; Cache level value (naturally aligned)
  BEQ   Finished
  MOV   R10, #0

Loop1   
  ADD   R2, R10, R10, LSR #1  ; Work out 3xcachelevel
  MOV   R12, R6, LSR R2       ; bottom 3 bits are the Cache type for this level
  AND   R12, R12, #7          ; get those 3 bits alone
  CMP   R12, #2
  BLT   Skip                    ; no cache or only instruction cache at this level
  MCR   p15, 2, R10, c0, c0, 0  ; write the Cache Size selection register (CSSELR) // OR in 1 for Instruction
  ISB                           ; ISB to sync the change to the CacheSizeID reg 
  MRC   p15, 1, R12, c0, c0, 0  ; reads current Cache Size ID register (CCSIDR)
  AND   R2, R12, #&7            ; extract the line length field
  ADD   R2, R2, #4              ; add 4 for the line length offset (log2 16 bytes)
  LDR   R4, =0x3FF
  ANDS  R4, R4, R12, LSR #3     ; R4 is the max number on the way size (right aligned)
  CLZ   R5, R4                  ; R5 is the bit position of the way size increment
  LDR   R7, =0x00007FFF
  ANDS  R7, R7, R12, LSR #13    ; R7 is the max number of the index size (right aligned)

Loop2   
  MOV   R9, R4                  ; R9 working copy of the max way size (right aligned)

Loop3   
  ORR   R0, R10, R9, LSL R5     ; factor in the way number and cache number into R11
  ORR   R0, R0, R7, LSL R2      ; factor in the index number

  BLX   R1

  SUBS  R9, R9, #1              ; decrement the way number
  BGE   Loop3
  SUBS  R7, R7, #1              ; decrement the index
  BGE   Loop2
Skip  
  ADD   R10, R10, #2            ; increment the cache number
  CMP   R3, R10
  BGT   Loop1
  
Finished
  LDMFD SP!, {r4-r12, lr}
  BX    LR

    END
