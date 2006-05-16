/** @file
  ScanMem32() implementation.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  ScanMem32Wrapper.c

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
  Scans a target buffer for a 32-bit value, and returns a pointer to the
  matching 32-bit value in the target buffer.

  This function searches target the buffer specified by Buffer and Length from
  the lowest address to the highest address at 32-bit increments for a 32-bit
  value that matches Value. If a match is found, then a pointer to the matching
  value in the target buffer is returned. If no match is found, then NULL is
  returned. If Length is 0, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer  Pointer to the target buffer to scan.
  @param  Length  Number of bytes in Buffer to scan.
  @param  Value   Value to search for in the target buffer.

  @return Pointer to the first occurrence or NULL if not found.
  @retval NULL  if Length == 0 or Value was not found.

**/
VOID *
EFIAPI
ScanMem32 (
  IN      CONST VOID                *Buffer,
  IN      UINTN                     Length,
  IN      UINT32                    Value
  )
{
  ASSERT (Buffer != NULL);
  ASSERT (((UINTN)Buffer & (sizeof (Value) - 1)) == 0);
  ASSERT (Length <= MAX_ADDRESS + (UINTN)Buffer + 1);

  if ((Length /= sizeof (Value)) == 0) {
    return NULL;
  }
  return (VOID*)InternalMemScanMem32 (Buffer, Length, Value);
}
