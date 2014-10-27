/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <AsmMacroIoLibV8.h>
#include <Library/ArmLib.h>

.text
.align 3

GCC_ASM_EXPORT(ArmPlatformPeiBootAction)
GCC_ASM_EXPORT(ArmPlatformGetCorePosition)
GCC_ASM_EXPORT(ArmPlatformGetPrimaryCoreMpId)
GCC_ASM_EXPORT(ArmPlatformIsPrimaryCore)

GCC_ASM_IMPORT(_gPcd_FixedAtBuild_PcdArmPrimaryCoreMask)


PrimaryCoreMpid:  .word    0x0

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 2) + CoreId
ASM_PFX(ArmPlatformGetCorePosition):
  and   x1, x0, #ARM_CORE_MASK
  and   x0, x0, #ARM_CLUSTER_MASK
  add   x0, x1, x0, LSR #7
  ret

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_PFX(ArmPlatformGetPrimaryCoreMpId):
  ldr   x0, =PrimaryCoreMpid
  ldrh  w0, [x0]
  ret

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformIsPrimaryCore):
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCoreMask, x1)
  ldrh  w1, [x1]
  and   x0, x0, x1

  ldr   x1, =PrimaryCoreMpid
  ldrh  w1, [x1]

  cmp   w0, w1
  mov   x0, #1
  mov   x1, #0
  csel  x0, x0, x1, eq
  ret

ASM_PFX(ArmPlatformPeiBootAction):
  // The trusted firmware passes the primary CPU MPID through x0 register.
  // Save it in a variable.
  ldr  x1, =PrimaryCoreMpid
  str  w0, [x1]
  ret

