/** @file
  ZeroMem() implementation.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  ZeroMemWrapper.c

  The following BaseMemoryLib instances share the same version of this file:

    BaseMemoryLib
    BaseMemoryLibMmx
    BaseMemoryLibSse2
    BaseMemoryLibRepStr
    PeiMemoryLib
    UefiMemoryLib

**/

#include "MemLibInternals.h"

/**
  Set Buffer to 0 for Size bytes.

  This function fills Length bytes of Buffer with zeros, and returns Buffer.

  If Buffer is NULL and Length > 0, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer Memory to set.
  @param  Size Number of bytes to set

  @return Buffer

**/
VOID *
EFIAPI
ZeroMem (
  IN      VOID                      *Buffer,
  IN      UINTN                     Length
  )
{
  ASSERT (!(Buffer == NULL && Length > 0));
  ASSERT (Length <= (MAX_ADDRESS - (UINTN)Buffer + 1));
  return InternalMemSetMem (Buffer, Length, 0);
}
