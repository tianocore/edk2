/** @file
  Implementation of synchronization functions on EBC.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  Synchronization.c

**/

UINT32
EFIAPI
InternalSyncCompareExchange32 (
  IN      volatile UINT32           *Value,
  IN      UINT32                    CompareValue,
  IN      UINT32                    ExchangeValue
  )
{
  return *Value != CompareValue ? *Value :
           ((*Value = ExchangeValue), CompareValue);
}

UINT64
EFIAPI
InternalSyncCompareExchange64 (
  IN      volatile UINT64           *Value,
  IN      UINT64                    CompareValue,
  IN      UINT64                    ExchangeValue
  )
{
  return *Value != CompareValue ? *Value :
           ((*Value = ExchangeValue), CompareValue);
}

UINT32
EFIAPI
InternalSyncIncrement (
  IN      volatile UINT32           *Value
  )
{
  return ++*Value;
}

UINT32
EFIAPI
InternalSyncDecrement (
  IN      volatile UINT32           *Value
  )
{
  return --*Value;
}
