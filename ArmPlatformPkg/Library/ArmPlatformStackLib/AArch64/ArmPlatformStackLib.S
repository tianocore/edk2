//
//  Copyright (c) 2012-2014, ARM Limited. All rights reserved.
//
//  SPDX-License-Identifier: BSD-2-Clause-Patent
//
//

#include <AsmMacroIoLibV8.h>

//VOID
//ArmPlatformStackSet (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ASM_FUNC(ArmPlatformStackSet)
  // Save parameters
  mov   x26, x3
  mov   x25, x2
  mov   x24, x1
  mov   x23, x0

  // Save the Link register
  mov   x27, x30

  // Identify Stack
  mov   x0, x1
  bl    ASM_PFX(ArmPlatformIsPrimaryCore)
  cmp   x0, #1

  // Restore parameters
  mov   x0, x23
  mov   x1, x24
  mov   x2, x25
  mov   x3, x26

  // Restore the Link register
  mov   x30, x27

  b.ne  0f

  b     ASM_PFX(ArmPlatformStackSetPrimary)
0:b     ASM_PFX(ArmPlatformStackSetSecondary)

//VOID
//ArmPlatformStackSetPrimary (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ASM_FUNC(ArmPlatformStackSetPrimary)
  // Add size of primary stack to StackBase
  add   x0, x0, x2

  // Compute SecondaryCoresCount * SecondaryCoreStackSize
  MOV32 (w1, FixedPcdGet32(PcdCoreCount) - 1)
  mul   x3, x3, x1

  // Set Primary Stack ((StackBase + PrimaryStackSize) + (SecondaryCoresCount * SecondaryCoreStackSize))
  add   sp, x0, x3

  ret

//VOID
//ArmPlatformStackSetSecondary (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ASM_FUNC(ArmPlatformStackSetSecondary)
  // Save the Link register
  mov   x24, x30
  mov   sp, x0

  // Get Core Position
  mov   x0, x1
  bl    ASM_PFX(ArmPlatformGetCorePosition)
  mov   x25, x0

  // Get Primary Core Position
  bl    ASM_PFX(ArmPlatformGetPrimaryCoreMpId)
  bl    ASM_PFX(ArmPlatformGetCorePosition)

  // Get Secondary Core Position. We should get consecutive secondary stack number from 1...(CoreCount-1)
  cmp   x25, x0

  // Decrement the position if after the primary core
  cinc  x25, x25, ls

  // Compute top of the secondary stack
  mul   x3, x3, x25

  // Set stack
  add   sp, sp, x3

  ret   x24
