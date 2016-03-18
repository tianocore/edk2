//
//  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
//  Copyright (c) 2015, Linaro Limited. All rights reserved.
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

GCC_ASM_IMPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_IMPORT(ArmReadMpidr)
GCC_ASM_IMPORT(ArmPlatformPeiBootAction)
GCC_ASM_IMPORT(ArmPlatformStackSet)
GCC_ASM_EXPORT(_ModuleEntryPoint)
ASM_GLOBAL ASM_PFX(mSystemMemoryEnd)

StartupAddr:                  .long ASM_PFX(CEntryPoint)
ASM_PFX(mSystemMemoryEnd):    .quad 0

__relocs:
  .long   __reloc_base - __relocs
  .long   __reloc_start - __relocs
  .long   __reloc_end - __relocs

ASM_PFX(_ModuleEntryPoint):
  //
  // We are built as a ET_DYN PIE executable, so we need to process all
  // relative relocations if we are executing from a different offset than we
  // were linked at. This is only possible if we are running from RAM.
  //

  adr   r12, __relocs
  ldrd  r4, r5, [r12]
  ldr   r6, [r12, #8]

  add   r4, r4, r12
  add   r5, r5, r12
  add   r6, r6, r12

.Lreloc_loop:
  cmp   r5, r6
  bhs   .Lreloc_done

  //
  // AArch32 uses the ELF32 REL format, which means each entry in the
  // relocation table consists of
  //
  //   UINT32 offset          : the relative offset of the value that needs to
  //                            be relocated
  //   UINT32 info            : relocation type and symbol index (the latter is
  //                            not used for R_ARM_RELATIVE relocations)
  //
  ldrd  r8, r9, [r5], #8      // read offset into r8 and info into r9
  cmp   r9, #23               // check info == R_ARM_RELATIVE?
  bne   .Lreloc_loop          // not a relative relocation? then skip

  ldr   r9, [r8, r4]          // read addend into r9
  add   r9, r9, r1            // add image base to addend to get relocated value
  str   r9, [r8, r4]          // write relocated value at offset
  b     .Lreloc_loop
.Lreloc_done:

  // Do early platform specific actions
  bl    ASM_PFX(ArmPlatformPeiBootAction)

  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   r10, r0

// Check if we can install the stack at the top of the System Memory or if we need
// to install the stacks at the bottom of the Firmware Device (case the FD is located
// at the top of the DRAM)
_SetupStackPosition:
  // Compute Top of System Memory
  ldr   r12, =PcdGet64 (PcdSystemMemoryBase)
  ldr   r1, [r12]
  ldr   r12, =PcdGet64 (PcdSystemMemorySize)
  ldrd  r2, r3, [r12]

  // calculate the top of memory, and record it in mSystemMemoryEnd
  adds  r2, r2, r1
  sub   r2, r2, #1
  addcs r3, r3, #1
  adr   r12, mSystemMemoryEnd
  strd  r2, r3, [r12]

  // truncate the memory used by UEFI to 4 GB range
  teq   r3, #0
  movne r1, #-1
  moveq r1, r2

  // Calculate Top of the Firmware Device
  ldr   r12, =PcdGet64 (PcdFdBaseAddress)
  ldr   r2, [r12]
  ldr   r3, =FixedPcdGet32 (PcdFdSize)
  sub   r3, r3, #1
  add   r3, r3, r2      // r3 = FdTop = PcdFdBaseAddress + PcdFdSize

  // UEFI Memory Size (stacks are allocated in this region)
  LoadConstantToReg (FixedPcdGet32(PcdSystemMemoryUefiRegionSize), r4)

  //
  // Reserve the memory for the UEFI region (contain stacks on its top)
  //

  // Calculate how much space there is between the top of the Firmware and the Top of the System Memory
  subs  r0, r1, r3   // r0 = SystemMemoryTop - FdTop
  bmi   _SetupStack  // Jump if negative (FdTop > SystemMemoryTop). Case when the PrePi is in XIP memory outside of the DRAM
  cmp   r0, r4
  bge   _SetupStack

  // Case the top of stacks is the FdBaseAddress
  mov   r1, r2

_SetupStack:
  // r1 contains the top of the stack (and the UEFI Memory)

  // Because the 'push' instruction is equivalent to 'stmdb' (decrement before), we need to increment
  // one to the top of the stack. We check if incrementing one does not overflow (case of DRAM at the
  // top of the memory space)
  adds  r11, r1, #1
  bcs   _SetupOverflowStack

_SetupAlignedStack:
  mov   r1, r11
  b     _GetBaseUefiMemory

_SetupOverflowStack:
  // Case memory at the top of the address space. Ensure the top of the stack is EFI_PAGE_SIZE
  // aligned (4KB)
  LoadConstantToReg (EFI_PAGE_MASK, r11)
  and   r11, r11, r1
  sub   r1, r1, r11

_GetBaseUefiMemory:
  // Calculate the Base of the UEFI Memory
  sub   r11, r1, r4

_GetStackBase:
  // r1 = The top of the Mpcore Stacks
  // Stack for the primary core = PrimaryCoreStack
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), r2)
  sub   r12, r1, r2

  // Stack for the secondary core = Number of Cores - 1
  LoadConstantToReg (FixedPcdGet32(PcdCoreCount), r0)
  sub   r0, r0, #1
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), r1)
  mul   r1, r1, r0
  sub   r12, r12, r1

  // r12 = The base of the MpCore Stacks (primary stack & secondary stacks)
  mov   r0, r12
  mov   r1, r10
  //ArmPlatformStackSet(StackBase, MpId, PrimaryStackSize, SecondaryStackSize)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCorePrimaryStackSize), r2)
  LoadConstantToReg (FixedPcdGet32(PcdCPUCoreSecondaryStackSize), r3)
  bl    ASM_PFX(ArmPlatformStackSet)

  // Is it the Primary Core ?
  mov   r0, r10
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   r0, #1
  bne   _PrepareArguments

_PrepareArguments:
  mov   r0, r10
  mov   r1, r11
  mov   r2, r12

  // Move sec startup address into a data register
  // Ensure we're jumping to FV version of the code (not boot remapped alias)
  ldr   r4, StartupAddr

  // Jump to PrePiCore C code
  //    r0 = MpId
  //    r1 = UefiMemoryBase
  //    r2 = StacksBase
  blx   r4

_NeverReturn:
  b _NeverReturn
