#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008-2009 Apple Inc. All rights reserved.
#
# All rights reserved. This program and the accompanying materials
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
.globl ASM_PFX(ArmCleanInvalidateDataCache)
.globl ASM_PFX(ArmCleanDataCache)
.globl ASM_PFX(ArmInvalidateDataCache)
.globl ASM_PFX(ArmInvalidateInstructionCache)
.globl ASM_PFX(ArmInvalidateDataCacheEntryByMVA)
.globl ASM_PFX(ArmCleanDataCacheEntryByMVA)
.globl ASM_PFX(ArmCleanInvalidateDataCacheEntryByMVA)
.globl ASM_PFX(ArmEnableMmu)
.globl ASM_PFX(ArmDisableMmu)
.globl ASM_PFX(ArmMmuEnabled)
.globl ASM_PFX(ArmEnableDataCache)
.globl ASM_PFX(ArmDisableDataCache)
.globl ASM_PFX(ArmEnableInstructionCache)
.globl ASM_PFX(ArmDisableInstructionCache)
.globl ASM_PFX(ArmEnableBranchPrediction)
.globl ASM_PFX(ArmDisableBranchPrediction)

.set DC_ON, (0x1<<2)
.set IC_ON, (0x1<<12)
.set XP_ON, (0x1<<23)

ASM_PFX(ArmInvalidateDataCacheEntryByMVA):
  mcr     p15, 0, r0, c7, c6, 1   @invalidate single data cache line                                           
  bx      lr


ASM_PFX(ArmCleanDataCacheEntryByMVA):
  mcr     p15, 0, r0, c7, c10, 1  @clean single data cache line     
  bx      lr


ASM_PFX(ArmCleanInvalidateDataCacheEntryByMVA):
  mcr     p15, 0, r0, c7, c14, 1  @clean and invalidate single data cache line
  bx      lr


ASM_PFX(ArmCleanDataCache):
  mcr     p15, 0, r0, c7, c10, 0  @ clean entire data cache
  bx      lr


ASM_PFX(ArmCleanInvalidateDataCache):
  mcr     p15, 0, r0, c7, c14, 0  @ clean and invalidate entire data cache
  bx      lr


ASM_PFX(ArmInvalidateDataCache):
  mcr     p15, 0, r0, c7, c6, 0  @ invalidate entire data cache
  bx      lr


ASM_PFX(ArmInvalidateInstructionCache):
  mcr     p15, 0, r0, c7, c5, 0  @invalidate entire instruction cache
  mov     R0,#0
  mcr     p15,0,R0,c7,c5,4       @Flush Prefetch buffer
  bx      lr

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
  mcr     p15,0,R0,c7,c10,4     @Data synchronization barrier
  mov     R0,#0
  mcr     p15,0,R0,c7,c5,4      @Flush Prefetch buffer
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

ASM_PFX(ArmEnableInstructionCache):
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0     @Read control register configuration data
  orr     R0,R0,R1             @Set I bit
  mcr     p15,0,r0,c1,c0,0     @Write control register configuration data
  bx      LR
  
ASM_PFX(ArmDisableInstructionCache):
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0     @Read control register configuration data
  bic     R0,R0,R1             @Clear I bit.
  mcr     p15,0,r0,c1,c0,0     @Write control register configuration data
  bx      LR

ASM_PFX(ArmEnableBranchPrediction):
  mrc     p15, 0, r0, c1, c0, 0
  orr     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  bx      LR

ASM_PFX(ArmDisableBranchPrediction):
  mrc     p15, 0, r0, c1, c0, 0
  bic     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  bx      LR

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
