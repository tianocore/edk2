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
#   SetMem16.asm
#
# Abstract:
#
#   SetMem16 function
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
#  _mem_SetMem16 (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT16 Value
#    )
#------------------------------------------------------------------------------
.global _InternalMemSetMem16
_InternalMemSetMem16:
    push    %edi
    movl    12(%esp), %edx
    movl    8(%esp), %edi
    xorl    %ecx, %ecx
    subl    %edi, %ecx
    andl    $15, %ecx                   # ecx + edi aligns on 16-byte boundary
    movl    16(%esp), %eax
    jz      L0
    shrl    %ecx
    cmpl    %edx, %ecx
    cmova   %edx, %ecx
    subl    %ecx, %edx
    rep
    stosw
L0: 
    movl    %edx, %ecx
    andl    $7, %edx
    shrl    $3, %ecx
    jz      @SetWords
    movd    %eax, %xmm0
    pshuflw $0, %xmm0, %xmm0
    movlhps %xmm0, %xmm0
L1:
    movntdq %xmm0, (%edi)
    addl    $16, %edi
    loop    L1
    mfence
@SetWords: 
    movl    %edx, %ecx
    rep
    stosw
    movl    8(%esp), %eax
    pop     %edi
    ret
