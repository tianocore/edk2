/** @file
  Unaligned access functions of BaseLib for RISCV64.

  Copyright (c) 2025, Xiang W <wangxiang@iscas.ac.cn>. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BaseLibInternals.h"

/**
  Reads a value from memory that may be unaligned.

  This function returns the value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer   The pointer to a value that may be unaligned.
  @param  BytesLen Byte length of the value to be read

  @return The value read from Buffer.

**/
inline
STATIC
UINT64
ReadUnalignedWithSize (
  IN CONST VOID  *Buffer,
  IN UINT64      BytesLen
  )
{
  UINT64  Index = 0;
  UINT64  Value = 0;

  ASSERT (Buffer != NULL);

  for (Index = BytesLen; Index > 0; Index--) {
    Value = (Value << 8) | ((CONST UINT8 *)Buffer)[Index - 1];
  }

  return Value;
}

/**
  Writes a value to memory that may be unaligned.

  This function writes the value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer   The pointer to a value that may be unaligned.
  @param  Value    value to write to Buffer.
  @param  BytesLen Byte length of the value to be write


  @return The value to write to Buffer.

**/
inline
STATIC
UINT64
WriteUnalignedWithSize (
  OUT VOID    *Buffer,
  IN  UINT64  Value,
  IN  UINT64  BytesLen
  )
{
  UINT64  Index = 0;
  UINT64  Temp  = Value;

  ASSERT (Buffer != NULL);

  for (Index = 0; Index < BytesLen; Index++) {
    ((UINT8 *)Buffer)[Index] = (UINT8)Temp;
    Temp                     = Temp >> 8;
  }

  return Value;
}

/**
  Reads a 16-bit value from memory that may be unaligned.

  This function returns the 16-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 16-bit value that may be unaligned.

  @return The 16-bit value read from Buffer.

**/
UINT16
EFIAPI
ReadUnaligned16 (
  IN CONST UINT16  *Buffer
  )
{
  return ReadUnalignedWithSize (Buffer, 2);
}

/**
  Writes a 16-bit value to memory that may be unaligned.

  This function writes the 16-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 16-bit value that may be unaligned.
  @param  Value   16-bit value to write to Buffer.

  @return The 16-bit value to write to Buffer.

**/
UINT16
EFIAPI
WriteUnaligned16 (
  OUT UINT16  *Buffer,
  IN  UINT16  Value
  )
{
  return WriteUnalignedWithSize (Buffer, Value, 2);
}

/**
  Reads a 24-bit value from memory that may be unaligned.

  This function returns the 24-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 24-bit value that may be unaligned.

  @return The 24-bit value read from Buffer.

**/
UINT32
EFIAPI
ReadUnaligned24 (
  IN CONST UINT32  *Buffer
  )
{
  return ReadUnalignedWithSize (Buffer, 3);
}

/**
  Writes a 24-bit value to memory that may be unaligned.

  This function writes the 24-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 24-bit value that may be unaligned.
  @param  Value   24-bit value to write to Buffer.

  @return The 24-bit value to write to Buffer.

**/
UINT32
EFIAPI
WriteUnaligned24 (
  OUT UINT32  *Buffer,
  IN  UINT32  Value
  )
{
  return WriteUnalignedWithSize (Buffer, Value, 3) & 0xffffff;
}

/**
  Reads a 32-bit value from memory that may be unaligned.

  This function returns the 32-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 32-bit value that may be unaligned.

  @return The 32-bit value read from Buffer.

**/
UINT32
EFIAPI
ReadUnaligned32 (
  IN CONST UINT32  *Buffer
  )
{
  return ReadUnalignedWithSize (Buffer, 4);
}

/**
  Writes a 32-bit value to memory that may be unaligned.

  This function writes the 32-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 32-bit value that may be unaligned.
  @param  Value   32-bit value to write to Buffer.

  @return The 32-bit value to write to Buffer.

**/
UINT32
EFIAPI
WriteUnaligned32 (
  OUT UINT32  *Buffer,
  IN  UINT32  Value
  )
{
  return WriteUnalignedWithSize (Buffer, Value, 4);
}

/**
  Reads a 64-bit value from memory that may be unaligned.

  This function returns the 64-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 64-bit value that may be unaligned.

  @return The 64-bit value read from Buffer.

**/
UINT64
EFIAPI
ReadUnaligned64 (
  IN CONST UINT64  *Buffer
  )
{
  return ReadUnalignedWithSize (Buffer, 8);
}

/**
  Writes a 64-bit value to memory that may be unaligned.

  This function writes the 64-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  The pointer to a 64-bit value that may be unaligned.
  @param  Value   64-bit value to write to Buffer.

  @return The 64-bit value to write to Buffer.

**/
UINT64
EFIAPI
WriteUnaligned64 (
  OUT UINT64  *Buffer,
  IN  UINT64  Value
  )
{
  return WriteUnalignedWithSize (Buffer, Value, 8);
}
