#
# ConvertAsm.py: Automatically generated from SetMem32.asm
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
#   SetMem32.S
#
# Abstract:
#
#   SetMem32 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
#  VOID *
#  EFIAPI
#  InternalMemSetMem32 (
#    IN VOID   *Buffer,
#    IN UINTN  Count,
#    IN UINT32 Value
#    );
#------------------------------------------------------------------------------
.intel_syntax noprefix
.globl ASM_PFX(InternalMemSetMem32)
ASM_PFX(InternalMemSetMem32):
    movq    mm0, r8                     # mm0 <- Value
    mov     rax, rcx                    # rax <- Buffer
    xchg    rcx, rdx                    # rcx <- Count  rdx <- Buffer
    shr     rcx, 1                      # rcx <- # of qwords to set
    jz      L_SetDwords
    pshufw  mm0, mm0, 0x44
L0:
    movntq  [rdx], mm0
    lea     rdx, [rdx + 8]              # use "lea" to avoid flag changes
    loop    L0
    mfence
L_SetDwords:
    jnc     L1
    movd    [rdx], mm0
L1:
    ret

