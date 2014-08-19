#------------------------------------------------------------------------------
#
# Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
#
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

  .text
  .align 2
  GCC_ASM_EXPORT(__muldi3)

ASM_PFX(__muldi3):
  stmfd  sp!, {r4, r5, r6, r7, lr}
  add  r7, sp, #12
  stmfd  sp!, {r8, r10, r11}
  ldr  r11, L4
  mov  r4, r0, lsr #16
  and  r8, r0, r11
  and  ip, r2, r11
  mul  lr, ip, r8
  mul  ip, r4, ip
  sub  sp, sp, #8
  add  r10, ip, lr, lsr #16
  and  ip, r10, r11
  and  lr, lr, r11
  mov  r6, r2, lsr #16
  str  r4, [sp, #4]
  add  r4, lr, ip, asl #16
  mul  ip, r8, r6
  mov  r5, r10, lsr #16
  add  r10, ip, r4, lsr #16
  and  ip, r10, r11
  and  lr, r4, r11
  add  r4, lr, ip, asl #16
  mul  r0, r3, r0
  add  ip, r5, r10, lsr #16
  ldr  r5, [sp, #4]
  mla  r0, r2, r1, r0
  mla  r5, r6, r5, ip
  mov  r10, r4
  add  r11, r0, r5
  mov  r1, r11
  mov  r0, r4
  sub  sp, r7, #24
  ldmfd  sp!, {r8, r10, r11}
  ldmfd  sp!, {r4, r5, r6, r7, pc}
  .p2align 2
L5:
  .align 2
L4:
  .long  65535
