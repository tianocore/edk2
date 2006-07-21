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
#   ARShiftU64.asm
#
# Abstract:
#
#   64-bit arithmetic right shift function for IA-32
#
#------------------------------------------------------------------------------

.global _InternalMathARShiftU64

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathARShiftU64 (
#   IN      UINT64                    Operand,
#   IN      UINTN                     Count
#   );
#------------------------------------------------------------------------------
_InternalMathARShiftU64:
    movb    12(%esp), %cl
    movl    8(%esp), %eax
    cltd
    testb   $32, %cl
    cmovz   %eax, %edx
    cmovz   4(%esp), %eax
    shrdl   %cl, %edx, %eax
    sar     %cl, %edx
    ret
