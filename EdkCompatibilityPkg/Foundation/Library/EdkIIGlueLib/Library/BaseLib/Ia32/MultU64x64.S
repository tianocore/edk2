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
#   MultU64x64.asm
#
# Abstract:
#
#   Calculate the product of a 64-bit integer and another 64-bit integer
#
#------------------------------------------------------------------------------

#include <EdkIIGlueBase.h>

.globl ASM_PFX(InternalMathMultU64x64)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathMultU64x64 (
#   IN      UINT64                    Multiplicand,
#   IN      UINT64                    Multiplier
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalMathMultU64x64):
    push    %ebx
    movl    8(%esp), %ebx
    movl    16(%esp), %edx
    movl    %ebx, %ecx
    movl    %edx, %eax
    imull   20(%esp), %ebx
    imull   12(%esp), %edx
    addl    %edx, %ebx
    mull    %ecx
    addl    %ebx, %edx
    pop     %ebx
    ret
