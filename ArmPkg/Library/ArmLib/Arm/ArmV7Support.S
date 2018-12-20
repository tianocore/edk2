#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2014, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <AsmMacroIoLib.h>

.set DC_ON, (0x1<<2)
.set IC_ON, (0x1<<12)
.set CTRL_M_BIT,  (1 << 0)
.set CTRL_C_BIT,  (1 << 2)
.set CTRL_B_BIT,  (1 << 7)
.set CTRL_I_BIT,  (1 << 12)


ASM_FUNC(ArmInvalidateDataCacheEntryByMVA)
  mcr     p15, 0, r0, c7, c6, 1   @invalidate single data cache line
  bx      lr

ASM_FUNC(ArmCleanDataCacheEntryByMVA)
  mcr     p15, 0, r0, c7, c10, 1  @clean single data cache line
  bx      lr


ASM_FUNC(ArmCleanDataCacheEntryToPoUByMVA)
  mcr     p15, 0, r0, c7, c11, 1  @clean single data cache line to PoU
  bx      lr

ASM_FUNC(ArmInvalidateInstructionCacheEntryToPoUByMVA)
  mcr     p15, 0, r0, c7, c5, 1  @Invalidate single instruction cache line to PoU
  mcr     p15, 0, r0, c7, c5, 7  @Invalidate branch predictor
  bx      lr

ASM_FUNC(ArmCleanInvalidateDataCacheEntryByMVA)
  mcr     p15, 0, r0, c7, c14, 1  @clean and invalidate single data cache line
  bx      lr


ASM_FUNC(ArmInvalidateDataCacheEntryBySetWay)
  mcr     p15, 0, r0, c7, c6, 2        @ Invalidate this line
  bx      lr


ASM_FUNC(ArmCleanInvalidateDataCacheEntryBySetWay)
  mcr     p15, 0, r0, c7, c14, 2       @ Clean and Invalidate this line
  bx      lr


ASM_FUNC(ArmCleanDataCacheEntryBySetWay)
  mcr     p15, 0, r0, c7, c10, 2       @ Clean this line
  bx      lr

ASM_FUNC(ArmInvalidateInstructionCache)
  mcr     p15,0,R0,c7,c5,0      @Invalidate entire instruction cache
  dsb
  isb
  bx      LR

ASM_FUNC(ArmEnableMmu)
  mrc     p15,0,R0,c1,c0,0
  orr     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0
  dsb
  isb
  bx      LR


ASM_FUNC(ArmDisableMmu)
  mrc     p15,0,R0,c1,c0,0
  bic     R0,R0,#1
  mcr     p15,0,R0,c1,c0,0      @Disable MMU

  mcr     p15,0,R0,c8,c7,0      @Invalidate TLB
  mcr     p15,0,R0,c7,c5,6      @Invalidate Branch predictor array
  dsb
  isb
  bx      LR

ASM_FUNC(ArmDisableCachesAndMmu)
  mrc   p15, 0, r0, c1, c0, 0           @ Get control register
  bic   r0, r0, #CTRL_M_BIT             @ Disable MMU
  bic   r0, r0, #CTRL_C_BIT             @ Disable D Cache
  bic   r0, r0, #CTRL_I_BIT             @ Disable I Cache
  mcr   p15, 0, r0, c1, c0, 0           @ Write control register
  dsb
  isb
  bx      LR

ASM_FUNC(ArmMmuEnabled)
  mrc     p15,0,R0,c1,c0,0
  and     R0,R0,#1
  bx      LR

ASM_FUNC(ArmEnableDataCache)
  ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  orr     R0,R0,R1              @Set C bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  dsb
  isb
  bx      LR

ASM_FUNC(ArmDisableDataCache)
  ldr     R1,=DC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  bic     R0,R0,R1              @Clear C bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  dsb
  isb
  bx      LR

ASM_FUNC(ArmEnableInstructionCache)
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  orr     R0,R0,R1              @Set I bit
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  dsb
  isb
  bx      LR

ASM_FUNC(ArmDisableInstructionCache)
  ldr     R1,=IC_ON
  mrc     p15,0,R0,c1,c0,0      @Read control register configuration data
  bic     R0,R0,R1              @Clear I bit.
  mcr     p15,0,r0,c1,c0,0      @Write control register configuration data
  dsb
  isb
  bx      LR

ASM_FUNC(ArmEnableSWPInstruction)
  mrc     p15, 0, r0, c1, c0, 0
  orr     r0, r0, #0x00000400
  mcr     p15, 0, r0, c1, c0, 0
  isb
  bx      LR

ASM_FUNC(ArmEnableBranchPrediction)
  mrc     p15, 0, r0, c1, c0, 0
  orr     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  dsb
  isb
  bx      LR

ASM_FUNC(ArmDisableBranchPrediction)
  mrc     p15, 0, r0, c1, c0, 0
  bic     r0, r0, #0x00000800
  mcr     p15, 0, r0, c1, c0, 0
  dsb
  isb
  bx      LR

ASM_FUNC(ArmSetLowVectors)
  mrc     p15, 0, r0, c1, c0, 0 @ Read SCTLR into R0 (Read control register configuration data)
  bic     r0, r0, #0x00002000   @ clear V bit
  mcr     p15, 0, r0, c1, c0, 0 @ Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      LR

ASM_FUNC(ArmSetHighVectors)
  mrc     p15, 0, r0, c1, c0, 0 @ Read SCTLR into R0 (Read control register configuration data)
  orr     r0, r0, #0x00002000   @ Set V bit
  mcr     p15, 0, r0, c1, c0, 0 @ Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      LR

