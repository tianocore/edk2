#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLibV8.h>
#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)
GCC_ASM_EXPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_EXPORT(ArmPlatformGetPrimaryCoreMpId)
GCC_ASM_EXPORT(ArmPlatformGetCorePosition)
GCC_ASM_EXPORT(ArmGetPhysAddrTop)

GCC_ASM_IMPORT(_gPcd_FixedAtBuild_PcdArmPrimaryCore)
GCC_ASM_IMPORT(_gPcd_FixedAtBuild_PcdArmPrimaryCoreMask)
GCC_ASM_IMPORT(_gPcd_FixedAtBuild_PcdCoreCount)

.LFdtMagic:
  .byte   0xd0, 0x0d, 0xfe, 0xed

.LArm64LinuxMagic:
  .byte   0x41, 0x52, 0x4d, 0x64

// VOID
// ArmPlatformPeiBootAction (
//   VOID   *DeviceTreeBaseAddress,   // passed by loader in x0
//   VOID   *ImageBase                // passed by FDF trampoline in x1
//   );
ASM_PFX(ArmPlatformPeiBootAction):
  mov   x29, x30            // preserve LR

  //
  // If we are booting from RAM using the Linux kernel boot protocol, x0 will
  // point to the DTB image in memory. Otherwise, we are just coming out of
  // reset, and x0 will be 0. Check also the FDT magic.
  //
  cbz   x0, .Lout
  ldr   w8, .LFdtMagic
  ldr   w9, [x0]
  cmp   w8, w9
  bne   .Lout

  //
  // The base of the runtime image has been preserved in x1. Check whether
  // the expected magic number can be found in the header.
  //
  ldr   w8, .LArm64LinuxMagic
  ldr   w9, [x1, #0x38]
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
  add   x7, x7, x1
  str   x1, [x8]
  str   x7, [x9]

  //
  // Copy the DTB to the slack space right after the 64 byte arm64/Linux style
  // image header at the base of this image (defined in the FDF), and record the
  // pointer in PcdDeviceTreeInitialBaseAddress.
  //
  adr   x8, PcdGet64 (PcdDeviceTreeInitialBaseAddress)
  add   x1, x1, #0x40
  str   x1, [x8]

  ldr   w8, [x0, #4]          // get DTB size (BE)
  mov   x9, x1
  rev   w8, w8
  add   x8, x8, x0
0:ldp   x6, x7, [x0], #16
  stp   x6, x7, [x9], #16
  cmp   x0, x8
  blt   0b

  //
  // Discover the memory size and offset from the DTB, and record in the
  // respective PCDs
  //
  mov   x0, x1
  bl    find_memnode    // returns (size, base) size in (x0, x1)
  cbz   x0, .Lout

  adr   x8, PcdGet64 (PcdSystemMemorySize)
  adr   x9, PcdGet64 (PcdSystemMemoryBase)
  str   x0, [x8]
  str   x1, [x9]

.Lout:
  ret    x29

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_PFX(ArmPlatformGetPrimaryCoreMpId):
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCore, x0)
  ldrh   w0, [x0]
  ret

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformIsPrimaryCore):
  mov   x0, #1
  ret

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ASM_PFX(ArmPlatformGetCorePosition):
  and   x1, x0, #ARM_CORE_MASK
  and   x0, x0, #ARM_CLUSTER_MASK
  add   x0, x1, x0, LSR #6
  ret

//EFI_PHYSICAL_ADDRESS
//GetPhysAddrTop (
//  VOID
//  );
ASM_PFX(ArmGetPhysAddrTop):
  mrs   x0, id_aa64mmfr0_el1
  adr   x1, .LPARanges
  and   x0, x0, #7
  ldrb  w1, [x1, x0]
  mov   x0, #1
  lsl   x0, x0, x1
  ret

//
// Bits 0..2 of the AA64MFR0_EL1 system register encode the size of the
// physical address space support on this CPU:
// 0 == 32 bits, 1 == 36 bits, etc etc
// 6 and 7 are reserved
//
.LPARanges:
  .byte 32, 36, 40, 42, 44, 48, -1, -1

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
