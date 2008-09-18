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
#   CompareMem.Asm
#
# Abstract:
#
#   CompareMem function
#
# Notes:
#
#   The following BaseMemoryLib instances contain the same copy of this file:
#
#       BaseMemoryLibRepStr
#       BaseMemoryLibMmx
#       BaseMemoryLibSse2
#       BaseMemoryLibOptDxe
#       BaseMemoryLibOptPei
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# INTN
# EFIAPI
# InternalMemCompareMem (
#   IN      CONST VOID                *DestinationBuffer,
#   IN      CONST VOID                *SourceBuffer,
#   IN      UINTN                     Length
#   );
#------------------------------------------------------------------------------
.global _InternalMemCompareMem;
_InternalMemCompareMem:
    push    %rsi
    push    %rdi
    mov     %rcx, %rsi
    mov     %rdx, %rdi
    mov     %r8, %rcx
    repe    cmpsb
    movzbq  -1(%rsi), %rax
    movzbq  -1(%rdi), %rdx
    sub     %rdx, %rax
    pop     %rdi
    pop     %rsi
    ret
