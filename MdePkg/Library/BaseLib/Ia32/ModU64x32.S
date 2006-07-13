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
#   DivU64x32.S
#
# Abstract:
#
#   Calculate the remainder of a 64-bit integer by a 32-bit integer
#
#------------------------------------------------------------------------------



     

.global _InternalMathModU64x32
_InternalMathModU64x32: 
    movl    8(%esp),%eax
    movl    12(%esp),%ecx
    xorl    %edx,%edx
    divl    %ecx
    movl    4(%esp),%eax
    divl    %ecx
    movl    %edx,%eax
    ret



