#------------------------------------------------------------------------------
#
# Copyright (c) 2013 - 2014, ARM Limited. All rights reserved.
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

#include <AsmMacroIoLibV8.h>

.text
.align 3
GCC_ASM_EXPORT (ArmReadCpuExCr)
GCC_ASM_EXPORT (ArmWriteCpuExCr)

ASM_PFX(ArmReadCpuExCr):
  mrs   x0, S3_1_c15_c2_1
  ret

ASM_PFX(ArmWriteCpuExCr):
  msr   S3_1_c15_c2_1, x0
  dsb   sy
  isb
  ret

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
