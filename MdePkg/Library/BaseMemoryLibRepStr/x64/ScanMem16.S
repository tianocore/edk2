#
# ConvertAsm.py: Automatically generated from ScanMem16.asm
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
#   ScanMem16.S
#
# Abstract:
#
#   ScanMem16 function
#
# Notes:
#
#   The following BaseMemoryLib instances share the same version of this file:
#
#       BaseMemoryLibRepStr
#       BaseMemoryLibMmx
#       BaseMemoryLibSse2
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# CONST VOID *
# EFIAPI
# InternalMemScanMem16 (
#   IN      CONST VOID                *Buffer,
#   IN      UINTN                     Length,
#   IN      UINT16                    Value
#   );
#------------------------------------------------------------------------------
.intel_syntax
.globl _InternalMemScanMem16
_InternalMemScanMem16:
    push    rdi
    mov     rdi, rcx
    mov     rax, r8
    mov     rcx, rdx
    repne   scasw
    lea     rax, [rdi - 2]
    cmovnz  rax, rcx
    pop     rdi
    ret

