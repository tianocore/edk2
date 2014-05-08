//
//  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

#include <AsmMacroIoLibV8.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

.text
.align 3

GCC_ASM_IMPORT(CEntryPoint)
GCC_ASM_IMPORT(ArmPlatformGetCorePosition)
GCC_ASM_IMPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(ArmPlatformPeiBootAction)
GCC_ASM_EXPORT(_ModuleEntryPoint)

StartupAddr: .8byte CEntryPoint

ASM_PFX(_ModuleEntryPoint):
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
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresStackBase), x1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), x2)
  add   x1, x1, x2

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
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), x2)
  mul   x0, x0, x2
  // SP = StackBase + StackOffset
  add   sp, x6, x0

_PrepareArguments:
  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  LoadConstantToReg (FixedPcdGet64(PcdFvBaseAddress), x2)
  add   x2, x2, #8
  ldr   x1, [x2]

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   x3, StartupAddr

  // Jump to PrePeiCore C code
  //    x0 = mp_id
  //    x1 = pei_core_address
  mov   x0, x5
  blr   x3

_SetupPrimaryCoreStack:
  // x1 contains the top of the primary stack
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), x2)

  // The reserved space for global variable must be 16-bytes aligned for pushing
  // 128-bit variable on the stack
  SetPrimaryStack (x1, x2, x3, x4)
  b     _PrepareArguments
