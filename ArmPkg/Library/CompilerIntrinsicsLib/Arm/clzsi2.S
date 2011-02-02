#------------------------------------------------------------------------------ 
#
# Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
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
  GCC_ASM_EXPORT(__clzsi2)

ASM_PFX(__clzsi2):
  @ frame_needed = 1, uses_anonymous_args = 0
  stmfd  sp!, {r7, lr}
  add  r7, sp, #0
  movs  r3, r0, lsr #16
  movne  r3, #16
  moveq  r3, #0
  movne  r9, #0
  moveq  r9, #16
  mov  r3, r0, lsr r3
  tst  r3, #65280
  movne  r0, #8
  moveq  r0, #0
  movne  lr, #0
  moveq  lr, #8
  mov  r3, r3, lsr r0
  tst  r3, #240
  movne  r0, #4
  moveq  r0, #0
  movne  ip, #0
  moveq  ip, #4
  mov  r3, r3, lsr r0
  tst  r3, #12
  movne  r0, #2
  moveq  r0, #0
  movne  r1, #0
  moveq  r1, #2
  mov  r2, r3, lsr r0
  add  r3, lr, r9
  add  r0, r3, ip
  add  r1, r0, r1
  mov  r0, r2, lsr #1
  eor  r0, r0, #1
  ands  r0, r0, #1
  mvnne  r0, #0
  rsb  r3, r2, #2
  and  r0, r0, r3
  add  r0, r1, r0
  ldmfd  sp!, {r7, pc}
