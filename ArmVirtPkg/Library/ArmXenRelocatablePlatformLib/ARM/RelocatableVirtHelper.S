#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
#  Copyright (c) 2014, Linaro Limited. All rights reserved.
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

#include <AsmMacroIoLib.h>
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

.LArm32LinuxMagic:
  .byte   0x18, 0x28, 0x6f, 0x01

ASM_PFX(ArmPlatformPeiBootAction):
  mov   r11, r14            // preserve LR
  mov   r10, r0             // preserve DTB pointer
  mov   r9, r1              // preserve base of image pointer

  //
  // If we are booting from RAM using the Linux kernel boot protocol, r0 will
  // point to the DTB image in memory. Otherwise, we are just coming out of
  // reset, and r0 will be 0.
  //
  teq   r0, #0
  beq   .Lout

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
  ldr   r8, =PcdGet64 (PcdFdBaseAddress)
  ldr   r7, =PcdGet64 (PcdFvBaseAddress)
  ldr   r6, [r8]
  ldr   r5, [r7]
  sub   r5, r5, r6
  add   r5, r5, r1
  str   r1, [r8]
  str   r5, [r7]

  //
  // Discover the memory size and offset from the DTB, and record in the
  // respective PCDs. This will also return false if a corrupt DTB is
  // encountered. Since we are calling a C function, use the window at the
  // beginning of the FD image as a temp stack.
  //
  ldr   r1, =PcdGet64 (PcdSystemMemorySize)
  ldr   r2, =PcdGet64 (PcdSystemMemoryBase)
  mov   sp, r5
  bl    FindMemnode
  teq   r0, #0
  beq   .Lout

  //
  // Copy the DTB to the slack space right after the 64 byte arm64/Linux style
  // image header at the base of this image (defined in the FDF), and record the
  // pointer in PcdDeviceTreeInitialBaseAddress.
  //
  ldr   r8, =PcdGet64 (PcdDeviceTreeInitialBaseAddress)
  add   r9, r9, #0x40
  str   r9, [r8]

  mov   r0, r9
  mov   r1, r10
  bl    CopyFdt

.Lout:
  bx    r11

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_PFX(ArmPlatformGetPrimaryCoreMpId):
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCore, r0)
  ldr    r0, [r0]
  bx     lr

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformIsPrimaryCore):
  mov   r0, #1
  bx    lr

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ASM_PFX(ArmPlatformGetCorePosition):
  and   r1, r0, #ARM_CORE_MASK
  and   r0, r0, #ARM_CLUSTER_MASK
  add   r0, r1, r0, LSR #6
  bx    lr

//EFI_PHYSICAL_ADDRESS
//GetPhysAddrTop (
//  VOID
//  );
ASM_PFX(ArmGetPhysAddrTop):
  mov   r0, #0x00000000
  mov   r1, #0x10000
  bx    lr

