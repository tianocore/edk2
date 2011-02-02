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
  GCC_ASM_EXPORT(__ucmpdi2)
  
ASM_PFX(__ucmpdi2):
  stmfd  sp!, {r4, r5, r8, lr}
  cmp  r1, r3
  mov  r8, r0
  mov  r4, r2
  mov  r5, r3
  bcc  L2
  bhi  L4
  cmp  r0, r2
  bcc  L2
  movls  r0, #1
  bls  L8
  b  L4
L2:
  mov  r0, #0
  b  L8
L4:
  mov  r0, #2
L8:
  ldmfd  sp!, {r4, r5, r8, pc}
