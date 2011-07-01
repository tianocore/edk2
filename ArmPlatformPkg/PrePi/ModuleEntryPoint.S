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

#start of the code section
.text
.align 3

#global symbols referenced by this module
GCC_ASM_IMPORT(CEntryPoint)

StartupAddr: .word    CEntryPoint

#make _ModuleEntryPoint as global
GCC_ASM_EXPORT(_ModuleEntryPoint)


ASM_PFX(_ModuleEntryPoint):
  // Identify CPU ID
  mrc   p15, 0, r0, c0, c0, 5
  and   r0, #0xf

_UefiMemoryBase:
#if FixedPcdGet32(PcdStandalone)
  // Compute Top of System Memory
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemorySize), r2)
  add   r1, r1, r2      // r1 = SystemMemoryTop = PcdSystemMemoryBase + PcdSystemMemorySize
#else
  // If it is not a Standalone, we must compute the top of the UEFI memory with the base of the FD
  LoadConstantToReg (FixedPcdGet32(PcdNormalFdBaseAddress), r1)
#endif

  // Compute Base of UEFI Memory
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryUefiRegionSize), r2)
  sub   r1, r1, r2      // r1 = SystemMemoryTop - PcdSystemMemoryUefiRegionSize = UefiMemoryBase

_SetupStack:
  // Compute Base of Normal stacks for CPU Cores
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresNonSecStackSize), r2)
  mul   r3, r0, r2      // r3 = core_id * stack_size = offset from the stack base
  sub   sp, r1, r3      // r3 = UefiMemoryBase - StackOffset = TopOfStack

  // Only allocate memory in top of the primary core stack
  cmp   r0, #0
  bne   _PrepareArguments

_AllocateGlobalPeiVariables:
  // Reserve top of the stack for Global PEI Variables (eg: PeiServicesTablePointer)
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), r4)
  // The reserved place must be 8-bytes aligned for pushing 64-bit variable on the stack
  and   r5, r4, #7
  rsb   r5, r5, #8
  add   r4, r4, r5
  sub   sp, sp, r4

_PrepareArguments:
  // Pass the StackBase to the C Entrypoint (UefiMemoryBase - StackSize - StackOffset)
  sub   r2, r1, r2
  sub   r2, r3
  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r3, StartupAddr

  // jump to PrePiCore C code
  //    r0 = core_id
  //    r1 = UefiMemoryBase
  //    r2 = StackBase
  blx   r3

