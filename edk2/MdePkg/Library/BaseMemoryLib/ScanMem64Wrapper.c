/** @file
  ScanMem64() implementation.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  The following BaseMemoryLib instances share the same version of this file:

    BaseMemoryLib
    BaseMemoryLibMmx
    BaseMemoryLibSse2
    BaseMemoryLibRepStr
    PeiMemoryLib
    DxeMemoryLib

**/

//
// Include common header file for this module.
//


#include "MemLibInternals.h"

/**
  Scans a target buffer for a 64-bit value, and returns a pointer to the matching 64-bit value
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from the lowest
  address to the highest address for a 64-bit value that matches Value.  If a match is found,
  then a pointer to the matching byte in the target buffer is returned.  If no match is found,
  then NULL is returned.  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 64-bit boundary, then ASSERT().
  If Length is not aligned on a 64-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

  @param  Buffer      Pointer to the target buffer to scan.
  @param  Length      Number of bytes in Buffer to scan.
  @param  Value       Value to search for in the target buffer.

  @return A pointer to the matching byte in the target buffer or NULL otherwise.

**/
VOID *
EFIAPI
ScanMem64 (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN UINT64      Value
  )
{
  if (0 == Length) {
    return NULL;
  }

  ASSERT (Buffer != NULL);
  ASSERT (0 == ((UINTN)Buffer & (sizeof (Value) - 1)));
  ASSERT ((Length - 1) <= (MAX_ADDRESS - (UINTN)Buffer));
  ASSERT (0 == (Length & (sizeof (Value) - 1)));

  return (VOID*)InternalMemScanMem64 (Buffer, Length / sizeof (Value), Value);
}
