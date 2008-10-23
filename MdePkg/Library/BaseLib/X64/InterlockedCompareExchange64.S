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
#   InterlockedCompareExchange64.S
#
# Abstract:
#
#   InterlockedCompareExchange64 function
#
# Notes:
#
#------------------------------------------------------------------------------


#------------------------------------------------------------------------------
# UINT64
# EFIAPI
# InterlockedCompareExchange64 (
#   IN      UINT64                    *Value,
#   IN      UINT64                    CompareValue,
#   IN      UINT64                    ExchangeValue
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(InternalSyncCompareExchange64)
.align 16
ASM_PFX(InternalSyncCompareExchange64):
    mov     %rdx, %rax
    lock    cmpxchg %r8,(%rcx)
    ret
