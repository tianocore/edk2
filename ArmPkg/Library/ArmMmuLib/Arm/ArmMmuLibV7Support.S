#------------------------------------------------------------------------------
#
# Copyright (c) 2016, Linaro Limited. All rights reserved.
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

#include <AsmMacroIoLib.h>

.text
.align 2

GCC_ASM_EXPORT (ArmReadIdMmfr0)
GCC_ASM_EXPORT (ArmHasMpExtensions)

#------------------------------------------------------------------------------

ASM_PFX (ArmHasMpExtensions):
  mrc     p15,0,R0,c0,c0,5
  // Get Multiprocessing extension (bit31)
  lsr     R0, R0, #31
  bx      LR

ASM_PFX(ArmReadIdMmfr0):
  mrc    p15, 0, r0, c0, c1, 4     @ Read ID_MMFR0 Register
  bx     lr

ASM_FUNCTION_REMOVE_IF_UNREFERENCED
