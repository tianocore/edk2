#
# ConvertAsm.py: Automatically generated from CopyMem.asm
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
#   CopyMem.S
#
# Abstract:
#
#   CopyMem function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
#  VOID *
#  EFIAPI
#  InternalMemCopyMem (
#    IN VOID   *Destination,
#    IN VOID   *Source,
#    IN UINTN  Count
#    )
#------------------------------------------------------------------------------
.intel_syntax noprefix
ASM_GLOBAL ASM_PFX(InternalMemCopyMem)
ASM_PFX(InternalMemCopyMem):
    push    rsi
    push    rdi
    mov     rsi, rdx                    # rsi <- Source
    mov     rdi, rcx                    # rdi <- Destination
    lea     r9, [rsi + r8 - 1]          # r9 <- End of Source
    cmp     rsi, rdi
    mov     rax, rdi                    # rax <- Destination as return value
    jae     L0
    cmp     r9, rdi
    jae     L_CopyBackward              # Copy backward if overlapped
L0:
    mov     rcx, r8
    and     r8, 7
    shr     rcx, 3                      # rcx <- # of Qwords to copy
    jz      L_CopyBytes
    movd    r10, mm0                    # (Save mm0 in r10)
L1:
    movq    mm0, [rsi]
    movntq  [rdi], mm0
    add     rsi, 8
    add     rdi, 8
    loop    L1
    mfence
    movd    mm0, r10                    # (Restore mm0)
    jmp     L_CopyBytes
L_CopyBackward:
    mov     rsi, r9                     # rsi <- End of Source
    lea     rdi, [rdi + r8 - 1]         # rdi <- End of Destination
    std                                 # set direction flag
L_CopyBytes:
    mov     rcx, r8
    rep     movsb                       # Copy bytes backward
    cld
    pop     rdi
    pop     rsi
    ret
