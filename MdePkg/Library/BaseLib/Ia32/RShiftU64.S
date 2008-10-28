#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation
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
#   RShiftU64.S
#
# Abstract:
#
#   64-bit logical right shift function for IA-32
#
#------------------------------------------------------------------------------

    .686:
    .code:

.globl ASM_PFX(InternalMathRShiftU64)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathRShiftU64 (
#   IN      UINT64                    Operand,
#   IN      UINTN                     Count
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalMathRShiftU64):
    movb    12(%esp), %cl               # cl <- Count
    xorl    %edx, %edx
    movl    8(%esp), %eax
    testb   $32, %cl                    # Count >= 32?
    cmovz   %eax, %edx
    cmovz   0x4(%esp), %eax
    shrdl   %cl, %edx, %eax
    shr     %cl, %edx
    ret
