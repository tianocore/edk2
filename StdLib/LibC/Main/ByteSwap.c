/** @file
  Byte Swap routines for endian-nes conversions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Library/BaseLib.h>
#include  <LibConfig.h>

#include <sys/bswap.h>

// Undefine macro versions of the functions to be defined below.
#undef  bswap16
#undef  bswap32
#undef  bswap64

/**
Switches the endianness of a 16-bit integer.

This function swaps the bytes in a 16-bit unsigned value to switch the value
from little endian to big endian or vice versa. The byte swapped value is
returned.

@param  Value     A 16-bit unsigned value.

@return The byte swapped Value.

**/
uint16_t bswap16(uint16_t Value)
{
  return SwapBytes16(Value);
}

/**
Switches the endianness of a 32-bit integer.

This function swaps the bytes in a 32-bit unsigned value to switch the value
from little endian to big endian or vice versa. The byte swapped value is
returned.

@param  Value A 32-bit unsigned value.

@return The byte swapped Value.

**/
uint32_t bswap32(uint32_t Value)
{
  return SwapBytes32(Value);
}

/**
Switches the endianness of a 64-bit integer.

This function swaps the bytes in a 64-bit unsigned value to switch the value
from little endian to big endian or vice versa. The byte swapped value is
returned.

@param  Value A 64-bit unsigned value.

@return The byte swapped Value.

**/
uint64_t bswap64(uint64_t Value)
{
  return SwapBytes64(Value);
}
