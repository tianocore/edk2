#------------------------------------------------------------------------------
#
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#------------------------------------------------------------------------------

ASM_GLOBAL ASM_PFX(internal_FPU_rmode)
ASM_PFX(internal_FPU_rmode):
  subl $4,%esp
  fnstcw (%esp)
  movl (%esp),%eax
  shrl $10,%eax
  andl $3,%eax
  addl $4,%esp
  ret
