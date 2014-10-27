#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011, ARM Limited. All rights reserved.
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

.text
.align 2
GCC_ASM_EXPORT(ArmDisableCachesAndMmu)
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
GCC_ASM_EXPORT(ArmSetLowVectors)
GCC_ASM_EXPORT(ArmSetHighVectors)
GCC_ASM_EXPORT(ArmIsMpCore)
GCC_ASM_EXPORT(ArmCallWFI)
GCC_ASM_EXPORT(ArmReadMpidr)
GCC_ASM_EXPORT(ArmUpdateTranslationTableEntry)
GCC_ASM_EXPORT(ArmEnableFiq)
GCC_ASM_EXPORT(ArmDisableFiq)
GCC_ASM_EXPORT(ArmEnableInterrupts)
GCC_ASM_EXPORT(ArmDisableInterrupts)
GCC_ASM_EXPORT (ArmEnableVFP)

Arm11PartNumberMask:  .word       0xFFF0
Arm11PartNumber:      .word       0xB020

.set DC_ON, (0x1<<2)
.set IC_ON, (0x1<<12)
.set XP_ON, (0x1<<23)
.set CTRL_M_BIT,  (1 << 0)
.set CTRL_C_BIT,  (1 << 2)
.set CTRL_I_BIT,  (1 << 12)

ASM_PFX(ArmDisableCachesAndMmu):
  mrc   p15, 0, r0, c1, c0, 0           @ Get control register
  bic   r0, r0, #CTRL_M_BIT             @ Disable MMU
  bic   r0, r0, #CTRL_C_BIT             @ Disable D Cache
  bic   r0, r0, #CTRL_I_BIT             @ Disable I Cache
  mcr   p15, 0, r0, c1, c0, 0           @ Write control register
  bx      LR

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
  LoadConstantToReg(DC_ON, R1)  @ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  orr     R0,R0,R1              @Set C bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  bx      LR

ASM_PFX(ArmDisableDataCache):
  LoadConstantToReg(DC_ON, R1)  @ldr     R1,=DC_ON
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

ASM_PFX(ArmDataMemoryBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C10, #5
  bx      LR

ASM_PFX(ArmDataSyncronizationBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C10, #4
  bx      LR

ASM_PFX(ArmInstructionSynchronizationBarrier):
  mov R0, #0
  mcr P15, #0, R0, C7, C5, #4
  bx      LR

ASM_PFX(ArmSetLowVectors):
  mrc     p15, 0, r0, c1, c0, 0 @ Read SCTLR into R0 (Read control register configuration data)
  bic     r0, r0, #0x00002000   @ clear V bit
  mcr     p15, 0, r0, c1, c0, 0 @ Write R0 into SCTLR (Write control register configuration data)
  bx      LR

ASM_PFX(ArmSetHighVectors):
  mrc     p15, 0, r0, c1, c0, 0 @ Read SCTLR into R0 (Read control register configuration data)
  orr     r0, r0, #0x00002000   @ clear V bit
  mcr     p15, 0, r0, c1, c0, 0 @ Write R0 into SCTLR (Write control register configuration data)
  bx      LR

ASM_PFX(ArmIsMpCore):
  push    { r1 }
  mrc     p15, 0, r0, c0, c0, 0
  # Extract Part Number to check it is an ARM11MP core (0xB02)
  LoadConstantToReg (Arm11PartNumberMask, r1)
  and     r0, r0, r1
  LoadConstantToReg (Arm11PartNumber, r1)
  cmp     r0, r1
  movne   r0, #0
  pop     { r1 }
  bx      lr

ASM_PFX(ArmCallWFI):
  wfi
  bx      lr

ASM_PFX(ArmReadMpidr):
  mrc     p15, 0, r0, c0, c0, 5       @ read MPIDR
  bx      lr

ASM_PFX(ArmEnableFiq):
  mrs     R0,CPSR
  bic     R0,R0,#0x40    @Enable FIQ interrupts
  msr     CPSR_c,R0
  bx      LR

ASM_PFX(ArmDisableFiq):
  mrs     R0,CPSR
  orr     R1,R0,#0x40    @Disable FIQ interrupts
  msr     CPSR_c,R1
  tst     R0,#0x80
  moveq   R0,#1
  movne   R0,#0
  bx      LR

ASM_PFX(ArmEnableInterrupts):
  mrs     R0,CPSR
  bic     R0,R0,#0x80    @Enable IRQ interrupts
  msr     CPSR_c,R0
  bx      LR

ASM_PFX(ArmDisableInterrupts):
  mrs     R0,CPSR
  orr     R1,R0,#0x80    @Disable IRQ interrupts
  msr     CPSR_c,R1
  tst     R0,#0x80
  moveq   R0,#1
  movne   R0,#0
  bx      LR

ASM_PFX(ArmEnableVFP):
  # Read CPACR (Coprocessor Access Control Register)
  mrc     p15, 0, r0, c1, c0, 2
  # Enable VPF access (Full Access to CP10, CP11) (V* instructions)
  orr     r0, r0, #0x00f00000
  # Write back CPACR (Coprocessor Access Control Register)
  mcr     p15, 0, r0, c1, c0, 2
  # Set EN bit in FPEXC. The Advanced SIMD and VFP extensions are enabled and operate normally.
  mov     r0, #0x40000000
  #TODO: Fixme - need compilation flag
  #fmxr    FPEXC, r0
  bx      lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
