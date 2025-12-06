/** @file
  Unaligned access functions of BaseLib.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BaseLibInternals.h"

#pragma pack(1)
typedef union {
  UINT16    Val16;
  UINT32    Val32;
  UINT64    Val64;
} MISALIGNED;
#pragma pack()

STATIC_ASSERT (ALIGNOF (MISALIGNED) == 1, "Alignment error");

/**
  Reads a 16-bit value from memory that may be unaligned.

  This function returns the 16-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 16-bit value that may be unaligned.

  @return The 16-bit value read from Buffer.

**/
UINT16
EFIAPI
ReadUnaligned16 (
  IN CONST VOID  *Buffer
  )
{
  CONST MISALIGNED  *Misaligned;

  ASSERT (Buffer != NULL);

  Misaligned = Buffer;

  return Misaligned->Val16;
}

/**
  Writes a 16-bit value to memory that may be unaligned.

  This function writes the 16-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 16-bit value that may be unaligned.
  @param  Value   16-bit value to write to Buffer.

  @return The 16-bit value to write to Buffer.

**/
UINT16
EFIAPI
WriteUnaligned16 (
  OUT VOID    *Buffer,
  IN  UINT16  Value
  )
{
  MISALIGNED  *Misaligned;

  ASSERT (Buffer != NULL);

  Misaligned = Buffer;

  return Misaligned->Val16 = Value;
}

/**
  Reads a 32-bit value from memory that may be unaligned.

  This function returns the 32-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 32-bit value that may be unaligned.

  @return The 32-bit value read from Buffer.

**/
UINT32
EFIAPI
ReadUnaligned32 (
  IN CONST VOID  *Buffer
  )
{
  CONST MISALIGNED  *Misaligned;

  ASSERT (Buffer != NULL);

  Misaligned = Buffer;

  return Misaligned->Val32;
}

/**
  Writes a 32-bit value to memory that may be unaligned.

  This function writes the 32-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 32-bit value that may be unaligned.
  @param  Value   The 32-bit value to write to Buffer.

  @return The 32-bit value to write to Buffer.

**/
UINT32
EFIAPI
WriteUnaligned32 (
  OUT VOID    *Buffer,
  IN  UINT32  Value
  )
{
  MISALIGNED  *Misaligned;

  ASSERT (Buffer != NULL);

  Misaligned = Buffer;

  return Misaligned->Val32 = Value;
}

/**
  Reads a 64-bit value from memory that may be unaligned.

  This function returns the 64-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 64-bit value that may be unaligned.

  @return The 64-bit value read from Buffer.

**/
UINT64
EFIAPI
ReadUnaligned64 (
  IN CONST VOID  *Buffer
  )
{
  CONST MISALIGNED  *Misaligned;

  ASSERT (Buffer != NULL);

  Misaligned = Buffer;

  return Misaligned->Val64;
}

/**
  Writes a 64-bit value to memory that may be unaligned.

  This function writes the 64-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 64-bit value that may be unaligned.
  @param  Value   The 64-bit value to write to Buffer.

  @return The 64-bit value to write to Buffer.

**/
UINT64
EFIAPI
WriteUnaligned64 (
  OUT VOID    *Buffer,
  IN  UINT64  Value
  )
{
  MISALIGNED  *Misaligned;

  ASSERT (Buffer != NULL);

  Misaligned = Buffer;

  return Misaligned->Val64 = Value;
}
