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
    lea     r9, [rsi + r8 - 1]          # r9 <- Last byte of Source
    cmp     rsi, rdi                    
    mov     rax, rdi                    # rax <- Destination as return value
    jae     _InternalMemCopyMem_al_0000 # Copy forward if Source > Destination
    cmp     r9, rdi                     # Overlapped?
    jae     _atSym_CopyBackward         # Copy backward if overlapped
_InternalMemCopyMem_al_0000:            
    xor     rcx, rcx                    
    sub     rcx, rdi                    # rcx <- -rdi
    and     rcx, 15                     # rcx + rsi should be 16 bytes aligned
    jz      _InternalMemCopyMem_al_0001 # skip if rcx == 0
    cmp     rcx, r8
    cmova   rcx, r8
    sub     r8, rcx
    rep     movsb
_InternalMemCopyMem_al_0001:
    mov     rcx, r8
    and     r8, 15
    shr     rcx, 4                      # rcx <- # of DQwords to copy
    jz      _atSym_CopyBytes
    movdqa  [rsp + 0x18], xmm0           # save xmm0 on stack
_InternalMemCopyMem_al_0002:
    movdqu  xmm0, [rsi]                 # rsi may not be 16-byte aligned
    movntdq [rdi], xmm0                 # rdi should be 16-byte aligned
    add     rsi, 16
    add     rdi, 16
    loop    _InternalMemCopyMem_al_0002
    mfence
    movdqa  xmm0, [rsp + 0x18]           # restore xmm0
    jmp     _atSym_CopyBytes            # copy remaining bytes
_atSym_CopyBackward:
    mov     rsi, r9                     # rsi <- Last byte of Source
    lea     rdi, [rdi + r8 - 1]         # rdi <- Last byte of Destination
    std
_atSym_CopyBytes:
    mov     rcx, r8
    rep     movsb
    cld
    pop     rdi
    pop     rsi
    ret
