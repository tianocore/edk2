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

.text
.align 3

GCC_ASM_EXPORT(ArmCallSmc)
GCC_ASM_EXPORT(ArmCallSmcArg1)
GCC_ASM_EXPORT(ArmCallSmcArg2)
GCC_ASM_EXPORT(ArmCallSmcArg3)

ASM_PFX(ArmCallSmc):
  bx     lr

// Arg1 in R1
ASM_PFX(ArmCallSmcArg1):
  bx     lr

ASM_PFX(ArmCallSmcArg2):
  bx     lr

ASM_PFX(ArmCallSmcArg3):
  bx     lr
