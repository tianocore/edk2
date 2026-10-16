/** @file
  Architecture Independent Base Memory Library Implementation.

  The following BaseMemoryLib instances contain the same copy of this file:
    BaseMemoryLib
    PeiMemoryLib
    UefiMemoryLib

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MemLibInternals.h"

#if defined (_MSC_VER) && !defined (__GNUC__) && !defined (__clang__)
VOID
_ReadWriteBarrier (
  VOID
  );

  #pragma intrinsic(_ReadWriteBarrier)
#endif

/**
  Emit a compiler barrier that keeps the compiler from scheduling any later
  memory access ahead of the preceding buffer clear.

  It emits no instructions; it only constrains compile-time ordering, and the
  constraint holds even when the caller is inlined under LTO.

**/
STATIC
VOID
InternalMemBarrier (
  VOID
  )
{
 #if defined (__GNUC__) || defined (__clang__)
  __asm__ __volatile__ ("" : : : "memory");
 #elif defined (_MSC_VER)
  _ReadWriteBarrier ();
 #else
  //
  // No portable compiler barrier is available for this toolchain.
  //
  #error "InternalMemBarrier: no compiler barrier is defined for this toolchain."
 #endif
}

/**
  Fills a target buffer with a 16-bit value, and returns the target buffer.

  @param  Buffer  The pointer to the target buffer to fill.
  @param  Length  The count of 16-bit value to fill.
  @param  Value   The value with which to fill Length bytes of Buffer.

  @return Buffer

**/
VOID *
EFIAPI
InternalMemSetMem16 (
  OUT     VOID    *Buffer,
  IN      UINTN   Length,
  IN      UINT16  Value
  )
{
  for ( ; Length != 0; Length--) {
    ((volatile UINT16 *)Buffer)[Length - 1] = Value;
  }

  return Buffer;
}

/**
  Fills a target buffer with a 32-bit value, and returns the target buffer.

  @param  Buffer  The pointer to the target buffer to fill.
  @param  Length  The count of 32-bit value to fill.
  @param  Value   The value with which to fill Length bytes of Buffer.

  @return Buffer

**/
VOID *
EFIAPI
InternalMemSetMem32 (
  OUT     VOID    *Buffer,
  IN      UINTN   Length,
  IN      UINT32  Value
  )
{
  for ( ; Length != 0; Length--) {
    ((volatile UINT32 *)Buffer)[Length - 1] = Value;
  }

  return Buffer;
}

/**
  Fills a target buffer with a 64-bit value, and returns the target buffer.

  @param  Buffer  The pointer to the target buffer to fill.
  @param  Length  The count of 64-bit value to fill.
  @param  Value   The value with which to fill Length bytes of Buffer.

  @return Buffer

**/
VOID *
EFIAPI
InternalMemSetMem64 (
  OUT     VOID    *Buffer,
  IN      UINTN   Length,
  IN      UINT64  Value
  )
{
  for ( ; Length != 0; Length--) {
    ((volatile UINT64 *)Buffer)[Length - 1] = Value;
  }

  return Buffer;
}

/**
  Set Buffer to 0 for Size bytes.

  @param  Buffer Memory to set.
  @param  Length The number of bytes to set.

  @return Buffer

**/
VOID *
EFIAPI
InternalMemZeroMem (
  OUT     VOID   *Buffer,
  IN      UINTN  Length
  )
{
  //
  // Zero the buffer through the worker, then emit a compiler barrier so a
  // caller's subsequent store cannot be scheduled ahead of the clear.
  //
  Buffer = InternalMemSetMem (Buffer, Length, 0);
  InternalMemBarrier ();

  return Buffer;
}

/**
  Compares two memory buffers of a given length.

  @param  DestinationBuffer The first memory buffer.
  @param  SourceBuffer      The second memory buffer.
  @param  Length            Length of DestinationBuffer and SourceBuffer memory
                            regions to compare. Must be non-zero.

  @return 0                 All Length bytes of the two buffers are identical.
  @retval Non-zero          The first mismatched byte in SourceBuffer subtracted from the first
                            mismatched byte in DestinationBuffer.

**/
INTN
EFIAPI
InternalMemCompareMem (
  IN      CONST VOID  *DestinationBuffer,
  IN      CONST VOID  *SourceBuffer,
  IN      UINTN       Length
  )
{
  while ((--Length != 0) &&
         (*(INT8 *)DestinationBuffer == *(INT8 *)SourceBuffer))
  {
    DestinationBuffer = (INT8 *)DestinationBuffer + 1;
    SourceBuffer      = (INT8 *)SourceBuffer + 1;
  }

  return (INTN)*(UINT8 *)DestinationBuffer - (INTN)*(UINT8 *)SourceBuffer;
}

