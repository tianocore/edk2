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
#   ZeroMem.asm
#
# Abstract:
#
#   ZeroMem function
#
# Notes:
#
#------------------------------------------------------------------------------

    .686: 
    #.MODEL flat,C
    .xmm: 
    .code: 

#------------------------------------------------------------------------------
#  VOID *
#  _mem_ZeroMem (
#    IN VOID   *Buffer,
#    IN UINTN  Count
#    )
#------------------------------------------------------------------------------
.global _InternalMemZeroMem
_InternalMemZeroMem:
    push    %edi
    movl    8(%esp), %edi
    movl    12(%esp), %edx
    xorl    %ecx, %ecx
    subl    %edi, %ecx
    xorl    %eax, %eax
    andl    $15, %ecx
    jz      L0
    cmpl    %edx, %ecx
    cmova   %edx, %ecx
    subl    %ecx, %edx
    rep
    stosb
L0: 
    movl    %edx, %ecx
    andl    $15, %edx
    shrl    $4, %ecx
    jz      @ZeroBytes
    pxor    %xmm0, %xmm0
L1: 
    movntdq %xmm0, (%edi)
    addl    $16, %edi
    loop    L1
    mfence
@ZeroBytes: 
    movl    %edx, %ecx
    rep
    stosb
    movl    8(%esp), %eax
    pop     %edi
    ret
