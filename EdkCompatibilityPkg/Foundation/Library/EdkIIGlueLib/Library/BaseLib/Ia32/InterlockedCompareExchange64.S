#------------------------------------------------------------------------------
#
# Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   InterlockedCompareExchange64.Asm
#
# Abstract:
#
#   InternalSyncCompareExchange64 function
#
# Notes:
#
#------------------------------------------------------------------------------

#include <EdkIIGlueBase.h>

.globl ASM_PFX(InternalSyncCompareExchange64)

#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InternalSyncCompareExchange64 (
#   IN      UINT64                    *Value,
#   IN      UINT64                    CompareValue,
#   IN      UINT64                    ExchangeValue
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalSyncCompareExchange64):
    push    %esi
    push    %ebx
    movl    12(%esp), %esi
    movl    16(%esp), %eax
    movl    20(%esp), %edx
    movl    24(%esp), %ebx
    movl    28(%esp), %ecx
    lock
    cmpxchg8b   (%esi)
    pop     %ebx
    pop     %esi
    ret
