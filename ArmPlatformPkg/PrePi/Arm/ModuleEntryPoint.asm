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

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

#include <Chipset/ArmV7.h>

  INCLUDE AsmMacroIoLib.inc

  IMPORT  CEntryPoint
  IMPORT  ArmPlatformIsPrimaryCore
  IMPORT  ArmReadMpidr
  IMPORT  ArmPlatformPeiBootAction
  IMPORT  ArmPlatformStackSet

  EXPORT  _ModuleEntryPoint

  PRESERVE8
  AREA    PrePiCoreEntryPoint, CODE, READONLY

StartupAddr        DCD      CEntryPoint

_ModuleEntryPoint
  // Do early platform specific actions
  bl    ArmPlatformPeiBootAction

  // Get ID of this CPU in Multicore system
  bl    ArmReadMpidr
  // Keep a copy of the MpId register value
  mov   r8, r0

_SetSVCMode
  // Enter SVC mode, Disable FIQ and IRQ
  mov     r1, #(CPSR_MODE_SVC :OR: CPSR_IRQ :OR: CPSR_FIQ)
  msr     CPSR_c, r1

// Check if we can install the stack at the top of the System Memory or if we need
// to install the stacks at the bottom of the Firmware Device (case the FD is located
// at the top of the DRAM)
_SetupStackPosition
  // Compute Top of System Memory
  LoadConstantToReg (FixedPcdGet64 (PcdSystemMemoryBase), r1)
  LoadConstantToReg (FixedPcdGet64 (PcdSystemMemorySize), r2)
  sub   r2, r2, #1
  add   r1, r1, r2      // r1 = SystemMemoryTop = PcdSystemMemoryBase + PcdSystemMemorySize

  // Calculate Top of the Firmware Device
  LoadConstantToReg (FixedPcdGet32(PcdFdBaseAddress), r2)
  LoadConstantToReg (FixedPcdGet32(PcdFdSize), r3)
  sub   r3, r3, #1
  add   r3, r3, r2      // r3 = FdTop = PcdFdBaseAddress + PcdFdSize

  // UEFI Memory Size (stacks are allocated in this region)
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryUefiRegionSize), r4)

  //
  // Reserve the memory for the UEFI region (contain stacks on its top)
  //

  // Calculate how much space there is between the top of the Firmware and the Top of the System Memory
  subs  r0, r1, r3      // r0 = SystemMemoryTop - FdTop
  bmi   _SetupStack     // Jump if negative (FdTop > SystemMemoryTop). Case when the PrePi is in XIP memory outside of the DRAM
  cmp   r0, r4
  bge   _SetupStack

  // Case the top of stacks is the FdBaseAddress
  mov   r1, r2

_SetupStack
  // r1 contains the top of the stack (and the UEFI Memory)

  // Because the 'push' instruction is equivalent to 'stmdb' (decrement before), we need to increment
  // one to the top of the stack. We check if incrementing one does not overflow (case of DRAM at the
  // top of the memory space)
  adds  r9, r1, #1
  bcs   _SetupOverflowStack

_SetupAlignedStack
  mov   r1, r9
  b     _GetBaseUefiMemory

_SetupOverflowStack
  // Case memory at the top of the address space. Ensure the top of the stack is EFI_PAGE_SIZE
  // aligned (4KB)
  LoadConstantToReg (EFI_PAGE_MASK, r9)
  and   r9, r9, r1
  sub   r1, r1, r9

_GetBaseUefiMemory
  // Calculate the Base of the UEFI Memory
  sub   r9, r1, r4

_GetStackBase
  // r1 = The top of the Mpcore Stacks
  // Stack for the primary core = PrimaryCoreStack
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), r2)
  sub   r10, r1, r2

  // Stack for the secondary core = Number of Cores - 1
  LoadConstantToReg (FixedPcdGet32(PcdCoreCount), r0)
  sub   r0, r0, #1
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), r1)
  mul   r1, r1, r0
  sub   r10, r10, r1

  // r10 = The base of the MpCore Stacks (primary stack & secondary stacks)
  mov   r0, r10
  mov   r1, r8
  //ArmPlatformStackSet(StackBase, MpId, PrimaryStackSize, SecondaryStackSize)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), r2)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), r3)
  bl    ArmPlatformStackSet

  // Is it the Primary Core ?
  mov   r0, r8
  bl    ArmPlatformIsPrimaryCore
  cmp   r0, #1
  bne   _PrepareArguments

_ReserveGlobalVariable
  LoadConstantToReg (FixedPcdGet32(PcdPeiGlobalVariableSize), r0)
  // InitializePrimaryStack($GlobalVariableSize, $Tmp1)
  InitializePrimaryStack r0, r1

_PrepareArguments
  mov   r0, r8
  mov   r1, r9
  mov   r2, r10
  mov   r3, sp

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r4, StartupAddr

  // Jump to PrePiCore C code
  //    r0 = MpId
  //    r1 = UefiMemoryBase
  //    r2 = StacksBase
  //    r3 = GlobalVariableBase
  blx   r4

_NeverReturn
  b _NeverReturn

  END
