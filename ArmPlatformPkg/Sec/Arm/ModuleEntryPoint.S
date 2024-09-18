//
//  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

ASM_FUNC(_ModuleEntryPoint)
  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

  // Get the top of the primary stacks (and the base of the secondary stacks)
  MOV32 (r1, FixedPcdGet64(PcdCPUCoresStackBase) + FixedPcdGet32(PcdCPUCorePrimaryStackSize))

  // Set up the stack pointer
  mov   sp, r1

  // Apply the init value to the entire stack
  MOV32 (r8, FixedPcdGet64 (PcdCPUCoresStackBase))
  MOV32 (r9, FixedPcdGet32 (PcdInitValueInTempStack))
  mov   r10, r9
  mov   r11, r9
  mov   r12, r9
0:stm   r8!, {r9-r12}
  cmp   r8, r1
  blt   0b

  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  MOV32 (r2, FixedPcdGet32(PcdFvBaseAddress))
  ldr   r0, [r2, #4]

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, =ASM_PFX(CEntryPoint)

  // Jump to PrePeiCore C code
  //    r0 = pei_core_address
  blx   r3