/**
  Scans a target buffer for an 8-bit value, and returns a pointer to the
  matching 8-bit value in the target buffer.

  @param  Buffer  The pointer to the target buffer to scan.
  @param  Length  The count of 8-bit value to scan. Must be non-zero.
  @param  Value   The value to search for in the target buffer.

  @return The pointer to the first occurrence, or NULL if not found.

**/
CONST VOID *
EFIAPI
InternalMemScanMem8 (
  IN      CONST VOID  *Buffer,
  IN      UINTN       Length,
  IN      UINT8       Value
  )
{
  CONST UINT8  *Pointer;

  Pointer = (CONST UINT8 *)Buffer;
  do {
    if (*Pointer == Value) {
      return Pointer;
    }

    ++Pointer;
  } while (--Length != 0);

  return NULL;
}

/**
  Scans a target buffer for a 16-bit value, and returns a pointer to the
  matching 16-bit value in the target buffer.

  @param  Buffer  The pointer to the target buffer to scan.
  @param  Length  The count of 16-bit value to scan. Must be non-zero.
  @param  Value   The value to search for in the target buffer.

  @return The pointer to the first occurrence, or NULL if not found.

**/
CONST VOID *
EFIAPI
InternalMemScanMem16 (
  IN      CONST VOID  *Buffer,
  IN      UINTN       Length,
  IN      UINT16      Value
  )
{
  CONST UINT16  *Pointer;

  Pointer = (CONST UINT16 *)Buffer;
  do {
    if (*Pointer == Value) {
      return Pointer;
    }

    ++Pointer;
  } while (--Length != 0);

  return NULL;
}

/**
  Scans a target buffer for a 32-bit value, and returns a pointer to the
  matching 32-bit value in the target buffer.

  @param  Buffer  The pointer to the target buffer to scan.
  @param  Length  The count of 32-bit value to scan. Must be non-zero.
  @param  Value   The value to search for in the target buffer.

  @return The pointer to the first occurrence, or NULL if not found.

**/
CONST VOID *
EFIAPI
InternalMemScanMem32 (
  IN      CONST VOID  *Buffer,
  IN      UINTN       Length,
  IN      UINT32      Value
  )
{
  CONST UINT32  *Pointer;

  Pointer = (CONST UINT32 *)Buffer;
  do {
    if (*Pointer == Value) {
      return Pointer;
    }

    ++Pointer;
  } while (--Length != 0);

  return NULL;
}

/**
  Scans a target buffer for a 64-bit value, and returns a pointer to the
  matching 64-bit value in the target buffer.

  @param  Buffer  The pointer to the target buffer to scan.
  @param  Length  The count of 64-bit value to scan. Must be non-zero.
  @param  Value   The value to search for in the target buffer.

  @return The pointer to the first occurrence, or NULL if not found.

**/
CONST VOID *
EFIAPI
InternalMemScanMem64 (
  IN      CONST VOID  *Buffer,
  IN      UINTN       Length,
  IN      UINT64      Value
  )
{
  CONST UINT64  *Pointer;

  Pointer = (CONST UINT64 *)Buffer;
  do {
    if (*Pointer == Value) {
      return Pointer;
    }

    ++Pointer;
  } while (--Length != 0);

  return NULL;
}

/**
  Checks whether the contents of a buffer are all zeros.

  @param  Buffer  The pointer to the buffer to be checked.
  @param  Length  The size of the buffer (in bytes) to be checked.

  @retval TRUE    Contents of the buffer are all zeros.
  @retval FALSE   Contents of the buffer are not all zeros.

**/
BOOLEAN
EFIAPI
InternalMemIsZeroBuffer (
  IN CONST VOID  *Buffer,
  IN UINTN       Length
  )
{
  CONST UINT8  *BufferData;
  UINTN        Index;

  BufferData = Buffer;
  for (Index = 0; Index < Length; Index++) {
    if (BufferData[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}
