#
#  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
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

.text
.align 2

GCC_ASM_EXPORT(ArmPlatformIsPrimaryCore)

GCC_ASM_IMPORT(_gPcd_FixedAtBuild_PcdArmPrimaryCore)
GCC_ASM_IMPORT(_gPcd_FixedAtBuild_PcdArmPrimaryCoreMask)

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_PFX(ArmPlatformIsPrimaryCore):
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCoreMask, r1)
  ldr   r1, [r1]
  and   r0, r0, r1
  LoadConstantToReg (_gPcd_FixedAtBuild_PcdArmPrimaryCore, r1)
  ldr   r1, [r1]
  cmp   r0, r1
  moveq r0, #1
  movne r0, #0
  bx    lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
