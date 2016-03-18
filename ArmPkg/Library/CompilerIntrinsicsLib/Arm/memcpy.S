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
  GCC_ASM_EXPORT(__aeabi_memcpy)
  GCC_ASM_EXPORT(memcpy)

ASM_PFX(__aeabi_memcpy):
ASM_PFX(memcpy):
  cmp     r2, #0
  bxeq    lr
  push    {lr}
  mov     lr, r0
L5:
  ldrb  r3, [r1], #1  @ zero_extendqisi2
  strb  r3, [lr], #1
  subs r2, r2, #1
  bne  L5
  pop  {pc}

