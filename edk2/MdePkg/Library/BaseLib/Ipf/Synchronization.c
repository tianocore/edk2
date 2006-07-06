/** @file
  Implementation of synchronization functions on Itanium.

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
  );

UINT32
EFIAPI
InternalSyncIncrement (
  IN      volatile UINT32           *Value
  )
{
  UINT32                            OriginalValue;

  do {
    OriginalValue = *Value;
  } while (OriginalValue != InternalSyncCompareExchange32 (
                              Value,
                              OriginalValue,
                              OriginalValue + 1
                              ));
  return OriginalValue + 1;
}

UINT32
EFIAPI
InternalSyncDecrement (
  IN      volatile UINT32           *Value
  )
{
  UINT32                            OriginalValue;

  do {
    OriginalValue = *Value;
  } while (OriginalValue != InternalSyncCompareExchange32 (
                              Value,
                              OriginalValue,
                              OriginalValue - 1
                              ));
  return OriginalValue - 1;
}
