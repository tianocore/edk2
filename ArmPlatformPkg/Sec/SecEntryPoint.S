//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
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
GCC_ASM_IMPORT(ArmPlatformSecBootAction)
GCC_ASM_IMPORT(ArmPlatformInitializeBootMemory)
GCC_ASM_IMPORT(ArmDisableInterrupts)
GCC_ASM_IMPORT(ArmDisableCachesAndMmu)
GCC_ASM_IMPORT(ArmWriteVBar)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(SecVectorTable)
GCC_ASM_IMPORT(ArmCpuSynchronizeWait)
GCC_ASM_EXPORT(_ModuleEntryPoint)

StartupAddr:        .word       ASM_PFX(CEntryPoint)

ASM_PFX(_ModuleEntryPoint):
  // First ensure all interrupts are disabled
  bl    ASM_PFX(ArmDisableInterrupts)

  // Ensure that the MMU and caches are off
  bl    ASM_PFX(ArmDisableCachesAndMmu)

  // Jump to Platform Specific Boot Action function
  blx   ASM_PFX(ArmPlatformSecBootAction)

  // Set VBAR to the start of the exception vectors in Secure Mode
  LoadConstantToReg (ASM_PFX(SecVectorTable), r0)
  bl    ASM_PFX(ArmWriteVBar)

_IdentifyCpu:
  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Get ID of this CPU in Multicore system
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCoreMask), r1)
  and   r5, r0, r1
  
  // Is it the Primary Core ?
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCore), r1)
  cmp   r5, r1
  // Only the primary core initialize the memory (SMC)
  beq   _InitMem
  
_WaitInitMem:
  mov   r0, #ARM_CPU_EVENT_BOOT_MEM_INIT
  bl    ASM_PFX(ArmCpuSynchronizeWait)
  // Now the Init Mem is initialized, we setup the secondary core stacks
  b     _SetupSecondaryCoreStack
  
_InitMem:
  // Initialize Init Boot Memory
  bl    ASM_PFX(ArmPlatformInitializeBootMemory)
  
  // Only Primary CPU could run this line (the secondary cores have jumped from _IdentifyCpu to _SetupStack)
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCore), r5)

_SetupPrimaryCoreStack:
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), r2)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecPrimaryStackSize), r3)
  // Calculate the Top of the Stack
  add   r2, r2, r3
  LoadConstantToReg (FixedPcdGet32(PcdSecGlobalVariableSize), r3)

  // The reserved space for global variable must be 8-bytes aligned for pushing
  // 64-bit variable on the stack
  SetPrimaryStack (r2, r3, r1)

  // Set all the SEC global variables to 0
  mov     r3, sp
  mov     r1, #0x0
_InitGlobals:
  str     r1, [r3], #4
  cmp     r3, r2
  bne     _InitGlobals

  b     _PrepareArguments

_SetupSecondaryCoreStack:
  // Get the Core Position (ClusterId * 4) + CoreId
  GetCorePositionInStack(r0, r5, r1)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   r0, r0, #1

  // Get the base of the stack for the secondary cores
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresSecStackBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecSecondaryStackSize), r2)
  add   r1, r1, r2

  // StackOffset = CorePos * StackSize
  mul   r0, r0, r2
  // SP = StackBase + StackOffset
  add   sp, r1, r0


_PrepareArguments:
  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, StartupAddr
  
  // Jump to SEC C code
  //    r0 = mp_id
  mov   r0, r5
  blx   r3
  
_NeverReturn:
  b _NeverReturn
