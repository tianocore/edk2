//
//  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
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
#include <AutoGen.h>

.text
.align 3

GCC_ASM_EXPORT(ArmPlatformStackSet)
GCC_ASM_EXPORT(ArmPlatformStackSetPrimary)
GCC_ASM_EXPORT(ArmPlatformStackSetSecondary)

GCC_ASM_IMPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_IMPORT(ArmPlatformGetCorePosition)
GCC_ASM_IMPORT(ArmPlatformGetPrimaryCoreMpId)

GCC_ASM_IMPORT(gPcd_FixedAtBuild_PcdCoreCount)

//VOID
//ArmPlatformStackSet (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ASM_PFX(ArmPlatformStackSet):
  // Save parameters
  mov   x6, x3
  mov   x5, x2
  mov   x4, x1
  mov   x3, x0

  // Save the Link register
  mov   x7, x30

  // Identify Stack
  mov   x0, x1
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   x0, #1

  // Restore parameters
  mov   x0, x3
  mov   x1, x4
  mov   x2, x5
  mov   x3, x6

  // Restore the Link register
  mov   x30, x7

  // Should be ASM_PFX(ArmPlatformStackSetPrimary) but generate linker error 'unsupported ELF EM_AARCH64'
  b.eq  ArmPlatformStackSetPrimaryL
  // Should be ASM_PFX(ArmPlatformStackSetSecondary) but generate linker error 'unsupported ELF EM_AARCH64'
  b.ne  ArmPlatformStackSetSecondaryL

//VOID
//ArmPlatformStackSetPrimary (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ArmPlatformStackSetPrimaryL:
ASM_PFX(ArmPlatformStackSetPrimary):
  // Save the Link register
  mov   x4, x30

  // Add stack of primary stack to StackBase
  add   x0, x0, x2

  // Compute SecondaryCoresCount * SecondaryCoreStackSize
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdCoreCount, x1)
  ldr   w1, [x1]
  sub   x1, x1, #1
  mul   x3, x3, x1

  // Set Primary Stack ((StackBase + PrimaryStackSize) + (SecondaryCoresCount * SecondaryCoreStackSize))
  add   sp, x0, x3

  br    x4

//VOID
//ArmPlatformStackSetSecondary (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ArmPlatformStackSetSecondaryL:
ASM_PFX(ArmPlatformStackSetSecondary):
  // Save the Link register
  mov   x4, x30
  mov   sp, x0

  // Get Core Position
  mov   x0, x1
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  mov   x5, x0

  // Get Primary Core Position
  bl    ASM_PFX(ArmPlatformGetPrimaryCoreMpId)
  bl    ASM_PFX(ArmPlatformGetCorePosition)

  // Get Secondary Core Position. We should get consecutive secondary stack number from 1...(CoreCount-1)
  cmp   x5, x0
  b.ls  1f
  // Decrement the position if after the primary core
  sub   x5, x5, #1
1:
  add   x5, x5, #1

  // Compute top of the secondary stack
  mul   x3, x3, x5

  // Set stack
  add   sp, sp, x3

  br    x4
