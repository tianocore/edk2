#
# ConvertAsm.py: Automatically generated from CopyMem.asm
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
.globl ASM_PFX(InternalMemCopyMem)
ASM_PFX(InternalMemCopyMem):
    push    rsi
    push    rdi
    mov     rsi, rdx                    # rsi <- Source
    mov     rdi, rcx                    # rdi <- Destination
    lea     r9, [rsi + r8 - 1]          # r9 <- End of Source
    cmp     rsi, rdi
    mov     rax, rdi                    # rax <- Destination as return value
    jae     _InternalMemCopyMem_al_0000
    cmp     r9, rdi
    jae     _atSym_CopyBackward         # Copy backward if overlapped
_InternalMemCopyMem_al_0000:
    mov     rcx, r8
    and     r8, 7
    shr     rcx, 3
    rep     movsq                       # Copy as many Qwords as possible
    jmp     _atSym_CopyBytes
_atSym_CopyBackward:
    mov     rsi, r9                     # rsi <- End of Source
    lea     rdi, [rdi + r8 - 1]         # esi <- End of Destination
    std                                 # set direction flag
_atSym_CopyBytes:
    mov     rcx, r8
    rep     movsb                       # Copy bytes backward
    cld
    pop     rdi
    pop     rsi
    ret

