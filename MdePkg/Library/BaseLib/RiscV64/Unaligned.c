/** @file
  RISC-V specific functionality for (un)aligned memory read/write.

  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BaseLibInternals.h"

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
  IN CONST UINT16              *Buffer
  )
{
  UINT16 Value;
  INT8 Count;

  ASSERT (Buffer != NULL);

  for (Count = sizeof (UINT16) - 1, Value = 0; Count >= 0 ; Count --) {
    Value = Value << 8;
    Value |= *((UINT8*)Buffer + Count);
  }
  return Value;
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
  OUT UINT16                    *Buffer,
  IN  UINT16                    Value
  )
{
  INT8 Count;
  UINT16 ValueTemp;

  ASSERT (Buffer != NULL);

  for (Count = 0, ValueTemp = Value; Count < sizeof (UINT16) ; Count ++) {
    *((UINT8*)Buffer + Count) = (UINT8)(ValueTemp & 0xff);
    ValueTemp = ValueTemp >> 8;
  }
  return Value;
}

/**
  Reads a 24-bit value from memory that may be unaligned.

  This function returns the 24-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 24-bit value that may be unaligned.

  @return The 24-bit value read from Buffer.

**/
UINT32
EFIAPI
ReadUnaligned24 (
  IN CONST UINT32              *Buffer
  )
{
  UINT32 Value;
  INT8 Count;

  ASSERT (Buffer != NULL);
  for (Count = 2, Value = 0; Count >= 0 ; Count --) {
    Value = Value << 8;
    Value |= *((UINT8*)Buffer + Count);
  }
  return Value;
}

/**
  Writes a 24-bit value to memory that may be unaligned.

  This function writes the 24-bit value specified by Value to Buffer. Value is
  returned. The function guarantees that the write operation does not produce
  an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 24-bit value that may be unaligned.
  @param  Value   24-bit value to write to Buffer.

  @return The 24-bit value to write to Buffer.

**/
UINT32
EFIAPI
WriteUnaligned24 (
  OUT UINT32                    *Buffer,
  IN  UINT32                    Value
  )
{
  INT8 Count;
  UINT32 ValueTemp;

  ASSERT (Buffer != NULL);
  for (Count = 0, ValueTemp = Value; Count < 3 ; Count ++) {
    *((UINT8*)Buffer + Count) = (UINT8)(ValueTemp & 0xff);
    ValueTemp = ValueTemp >> 8;
  }
  return Value;
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
  IN CONST UINT32              *Buffer
  )
{
  UINT32 Value;
  INT8 Count;

  ASSERT (Buffer != NULL);

  for (Count = sizeof (UINT32) - 1, Value = 0; Count >= 0 ; Count --) {
    Value = Value << 8;
    Value |= *((UINT8*)Buffer + Count);
  }
  return Value;
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
  OUT UINT32                    *Buffer,
  IN  UINT32                    Value
  )
{
  INT8 Count;
  UINT32 ValueTemp;

  ASSERT (Buffer != NULL);
  for (Count = 0, ValueTemp = Value; Count < sizeof (UINT32) ; Count ++) {
    *((UINT8*)Buffer + Count) = (UINT8)(ValueTemp & 0xff);
    ValueTemp = ValueTemp >> 8;
  }
  return Value;
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
  IN CONST UINT64              *Buffer
  )
{
  UINT64 Value;
  INT8 Count;

  ASSERT (Buffer != NULL);
  for (Count = sizeof (UINT64) - 1, Value = 0; Count >= 0 ; Count --) {
    Value = Value << 8;
    Value |= *((UINT8*)Buffer + Count);
  }
  return Value;
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
  OUT UINT64                    *Buffer,
  IN  UINT64                    Value
  )
{
  INT8 Count;
  UINT64 ValueTemp;

  ASSERT (Buffer != NULL);
  for (Count = 0, ValueTemp = Value; Count < sizeof (UINT64) ; Count ++) {
    *((UINT8*)Buffer + Count) = (UINT8)(ValueTemp & 0xff);
    ValueTemp = ValueTemp >> 8;
  }
  return Value;
}
