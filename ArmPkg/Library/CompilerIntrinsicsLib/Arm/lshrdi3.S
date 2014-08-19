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
  GCC_ASM_EXPORT(__lshrdi3)

ASM_PFX(__lshrdi3):
  cmp  r2, #31
  bls  L2
  cmp  r2, #63
  subls  r2, r2, #32
  movls  r2, r1, lsr r2
  movhi  r2, #0
  mov  r0, r2
  mov  r1, #0
  bx  lr
L2:
  cmp  r2, #0
  rsbne  r3, r2, #32
  movne  r3, r1, asl r3
  movne  r1, r1, lsr r2
  orrne  r0, r3, r0, lsr r2
  bx  lr
