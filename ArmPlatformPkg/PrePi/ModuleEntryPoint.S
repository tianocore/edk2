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

# Global symbols referenced by this module
GCC_ASM_IMPORT(CEntryPoint)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_EXPORT(_ModuleEntryPoint)

StartupAddr: .word    CEntryPoint


ASM_PFX(_ModuleEntryPoint):
  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  LoadConstantToReg (FixedPcdGet32(PcdArmPrimaryCoreMask), r1)
  and   r0, r0, r1

_SetSVCMode:
  // Enter SVC mode
  mov     r1, #0x13|0x80|0x40
  msr     CPSR_c, r1

// Check if we can install the size at the top of the System Memory or if we need
// to install the stacks at the bottom of the Firmware Device (case the FD is located
// at the top of the DRAM)
_SetupStackPosition:
  // Compute Top of System Memory
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryBase), r1)
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemorySize), r2)
  add   r1, r1, r2      // r1 = SystemMemoryTop = PcdSystemMemoryBase + PcdSystemMemorySize

  // Calculate Top of the Firmware Device
  LoadConstantToReg (FixedPcdGet32(PcdNormalFdBaseAddress), r2)
  LoadConstantToReg (FixedPcdGet32(PcdNormalFdSize), r3)
  add   r3, r3, r2      // r4 = FdTop = PcdNormalFdBaseAddress + PcdNormalFdSize

  // UEFI Memory Size (stacks are allocated in this region)
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryUefiRegionSize), r4)

  //
  // Reserve the memory for the UEFI region (contain stacks on its top)
  //

  // Calculate how much space there is between the top of the Firmware and the Top of the System Memory
  subs	r5, r1, r3		// r5 = SystemMemoryTop - FdTop
  bmi	_SetupStack		// Jump if negative (FdTop > SystemMemoryTop)
  cmp	r5, r4
  bge	_SetupStack

  // Case the top of stacks is the FdBaseAddress
  mov	r1, r2

_SetupStack:
  // Compute Base of Normal stacks for CPU Cores
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoresNonSecStackSize), r5)
  mul   r3, r0, r5      // r3 = core_id * stack_size = offset from the stack base
  sub   sp, r1, r3      // r3 = (SystemMemoryTop|FdBaseAddress) - StackOffset = TopOfStack

  // Calculate the Base of the UEFI Memory
  sub	r1, r1, r4

  // Only allocate memory for global variables at top of the primary core stack
  cmp   r0, #0
  bne   _PrepareArguments

_AllocateGlobalPrePiVariables:
  // Reserve top of the stack for Global PEI Variables (eg: PeiServicesTablePointer)
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), r4)
  // The reserved place must be 8-bytes aligned for pushing 64-bit variable on the stack
  and   r5, r4, #7
  rsb   r5, r5, #8
  add   r4, r4, r5
  sub   sp, sp, r4

_PrepareArguments:
  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r2, StartupAddr

  // Jump to PrePiCore C code
  //    r0 = MpId
  //    r1 = UefiMemoryBase
  blx   r2

