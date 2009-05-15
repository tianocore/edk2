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
#   ScanMem32.Asm
#
# Abstract:
#
#   ScanMem32 function
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

.globl ASM_PFX(InternalMemScanMem32)

#------------------------------------------------------------------------------
# CONST VOID *
# EFIAPI
# InternalMemScanMem32 (
#   IN      CONST VOID                *Buffer,
#   IN      UINTN                     Length,
#   IN      UINT32                    Value
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalMemScanMem32):
    push    %edi
    movl    12(%esp), %ecx
    movl    8(%esp), %edi
    movl    16(%esp), %eax
    repne   scasl
    leal    -4(%edi), %eax
    cmovnz  %ecx, %eax
    pop     %edi
    ret
