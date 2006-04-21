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
    UefiMemoryLib

**/

/**
  This function copies a source GUID to a destination GUID.

  This function copies the contents of the 128-bit GUID specified by SourceGuid
  to DestinationGuid, and returns DestinationGuid.

  If DestinationGuid is NULL, then ASSERT().
  If SourceGuid is NULL, then ASSERT().

  @param  DestinationGuid Pointer to the destination GUID.
  @param  SourceGuid Pointer to the source GUID.

  @return DestinationGuid

**/
GUID *
EFIAPI
CopyGuid (
  OUT     GUID                      *DestinationGuid,
  IN      CONST GUID                *SourceGuid
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
  Compares two GUIDs

  This function compares Guid1 to Guid2. If the GUIDs are identical then TRUE
  is returned. If there are any bit differences in the two GUIDs, then FALSE is
  returned.

  If Guid1 is NULL, then ASSERT().
  If Guid2 is NULL, then ASSERT().

  @param  Guid1 guid to compare
  @param  Guid2 guid to compare

  @retval TRUE  if Guid1 == Guid2
  @retval FALSE if Guid1 != Guid2

**/
BOOLEAN
EFIAPI
CompareGuid (
  IN      CONST GUID                *Guid1,
  IN      CONST GUID                *Guid2
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
  the lowest address to the highest address at 128-bit increments for the
  128-bit GUID value that matches Guid. If a match is found, then a pointer to
  the matching GUID in the target buffer is returned. If no match is found,
  then NULL is returned. If Length is 0, then NULL is returned.

  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 64-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer  Pointer to the target buffer to scan.
  @param  Length  Number of bytes in Buffer to scan.
  @param  Guid    Value to search for in the target buffer.

  @return Pointer to the first occurrence.
  @retval NULL  if Length == 0 or Guid was not found.
**/
VOID *
EFIAPI
ScanGuid (
  IN      CONST VOID                *Buffer,
  IN      UINTN                     Length,
  IN      CONST GUID                *Guid
  )
{
  CONST GUID                        *GuidPtr;

  GuidPtr = (GUID*)Buffer;
  Buffer = GuidPtr + Length / sizeof (*GuidPtr);
  while (GuidPtr < (CONST GUID*)Buffer) {
    if (CompareGuid (GuidPtr, Guid)) {
      return (VOID*)GuidPtr;
    }
    GuidPtr++;
  }
  return NULL;
}
