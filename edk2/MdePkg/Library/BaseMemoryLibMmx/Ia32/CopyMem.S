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
#   CopyMem.asm
#
# Abstract:
#
#   CopyMem function
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
#  _mem_CopyMem (
#    IN VOID   *Destination,
#    IN VOID   *Source,
#    IN UINTN  Count
#    )
#------------------------------------------------------------------------------
.global _InternalMemCopyMem
_InternalMemCopyMem:
    push    %esi
    push    %edi
    movl    16(%esp), %esi              # esi <- Source
    movl    12(%esp), %edi              # edi <- Destination
    movl    20(%esp), %edx              # edx <- Count
    leal    -1(%edi,%edx,), %eax        # eax <- End of Destination
    cmpl    %edi, %esi
    jae     L0
    cmpl    %esi, %eax                  # Overlapped?
    jae     @CopyBackward               # Copy backward if overlapped
L0: 
    xorl    %ecx, %ecx
    subl    %esi, %ecx
    andl    $7, %ecx                    # ecx + esi aligns on 8-byte boundary
    jz      L1
    cmpl    %edx, %ecx
    cmova   %edx, %ecx
    subl    %ecx, %edx                  # edx <- remaining bytes to copy
    rep
    movsb
L1: 
    movl    %edx, %ecx
    andl    $7, %edx
    shrl    $3, %ecx                    # ecx <- # of Qwords to copy
    jz      @CopyBytes
    pushl   %eax
    pushl   %eax
    movq    %mm0, (%esp)                # save mm0
L2: 
    movq    (%esi), %mm0
    movntq  %mm0, (%edi)
    addl    $8, %esi
    addl    $8, %edi
    loop    L2
    mfence
    movq    (%esp), %mm0                # restore mm0
    popl    %ecx                        # stack cleanup
    popl    %ecx                        # stack cleanup
    jmp     @CopyBytes
@CopyBackward: 
    movl    %eax, %edi                  # edi <- Last byte in Destination
    leal    -1(%esi,%edx,), %esi        # esi <- Last byte in Source
    std
@CopyBytes: 
    movl    %edx, %ecx
    rep
    movsb
    cld
    movl    12(%esp), %eax
    pop    %esi
    pop    %edi
    ret
