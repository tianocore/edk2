#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   DivU64x32.asm
#
# Abstract:
#
#   Calculate the quotient of a 64-bit integer by a 32-bit integer
#
#------------------------------------------------------------------------------

#include <EdkIIGlueBase.h>

.globl ASM_PFX(InternalMathDivU64x32)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathDivU64x32 (
#   IN      UINT64                    Dividend,
#   IN      UINT32                    Divisor
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalMathDivU64x32):
    movl    8(%esp), %eax
    movl    12(%esp), %ecx
    xorl    %edx, %edx
    divl    %ecx
    push    %eax
    movl    8(%esp), %eax
    divl    %ecx
    pop     %edx
    ret
