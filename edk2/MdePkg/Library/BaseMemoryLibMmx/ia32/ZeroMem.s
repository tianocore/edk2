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
    movl    12(%esp), %ecx
    movl    %ecx, %edx
    shrl    $3, %ecx
    jz      @ZeroBytes
    pxor    %mm0, %mm0
L0: 
    movntq  %mm0, (%edi)
    addl    $8, %edi
    loop    L0
    mfence
@ZeroBytes: 
    andl    $7, %edx
    xorl    %eax, %eax
    movl    %edx, %ecx
    rep
    stosb
    movl    8(%esp), %eax
    pop     %edi
    ret
