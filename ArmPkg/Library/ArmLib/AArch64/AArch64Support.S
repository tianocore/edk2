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

#include <AArch64/AArch64.h>
#include <AsmMacroLib.h>

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

ASM_FUNC(ArmInvalidateInstructionCache)
  ic      iallu       // Invalidate entire instruction cache
  dsb     sy
  isb
  ret


ASM_FUNC(ArmEnableMmu)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1       // Read System control register EL1
   b       4f
2: mrs     x0, sctlr_el2       // Read System control register EL2
4: orr     x0, x0, #CTRL_M_BIT // Set MMU enable bit
   EL1_OR_EL2(x1)
1: tlbi    vmalle1
   dsb     nsh
   isb
   msr     sctlr_el1, x0       // Write back
   b       4f
2: tlbi    alle2
   dsb     nsh
   isb
   msr     sctlr_el2, x0       // Write back
4: isb
   ret


ASM_FUNC(ArmDisableMmu)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Read System Control Register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Read System Control Register EL2
4: and     x0, x0, #~CTRL_M_BIT  // Clear MMU enable bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back
   tlbi    vmalle1
   b       4f
2: msr     sctlr_el2, x0        // Write back
   tlbi    alle2
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableCachesAndMmu)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: mov     x1, #~(CTRL_M_BIT | CTRL_C_BIT | CTRL_I_BIT)  // Disable MMU, D & I caches
   and     x0, x0, x1
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmMmuEnabled)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: and     x0, x0, #CTRL_M_BIT
   ret


ASM_FUNC(ArmEnableDataCache)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: orr     x0, x0, #CTRL_C_BIT  // Set C bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableDataCache)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: and     x0, x0, #~CTRL_C_BIT  // Clear C bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmEnableInstructionCache)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: orr     x0, x0, #CTRL_I_BIT  // Set I bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
4: dsb     sy
   isb
   ret


ASM_FUNC(ArmDisableInstructionCache)
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: and     x0, x0, #~CTRL_I_BIT  // Clear I bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
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
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: and     x0, x0, #~CTRL_A_BIT  // Clear A (alignment check) bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
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
   EL1_OR_EL2(x1)
1: mrs     x0, sctlr_el1        // Get control register EL1
   b       4f
2: mrs     x0, sctlr_el2        // Get control register EL2
4: bic     x0, x0, #CTRL_SA_BIT // Clear SA (stack alignment check) bit
   EL1_OR_EL2(x1)
1: msr     sctlr_el1, x0        // Write back control register
   b       4f
2: msr     sctlr_el2, x0        // Write back control register
4: dsb     sy
   isb
   ret


// Always turned on in AArch64. Else implementation specific. Leave in for C compatibility for now
ASM_FUNC(ArmEnableBranchPrediction)
  ret


// Always turned on in AArch64. Else implementation specific. Leave in for C compatibility for now.
ASM_FUNC(ArmDisableBranchPrediction)
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
   EL1_OR_EL2(x1)
1: msr   vbar_el1, x0            // Set the Address of the EL1 Vector Table in the VBAR register
   b     4f
2: msr   vbar_el2, x0            // Set the Address of the EL2 Vector Table in the VBAR register
4: isb
   ret

ASM_FUNC(ArmReadVBar)
   EL1_OR_EL2(x1)
1: mrs   x0, vbar_el1            // Set the Address of the EL1 Vector Table in the VBAR register
   ret
2: mrs   x0, vbar_el2            // Set the Address of the EL2 Vector Table in the VBAR register
   ret

ASM_FUNC(ArmEnableVFP)
  // FP support is mandated by the UEFI specification, no need to check ID registers
  // Make sure VFP exceptions are not trapped (to any exception level).
  mrs   x0, cpacr_el1           // Read EL1 Coprocessor Access Control Register (CPACR)
  orr   x0, x0, #CPACR_VFP_BITS // Disable FVP traps to EL1
  msr   cpacr_el1, x0           // Write back EL1 Coprocessor Access Control Register (CPACR)
  mov   x1, #AARCH64_CPTR_TFP   // TFP Bit for trapping VFP Exceptions
  EL1_OR_EL2(x2)
1:ret                           // Not configurable in EL1
2:mrs   x0, cptr_el2            // Disable VFP traps to EL2
  bic   x0, x0, x1
  msr   cptr_el2, x0
  ret


ASM_FUNC(ArmCallWFI)
  wfi
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


ASM_FUNC(ArmReadIdAA64Dfr0)
  mrs   x0, ID_AA64DFR0_EL1
  ret


ASM_FUNC(ArmReadIdAA64Dfr1)
  mrs   x0, ID_AA64DFR1_EL1
  ret


ASM_FUNC(ArmReadIdAA64Isar0)
  mrs   x0, ID_AA64ISAR0_EL1
  ret


ASM_FUNC(ArmReadIdAA64Isar1)
  mrs   x0, ID_AA64ISAR1_EL1
  ret


ASM_FUNC(ArmReadIdAA64Isar2)
  mrs   x0, ID_AA64ISAR2_EL1
  ret


ASM_FUNC(ArmReadIdAA64Mmfr0)
  mrs   x0, ID_AA64MMFR0_EL1
  ret


ASM_FUNC(ArmReadIdAA64Mmfr1)
  mrs   x0, ID_AA64MMFR1_EL1
  ret


ASM_FUNC(ArmReadIdAA64Mmfr2)
  mrs   x0, ID_AA64MMFR2_EL1
  ret


ASM_FUNC(ArmReadIdAA64Pfr0)
  mrs   x0, ID_AA64PFR0_EL1
  ret


ASM_FUNC(ArmReadIdAA64Pfr1)
  mrs   x0, ID_AA64PFR1_EL1
  ret

ASM_FUNC(ArmReadIdAA64Pfr2)
  mrs   x0, ID_AA64PFR2_EL1
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
