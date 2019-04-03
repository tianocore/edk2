//
//  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroIoLibV8.h>

ASM_FUNC(_ModuleEntryPoint)
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
_SystemMemoryEndInit:
  ldr   x1, mSystemMemoryEnd

_SetupStackPosition:
  // r1 = SystemMemoryTop

  // Calculate Top of the Firmware Device
  MOV64 (x2, FixedPcdGet64(PcdFdBaseAddress))
  MOV32 (x3, FixedPcdGet32(PcdFdSize) - 1)
  sub   x3, x3, #1
  add   x3, x3, x2      // x3 = FdTop = PcdFdBaseAddress + PcdFdSize

  // UEFI Memory Size (stacks are allocated in this region)
  MOV32 (x4, FixedPcdGet32(PcdSystemMemoryUefiRegionSize))

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
  and   x1, x1, ~EFI_PAGE_MASK

_GetBaseUefiMemory:
  // Calculate the Base of the UEFI Memory
  sub   x11, x1, x4

_GetStackBase:
  // r1 = The top of the Mpcore Stacks
  // Stack for the primary core = PrimaryCoreStack
  MOV32 (x2, FixedPcdGet32(PcdCPUCorePrimaryStackSize))
  sub   x12, x1, x2

  // Stack for the secondary core = Number of Cores - 1
  MOV32 (x1, (FixedPcdGet32(PcdCoreCount) - 1) * FixedPcdGet32(PcdCPUCoreSecondaryStackSize))
  sub   x12, x12, x1

  // x12 = The base of the MpCore Stacks (primary stack & secondary stacks)
  mov   x0, x12
  mov   x1, x10
  //ArmPlatformStackSet(StackBase, MpId, PrimaryStackSize, SecondaryStackSize)
  MOV32 (x2, FixedPcdGet32(PcdCPUCorePrimaryStackSize))
  MOV32 (x3, FixedPcdGet32(PcdCPUCoreSecondaryStackSize))
  bl    ASM_PFX(ArmPlatformStackSet)

  // Is it the Primary Core ?
  mov   x0, x10
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   x0, #1
  bne   _PrepareArguments

_PrepareArguments:
  mov   x0, x10
  mov   x1, x11
  mov   x2, x12

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   x4, =ASM_PFX(CEntryPoint)

  // Set the frame pointer to NULL so any backtraces terminate here
  mov   x29, xzr

  // Jump to PrePiCore C code
  //    x0 = MpId
  //    x1 = UefiMemoryBase
  //    x2 = StacksBase
  blr   x4

_NeverReturn:
  b _NeverReturn
