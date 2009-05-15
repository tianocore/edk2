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
#   InterlockedCompareExchange32 function
#
# Notes:
#
#------------------------------------------------------------------------------

#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# InterlockedCompareExchange32 (
#   IN      UINT32                    *Value,
#   IN      UINT32                    CompareValue,
#   IN      UINT32                    ExchangeValue
#   );
#------------------------------------------------------------------------------
.global ASM_PFX(InternalSyncCompareExchange32)
ASM_PFX(InternalSyncCompareExchange32):
    mov     %edx, %eax
    lock    cmpxchg %r8d, (%rcx)
    ret
