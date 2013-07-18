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
  str   x1, [sp, #-0x10]!
  mov   x1, x0
  ldr   x0,[x1]
  smc   #0
  str   x0,[x1]
  ldr   x1, [sp], #0x10
  ret

ASM_PFX(ArmCallSmcArg1):
  stp   x2, x3, [sp, #-0x10]!
  mov   x2, x0
  mov   x3, x1
  ldr   x0,[x2]
  ldr   x1,[x3]
  smc   #0
  str   x0,[x2]
  str   x1,[x3]
  ldp   x2, x3, [sp], #0x10
  ret

ASM_PFX(ArmCallSmcArg2):
  stp   x3, x4, [sp, #-0x10]!
  str   x5, [sp, #-8]!
  mov   x3, x0
  mov   x4, x1
  mov   x5, x2
  ldr   x0,[x3]
  ldr   x1,[x4]
  ldr   x2,[x5]
  smc   #0
  str   x0,[x3]
  str   x1,[x4]
  str   x2,[x5]
  ldr   x5, [sp], #8
  ldp   x3, x4, [sp], #0x10
  ret

ASM_PFX(ArmCallSmcArg3):
  stp   x4, x5, [sp, #-0x10]!
  stp   x6, x7, [sp, #-0x10]!
  mov   x4, x0
  mov   x5, x1
  mov   x6, x2
  mov   x7, x3
  ldr   x0,[x4]
  ldr   x1,[x5]
  ldr   x2,[x6]
  ldr   x3,[x7]
  smc   #0
  str   x0,[x4]
  str   x1,[x5]
  str   x2,[x6]
  str   x3,[x7]
  ldp   x4, x5, [sp], #0x10
  ldp   x6, x7, [sp], #0x10
  ret
