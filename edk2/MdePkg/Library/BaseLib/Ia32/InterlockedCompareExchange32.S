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
#   InterlockedCompareExchange32.Asm
#
# Abstract:
#
#   InternalSyncCompareExchange32 function
#
# Notes:
#
#------------------------------------------------------------------------------

.globl ASM_PFX(InternalSyncCompareExchange32)

#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# InternalSyncCompareExchange32 (
#   IN      UINT32                    *Value,
#   IN      UINT32                    CompareValue,
#   IN      UINT32                    ExchangeValue
#   );
#------------------------------------------------------------------------------
ASM_PFX(InternalSyncCompareExchange32):
    movl    4(%esp), %ecx
    movl    8(%esp), %eax
    movl    12(%esp), %edx
    lock
    cmpxchgl    %edx, (%ecx)
    ret
