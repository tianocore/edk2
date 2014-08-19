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
  GCC_ASM_EXPORT(__umoddi3)

ASM_PFX(__umoddi3):
  stmfd  sp!, {r7, lr}
  add  r7, sp, #0
  sub  sp, sp, #16
  add  ip, sp, #8
  str  ip, [sp, #0]
  bl  ASM_PFX(__udivmoddi4)
  ldrd  r0, [sp, #8]
  sub  sp, r7, #0
  ldmfd  sp!, {r7, pc}

