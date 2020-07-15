//
//  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroIoLibV8.h>

ASM_FUNC(_ModuleEntryPoint)
  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

// NOTE: We could be booting from EL3, EL2 or EL1. Need to correctly detect
//       and configure the system accordingly. EL2 is default if possible.
// If we started in EL3 we need to switch and run at EL2.
// If we are running at EL2 stay in EL2
// If we are starting at EL1 stay in EL1.

// If started at EL3 Sec is run and switches to EL2 before jumping to PEI.
// If started at EL1 or EL2 Sec jumps directly to PEI without making any
// changes.

// Which EL are we running at? Every EL needs some level of setup...
// We should not run this code in EL3
  EL1_OR_EL2(x0)
1:bl    ASM_PFX(SetupExceptionLevel1)
  b     ASM_PFX(MainEntryPoint)
2:bl    ASM_PFX(SetupExceptionLevel2)
  b     ASM_PFX(MainEntryPoint)

ASM_PFX(MainEntryPoint):
  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x5, x0

  // Is it the Primary Core ?
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)

  // Get the top of the primary stacks (and the base of the secondary stacks)
  MOV64 (x1, FixedPcdGet64(PcdCPUCoresStackBase) + FixedPcdGet32(PcdCPUCorePrimaryStackSize))

  // x0 is equal to 1 if I am the primary core
  cmp   x0, #1
  b.eq   _SetupPrimaryCoreStack

_SetupSecondaryCoreStack:
  // x1 contains the base of the secondary stacks

  // Get the Core Position
  mov   x6, x1      // Save base of the secondary stacks
  mov   x0, x5
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   x0, x0, #1

  // StackOffset = CorePos * StackSize
  MOV32 (x2, FixedPcdGet32(PcdCPUCoreSecondaryStackSize))
  mul   x0, x0, x2
  // SP = StackBase + StackOffset
  add   sp, x6, x0

_PrepareArguments:
  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  MOV64 (x2, FixedPcdGet64(PcdFvBaseAddress))
  ldr   x1, [x2, #8]

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   x3, =ASM_PFX(CEntryPoint)

  // Set the frame pointer to NULL so any backtraces terminate here
  mov   x29, xzr

  // Jump to PrePeiCore C code
  //    x0 = mp_id
  //    x1 = pei_core_address
  mov   x0, x5
  blr   x3

_SetupPrimaryCoreStack:
  mov   sp, x1
  MOV64 (x8, FixedPcdGet64 (PcdCPUCoresStackBase))
  MOV64 (x9, FixedPcdGet32 (PcdInitValueInTempStack) |\
             FixedPcdGet32 (PcdInitValueInTempStack) << 32)
0:stp   x9, x9, [x8], #16
  cmp   x8, x1
  b.lt  0b
  b     _PrepareArguments
