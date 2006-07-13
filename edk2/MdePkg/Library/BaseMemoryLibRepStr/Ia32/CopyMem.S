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
#   CopyMem.Asm
#
# Abstract:
#
#   CopyMem function
#
# Notes:
#
#------------------------------------------------------------------------------

    .386: 
    .code: 

.global _InternalMemCopyMem
_InternalMemCopyMem:
    push    %esi
    push    %edi
    movl    16(%esp),%esi               # esi <- Source
    movl    12(%esp),%edi               # edi <- Destination
    movl    20(%esp),%edx               # edx <- Count
    leal    -1(%edi,%edx),%eax          # eax <- End of Destination
    cmpl    %edi,%esi
    jae     L0
    cmpl    %esi,%eax
    jae     @CopyBackward               # Copy backward if overlapped
L0: 
    movl    %edx,%ecx
    andl    $3,%edx
    shrl    $2,%ecx
    rep
    movsl                               # Copy as many Dwords as possible
    jmp     @CopyBytes
@CopyBackward: 
    movl    %eax,%edi                   # edi <- End of Destination
    leal    -1(%esi,%edx),%esi          # esi <- End of Source
    std
@CopyBytes: 
    movl    %edx,%ecx
    rep
    movsb                               # Copy bytes backward
    cld
    movl    12(%esp),%eax               # eax <- Destination as return value
    pop     %edi
    pop     %esi
    ret
