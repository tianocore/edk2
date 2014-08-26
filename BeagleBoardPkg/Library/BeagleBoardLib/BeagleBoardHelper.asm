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

  EXPORT    ArmPlatformPeiBootAction
  EXPORT    ArmPlatformIsPrimaryCore
  EXPORT    ArmPlatformGetPrimaryCoreMpId

  IMPORT    ArmReadMpidr

  AREA BeagleBoardHelper, CODE, READONLY

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ArmPlatformIsPrimaryCore FUNCTION
  // BeagleBoard has a single core. We must always return 1.
  mov   r0, #1
  bx    lr
  ENDFUNC

ArmPlatformPeiBootAction FUNCTION
  bx    lr
  ENDFUNC

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ArmPlatformGetPrimaryCoreMpId FUNCTION
  // The BeagleBoard is a uniprocessor platform. The MPIDR of primary core is
  // always the MPIDR of the calling CPU.
  b     ArmReadMpidr
  ENDFUNC

  END
