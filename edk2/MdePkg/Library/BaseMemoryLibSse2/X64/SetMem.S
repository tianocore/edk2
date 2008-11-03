#
# ConvertAsm.py: Automatically generated from SetMem.asm
#
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
#   SetMem.S
#
# Abstract:
#
#   SetMem function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
#  VOID *
#  EFIAPI
#  InternalMemSetMem (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT8  Value
#    )
#------------------------------------------------------------------------------
.intel_syntax noprefix
.globl ASM_PFX(InternalMemSetMem)
ASM_PFX(InternalMemSetMem):
    push    rdi
    mov     rdi, rcx                    # rdi <- Buffer
    mov     al, r8b                     # al <- Value
    mov     r9, rdi                     # r9 <- Buffer as return value
    xor     rcx, rcx
    sub     rcx, rdi
    and     rcx, 15                     # rcx + rdi aligns on 16-byte boundary
    jz      L0
    cmp     rcx, rdx
    cmova   rcx, rdx
    sub     rdx, rcx
    rep     stosb
L0:
    mov     rcx, rdx
    and     rdx, 15
    shr     rcx, 4
    jz      L_SetBytes
    mov     ah, al                      # ax <- Value repeats twice
    movdqa  [rsp + 0x10], xmm0          # save xmm0
    movd    xmm0, eax                   # xmm0[0..16] <- Value repeats twice
    pshuflw xmm0, xmm0, 0               # xmm0[0..63] <- Value repeats 8 times
    movlhps xmm0, xmm0                  # xmm0 <- Value repeats 16 times
L1:
    movntdq [rdi], xmm0                 # rdi should be 16-byte aligned
    add     rdi, 16
    loop    L1
    mfence
    movdqa  xmm0, [rsp + 0x10]          # restore xmm0
L_SetBytes:
    mov     ecx, edx                    # high 32 bits of rcx are always zero
    rep     stosb
    mov     rax, r9                     # rax <- Return value
    pop     rdi
    ret
