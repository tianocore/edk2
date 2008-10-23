#
# ConvertAsm.py: Automatically generated from SetMem64.asm
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
#   SetMem64.S
#
# Abstract:
#
#   SetMem64 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
#  VOID *
#  InternalMemSetMem64 (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT64 Value
#    )
#------------------------------------------------------------------------------
.intel_syntax noprefix
.globl ASM_PFX(InternalMemSetMem64)
ASM_PFX(InternalMemSetMem64):
    mov     rax, rcx                    # rax <- Buffer
    xchg    rcx, rdx                    # rcx <- Count & rdx <- Buffer
    test    dl, 8
    movq    xmm0, r8
    jz      L0
    mov     [rdx], r8
    add     rdx, 8
    dec     rcx
L0:
    shr     rcx, 1
    jz      L_SetQwords
    movlhps xmm0, xmm0
L1:
    movntdq [rdx], xmm0
    lea     rdx, [rdx + 16]
    loop    L1
    mfence
L_SetQwords:
    jnc     L2
    mov     [rdx], r8
L2:
    ret

