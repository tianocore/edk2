/** @file
  Memory-only library functions with no library constructor/destructor

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __BASE_MEMORY_LIB__
#define __BASE_MEMORY_LIB__

/**
  Copies a source buffer to a destination buffer, and returns the destination buffer.

  This function copies Length bytes from SourceBuffer to DestinationBuffer, and returns
  DestinationBuffer.  The implementation must be reentrant, and it must handle the case
  where SourceBuffer overlaps DestinationBuffer.
  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  DestinationBuffer   Pointer to the destination buffer of the memory copy.
  @param  SourceBuffer        Pointer to the source buffer of the memory copy.
  @param  Length              Number of bytes to copy from SourceBuffer to DestinationBuffer.

  @return DestinationBuffer.

**/
VOID *
EFIAPI
CopyMem (
  OUT VOID       *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  );

/**
  Fills a target buffer with a byte value, and returns the target buffer.

  This function fills Length bytes of Buffer with Value, and returns Buffer.
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer    Memory to set.
  @param  Length    Number of bytes to set.
  @param  Value     Value of the set operation.

  @return Buffer.

**/
VOID *
EFIAPI
SetMem (
  OUT VOID  *Buffer,
  IN UINTN  Length,
  IN UINT8  Value
  );

/**
  Fills a target buffer with a 16-bit value, and returns the target buffer.

  This function fills Length bytes of Buffer with the 16-bit value specified by
  Value, and returns Buffer. Value is repeated every 16-bits in for Length
  bytes of Buffer.

  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().
  If Buffer is not aligned on a 16-bit boundary, then ASSERT().
  If Length is not aligned on a 16-bit boundary, then ASSERT().

  @param  Buffer  Pointer to the target buffer to fill.
  @param  Length  Number of bytes in Buffer to fill.
  @param  Value   Value with which to fill Length bytes of Buffer.

  @return Buffer.

**/
VOID *
EFIAPI
SetMem16 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT16  Value
  );

/**
  Fills a target buffer with a 32-bit value, and returns the target buffer.

  This function fills Length bytes of Buffer with the 32-bit value specified by
  Value, and returns Buffer. Value is repeated every 32-bits in for Length
  bytes of Buffer.

  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is not aligned on a 32-bit boundary, then ASSERT().

  @param  Buffer  Pointer to the target buffer to fill.
  @param  Length  Number of bytes in Buffer to fill.
  @param  Value   Value with which to fill Length bytes of Buffer.

  @return Buffer.

**/
VOID *
EFIAPI
SetMem32 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT32  Value
  );

/**
  Fills a target buffer with a 64-bit value, and returns the target buffer.

  This function fills Length bytes of Buffer with the 64-bit value specified by
  Value, and returns Buffer. Value is repeated every 64-bits in for Length
  bytes of Buffer.

  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().
  If Buffer is not aligned on a 64-bit boundary, then ASSERT().
  If Length is not aligned on a 64-bit boundary, then ASSERT().

  @param  Buffer  Pointer to the target buffer to fill.
  @param  Length  Number of bytes in Buffer to fill.
  @param  Value   Value with which to fill Length bytes of Buffer.

  @return Buffer.

**/
VOID *
EFIAPI
SetMem64 (
  OUT VOID   *Buffer,
  IN UINTN   Length,
  IN UINT64  Value
  );

/**
  Fills a target buffer with zeros, and returns the target buffer.

  This function fills Length bytes of Buffer with zeros, and returns Buffer.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer      Pointer to the target buffer to fill with zeros.
  @param  Length      Number of bytes in Buffer to fill with zeros.

  @return Buffer.

**/
VOID *
EFIAPI
ZeroMem (
  OUT VOID  *Buffer,
  IN UINTN  Length
  );

/**
  Compares the contents of two buffers.

  This function compares Length bytes of SourceBuffer to Length bytes of DestinationBuffer.
  If all Length bytes of the two buffers are identical, then 0 is returned.  Otherwise, the
  value returned is the first mismatched byte in SourceBuffer subtracted from the first
  mismatched byte in DestinationBuffer.
  If Length > 0 and DestinationBuffer is NULL and Length > 0, then ASSERT().
  If Length > 0 and SourceBuffer is NULL and Length > 0, then ASSERT().
  If Length is greater than (MAX_ADDRESS - DestinationBuffer + 1), then ASSERT().
  If Length is greater than (MAX_ADDRESS - SourceBuffer + 1), then ASSERT().

  @param  DestinationBuffer Pointer to the destination buffer to compare.
  @param  SourceBuffer      Pointer to the source buffer to compare.
  @param  Length            Number of bytes to compare.

  @return 0                 All Length bytes of the two buffers are identical.
  @retval Non-zero          The first mismatched byte in SourceBuffer subtracted from the first
                            mismatched byte in DestinationBuffer.

**/
INTN
EFIAPI
CompareMem (
  IN CONST VOID  *DestinationBuffer,
  IN CONST VOID  *SourceBuffer,
  IN UINTN       Length
  );

/**
  Scans a target buffer for an 8-bit value, and returns a pointer to the matching 8-bit value
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from the lowest
  address to the highest address for an 8-bit value that matches Value.  If a match is found,
  then a pointer to the matching byte in the target buffer is returned.  If no match is found,
  then NULL is returned.  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer      Pointer to the target buffer to scan.
  @param  Length      Number of bytes in Buffer to scan.
  @param  Value       Value to search for in the target buffer.

  @return A pointer to the matching byte in the target buffer or NULL otherwise.

**/
VOID *
EFIAPI
ScanMem8 (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN UINT8       Value
  );

/**
  Scans a target buffer for a 16-bit value, and returns a pointer to the matching 16-bit value
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from the lowest
  address to the highest address for a 16-bit value that matches Value.  If a match is found,
  then a pointer to the matching byte in the target buffer is returned.  If no match is found,
  then NULL is returned.  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-bit boundary, then ASSERT().
  If Length is not aligned on a 16-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer      Pointer to the target buffer to scan.
  @param  Length      Number of bytes in Buffer to scan.
  @param  Value       Value to search for in the target buffer.

  @return A pointer to the matching byte in the target buffer or NULL otherwise.

**/
VOID *
EFIAPI
ScanMem16 (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN UINT16      Value
  );

/**
  Scans a target buffer for a 32-bit value, and returns a pointer to the matching 32-bit value
  in the target buffer.

  This function searches target the buffer specified by Buffer and Length from the lowest
  address to the highest address for a 32-bit value that matches Value.  If a match is found,
  then a pointer to the matching byte in the target buffer is returned.  If no match is found,
  then NULL is returned.  If Length is 0, then NULL is returned.
  If Length > 0 and Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 32-bit boundary, then ASSERT().
  If Length is not aligned on a 32-bit boundary, then ASSERT().
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

  @param  Buffer      Pointer to the target buffer to scan.
  @param  Length      Number of bytes in Buffer to scan.
  @param  Value       Value to search for in the target buffer.

  @return A pointer to the matching byte in the target buffer or NULL otherwise.

**/
VOID *
EFIAPI
ScanMem32 (
  IN CONST VOID  *Buffer,
  IN UINTN       Length,
  IN UINT32      Value
  );

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
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

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
  );

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
  );

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
  );

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
  If Length is greater than (MAX_ADDRESS - Buffer + 1), then ASSERT().

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
  );

#endif
