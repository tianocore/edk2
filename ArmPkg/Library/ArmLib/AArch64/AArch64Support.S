#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
# Copyright (c) 2011 - 2017, ARM Limited. All rights reserved.
# Copyright (c) 2016, Linaro Limited. All rights reserved.
# Copyright (c) 2020, NUVIA Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#------------------------------------------------------------------------------

#include <Chipset/AArch64.h>
#include <AsmMacroIoLibV8.h>

.set CTRL_M_BIT,      (1 << 0)
.set CTRL_A_BIT,      (1 << 1)
.set CTRL_C_BIT,      (1 << 2)
.set CTRL_SA_BIT,     (1 << 3)
.set CTRL_I_BIT,      (1 << 12)
.set CTRL_V_BIT,      (1 << 12)
.set CPACR_VFP_BITS,  (3 << 20)

ASM_FUNC(ArmInvalidateDataCacheEntryByMVA)
  dc      ivac, x0    // Invalidate single data cache line
  ret


ASM_FUNC(ArmCleanDataCacheEntryByMVA)
  dc      cvac, x0    // Clean single data cache line
  ret


ASM_FUNC(ArmCleanDataCacheEntryToPoUByMVA)
  dc      cvau, x0    // Clean single data cache line to PoU
  ret

ASM_FUNC(ArmInvalidateInstructionCacheEntryToPoUByMVA)
  ic      ivau, x0    // Invalidate single instruction cache line to PoU
  ret


ASM_FUNC(ArmCleanInvalidateDataCacheEntryByMVA)
  dc      civac, x0   // Clean and invalidate single data cache line
  ret


ASM_FUNC(ArmInvalidateDataCacheEntryBySetWay)
  dc      isw, x0     // Invalidate this line
  ret


ASM_FUNC(ArmCleanInvalidateDataCacheEntryBySetWay)
  dc      cisw, x0    // Clean and Invalidate this line
  ret


ASM_FUNC(ArmCleanDataCacheEntryBySetWay)
  dc      csw, x0     // Clean this line
  ret


ASM_FUNC(ArmInvalidateInstructionCache)
  ic      iallu       // Invalidate entire instruction cache
  dsb     sy
  isb
  ret


ASM_FUNC(ArmEnableMmu)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1       // Read System control register EL1
   b       4f
2: mrs     x0, sctlr_el2       // Read System control register EL2
   b       4f
3: mrs     x0, sctlr_el3       // Read System control register EL3
4: orr     x0, x0, #CTRL_M_BIT // Set MMU enable bit
   EL1_OR_EL2_OR_EL3(x1)
1: tlbi    vmalle1
   dsb     nsh
   isb
   msr     sctlr_el1, x0       // Write back
   b       4f
2: tlbi    alle2
   dsb     nsh
   isb
   msr     sctlr_el2, x0       // Write back
   b       4f
3: tlbi    alle3
   dsb     nsh
   isb
   msr     sctlr_el3, x0       // Write back
4: isb
   ret


ASM_FUNC(ArmDisableMmu)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Read System Control Register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Read System Control Register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Read System Control Register EL3
4: and     x0, x0, #~CTRL_M_BIT  // Clear MMU enable bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back
   tlbi    vmalle1
   b       4f
2: msr     sctlr_el2, x0        // Write back
   tlbi    alle2
   b       4f
3: msr     sctlr_el3, x0        // Write back
   tlbi    alle3
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableCachesAndMmu)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: mov     x1, #~(CTRL_M_BIT | CTRL_C_BIT | CTRL_I_BIT)  // Disable MMU, D & I caches
   and     x0, x0, x1
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmMmuEnabled)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: and     x0, x0, #CTRL_M_BIT
   ret


ASM_FUNC(ArmEnableDataCache)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: orr     x0, x0, #CTRL_C_BIT  // Set C bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableDataCache)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: and     x0, x0, #~CTRL_C_BIT  // Clear C bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmEnableInstructionCache)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: orr     x0, x0, #CTRL_I_BIT  // Set I bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableInstructionCache)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: and     x0, x0, #~CTRL_I_BIT  // Clear I bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmEnableAlignmentCheck)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       3f
2: mrs     x0, sctlr_el2        // Get control register EL2
3: orr     x0, x0, #CTRL_A_BIT  // Set A (alignment check) bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       3f
2: msr     sctlr_el2, x0        // Write back control register
3: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableAlignmentCheck)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: and     x0, x0, #~CTRL_A_BIT  // Clear A (alignment check) bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret

