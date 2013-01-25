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
  GCC_ASM_EXPORT(memcpy)

ASM_PFX(memcpy):
  stmfd  sp!, {r7, lr}
  mov  ip, #0
  add  r7, sp, #0
  mov  lr, r0
  b  L4
L5:
  ldrb  r3, [r1], #1  @ zero_extendqisi2
  add  ip, ip, #1
  and  r3, r3, #255
  strb  r3, [lr], #1
L4:
  cmp  ip, r2
  bne  L5
  ldmfd  sp!, {r7, pc}

