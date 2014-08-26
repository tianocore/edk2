//
//  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
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
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformStackSet
  EXPORT  ArmPlatformStackSetPrimary
  EXPORT  ArmPlatformStackSetSecondary

  IMPORT  ArmPlatformIsPrimaryCore
  IMPORT  ArmPlatformGetCorePosition
  IMPORT  ArmPlatformGetPrimaryCoreMpId

  IMPORT  _gPcd_FixedAtBuild_PcdCoreCount

  PRESERVE8
  AREA    ArmPlatformStackLib, CODE, READONLY

//VOID
//ArmPlatformStackSet (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ArmPlatformStackSet FUNCTION
  // Save parameters
  mov   r6, r3
  mov   r5, r2
  mov   r4, r1
  mov   r3, r0

  // Save the Link register
  mov   r7, lr

  // Identify Stack
  mov   r0, r1
  bl    ArmPlatformIsPrimaryCore
  cmp   r0, #1

  // Restore parameters
  mov   r0, r3
  mov   r1, r4
  mov   r2, r5
  mov   r3, r6

  // Restore the Link register
  mov   lr, r7

  beq   ArmPlatformStackSetPrimary
  bne   ArmPlatformStackSetSecondary
  ENDFUNC

//VOID
//ArmPlatformStackSetPrimary (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ArmPlatformStackSetPrimary FUNCTION
  mov   r4, lr

  // Add stack of primary stack to StackBase
  add   r0, r0, r2

  // Compute SecondaryCoresCount * SecondaryCoreStackSize
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdCoreCount, r1)
  ldr   r1, [r1]
  sub   r1, #1
  mul   r3, r3, r1

  // Set Primary Stack ((StackBase + PrimaryStackSize) + (SecondaryCoresCount * SecondaryCoreStackSize))
  add   sp, r0, r3

  bx    r4
  ENDFUNC

//VOID
//ArmPlatformStackSetSecondary (
//  IN UINTN StackBase,
//  IN UINTN MpId,
//  IN UINTN PrimaryStackSize,
//  IN UINTN SecondaryStackSize
//  );
ArmPlatformStackSetSecondary FUNCTION
  mov   r4, lr
  mov   sp, r0

  // Get Core Position
  mov   r0, r1
  bl ArmPlatformGetCorePosition
  mov   r5, r0

  // Get Primary Core Position
  bl ArmPlatformGetPrimaryCoreMpId
  bl ArmPlatformGetCorePosition

  // Get Secondary Core Position. We should get consecutive secondary stack number from 1...(CoreCount-1)
  cmp   r5, r0
  subhi r5, r5, #1
  add   r5, r5, #1

  // Compute top of the secondary stack
  mul   r3, r3, r5

  // Set stack
  add   sp, sp, r3

  bx r4
  ENDFUNC

  END
