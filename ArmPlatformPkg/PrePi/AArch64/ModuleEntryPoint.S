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

#include <AsmMacroIoLibV8.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

.text
.align 3

GCC_ASM_IMPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(ArmPlatformPeiBootAction)
GCC_ASM_IMPORT(ArmPlatformStackSet)
GCC_ASM_EXPORT(_ModuleEntryPoint)

StartupAddr:        .8byte ASM_PFX(CEntryPoint)

ASM_PFX(_ModuleEntryPoint):
  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x10, x0

_SetSVCMode:
// Check if we can install the stack at the top of the System Memory or if we need
// to install the stacks at the bottom of the Firmware Device (case the FD is located
// at the top of the DRAM)
_SetupStackPosition:
  // Compute Top of System Memory
  LoadConstantToReg (FixedPcdGet64 (PcdSystemMemoryBase), x1)
  LoadConstantToReg (FixedPcdGet64 (PcdSystemMemorySize), x2)
  sub   x2, x2, #1
  add   x1, x1, x2      // x1 = SystemMemoryTop = PcdSystemMemoryBase + PcdSystemMemorySize

  // Calculate Top of the Firmware Device
  LoadConstantToReg (FixedPcdGet32(PcdFdBaseAddress), x2)
  LoadConstantToReg (FixedPcdGet32(PcdFdSize), x3)
  sub   x3, x3, #1
  add   x3, x3, x2      // x3 = FdTop = PcdFdBaseAddress + PcdFdSize

  // UEFI Memory Size (stacks are allocated in this region)
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryUefiRegionSize), x4)

  //
  // Reserve the memory for the UEFI region (contain stacks on its top)
  //

  // Calculate how much space there is between the top of the Firmware and the Top of the System Memory
  subs  x0, x1, x3   // x0 = SystemMemoryTop - FdTop
  b.mi  _SetupStack  // Jump if negative (FdTop > SystemMemoryTop). Case when the PrePi is in XIP memory outside of the DRAM
  cmp   x0, x4
  b.ge  _SetupStack

  // Case the top of stacks is the FdBaseAddress
  mov   x1, x2

_SetupStack:
  // x1 contains the top of the stack (and the UEFI Memory)

  // Because the 'push' instruction is equivalent to 'stmdb' (decrement before), we need to increment
  // one to the top of the stack. We check if incrementing one does not overflow (case of DRAM at the
  // top of the memory space)
  adds  x11, x1, #1
  b.cs  _SetupOverflowStack

_SetupAlignedStack:
  mov   x1, x11
  b     _GetBaseUefiMemory

_SetupOverflowStack:
  // Case memory at the top of the address space. Ensure the top of the stack is EFI_PAGE_SIZE
  // aligned (4KB)
  LoadConstantToReg (EFI_PAGE_MASK, x11)
  and   x11, x11, x1
  sub   x1, x1, x11

_GetBaseUefiMemory:
  // Calculate the Base of the UEFI Memory
  sub   x11, x1, x4

_GetStackBase:
  // r1 = The top of the Mpcore Stacks
  // Stack for the primary core = PrimaryCoreStack
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), x2)
  sub   x12, x1, x2

  // Stack for the secondary core = Number of Cores - 1
  LoadConstantToReg (FixedPcdGet32(PcdCoreCount), x0)
  sub   x0, x0, #1
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), x1)
  mul   x1, x1, x0
  sub   x12, x12, x1

  // x12 = The base of the MpCore Stacks (primary stack & secondary stacks)
  mov   x0, x12
  mov   x1, x10
  //ArmPlatformStackSet(StackBase, MpId, PrimaryStackSize, SecondaryStackSize)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), x2)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), x3)
  bl    ASM_PFX(ArmPlatformStackSet)

  // Is it the Primary Core ?
  mov   x0, x10
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   x0, #1
  bne   _PrepareArguments

_ReserveGlobalVariable:
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), x0)
  // InitializePrimaryStack($GlobalVariableSize, $Tmp1, $Tmp2)
  InitializePrimaryStack(x0, x1, x2)

_PrepareArguments:
  mov   x0, x10
  mov   x1, x11
  mov   x2, x12
  mov   x3, sp

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   x4, StartupAddr

  // Jump to PrePiCore C code
  //    x0 = MpId
  //    x1 = UefiMemoryBase
  //    x2 = StacksBase
  //    x3 = GlobalVariableBase
  blr   x4

_NeverReturn:
  b _NeverReturn
