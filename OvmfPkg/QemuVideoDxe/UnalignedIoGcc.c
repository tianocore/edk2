/** @file
  Unaligned Port I/O. This file has compiler specifics for GCC as there is no
  ANSI C standard for doing IO.

  Based on IoLibGcc.c.

  Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "UnalignedIoInternal.h"

/**
  Performs a 32-bit write to the specified, possibly unaligned I/O-type
  address.

  Writes the 32-bit I/O port specified by Port with the value specified by
  Value and returns Value. This function must guarantee that all I/O read and
  write operations are serialized.

  If 32-bit unaligned I/O port operations are not supported, then ASSERT().

  @param[in]  Port   I/O port address
  @param[in]  Value  32-bit word to write

  @return The value written to the I/O port.

**/
UINT32
UnalignedIoWrite32 (
  IN      UINTN                     Port,
  IN      UINT32                    Value
  )
{
  __asm__ __volatile__ ( "outl %0, %1" : : "a" (Value), "d" ((UINT16)Port) );
  return Value;
}

/**
  Reads a 32-bit word from the specified, possibly unaligned I/O-type address.

  Reads the 32-bit I/O port specified by Port. The 32-bit read value is
  returned. This function must guarantee that all I/O read and write operations
  are serialized.

  If 32-bit unaligned I/O port operations are not supported, then ASSERT().

  @param[in]  Port  The I/O port to read.

  @return The value read.

**/
UINT32
UnalignedIoRead32 (
  IN      UINTN                     Port
  )
{
  UINT32 Data;
  __asm__ __volatile__ ( "inl %1, %0" : "=a" (Data) : "d" ((UINT16)Port) );
  return Data;
}

