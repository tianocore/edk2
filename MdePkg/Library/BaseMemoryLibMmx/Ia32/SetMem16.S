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
    movl    %edx, %ecx
    andl    $3, %edx
    shrl    $2, %ecx
    movl    16(%esp), %eax
    jz      @SetWords
    movd    %eax, %mm0
    pshufw  $0, %mm0, %mm0
L0: 
    movntq  %mm0, (%edi)
    addl    $8, %edi
    loop    L0
    mfence
@SetWords: 
    movl    %edx, %ecx
    rep
    stosw
    movl    8(%esp), %eax
    pop     %edi
    ret
