#
# ConvertAsm.py: Automatically generated from SetMem16.asm
#
#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2008, Intel Corporation
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
#   SetMem16.S
#
# Abstract:
#
#   SetMem16 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
#  VOID *
#  EFIAPI
#  InternalMemSetMem16 (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT16 Value
#    )
#------------------------------------------------------------------------------
.intel_syntax noprefix
.globl ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    rdi
    mov     rdi, rcx
    mov     r9, rdi
    xor     rcx, rcx
    sub     rcx, rdi
    and     rcx, 15
    mov     rax, r8
    jz      L0
    shr     rcx, 1
    cmp     rcx, rdx
    cmova   rcx, rdx
    sub     rdx, rcx
    rep     stosw
L0:
    mov     rcx, rdx
    and     edx, 7
    shr     rcx, 3
    jz      L_SetWords
    movd    xmm0, eax
    pshuflw xmm0, xmm0, 0
    movlhps xmm0, xmm0
L1:
    movntdq [rdi], xmm0
    add     rdi, 16
    loop    L1
    mfence
L_SetWords:
    mov     ecx, edx
    rep     stosw
    mov     rax, r9
    pop     rdi
    ret
