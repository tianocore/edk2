#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   CpuId.Asm
#
# Abstract:
#
#   AsmCpuid function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathSwapBytes64 (
#   IN      UINT64                    Operand
#   );
#------------------------------------------------------------------------------
.globl ASM_PFX(InternalMathSwapBytes64)
ASM_PFX(InternalMathSwapBytes64):
    movl    8(%esp), %eax
    movl    4(%esp), %edx
    bswapl  %eax
    bswapl  %edx
    ret
