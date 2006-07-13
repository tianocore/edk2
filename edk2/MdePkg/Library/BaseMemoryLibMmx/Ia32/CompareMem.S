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
#   CompareMem.Asm
#
# Abstract:
#
#   CompareMem function
#
# Notes:
#
#   The following BaseMemoryLib instances share the same version of this file:
#
#       BaseMemoryLibRepStr
#       BaseMemoryLibMmx
#       BaseMemoryLibSse2
#
#------------------------------------------------------------------------------

    .686: 
    #.MODEL flat,C
    .code: 

.global _InternalMemCompareMem
_InternalMemCompareMem:
    push    %esi
    push    %edi
    movl    12(%esp), %esi
    movl    16(%esp), %edi
    movl    20(%esp), %ecx
    repe    cmpsb
    movzbl  -1(%esi), %eax
    movzbl  -1(%edi), %edx
    subl    %edx, %eax
    pop     %edi
    pop     %esi
    ret


