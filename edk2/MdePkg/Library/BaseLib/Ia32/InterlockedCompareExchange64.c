/** @file
  InterlockedCompareExchange64 function

  Copyright (c) 2006 - 2007, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// Include common header file for this module.
//


UINT64
EFIAPI
InternalSyncCompareExchange64 (
  IN      UINT64                    *Value,
  IN      UINT64                    CompareValue,
  IN      UINT64                    ExchangeValue
  )
{
  _asm {
    mov     esi, Value
    mov     eax, dword ptr [CompareValue + 0]
    mov     edx, dword ptr [CompareValue + 4]
    mov     ebx, dword ptr [ExchangeValue + 0]
    mov     ecx, dword ptr [ExchangeValue + 4]
    lock    cmpxchg8b   qword ptr [esi]
  }
}
