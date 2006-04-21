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



     

.global _ARShiftU64
_ARShiftU64: 
    movb    12(%esp),%cl
    movl    8(%esp),%eax
    cltd
    testb   $32,%cl
# MISMATCH: "    cmovz   edx, eax"
    cmovz   %eax, %edx
# MISMATCH: "    cmovz   eax, [esp + 4]"
    cmovz   4(%esp), %eax
    shrdl   %cl,%edx,%eax
    sar     %cl,%edx
    ret



