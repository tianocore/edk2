//
//  Copyright (c) 2011-2023, Arm Limited. All rights reserved.
//  Copyright (c) 2015-2016, Linaro Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroLib.h>

ASM_FUNC(_ModuleEntryPoint)
  //
  // If we are booting from RAM using the Linux kernel boot protocol, x0 will
  // point to the DTB image in memory. Otherwise, use the default value defined
  // by the platform.
  //
  cbnz  x0, 0f
  ldr   x0, PcdGet64 (PcdDeviceTreeInitialBaseAddress)

0:mov   x28, x0             // preserve DTB pointer
  mov   x27, x1             // preserve base of image pointer

  // Enable Floating Point. This needs to be done before entering C code, which
  // may use FP/SIMD registers.
  bl    ArmEnableVFP

  bl    ASM_PFX(DiscoverDramFromDt)

  // Check if we are in a Realm and configure
  // the System Memory as Protected RAM.
  bl    ASM_PFX(ArmCcaConfigureSystemMemory)

  // Get ID of this CPU in Multicore system
  bl    ASM_PFX(ArmReadMpidr)
  // Keep a copy of the MpId register value
  mov   x20, x0

// Check if we can install the stack at the top of the System Memory or if we need
// to install the stacks at the bottom of the Firmware Device (case the FD is located
// at the top of the DRAM)
_SetupStackPosition:
  // Compute Top of System Memory
  ldr   x1, PcdGet64 (PcdSystemMemoryBase)
  ldr   x2, PcdGet64 (PcdSystemMemorySize)
  sub   x2, x2, #1
  add   x1, x1, x2      // x1 = SystemMemoryTop = PcdSystemMemoryBase + PcdSystemMemorySize

  // Calculate Top of the Firmware Device
  ldr   x2, PcdGet64 (PcdFdBaseAddress)
  MOV32 (w3, FixedPcdGet32 (PcdFdSize) - 1)
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
  adds  x21, x1, #1
  b.cs  _SetupOverflowStack

_SetupAlignedStack:
  mov   x1, x21
  b     _GetBaseUefiMemory

_SetupOverflowStack:
  // Case memory at the top of the address space. Ensure the top of the stack is EFI_PAGE_SIZE
  // aligned (4KB)
  and   x1, x1, ~EFI_PAGE_MASK

_GetBaseUefiMemory:
  // Calculate the Base of the UEFI Memory
  sub   x21, x1, x4

_GetStackBase:
  // r1 = The top of the Mpcore Stacks
  mov   sp, x1

  // Stack for the primary core = PrimaryCoreStack
  MOV32 (x2, FixedPcdGet32(PcdCPUCorePrimaryStackSize))
  sub   x22, x1, x2

  mov   x0, x20
  mov   x1, x21
  mov   x2, x22

  // Set the frame pointer to NULL so any backtraces terminate here
  mov   x29, xzr

  // Jump to PrePiCore C code
  //    x0 = MpId
  //    x1 = UefiMemoryBase
  //    x2 = StacksBase
  bl    ASM_PFX(CEntryPoint)

_NeverReturn:
  b _NeverReturn

// VOID
// DiscoverDramFromDt (
//   VOID   *DeviceTreeBaseAddress,   // passed by loader in x0, preserved in x28
//   VOID   *ImageBase                // passed by FDF trampoline in x1, preserved in x27
//   );
ASM_PFX(DiscoverDramFromDt):
  mov   x29, x30            // preserve LR

  //
  // The base of the runtime image has been preserved in x27. Check whether
  // the expected magic number can be found in the header.
  //
  ldr   w8, .LArm64LinuxMagic
  ldr   w9, [x27, #0x38]
  cmp   w8, w9
  bne   .Lout

  //
  //
  // OK, so far so good. We have confirmed that we likely have a DTB and are
  // booting via the arm64 Linux boot protocol. Update the base-of-image PCD
  // to the actual relocated value, and add the shift of PcdFdBaseAddress to
  // PcdFvBaseAddress as well
  //
  adr   x8, PcdGet64 (PcdFdBaseAddress)
  adr   x9, PcdGet64 (PcdFvBaseAddress)
  ldr   x6, [x8]
  ldr   x7, [x9]
  sub   x7, x7, x6
  add   x7, x7, x27
  str   x27, [x8]
  str   x7, [x9]

  //
  // The runtime address may be different from the link time address so fix
  // up the PE/COFF relocations. Since we are calling a C function, use the
  // window at the beginning of the FD image as a temp stack.
  //
  mov   x0, x7
  adr   x1, PeCoffLoaderImageReadFromMemory
  mov   sp, x7
  bl    RelocatePeCoffImage

  //
  // Discover the memory size and offset from the DTB, and record in the
  // respective PCDs. This will also return false if a corrupt DTB is
  // encountered.
  //
  mov   x0, x28
  adr   x1, PcdGet64 (PcdSystemMemoryBase)
  adr   x2, PcdGet64 (PcdSystemMemorySize)
  bl    FindMemnode
  cbz   x0, .Lout

  //
  // Copy the DTB to the slack space right after the 64 byte arm64/Linux style
  // image header at the base of this image (defined in the FDF), and record the
  // pointer in PcdDeviceTreeInitialBaseAddress.
  //
  adr   x8, PcdGet64 (PcdDeviceTreeInitialBaseAddress)
  add   x27, x27, #0x40
  str   x27, [x8]

  mov   x0, x27
  mov   x1, x28
  bl    CopyFdt

.Lout:
  ret    x29

.LArm64LinuxMagic:
  .byte   0x41, 0x52, 0x4d, 0x64
