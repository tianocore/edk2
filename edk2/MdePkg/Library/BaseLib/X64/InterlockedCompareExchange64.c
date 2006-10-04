/** @file
  InterlockedCompareExchange64 function

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if _MSC_EXTENSIONS

//
// Microsoft Visual Studio 7.1 Function Prototypes for I/O Intrinsics
//
__int64 _InterlockedCompareExchange64(
   __int64 volatile * Destination,
   __int64 Exchange,
   __int64 Comperand
);

#pragma intrinsic(_InterlockedCompareExchange64)

UINT64
EFIAPI
InternalSyncCompareExchange64 (
  IN      UINT64                    *Value,
  IN      UINT64                    CompareValue,
  IN      UINT64                    ExchangeValue
  )
{
  return _InterlockedCompareExchange64 (Value, ExchangeValue, CompareValue);
}

#endif
