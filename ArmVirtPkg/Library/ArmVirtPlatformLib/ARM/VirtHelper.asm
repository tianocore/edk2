//
//  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
//  Copyright (c) 2014, Linaro Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/ArmLib.h>
#include <Library/PcdLib.h>
#include <AutoGen.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformPeiBootAction
  EXPORT  ArmPlatformIsPrimaryCore
  EXPORT  ArmPlatformGetPrimaryCoreMpId
  EXPORT  ArmPlatformGetCorePosition
  EXPORT  ArmGetPhysAddrTop

  IMPORT  _gPcd_FixedAtBuild_PcdArmPrimaryCore
  IMPORT  _gPcd_FixedAtBuild_PcdArmPrimaryCoreMask
  IMPORT  _gPcd_FixedAtBuild_PcdCoreCount

  AREA VirtHelper, CODE, READONLY

ArmPlatformPeiBootAction FUNCTION
  bx    lr
  ENDFUNC

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ArmPlatformGetPrimaryCoreMpId FUNCTION
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCore, r0)
  ldr    r0, [r0]
  bx     lr
  ENDFUNC

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ArmPlatformIsPrimaryCore FUNCTION
  mov   r0, #1
  bx    lr
  ENDFUNC

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 4) + CoreId
ArmPlatformGetCorePosition FUNCTION
  and   r1, r0, #ARM_CORE_MASK
  and   r0, r0, #ARM_CLUSTER_MASK
  add   r0, r1, r0, LSR #6
  bx    lr
  ENDFUNC

//EFI_PHYSICAL_ADDRESS
//GetPhysAddrTop (
//  VOID
//  );
ArmGetPhysAddrTop FUNCTION
  mov   r0, #0x00000000
  mov   r1, #0x10000
  bx    lr
  ENDFUNC

  END
