//
//  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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
#include <AsmMacroIoLib.h>
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

StartupAddr:        .word       ASM_PFX(CEntryPoint)

ASM_PFX(_ModuleEntryPoint):
  // First ensure all interrupts are disabled
  bl    ASM_PFX(ArmDisableInterrupts)

  // Ensure that the MMU and caches are off
  bl    ASM_PFX(ArmDisableCachesAndMmu)

  // By default, we are doing a cold boot
  mov   r10, #ARM_SEC_COLD_BOOT

  // Jump to Platform Specific Boot Action function
  blx   ASM_PFX(ArmPlatformSecBootAction)

_IdentifyCpu:
  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   r9, r0

  // Is it the Primary Core ?
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   r0, #1
  // Only the primary core initialize the memory (SMC)
  beq   _InitMem

_WaitInitMem:
  // If we are not doing a cold boot in this case we should assume the Initial Memory to be already initialized
  // Otherwise we have to wait the Primary Core to finish the initialization
  cmp   r10, #ARM_SEC_COLD_BOOT
  bne   _SetupSecondaryCoreStack

  // Wait for the primary core to initialize the initial memory (event: BOOT_MEM_INIT)
  bl    ASM_PFX(ArmCallWFE)
  // Now the Init Mem is initialized, we setup the secondary core stacks
  b     _SetupSecondaryCoreStack

_InitMem:
  // If we are not doing a cold boot in this case we should assume the Initial Memory to be already initialized
  cmp   r10, #ARM_SEC_COLD_BOOT
  bne   _SetupPrimaryCoreStack

  // Initialize Init Boot Memory
  bl    ASM_PFX(ArmPlatformSecBootMemoryInit)

_SetupPrimaryCoreStack:
  // Get the top of the primary stacks (and the base of the secondary stacks)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecPrimaryStackSize), r2)
  add   r1, r1, r2

  LoadConstantToReg (FixedPcdGet32(PcdSecGlobalVariableSize), r2)

  // The reserved space for global variable must be 8-bytes aligned for pushing
  // 64-bit variable on the stack
  SetPrimaryStack (r1, r2, r3)
  b     _PrepareArguments

_SetupSecondaryCoreStack:
  // Get the top of the primary stacks (and the base of the secondary stacks)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecPrimaryStackSize), r2)
  add   r6, r1, r2

  // Get the Core Position
  mov   r0, r9
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   r0, r0, #1

  // StackOffset = CorePos * StackSize
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecSecondaryStackSize), r2)
  mul   r0, r0, r2
  // SP = StackBase + StackOffset
  add   sp, r6, r0

_PrepareArguments:
  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, StartupAddr

  // Jump to SEC C code
  //    r0 = mp_id
  //    r1 = Boot Mode
  mov   r0, r9
  mov   r1, r10
  blx   r3

_NeverReturn:
  b _NeverReturn
