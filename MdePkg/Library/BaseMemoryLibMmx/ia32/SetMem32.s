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
#   SetMem32.asm
#
# Abstract:
#
#   SetMem32 function
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
#  _mem_SetMem32 (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT32 Value
#    )
#------------------------------------------------------------------------------
.global _InternalMemSetMem32
_InternalMemSetMem32:
    push    %edi
    movl    12(%esp), %edx
    movl    8(%esp), %edi
    movl    %edx, %ecx
    shrl    %ecx
    movd    16(%esp), %mm0
    movl    %edi, %eax
    jz      @SetDwords
    pshufw  $0x44, %mm0, %mm0
L0: 
    movntq  %mm0, (%edi)
    addl    $8, %edi
    loopl   L0
    mfence
@SetDwords: 
    testb   $1, %dl
    jz      @F
    movd    %mm0, (%edi)
L1: 
    pop     %edi
    ret
