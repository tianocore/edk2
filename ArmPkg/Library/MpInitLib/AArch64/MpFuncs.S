#===============================================================================
#  Copyright (c) 2021 NUVIA Inc. All rights reserved.
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#===============================================================================

.text
.align 3

#include <AsmMacroIoLibV8.h>
#include <IndustryStandard/ArmStdSmc.h>

#include "InternalMpInitLib.h"

GCC_ASM_IMPORT (gApStacksBase)
GCC_ASM_IMPORT (gProcessorIDs)
GCC_ASM_IMPORT (ApProcedure)
GCC_ASM_IMPORT (gApStackSize)

GCC_ASM_EXPORT (ApEntryPoint)

StartupAddr:        .8byte ASM_PFX(ApProcedure)

// Entry-point for the AP
// VOID
// ApEntryPoint (
//   VOID
//   );
ASM_PFX(ApEntryPoint):
  mrs x0, mpidr_el1
  // Mask the non-affinity bits
  ldr x1, =0xff00ffffff
  and x0, x0, x1
  ldr x1, gProcessorIDs
  mov x2, 0                   // x2 = processor index
  mov x3, 0                   // x3 = address offset

// Find index in gProcessorIDs for current processor
1:
  ldr x4, [x1, x3]            // x4 = gProcessorIDs + x3
  ldr x5, =0xffffffffff
  cmp x4, x5                  // check if we've reached the end of gProcessorIDs
  beq ProcessorNotFound
  add x3, x3, 8               // x3 += sizeof (*gProcessorIDs)
  add x2, x2, 1               // x2++
  cmp x0, x4                  // if mpidr_el1 != *(gProcessorIDs + x3) then loop
  bne 1b
  sub x2, x2, 1

// Calculate stack address
  // x2 contains the index for the current processor
  ldr x0, gApStacksBase
  ldr x1, gApStackSize
  mul x3, x2, x1              // x3 = ProcessorIndex * gApStackSize
  add x4, x0, x3              // x4 = gApStacksBase + x3
  add sp, x4, x1              // sp = x4 + gApStackSize

  ldr x0, StartupAddr         // ASM_PFX(ApProcedure)
  blr x0                      // doesn't return

ProcessorNotFound:
// Turn off the processor
  MOV32 (w0, ARM_SMC_ID_PSCI_CPU_OFF)
  smc #0
  b .