ASM_FUNC(ArmV7AllDataCachesOperation)
  stmfd SP!,{r4-r12, LR}
  mov   R1, R0                @ Save Function call in R1
  mrc   p15, 1, R6, c0, c0, 1 @ Read CLIDR
  ands  R3, R6, #0x7000000    @ Mask out all but Level of Coherency (LoC)
  mov   R3, R3, LSR #23       @ Cache level value (naturally aligned)
  beq   L_Finished
  mov   R10, #0

Loop1:
  add   R2, R10, R10, LSR #1  @ Work out 3xcachelevel
  mov   R12, R6, LSR R2       @ bottom 3 bits are the Cache type for this level
  and   R12, R12, #7          @ get those 3 bits alone
  cmp   R12, #2
  blt   L_Skip                  @ no cache or only instruction cache at this level
  mcr   p15, 2, R10, c0, c0, 0  @ write the Cache Size selection register (CSSELR) // OR in 1 for Instruction
  isb                           @ isb to sync the change to the CacheSizeID reg
  mrc   p15, 1, R12, c0, c0, 0  @ reads current Cache Size ID register (CCSIDR)
  and   R2, R12, #0x7           @ extract the line length field
  add   R2, R2, #4              @ add 4 for the line length offset (log2 16 bytes)
@  ldr   R4, =0x3FF
  mov   R4, #0x400
  sub   R4, R4, #1
  ands  R4, R4, R12, LSR #3     @ R4 is the max number on the way size (right aligned)
  clz   R5, R4                  @ R5 is the bit position of the way size increment
@  ldr   R7, =0x00007FFF
  mov   R7, #0x00008000
  sub   R7, R7, #1
  ands  R7, R7, R12, LSR #13    @ R7 is the max number of the index size (right aligned)

Loop2:
  mov   R9, R4                  @ R9 working copy of the max way size (right aligned)

Loop3:
  orr   R0, R10, R9, LSL R5     @ factor in the way number and cache number into R11
  orr   R0, R0, R7, LSL R2      @ factor in the index number

  blx   R1

  subs  R9, R9, #1              @ decrement the way number
  bge   Loop3
  subs  R7, R7, #1              @ decrement the index
  bge   Loop2
L_Skip:
  add   R10, R10, #2            @ increment the cache number
  cmp   R3, R10
  bgt   Loop1

L_Finished:
  dsb
  ldmfd SP!, {r4-r12, lr}
  bx    LR

ASM_FUNC(ArmDataMemoryBarrier)
  dmb
  bx      LR

ASM_FUNC(ArmDataSynchronizationBarrier)
  dsb
  bx      LR

ASM_FUNC(ArmInstructionSynchronizationBarrier)
  isb
  bx      LR

ASM_FUNC(ArmReadVBar)
  # Set the Address of the Vector Table in the VBAR register
  mrc     p15, 0, r0, c12, c0, 0
  bx      lr

ASM_FUNC(ArmWriteVBar)
  # Set the Address of the Vector Table in the VBAR register
  mcr     p15, 0, r0, c12, c0, 0
  # Ensure the SCTLR.V bit is clear
  mrc     p15, 0, r0, c1, c0, 0 @ Read SCTLR into R0 (Read control register configuration data)
  bic     r0, r0, #0x00002000   @ clear V bit
  mcr     p15, 0, r0, c1, c0, 0 @ Write R0 into SCTLR (Write control register configuration data)
  isb
  bx      lr

ASM_FUNC(ArmEnableVFP)
  # Read CPACR (Coprocessor Access Control Register)
  mrc     p15, 0, r0, c1, c0, 2
  # Enable VPF access (Full Access to CP10, CP11) (V* instructions)
  orr     r0, r0, #0x00f00000
  # Write back CPACR (Coprocessor Access Control Register)
  mcr     p15, 0, r0, c1, c0, 2
  isb
  # Set EN bit in FPEXC. The Advanced SIMD and VFP extensions are enabled and operate normally.
  mov     r0, #0x40000000
#ifndef __clang__
  mcr     p10,#0x7,r0,c8,c0,#0
#else
  # Set the FPU model so Clang does not choke on the next instruction
  .fpu    neon
  vmsr    fpexc, r0
#endif
  bx      lr

ASM_FUNC(ArmCallWFI)
  wfi
  bx      lr

#Note: Return 0 in Uniprocessor implementation
ASM_FUNC(ArmReadCbar)
  mrc     p15, 4, r0, c15, c0, 0  @ Read Configuration Base Address Register
  bx      lr

ASM_FUNC(ArmReadMpidr)
  mrc     p15, 0, r0, c0, c0, 5       @ read MPIDR
  bx      lr

ASM_FUNC(ArmReadTpidrurw)
  mrc     p15, 0, r0, c13, c0, 2    @ read TPIDRURW
  bx      lr

ASM_FUNC(ArmWriteTpidrurw)
  mcr     p15, 0, r0, c13, c0, 2    @ write TPIDRURW
  bx      lr

ASM_FUNC(ArmIsArchTimerImplemented)
  mrc    p15, 0, r0, c0, c1, 1     @ Read ID_PFR1
  and    r0, r0, #0x000F0000
  bx     lr

ASM_FUNC(ArmReadIdPfr1)
  mrc    p15, 0, r0, c0, c1, 1     @ Read ID_PFR1 Register
  bx     lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
