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

#include <AutoGen.h>
#include <AsmMacroIoLibV8.h>
#include "SecInternal.h"

.text
.align 3

GCC_ASM_IMPORT(CEntryPoint)
GCC_ASM_IMPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_IMPORT(ArmPlatformGetCorePosition)
GCC_ASM_IMPORT(ArmPlatformSecBootAction)
GCC_ASM_IMPORT(ArmPlatformSecBootMemoryInit)
GCC_ASM_IMPORT(ArmDisableInterrupts)
GCC_ASM_IMPORT(ArmDisableCachesAndMmu)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(ArmCallWFE)
GCC_ASM_EXPORT(_ModuleEntryPoint)

StartupAddr:        .8byte       ASM_PFX(CEntryPoint)

ASM_PFX(_ModuleEntryPoint):

// NOTE: We could be booting from EL3, EL2 or EL1. Need to correctly detect
//       and configure the system accordingly. EL2 is default if possible.
// If we started in EL3 we need to switch and run at EL2.
// If we are running at EL2 stay in EL2
// If we are starting at EL1 stay in EL1.

// Sec only runs in EL3. Othewise we jump to PEI without changing anything.
// If Sec runs we change to EL2 before switching to PEI.

// Which EL are we running at? Every EL needs some level of setup...
  EL1_OR_EL2_OR_EL3(x0)
1:// If we are at EL1 or EL2 leave SEC for PEI.
2:b     ASM_PFX(JumpToPEI)
  // If we are at EL3 we need to configure it and switch to EL2
3:b     ASM_PFX(MainEntryPoint)

ASM_PFX(MainEntryPoint):
  // First ensure all interrupts are disabled
  bl    ASM_PFX(ArmDisableInterrupts)

  // Ensure that the MMU and caches are off
  bl    ASM_PFX(ArmDisableCachesAndMmu)

  // By default, we are doing a cold boot
  mov   x10, #ARM_SEC_COLD_BOOT

  // Jump to Platform Specific Boot Action function
  bl    ASM_PFX(ArmPlatformSecBootAction)

_IdentifyCpu:
  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x5, x0

  // Is it the Primary Core ?
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   x0, #1
  // Only the primary core initialize the memory (SMC)
  b.eq  _InitMem

_WaitInitMem:
  // If we are not doing a cold boot in this case we should assume the Initial Memory to be already initialized
  // Otherwise we have to wait the Primary Core to finish the initialization
  cmp   x10, #ARM_SEC_COLD_BOOT
  b.ne  _SetupSecondaryCoreStack

  // Wait for the primary core to initialize the initial memory (event: BOOT_MEM_INIT)
  bl    ASM_PFX(ArmCallWFE)
  // Now the Init Mem is initialized, we setup the secondary core stacks
  b     _SetupSecondaryCoreStack

_InitMem:
  // If we are not doing a cold boot in this case we should assume the Initial Memory to be already initialized
  cmp   x10, #ARM_SEC_COLD_BOOT
  b.ne  _SetupPrimaryCoreStack

  // Initialize Init Boot Memory
  bl    ASM_PFX(ArmPlatformSecBootMemoryInit)

_SetupPrimaryCoreStack:
  // Get the top of the primary stacks (and the base of the secondary stacks)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), x1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecPrimaryStackSize), x2)
  add   x1, x1, x2

  LoadConstantToReg (FixedPcdGet32(PcdSecGlobalVariableSize), x2)

  // The reserved space for global variable must be 8-bytes aligned for pushing
  // 64-bit variable on the stack
  SetPrimaryStack (x1, x2, x3, x4)
  b     _PrepareArguments

_SetupSecondaryCoreStack:
  // Get the top of the primary stacks (and the base of the secondary stacks)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), x1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecPrimaryStackSize), x2)
  add   x6, x1, x2

  // Get the Core Position
  mov   x0, x5
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   x0, x0, #1

  // StackOffset = CorePos * StackSize
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecSecondaryStackSize), x2)
  mul   x0, x0, x2
  // SP = StackBase + StackOffset
  add   sp, x6, x0

_PrepareArguments:
  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   x3, StartupAddr

  // Jump to SEC C code
  //    r0 = mp_id
  //    r1 = Boot Mode
  mov   x0, x5
  mov   x1, x10
  blr   x3

  ret

ASM_PFX(JumpToPEI):
  LoadConstantToReg (FixedPcdGet32(PcdFvBaseAddress), x0)
  blr   x0
