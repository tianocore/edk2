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
# VOID *
# EFIAPI
# InterlockedCompareExchangePointer (
#   IN      VOID                      **Value,
#   IN      VOID                      *CompareValue,
#   IN      VOID                      *ExchangeValue
#   );
#------------------------------------------------------------------------------
.global _InterlockedCompareExchangePointer
_InterlockedCompareExchangePointer: 
    #
    # InterlockedCompareExchangePointer() shares the same code as
    # InterlockedCompareExchange32() on IA32 and thus no code inside this
    # function
    #


#------------------------------------------------------------------------------
# UINT32
# EFIAPI
# InterlockedCompareExchange32 (
#   IN      UINT32                    *Value,
#   IN      UINT32                    CompareValue,
#   IN      UINT32                    ExchangeValue
#   );
#------------------------------------------------------------------------------
.global _InterlockedCompareExchange32
_InterlockedCompareExchange32: 
    movl    4(%esp),%ecx
    movl    8(%esp),%eax
    movl    12(%esp),%edx
    lock    cmpxchgl %edx,(%ecx)
    ret



