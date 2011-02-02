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
  GCC_ASM_EXPORT(__divdi3)
  
ASM_PFX(__divdi3):
  @ args = 0, pretend = 0, frame = 0
  @ frame_needed = 1, uses_anonymous_args = 0
  stmfd  sp!, {r4, r5, r7, lr}
  mov  r4, r3, asr #31
  add  r7, sp, #8
  stmfd  sp!, {r10, r11}
  mov  r10, r1, asr #31
  sub  sp, sp, #8
  mov  r11, r10
  mov  r5, r4
  eor  r0, r0, r10
  eor  r1, r1, r10
  eor  r2, r2, r4
  eor  r3, r3, r4
  subs  r2, r2, r4
  sbc  r3, r3, r5
  mov  ip, #0
  subs  r0, r0, r10
  sbc  r1, r1, r11
  str  ip, [sp, #0]
  bl  ASM_PFX(__udivmoddi4)
  eor  r2, r10, r4
  eor  r3, r10, r4
  eor  r0, r0, r2
  eor  r1, r1, r3
  subs  r0, r0, r2
  sbc  r1, r1, r3
  sub  sp, r7, #16
  ldmfd  sp!, {r10, r11}
  ldmfd  sp!, {r4, r5, r7, pc}
