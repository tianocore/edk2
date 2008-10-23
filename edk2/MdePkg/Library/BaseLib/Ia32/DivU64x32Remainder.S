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
#   DivError.S
#
# Abstract:
#
#   Set error flag for all division functions
#
#------------------------------------------------------------------------------

.globl ASM_PFX(InternalMathDivRemU64x32)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalMathDivRemU64x32 (
#   IN      UINT64                    Dividend,
#   IN      UINT32                    Divisor,
#   OUT     UINT32                    *Remainder
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalMathDivRemU64x32):
    movl    12(%esp), %ecx
    movl    8(%esp), %eax
    xorl    %edx, %edx
    divl    %ecx
    push    %eax
    movl    8(%esp), %eax
    divl    %ecx
    movl    20(%esp), %ecx
    jecxz   L1
    movl    %edx, (%ecx)
L1:
    pop     %edx
    ret
