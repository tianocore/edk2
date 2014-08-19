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

.text
.align 2
GCC_ASM_EXPORT(ArmCleanInvalidateDataCache)
GCC_ASM_EXPORT(ArmCleanDataCache)
GCC_ASM_EXPORT(ArmInvalidateDataCache)
GCC_ASM_EXPORT(ArmInvalidateInstructionCache)
GCC_ASM_EXPORT(ArmInvalidateDataCacheEntryByMVA)
GCC_ASM_EXPORT(ArmCleanDataCacheEntryByMVA)
GCC_ASM_EXPORT(ArmCleanInvalidateDataCacheEntryByMVA)
GCC_ASM_EXPORT(ArmEnableMmu)
GCC_ASM_EXPORT(ArmDisableMmu)
GCC_ASM_EXPORT(ArmMmuEnabled)
GCC_ASM_EXPORT(ArmEnableDataCache)
GCC_ASM_EXPORT(ArmDisableDataCache)
GCC_ASM_EXPORT(ArmEnableInstructionCache)
GCC_ASM_EXPORT(ArmDisableInstructionCache)
GCC_ASM_EXPORT(ArmEnableBranchPrediction)
GCC_ASM_EXPORT(ArmDisableBranchPrediction)
GCC_ASM_EXPORT(ArmDataMemoryBarrier)
GCC_ASM_EXPORT(ArmDataSyncronizationBarrier)
GCC_ASM_EXPORT(ArmInstructionSynchronizationBarrier)


.set DC_ON, (1<<2)
.set IC_ON, (1<<12)

#------------------------------------------------------------------------------

ASM_PFX(ArmInvalidateDataCacheEntryByMVA):
  mcr     p15, 0, r0, c7, c6, 1   @ invalidate single data cache line
  bx      lr

ASM_PFX(ArmCleanDataCacheEntryByMVA):
  mcr     p15, 0, r0, c7, c10, 1  @ clean single data cache line
  bx      lr

ASM_PFX(ArmCleanInvalidateDataCacheEntryByMVA):
  mcr     p15, 0, r0, c7, c14, 1  @ clean and invalidate single data cache line
  bx      lr

ASM_PFX(ArmEnableInstructionCache):
  ldr    r1,=IC_ON
  mrc    p15,0,r0,c1,c0,0     @Read control register configuration data
  orr    r0,r0,r1             @Set I bit
  mcr    p15,0,r0,c1,c0,0     @Write control register configuration data
  bx     LR

ASM_PFX(ArmDisableInstructionCache):
  ldr    r1,=IC_ON
  mrc    p15,0,r0,c1,c0,0     @Read control register configuration data
  bic    r0,r0,r1             @Clear I bit.
  mcr    p15,0,r0,c1,c0,0     @Write control register configuration data
  bx     LR

ASM_PFX(ArmInvalidateInstructionCache):
  mov     r0,#0
  mcr     p15,0,r0,c7,c5,0     @Invalidate entire Instruction cache.
                         @Also flushes the branch target cache.
  mov     r0,#0
  mcr     p15,0,r0,c7,c10,4    @Data write buffer
  bx      LR

ASM_PFX(ArmEnableMmu):
  mrc     p15,0,R0,c1,c0,0
  orr     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  bx      LR

ASM_PFX(ArmMmuEnabled):
  mrc     p15,0,R0,c1,c0,0
  and     R0,R0,#1
  bx      LR

ASM_PFX(ArmDisableMmu):
  mrc     p15,0,R0,c1,c0,0
  bic     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  mov     R0,#0
  mcr     p15,0,R0,c7,c10,4     @Drain write buffer
  bx      LR

ASM_PFX(ArmEnableDataCache):
  ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  orr     R0,R0,R1              @Set C bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  bx      LR

ASM_PFX(ArmDisableDataCache):
  ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  bic     R0,R0,R1              @Clear C bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  bx      LR

ASM_PFX(ArmCleanDataCache):
  mrc     p15,0,r15,c7,c10,3
  bne     ASM_PFX(ArmCleanDataCache)
  mov     R0,#0
  mcr     p15,0,R0,c7,c10,4  @Drain write buffer
  bx      LR

ASM_PFX(ArmInvalidateDataCache):
  mov     R0,#0
  mcr     p15,0,R0,c7,c6,0        @Invalidate entire data cache
  mov     R0,#0
  mcr     p15,0,R0,c7,c10,4       @Drain write buffer
  bx      LR

ASM_PFX(ArmCleanInvalidateDataCache):
  mrc     p15,0,r15,c7,c14,3
  bne     ASM_PFX(ArmCleanInvalidateDataCache)
  mov     R0,#0
  mcr     p15,0,R0,c7,c10,4        @Drain write buffer
  bx      LR

ASM_PFX(ArmEnableBranchPrediction):
  bx      LR                      @Branch prediction is not supported.

ASM_PFX(ArmDisableBranchPrediction):
  bx      LR                      @Branch prediction is not supported.

ASM_PFX(ArmDataMemoryBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C10, #5    @ check if this is OK?
  bx      LR

ASM_PFX(ArmDataSyncronizationBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C10, #4   @ check if this is OK?
  bx      LR

ASM_PFX(ArmInstructionSynchronizationBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C5, #4     @ check if this is OK?
  bx      LR

ASM_FUNCTION_REMOVE_IF_UNREFERENCED

