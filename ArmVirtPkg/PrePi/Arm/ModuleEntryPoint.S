//
//  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
//  Copyright (c) 2015-2016, Linaro Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

ASM_FUNC(_ModuleEntryPoint)
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
  LDRL  (r1, PcdGet64 (PcdSystemMemoryBase))
  ADRL  (r12, PcdGet64 (PcdSystemMemorySize))
  ldrd  r2, r3, [r12]

  // calculate the top of memory
  adds  r2, r2, r1
  sub   r2, r2, #1
  addcs r3, r3, #1

  // truncate the memory used by UEFI to 4 GB range
  teq   r3, #0
  movne r1, #-1
  moveq r1, r2

  // Calculate Top of the Firmware Device
  LDRL  (r2, PcdGet64 (PcdFdBaseAddress))
  MOV32 (r3, FixedPcdGet32 (PcdFdSize) - 1)
  add   r3, r3, r2      // r3 = FdTop = PcdFdBaseAddress + PcdFdSize

  // UEFI Memory Size (stacks are allocated in this region)
  MOV32 (r4, FixedPcdGet32(PcdSystemMemoryUefiRegionSize))

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
  MOV32 (r11, (~EFI_PAGE_MASK) & 0xffffffff)
  and   r1, r1, r11

_GetBaseUefiMemory:
  // Calculate the Base of the UEFI Memory
  sub   r11, r1, r4

_GetStackBase:
  // r1 = The top of the Mpcore Stacks
  mov   sp, r1

  // Stack for the primary core = PrimaryCoreStack
  MOV32 (r2, FixedPcdGet32(PcdCPUCorePrimaryStackSize))
  sub   r9, r1, r2

  mov   r0, r10
  mov   r1, r11
  mov   r2, r9

  // Jump to PrePiCore C code
  //    r0 = MpId
  //    r1 = UefiMemoryBase
  //    r2 = StacksBase
  bl    ASM_PFX(CEntryPoint)

_NeverReturn:
  b _NeverReturn

ASM_PFX(ArmPlatformPeiBootAction):
  //
  // If we are booting from RAM using the Linux kernel boot protocol, r0 will
  // point to the DTB image in memory. Otherwise, use the default value defined
  // by the platform.
  //
  teq   r0, #0
  bne   0f
  LDRL  (r0, PcdGet64 (PcdDeviceTreeInitialBaseAddress))

0:mov   r11, r14            // preserve LR
  mov   r10, r0             // preserve DTB pointer
  mov   r9, r1              // preserve base of image pointer

  //
  // The base of the runtime image has been preserved in r1. Check whether
  // the expected magic number can be found in the header.
  //
  ldr   r8, .LArm32LinuxMagic
  ldr   r7, [r1, #0x24]
  cmp   r7, r8
  bne   .Lout

  //
  //
  // OK, so far so good. We have confirmed that we likely have a DTB and are
  // booting via the ARM Linux boot protocol. Update the base-of-image PCD
  // to the actual relocated value, and add the shift of PcdFdBaseAddress to
  // PcdFvBaseAddress as well
  //
  ADRL  (r8, PcdGet64 (PcdFdBaseAddress))
  ADRL  (r7, PcdGet64 (PcdFvBaseAddress))
  ldr   r6, [r8]
  ldr   r5, [r7]
  sub   r5, r5, r6
  add   r5, r5, r1
  str   r1, [r8]
  str   r5, [r7]

  //
  // The runtime address may be different from the link time address so fix
  // up the PE/COFF relocations. Since we are calling a C function, use the
  // window at the beginning of the FD image as a temp stack.
  //
  mov   r0, r5
  ADRL  (r1, PeCoffLoaderImageReadFromMemory)
  mov   sp, r5
  bl    RelocatePeCoffImage

  //
  // Discover the memory size and offset from the DTB, and record in the
  // respective PCDs. This will also return false if a corrupt DTB is
  // encountered.
  //
  mov   r0, r10
  ADRL  (r1, PcdGet64 (PcdSystemMemoryBase))
  ADRL  (r2, PcdGet64 (PcdSystemMemorySize))
  bl    FindMemnode
  teq   r0, #0
  beq   .Lout

  //
  // Copy the DTB to the slack space right after the 64 byte arm64/Linux style
  // image header at the base of this image (defined in the FDF), and record the
  // pointer in PcdDeviceTreeInitialBaseAddress.
  //
  ADRL  (r8, PcdGet64 (PcdDeviceTreeInitialBaseAddress))
  add   r9, r9, #0x40
  str   r9, [r8]

  mov   r0, r9
  mov   r1, r10
  bl    CopyFdt

.Lout:
  bx    r11

.LArm32LinuxMagic:
  .byte   0x18, 0x28, 0x6f, 0x01
