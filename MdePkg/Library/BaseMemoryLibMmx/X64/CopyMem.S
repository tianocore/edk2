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


#------------------------------------------------------------------------------
# VOID *
# EFIAPI
# InternalMemCopyMem (
#   OUT     VOID                      *DestinationBuffer,
#   IN      CONST VOID                *SourceBuffer,
#   IN      UINTN                     Length
#   );
#------------------------------------------------------------------------------
.global _InternalMemCopyMem;
_InternalMemCopyMem:
    push    %rsi
    push    %rdi
    mov     %rdi, %rsi
    mov     %rcx, %rdi
    lea     -1(%r8,%rsi,1),%r9
    cmp     %rdi, %rsi
    mov     %rdi, %rax
    jae     L1
    cmp     %rdi, %r9
    jae     LCopyBackward               # Copy backward if overlapped
L1: 
    mov     %r8, %rcx
    and     $7, %r8
    shr     $3, %rcx
    jz      LCopyBytes
    movd    %mm0, %r10                  # (Save mm0 in r10)
L2: 
    movd    (%rsi), %mm0
    movntq  %mm0, (%rdi)
    add     $8, %rsi
    add     $8, %rdi
    loop    L2
    mfence
    movd    %r10, %mm0                  #(Restore mm0)
    jmp     LCopyBytes
LCopyBackward: 
    mov     %r9, %rsi
    lea     -1(%r8,%rdi,1),%rdi
    std                                 # set direction flag
LCopyBytes: 
    mov     %r8, %rcx
    rep
    movsb                               # Copy bytes backward
    cld
    pop     %rdi
    pop     %rsi
    ret