ASM_FUNC(ArmEnableStackAlignmentCheck)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       3f
2: mrs     x0, sctlr_el2        // Get control register EL2
3: orr     x0, x0, #CTRL_SA_BIT // Set SA (stack alignment check) bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       3f
2: msr     sctlr_el2, x0        // Write back control register
3: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableStackAlignmentCheck)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
   b       4f
3: mrs     x0, sctlr_el3        // Get control register EL3
4: bic     x0, x0, #CTRL_SA_BIT // Clear SA (stack alignment check) bit
   EL1_OR_EL2_OR_EL3(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
   b       4f
3: msr     sctlr_el3, x0        // Write back control register
4: dsb     sy
   isb
   ret


// Always turned on in AArch64. Else implementation specific. Leave in for C compatibility for now
ASM_FUNC(ArmEnableBranchPrediction)
  ret


// Always turned on in AArch64. Else implementation specific. Leave in for C compatibility for now.
ASM_FUNC(ArmDisableBranchPrediction)
  ret


ASM_FUNC(AArch64AllDataCachesOperation)
// We can use regs 0-7 and 9-15 without having to save/restore.
// Save our link register on the stack. - The stack must always be quad-word aligned
  stp   x29, x30, [sp, #-16]!
  mov   x29, sp
  mov   x1, x0                  // Save Function call in x1
  mrs   x6, clidr_el1           // Read EL1 CLIDR
  and   x3, x6, #0x7000000      // Mask out all but Level of Coherency (LoC)
  lsr   x3, x3, #23             // Left align cache level value - the level is shifted by 1 to the
                                // right to ease the access to CSSELR and the Set/Way operation.
  cbz   x3, L_Finished          // No need to clean if LoC is 0
  mov   x10, #0                 // Start clean at cache level 0

Loop1:
  add   x2, x10, x10, lsr #1    // Work out 3x cachelevel for cache info
  lsr   x12, x6, x2             // bottom 3 bits are the Cache type for this level
  and   x12, x12, #7            // get those 3 bits alone
  cmp   x12, #2                 // what cache at this level?
  b.lt  L_Skip                  // no cache or only instruction cache at this level
  msr   csselr_el1, x10         // write the Cache Size selection register with current level (CSSELR)
  isb                           // isb to sync the change to the CacheSizeID reg
  mrs   x12, ccsidr_el1         // reads current Cache Size ID register (CCSIDR)
  and   x2, x12, #0x7           // extract the line length field
  add   x2, x2, #4              // add 4 for the line length offset (log2 16 bytes)
  mov   x4, #0x400
  sub   x4, x4, #1
  and   x4, x4, x12, lsr #3     // x4 is the max number on the way size (right aligned)
  clz   w5, w4                  // w5 is the bit position of the way size increment
  mov   x7, #0x00008000
  sub   x7, x7, #1
  and   x7, x7, x12, lsr #13    // x7 is the max number of the index size (right aligned)

Loop2:
  mov   x9, x4                  // x9 working copy of the max way size (right aligned)

Loop3:
  lsl   x11, x9, x5
  orr   x0, x10, x11            // factor in the way number and cache number
  lsl   x11, x7, x2
  orr   x0, x0, x11             // factor in the index number

  blr   x1                      // Goto requested cache operation

  subs  x9, x9, #1              // decrement the way number
  b.ge  Loop3
  subs  x7, x7, #1              // decrement the index
  b.ge  Loop2
L_Skip:
  add   x10, x10, #2            // increment the cache number
  cmp   x3, x10
  b.gt  Loop1

L_Finished:
  dsb   sy
  isb
  ldp   x29, x30, [sp], #0x10
  ret


ASM_FUNC(ArmDataMemoryBarrier)
  dmb   sy
  ret


ASM_FUNC(ArmDataSynchronizationBarrier)
  dsb   sy
  ret


ASM_FUNC(ArmInstructionSynchronizationBarrier)
  isb
  ret


ASM_FUNC(ArmWriteVBar)
   EL1_OR_EL2_OR_EL3(x1)
1: msr   vbar_el1, x0            // Set the Address of the EL1 Vector Table in the VBAR register
   b     4f
2: msr   vbar_el2, x0            // Set the Address of the EL2 Vector Table in the VBAR register
   b     4f
3: msr   vbar_el3, x0            // Set the Address of the EL3 Vector Table in the VBAR register
4: isb
   ret

ASM_FUNC(ArmReadVBar)
   EL1_OR_EL2_OR_EL3(x1)
1: mrs   x0, vbar_el1            // Set the Address of the EL1 Vector Table in the VBAR register
   ret
2: mrs   x0, vbar_el2            // Set the Address of the EL2 Vector Table in the VBAR register
   ret
3: mrs   x0, vbar_el3            // Set the Address of the EL3 Vector Table in the VBAR register
   ret


ASM_FUNC(ArmEnableVFP)
  // Check whether floating-point is implemented in the processor.
  mov   x1, x30                 // Save LR
  bl    ArmReadIdAA64Pfr0       // Read EL1 Processor Feature Register (PFR0)
  mov   x30, x1                 // Restore LR
  ubfx  x0, x0, #16, #4         // Extract the FP bits 16:19
  cmp   x0, #0xF                // Check if FP bits are '1111b',
                                // i.e. Floating Point not implemented
  b.eq  4f                      // Exit when VFP is not implemented.

  // FVP is implemented.
  // Make sure VFP exceptions are not trapped (to any exception level).
  mrs   x0, cpacr_el1           // Read EL1 Coprocessor Access Control Register (CPACR)
  orr   x0, x0, #CPACR_VFP_BITS // Disable FVP traps to EL1
  msr   cpacr_el1, x0           // Write back EL1 Coprocessor Access Control Register (CPACR)
  mov   x1, #AARCH64_CPTR_TFP   // TFP Bit for trapping VFP Exceptions
  EL1_OR_EL2_OR_EL3(x2)
1:ret                           // Not configurable in EL1
2:mrs   x0, cptr_el2            // Disable VFP traps to EL2
  bic   x0, x0, x1
  msr   cptr_el2, x0
  ret
3:mrs   x0, cptr_el3            // Disable VFP traps to EL3
  bic   x0, x0, x1
  msr   cptr_el3, x0
4:ret


ASM_FUNC(ArmCallWFI)
  wfi
  ret

ASM_FUNC(ArmReadIdAA64Mmfr2)
  mrs   x0, ID_AA64MMFR2_EL1           // read EL1 MMFR2
  ret

ASM_FUNC(ArmReadMpidr)
  mrs   x0, mpidr_el1           // read EL1 MPIDR
  ret


// Keep old function names for C compatibility for now. Change later?
ASM_FUNC(ArmReadTpidrurw)
  mrs   x0, tpidr_el0           // read tpidr_el0 (v7 TPIDRURW) -> (v8 TPIDR_EL0)
  ret


// Keep old function names for C compatibility for now. Change later?
ASM_FUNC(ArmWriteTpidrurw)
  msr   tpidr_el0, x0           // write tpidr_el0 (v7 TPIDRURW) -> (v8 TPIDR_EL0)
  ret


// Arch timers are mandatory on AArch64
ASM_FUNC(ArmIsArchTimerImplemented)
  mov   x0, #1
  ret


ASM_FUNC(ArmReadIdAA64Pfr0)
  mrs   x0, id_aa64pfr0_el1   // Read ID_AA64PFR0 Register
  ret


// VOID ArmWriteHcr(UINTN Hcr)
ASM_FUNC(ArmWriteHcr)
  msr   hcr_el2, x0        // Write the passed HCR value
  ret

// UINTN ArmReadHcr(VOID)
ASM_FUNC(ArmReadHcr)
  mrs   x0, hcr_el2
  ret

// UINTN ArmReadCurrentEL(VOID)
ASM_FUNC(ArmReadCurrentEL)
  mrs   x0, CurrentEL
  ret

// UINT32 ArmReadCntHctl(VOID)
ASM_FUNC(ArmReadCntHctl)
  mrs   x0, cnthctl_el2
  ret

// VOID ArmWriteCntHctl(UINT32 CntHctl)
ASM_FUNC(ArmWriteCntHctl)
  msr   cnthctl_el2, x0
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
