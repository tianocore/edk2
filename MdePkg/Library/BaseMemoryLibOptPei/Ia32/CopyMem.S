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

.globl _InternalMemCopyMem

#------------------------------------------------------------------------------
#  VOID *
#  EFIAPI
#  InternalMemCopyMem (
#    IN VOID   *Destination,
#    IN VOID   *Source,
#    IN UINTN  Count
#    );
#------------------------------------------------------------------------------
_InternalMemCopyMem:
    push    %esi
    push    %edi
    movl    16(%esp), %esi              # esi <- Source
    movl    12(%esp), %edi              # edi <- Destination
    movl    20(%esp), %edx              # edx <- Count
    cmpl    %esi, %edi
    je      L_CopyDone
    cmpl    %edx, $0
    je      L_CopyDone
    leal    -1(%esi, %edx), %eax        # eax <- End of Source
    cmpl    %esi, %edi
    jae     L_CopyBytes
    cmpl    %eax, %edi
    jb      L_CopyBytes                 # Copy backward if overlapped
    movl    %esi, %eax                  # esi <- End of Source
    leal    -1(%edi, %edx), %edi        # edi <- End of Destination
    std
L_CopyBytes:
    movl    %ecx, %edx
    rep
    movsb                               # Copy bytes backward
    cld
L_CopyDone:
    movl    12(%esp), %eax              # eax <- Destination as return value
    pop     %edi
    pop     %esi
    ret
