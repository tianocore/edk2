//
//  Copyright (c) 2013, ARM Limited. All rights reserved.
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

#include <Library/ArmLib.h>

#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformPeiBootAction
  EXPORT  ArmPlatformIsPrimaryCore
  EXPORT  ArmPlatformGetPrimaryCoreMpId
  EXPORT  ArmPlatformGetCorePosition

  AREA CTA9x4Helper, CODE, READONLY

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ArmPlatformGetPrimaryCoreMpId FUNCTION
  mov32 r0, FixedPcdGet32(PcdArmPrimaryCore)
  bx      lr
  ENDFUNC

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ArmPlatformIsPrimaryCore FUNCTION
  mov32 r1, FixedPcdGet32(PcdArmPrimaryCoreMask)
  and   r0, r0, r1
  mov32 r1, FixedPcdGet32(PcdArmPrimaryCore)
  cmp   r0, r1
  moveq r0, #1
  movne r0, #0
  bx    lr
  ENDFUNC

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
ArmPlatformGetCorePosition FUNCTION
  and   r0, r0, #ARM_CORE_MASK
  bx    lr
  ENDFUNC

ArmPlatformPeiBootAction FUNCTION
  bx    lr
  ENDFUNC

  END
