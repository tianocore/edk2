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
#   LRotU64.asm
#
# Abstract:
#
#   64-bit left rotation for Ia32
#
#------------------------------------------------------------------------------

.globl _InternalMathLRotU64

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathLRotU64 (
#   IN      UINT64                    Operand,
#   IN      UINTN                     Count
#   );
#------------------------------------------------------------------------------
_InternalMathLRotU64:
    push    %ebx
    movb    16(%esp), %cl
    movl    12(%esp), %edx
    movl    8(%esp), %eax
    shldl   %cl, %edx, %ebx
    shldl   %cl, %eax, %edx
    rorl    %cl, %ebx
    shldl   %cl, %ebx, %eax
    testb   $32, %cl
    cmovnz  %eax, %ecx
    cmovnz  %edx, %eax
    cmovnz  %ecx, %edx
    pop     %ebx
    ret
