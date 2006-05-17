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
#   SetMem.asm
#
# Abstract:
#
#   SetMem function
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
#  _mem_SetMem (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT8  Value
#    )
#------------------------------------------------------------------------------
.global _InternalMemSetMem
_InternalMemSetMem:
    push    %edi
    movl    12(%esp), %ecx              # ecx <- Count
    movl    8(%esp), %edi               # edi <- Buffer
    movl    %ecx, %edx
    shrl    $3, %ecx                    # # of Qwords to set
    movb    16(%esp), %al               # al <- Value
    jz      @SetBytes
    movb    %al, %ah                    # ax <- Value | (Value << 8)
    pushl   %ecx
    pushl   %ecx
    movq    %mm0, (%esp)                # save mm0
    movd    %eax, %mm0
    pshufw  $0x0,%mm0,%mm0
L0: 
    movntq  %mm0, (%edi)
    addl    $8, %edi
    loop    L0
    mfence
    movq    (%esp), %mm0                # restore mm0
    popl    %ecx                        # stack cleanup
    popl    %ecx
@SetBytes: 
    andl    $7, %edx
    movl    %edx, %ecx
    rep
    stosb
    movl    8(%esp), %eax               # eax <- Buffer as return value
    pop     %edi
    ret
