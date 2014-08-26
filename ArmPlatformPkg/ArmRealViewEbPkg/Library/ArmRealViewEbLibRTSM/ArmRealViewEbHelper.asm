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
#include <Library/ArmLib.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformPeiBootAction
  EXPORT  ArmPlatformGetCorePosition
  EXPORT  ArmPlatformIsPrimaryCore

  IMPORT  _gPcd_FixedAtBuild_PcdArmPrimaryCore
  IMPORT  _gPcd_FixedAtBuild_PcdArmPrimaryCoreMask

  AREA    ArmRealViewEbHelper, CODE, READONLY

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
ArmPlatformGetCorePosition FUNCTION
  and   r1, r0, #ARM_CORE_MASK
  and   r0, r0, #ARM_CLUSTER_MASK
  add   r0, r1, r0, LSR #7
  bx    lr
  ENDFUNC

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ArmPlatformIsPrimaryCore FUNCTION
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCoreMask, r1)
  ldr   r1, [r1]
  and   r0, r0, r1
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCore, r1)
  ldr   r1, [r1]
  cmp   r0, r1
  moveq r0, #1
  movne r0, #0
  bx    lr
  ENDFUNC

ArmPlatformPeiBootAction FUNCTION
  bx    lr
  ENDFUNC

  END
