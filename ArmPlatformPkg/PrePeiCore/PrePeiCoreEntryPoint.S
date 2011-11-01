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

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

.text
.align 3

GCC_ASM_IMPORT(CEntryPoint)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_EXPORT(_ModuleEntryPoint)

StartupAddr: .word    CEntryPoint

ASM_PFX(_ModuleEntryPoint):
  // Identify CPU ID
  bl    ASM_PFX(ArmReadMpidr)
  // Get ID of this CPU in Multicore system
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCoreMask), r1)
  and   r0, r0, r1

  // Calculate the top of the primary stack
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresStackBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), r2)
  add   r2, r2, r1

  // Is it the Primary Core ?
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCore), r1)
  cmp   r0, r1
  beq   _SetupPrimaryCoreStack

_SetupSecondaryCoreStack:
  // r2 = Top of the primary stack = Base of the Secondary Stacks

  // Get the position of the cores (ClusterId * 4) + CoreId
  GetCorePositionInStack(r3, r0, r1)
  // The stack starts at the top of the stack region. Add '1' to the Core Position to get the top of the stack
  add   r3, r3, #1

  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), r1)
  // StackOffset = CorePos * StackSize
  mul   r3, r3, r1
  // SP = StackBase + StackOffset
  add   sp, r2, r3

  b     _PrepareArguments

_SetupPrimaryCoreStack:
  // r2 = Top of the primary stack
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), r3)

  // The reserved space for global variable must be 8-bytes aligned for pushing
  // 64-bit variable on the stack
  SetPrimaryStack (r2, r3, r1)

  // Set all the PEI global variables to 0
  mov   r3, sp
  mov   r1, #0x0
_InitGlobals:
  str   r1, [r3], #4
  cmp   r3, r2
  bne   _InitGlobals

_PrepareArguments:
  // The PEI Core Entry Point has been computed by GenFV and stored in the second entry of the Reset Vector
  LoadConstantToReg (FixedPcdGet32(PcdFvBaseAddress), r2)
  add   r2, r2, #4
  ldr   r1, [r2]

  // move sec startup address into a data register
  // ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r2, StartupAddr

  // jump to PrePeiCore C code
  //    r0 = mp_id
  //    r1 = pei_core_address
  blx   r2

_NeverReturn:
  b _NeverReturn
