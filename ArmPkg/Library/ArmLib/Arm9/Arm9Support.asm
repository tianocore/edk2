//------------------------------------------------------------------------------ 
//
// Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
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

    EXPORT  ArmCleanInvalidateDataCache
    EXPORT  ArmCleanDataCache
    EXPORT  ArmInvalidateDataCache
    EXPORT  ArmInvalidateInstructionCache
    EXPORT  ArmInvalidateDataCacheEntryByMVA
    EXPORT  ArmCleanDataCacheEntryByMVA
    EXPORT  ArmCleanInvalidateDataCacheEntryByMVA
    EXPORT  ArmEnableMmu
    EXPORT  ArmDisableMmu
    EXPORT  ArmMmuEnabled
    EXPORT  ArmEnableDataCache
    EXPORT  ArmDisableDataCache
    EXPORT  ArmEnableInstructionCache
    EXPORT  ArmDisableInstructionCache
    EXPORT  ArmEnableBranchPrediction
    EXPORT  ArmDisableBranchPrediction
    EXPORT  ArmDataMemoryBarrier
    EXPORT  ArmDataSyncronizationBarrier
    EXPORT  ArmInstructionSynchronizationBarrier


DC_ON       EQU     ( 0x1:SHL:2 )
IC_ON       EQU     ( 0x1:SHL:12 )

    AREA    ArmCacheLib, CODE, READONLY
    PRESERVE8


ArmInvalidateDataCacheEntryByMVA
  MCR     p15, 0, r0, c7, c6, 1   ; invalidate single data cache line                                           
  BX      lr


ArmCleanDataCacheEntryByMVA
  MCR     p15, 0, r0, c7, c10, 1  ; clean single data cache line     
  BX      lr


ArmCleanInvalidateDataCacheEntryByMVA
  MCR     p15, 0, r0, c7, c14, 1  ; clean and invalidate single data cache line
  BX      lr

ArmEnableInstructionCache
  LDR     R1,=IC_ON
  MRC     p15,0,R0,c1,c0,0     ;Read control register configuration data
  ORR     R0,R0,R1             ;Set I bit
  MCR     p15,0,r0,c1,c0,0     ;Write control register configuration data
  BX      LR
  
ArmDisableInstructionCache
  LDR     R1,=IC_ON
  MRC     p15,0,R0,c1,c0,0     ;Read control register configuration data
  BIC     R0,R0,R1             ;Clear I bit.
  MCR     p15,0,r0,c1,c0,0     ;Write control register configuration data
  BX      LR

ArmInvalidateInstructionCache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c5,0      ;Invalidate entire instruction cache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c10,4     ;Drain write buffer
  BX      LR

ArmEnableMmu
  mrc     p15,0,R0,c1,c0,0
  orr     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  bx      LR

ArmMmuEnabled
  mrc     p15,0,R0,c1,c0,0
  and     R0,R0,#1
  bx      LR

ArmDisableMmu
  mrc     p15,0,R0,c1,c0,0
  bic     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  mov     R0,#0
  mcr     p15,0,R0,c7,c10,4     ;Drain write buffer
  bx      LR

ArmEnableDataCache
  LDR     R1,=DC_ON
  MRC     p15,0,R0,c1,c0,0      ;Read control register configuration data
  ORR     R0,R0,R1              ;Set C bit
  MCR     p15,0,r0,c1,c0,0      ;Write control register configuration data
  BX      LR
    
ArmDisableDataCache
  LDR     R1,=DC_ON
  MRC     p15,0,R0,c1,c0,0      ;Read control register configuration data
  BIC     R0,R0,R1              ;Clear C bit
  MCR     p15,0,r0,c1,c0,0      ;Write control register configuration data
  BX      LR

ArmCleanDataCache
  MRC     p15,0,r15,c7,c10,3
  BNE     ArmCleanDataCache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c10,4      ;Drain write buffer
  BX      LR

ArmInvalidateDataCache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c6,0      ;Invalidate entire data cache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c10,4     ;Drain write buffer
  BX      LR
  
ArmCleanInvalidateDataCache
  MRC     p15,0,r15,c7,c14,3
  BNE     ArmCleanInvalidateDataCache
  MOV     R0,#0
  MCR     p15,0,R0,c7,c10,4      ;Drain write buffer
  BX      LR

ArmEnableBranchPrediction
  bx      LR                    ;Branch prediction is not supported.

ArmDisableBranchPrediction
  bx      LR                    ;Branch prediction is not supported.

ASM_PFX(ArmDataMemoryBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C10, #5  ; Check to see if this is correct
  bx      LR
  
ASM_PFX(ArmDataSyncronizationBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C10, #4 ; Check to see if this is correct
  bx      LR
  
ASM_PFX(ArmInstructionSynchronizationBarrier):
  MOV R0, #0
  MCR P15, #0, R0, C7, C5, #4 ; Check to see if this is correct
  bx      LR

    END
