/** @file
  Implementation of TestAndClearBit using compare-exchange primitive

  Copyright (C) 2015, Linaro Ltd.
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/SynchronizationLib.h>

INT32
EFIAPI
TestAndClearBit (
  IN INT32            Bit,
  IN VOID             *Address
  )
{
  UINT16    Word, Read;
  UINT16    Mask;

  //
  // Calculate the effective address relative to 'Address' based on the
  // higher order bits of 'Bit'. Use signed shift instead of division to
  // ensure we round towards -Inf, and end up with a positive shift in
  // 'Bit', even if 'Bit' itself is negative.
  //
  Address = (VOID*)((UINT8*) Address + ((Bit >> 4) * sizeof(UINT16)));
  Mask = 1U << (Bit & 15);

  for (Word = *(UINT16 *) Address; Word & Mask; Word = Read) {
    Read = InterlockedCompareExchange16 (Address, Word, Word & ~Mask);
    if (Read == Word) {
      return 1;
    }
  }
  return 0;
}
