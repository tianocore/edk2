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
ASM_GLOBAL ASM_PFX(InternalMemSetMem16)
ASM_PFX(InternalMemSetMem16):
    push    rdi
    mov     rdi, rcx
    mov     rax, r8
    xchg    rcx, rdx
    rep     stosw
    mov     rax, rdx
    pop     rdi
    ret

