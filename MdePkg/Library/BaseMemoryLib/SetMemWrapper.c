/** @file
  SetMem() implementation.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  SetMemWrapper.c

  The following BaseMemoryLib instances share the same version of this file:

    BaseMemoryLib
    BaseMemoryLibMmx
    BaseMemoryLibSse2
    BaseMemoryLibRepStr
    PeiMemoryLib
    UefiMemoryLib

**/

#include "MemLibWrappers.h"

/**
  Set Buffer to Value for Size bytes.

  This function fills Length bytes of Buffer with Value, and returns Buffer.

  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer Memory to set.
  @param  Size Number of bytes to set
  @param  Value Value of the set operation.

  @return Buffer

**/
VOID *
EFIAPI
SetMem (
  IN      VOID                      *Buffer,
  IN      UINTN                     Size,
  IN      UINT8                     Value
  )
{
  ASSERT (Size <= MAX_ADDRESS - (UINTN)Buffer + 1);
  return InternalMemSetMem (Buffer, Size, Value);
}
