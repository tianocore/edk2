/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Math.c

Abstract:

  Math worker functions. 

--*/

#include "Tiano.h"

UINT64
LShiftU64 (
  IN UINT64   Operand,
  IN UINTN    Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be left shifted by 32 bits and 
  returns the shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  Count   - Number of times to shift left.
 
Returns:

  Value shifted left identified by the Count.

--*/
{
  return Operand << Count;
}

UINT64
MultU64x32 (
  IN UINT64   Multiplicand,
  IN UINTN    Multiplier
  )
/*++  
  
Routine Description:

  This routine allows a 64 bit value to be multiplied with a 32 bit 
  value returns 64bit result.
  No checking if the result is greater than 64bits

Arguments:

  Multiplicand  - multiplicand
  Multiplier    - multiplier

Returns:

  Multiplicand * Multiplier
  
--*/
{
  return Multiplicand * Multiplier;
}

UINT64
Power10U64 (
  IN UINT64   Operand,
  IN UINTN    Power
  )
{
  UINT64 Result;

  Result = Operand;
  while (Power-- > 0) {
    Result *= 10;
  }
  return Result;
}

UINT64
RShiftU64 (
  IN UINT64   Operand,
  IN UINTN    Count
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be right shifted by 32 bits and returns the 
  shifted value.
  Count is valid up 63. (Only Bits 0-5 is valid for Count)

Arguments:

  Operand - Value to be shifted
  Count   - Number of times to shift right.
 
Returns:

  Value shifted right identified by the Count.

--*/
{
  return Operand >> Count;
}

UINT64
DivU64x32 (
  IN UINT64   Dividend,
  IN UINTN    Divisor,
  OUT UINTN   *Remainder OPTIONAL
  )
/*++

Routine Description:

  This routine allows a 64 bit value to be divided with a 32 bit value returns 
  64bit result and the Remainder.

Arguments:

  Dividend  - dividend
  Divisor   - divisor
  Remainder - buffer for remainder
 
Returns:

  Dividend  / Divisor
  Remainder = Dividend mod Divisor

--*/
{
  if (Remainder != NULL) {
    *Remainder = Dividend % Divisor;
  }

  return Dividend / Divisor;
}

UINT8
Log2 (
  IN UINT64   Operand
  )
/*++

Routine Description:

  This function computes rounded down log2 of the Operand.  This is an equivalent
  of the position of the highest bit set in the Operand treated as a mask.
  E.g., Log2 (0x0001) == 0, Log2 (0x0002) == 1, Log2 (0x0003) == 1, Log2 (0x0005) == 2
  Log2 (0x4000) == 14, Log2 (0x8000) == 15, Log2 (0xC000) == 15, Log2 (0xFFFF) == 15, etc.

Arguments:
  Operand - value of which the Log2 is to be computed.

Returns:
  Rounded down log2 of the Operand or 0xFF if zero passed in.

--*/
{
  UINT8 Bitpos;
  Bitpos = 0;

  if (Operand == 0) {
    return (0xff);
  }

  while (Operand != 0) {
    Operand >>= 1;
    Bitpos++;
  }
  return (UINT8)(Bitpos - 1);

}

UINT64
GetPowerOfTwo (
  IN UINT64 Operand
  )
/*++

Routine Description:

  Calculates the largest integer that is both 
  a power of two and less than Input

Arguments:

  Operand  - value to calculate power of two

Returns:

  the largest integer that is both  a power of 
  two and less than Input

--*/
{
  UINT8 Bitpos;
  Bitpos = 0;

  if (Operand == 0) {
    return 0;
  }

  while (Operand != 0) {
    Operand >>= 1;
    Bitpos++;
  }

  Operand = 1;
  Bitpos--;
  while (Bitpos != 0) {
    Operand <<= 1;
    Bitpos--;
  }

  return Operand;
}
