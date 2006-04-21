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
#   DivError.asm
#
# Abstract:
#
#   Set error flag for all division functions
#
#------------------------------------------------------------------------------



     

.global _InternalMathDivRemU64x32
_InternalMathDivRemU64x32: 
    movl    12(%esp),%ecx
    movl    8(%esp),%eax
    xorl    %edx,%edx
    divl    %ecx
    pushl   %eax
    movl    8(%esp),%eax
    divl    %ecx
    movl    20(%esp),%ecx
    jecxz   L1
    movl    %edx,(%ecx)
L1: 
    popl    %edx
    ret



