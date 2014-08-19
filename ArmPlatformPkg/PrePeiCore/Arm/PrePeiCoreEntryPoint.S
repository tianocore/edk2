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

#include <AsmMacroIoLib.h>
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

StartupAddr: .word    CEntryPoint

ASM_PFX(_ModuleEntryPoint):
  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   r5, r0

  // Is it the Primary Core ?
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)

  // Get the top of the primary stacks (and the base of the secondary stacks)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresStackBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), r2)
  add   r1, r1, r2

  // r0 is equal to 1 if I am the primary core
  cmp   r0, #1
  beq   _SetupPrimaryCoreStack

_SetupSecondaryCoreStack:
  // r1 contains the base of the secondary stacks

  // Get the Core Position
  mov   r6, r1      // Save base of the secondary stacks
  mov   r0, r5
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   r0, r0, #1

  // StackOffset = CorePos * StackSize
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), r2)
  mul   r0, r0, r2
  // SP = StackBase + StackOffset
  add   sp, r6, r0

_PrepareArguments:
  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  LoadConstantToReg (FixedPcdGet32(PcdFvBaseAddress), r2)
  add   r2, r2, #4
  ldr   r1, [r2]

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, StartupAddr

  // Jump to PrePeiCore C code
  //    r0 = mp_id
  //    r1 = pei_core_address
  mov   r0, r5
  blx   r3

_SetupPrimaryCoreStack:
  // r1 contains the top of the primary stack
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), r2)

  // The reserved space for global variable must be 8-bytes aligned for pushing
  // 64-bit variable on the stack
  SetPrimaryStack (r1, r2, r3)
  b     _PrepareArguments

_NeverReturn:
  b _NeverReturn
