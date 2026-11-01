#===============================================================================
#  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#===============================================================================

.text
.align 3

#include <AsmMacroLib.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <Library/ArmLib.h>

#include "MpServicesInternal.h"

GCC_ASM_IMPORT (gProcessorIDs)
GCC_ASM_IMPORT (ApProcedure)
GCC_ASM_IMPORT (gTcr)
GCC_ASM_IMPORT (gTtbr0)
GCC_ASM_IMPORT (gMair)
GCC_ASM_IMPORT (gHcr)
GCC_ASM_IMPORT (ArmWriteHcr)
GCC_ASM_IMPORT (ArmEnableVFP)

GCC_ASM_EXPORT (ApEntryPoint)

// Entry-point for the AP
// VOID
// ApEntryPoint (
//   VOID
//   );
ASM_PFX(ApEntryPoint):
  // x0 contains the context_id passed to PSCI CPU_ON by DispatchCpu, which is
  // the top of this AP's stack. Keep it in a callee-saved register.
  mov x19, x0

  // Restore HCR_EL2 when running at EL2
  EL1_OR_EL2(x1)
1:
  b     3f
2:
  LDR_LIT(x0, gHcr)
  bl ArmWriteHcr

3:
  // Configure the MMU and caches
  LDR_LIT(x0, gTcr)
  bl ArmSetTCR
  LDR_LIT(x0, gTtbr0)
  bl ArmSetTTBR0
  LDR_LIT(x0, gMair)
  bl ArmSetMAIR
  bl ArmDisableAlignmentCheck
  bl ArmEnableStackAlignmentCheck
  bl ArmEnableInstructionCache
  bl ArmEnableDataCache
  bl ArmEnableMmu

  mrs x0, mpidr_el1
  // Mask the non-affinity bits
  bic x0, x0, 0x00ff000000
  and x0, x0, 0xffffffffff
  LDR_LIT(x1, gProcessorIDs)
  mov x2, 0                   // x2 = processor index

// Find index in gProcessorIDs for current processor
1:
  ldr x3, [x1, x2, lsl #3]    // x4 = gProcessorIDs + x2 * 8
  cmp x3, #-1                 // check if we've reached the end of gProcessorIDs
  beq ProcessorNotFound
  add x2, x2, 1               // x2++
  cmp x0, x3                  // if mpidr_el1 != gProcessorIDs[x] then loop
  bne 1b

  mov sp, x19                 // stack passed by DispatchCpu
  mov x29, xzr
  bl ArmEnableVFP             // Do not trap SIMD instructions
  bl ApProcedure              // doesn't return

ProcessorNotFound:
// Turn off the processor
  MOV32 (w0, ARM_SMC_ID_PSCI_CPU_OFF)
  smc #0
  b .
