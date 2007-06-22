/** @file
  Implementation of GUID functions.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  MemLibGuid.c

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
#include "CommonHeader.h"

/**
  Copies a source GUID to a destination GUID.

  This function copies the contents of the 128-bit GUID specified by SourceGuid to
  DestinationGuid, and returns DestinationGuid.
  If DestinationGuid is NULL, then ASSERT().
  If SourceGuid is NULL, then ASSERT().

  @param  DestinationGuid   Pointer to the destination GUID.
  @param  SourceGuid        Pointer to the source GUID.

  @return DestinationGuid.

**/
GUID *
EFIAPI
CopyGuid (
  OUT GUID       *DestinationGuid,
  IN CONST GUID  *SourceGuid
  )
{
  WriteUnaligned64 (
    (UINT64*)DestinationGuid,
    ReadUnaligned64 ((CONST UINT64*)SourceGuid)
    );
  WriteUnaligned64 (
    (UINT64*)DestinationGuid + 1,
    ReadUnaligned64 ((CONST UINT64*)SourceGuid + 1)
    );
  return DestinationGuid;
}

/**
  Compares two GUIDs.

  This function compares Guid1 to Guid2.  If the GUIDs are identical then TRUE is returned.
  If there are any bit differences in the two GUIDs, then FALSE is returned.
  If Guid1 is NULL, then ASSERT().
  If Guid2 is NULL, then ASSERT().

  @param  Guid1       A pointer to a 128 bit GUID.
  @param  Guid2       A pointer to a 128 bit GUID.

  @retval TRUE        Guid1 and Guid2 are identical.
  @retval FALSE       Guid1 and Guid2 are not identical.

**/
BOOLEAN
EFIAPI
CompareGuid (
  IN CONST GUID  *Guid1,
  IN CONST GUID  *Guid2
  )
{
  return (BOOLEAN)(
           ReadUnaligned64 ((CONST UINT64*)Guid1)
             == ReadUnaligned64 ((CONST UINT64*)Guid2) &&
           ReadUnaligned64 ((CONST UINT64*)Guid1 + 1)
             == ReadUnaligned64 ((CONST UINT64*)Guid2 + 1)
           );
}

/**
  Scans a target buffer for a GUID, and returns a pointer to the matching GUID
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from
  the lowest address to the highest address at 128-bit increments for the 128-bit
  GUID value that matches Guid.  If a match is found, then a pointer to the matching
  GUID in the target buffer is returned.  If no match is found, then NULL is returned.
  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is not aligned on a 128-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS ? Buffer + 1), then ASSERT(). 

  @param  Buffer  Pointer to the target buffer to scan.
  @param  Length  Number of bytes in Buffer to scan.
  @param  Guid    Value to search for in the target buffer.

  @return A pointer to the matching Guid in the target buffer or NULL otherwise.

**/
VOID *
EFIAPI
ScanGuid (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN CONST GUID  *Guid
  )
{
  CONST GUID                        *GuidPtr;

  ASSERT (((UINTN)Buffer & (sizeof (Guid->Data1) - 1)) == 0);
  ASSERT (Length <= (MAX_ADDRESS - (UINTN)Buffer + 1));
  ASSERT ((Length & (sizeof (*GuidPtr) - 1)) == 0);

  GuidPtr = (GUID*)Buffer;
  Buffer  = GuidPtr + Length / sizeof (*GuidPtr);
  while (GuidPtr < (CONST GUID*)Buffer) {
    if (CompareGuid (GuidPtr, Guid)) {
      return (VOID*)GuidPtr;
    }
    GuidPtr++;
  }
  return NULL;
}
