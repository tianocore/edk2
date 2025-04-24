//
//  Copyright (c) 2011 - 2025, Arm Limited. All rights reserved.<BR>
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
// @Par Reference(s):
// - Firmware Handoff specification [https://firmwarehandoff.github.io/firmware_handoff/main]
//

#include <AsmMacroLib.h>

ASM_FUNC(_ModuleEntryPoint)

  // Check if register assignment at handoff matches spec
  MOV64 (x4, 0x14a0fb10b)
  cmp   x1, x4
  // Skip TransferList init if x1 is not equal to the TransferList signature
  b.ne  _SkipTransferList

  // Skip TransferList init if x2 is not equal to 0
  cbnz  x2, _SkipTransferList

  // Set the TransferList Base Address from register x3
  mov   x6, x3

_SkipTransferList:
  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

  // Enable Floating Point. This needs to be done before entering C code, which
  // may use FP/SIMD registers.
  bl    ArmEnableVFP

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
  b.mi  _SetupStack  // Jump if negative (FdTop > SystemMemoryTop). Case when SEC is in XIP memory outside of the DRAM
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
  sub   x0, x1, x4

_GetStackBase:
  // r1 = The top of the stack
  mov   sp, x1

  // Stack for the primary core = PrimaryCoreStack
  MOV32 (x2, FixedPcdGet32(PcdCPUCorePrimaryStackSize))
  sub   x1, x1, x2

  // Pass Transfer List Base Address
  mov   x2, x6
  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   x4, =ASM_PFX(CEntryPoint)

  // Set the frame pointer to NULL so any backtraces terminate here
  mov   x29, xzr

  // Jump to SEC C code
  //    x0 = UefiMemoryBase
  //    x1 = StacksBase
  blr   x4

_NeverReturn:
  b _NeverReturn
