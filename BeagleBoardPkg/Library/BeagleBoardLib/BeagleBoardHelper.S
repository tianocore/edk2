#
#  Copyright (c) 2012-2013, ARM Limited. All rights reserved.
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
#include <AutoGen.h>

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformIsPrimaryCore)
GCC_ASM_EXPORT(ArmPlatformGetPrimaryCoreMpId)
GCC_ASM_EXPORT(ArmPlatformPeiBootAction)

GCC_ASM_IMPORT(ArmReadMpidr)

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformIsPrimaryCore):
  // BeagleBoard has a single core. We must always return 1.
  mov   r0, #1
  bx    lr

ASM_PFX(ArmPlatformPeiBootAction):
  bx    lr

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_PFX(ArmPlatformGetPrimaryCoreMpId):
  // The BeagleBoard is a uniprocessor platform. The MPIDR of primary core is
  // always the MPIDR of the calling CPU.
  b   ASM_PFX(ArmReadMpidr)

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
