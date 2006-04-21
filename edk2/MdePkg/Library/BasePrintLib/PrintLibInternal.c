/** @file
  Print Library worker functions.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PrintLibInternal.c

**/

#include "PrintLibInternal.h"

static CONST CHAR8 mHexStr[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

CHAR8 *
BasePrintLibFillBuffer (
  CHAR8   *Buffer,
  INTN    Length,
  UINTN   Character,
  INTN    Increment
  )
{
  INTN  Index;

  for (Index = 0; Index < Length; Index++) {
    *Buffer       =  (CHAR8) Character;
    *(Buffer + 1) =  (CHAR8) (Character >> 8);
    Buffer        += Increment;
  }
  return Buffer;
}

/**
  Print worker function that prints a Value as a decimal number in Buffer.

  @param  Buffer Location to place the Unicode or ASCII string of Value.
  
  @param  Value Value to convert to a Decimal or Hexidecimal string in Buffer.
  
  @param  Flags Flags to use in printing string, see file header for details.
  
  @param  Precision Minimum number of digits to return in the ASCII string

  @return Number of characters printed.

**/
UINTN
EFIAPI
BasePrintLibValueToString (
  IN OUT CHAR8  *Buffer,
  IN INT64      Value,
  IN UINTN      Radix
  )
{
  UINTN   Digits;
  UINT32  Remainder;

  //
  // Loop to convert one digit at a time in reverse order
  //
  *(Buffer++) = 0;
  Digits = 0;
  do {
    Value = (INT64)DivU64x32Remainder ((UINT64)Value, (UINT32)Radix, &Remainder);
    *(Buffer++) = mHexStr[Remainder];
    Digits++;
  } while (Value != 0);
  return Digits;
}

UINTN
BasePrintLibConvertValueToString (
  IN OUT CHAR8   *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width,
  IN UINTN       Increment
  )
{
  CHAR8  *OriginalBuffer;
  CHAR8  ValueBuffer[MAXIMUM_VALUE_CHARACTERS];
  UINTN  Count;
  UINTN  Digits;
  UINTN  Index;

  OriginalBuffer = Buffer;

  if (Width == 0 || (Flags & COMMA_TYPE) != 0) {
    Flags &= (~PREFIX_ZERO);
  }

  if (Width == 0 || Width > (MAXIMUM_VALUE_CHARACTERS - 1)) {
    Width = MAXIMUM_VALUE_CHARACTERS - 1;
  }

  if (Value < 0) {
    Value = -Value;
    Buffer = BasePrintLibFillBuffer (Buffer, 1, '-', Increment);
  }

  Count = BasePrintLibValueToString (ValueBuffer, Value, 10);

  if ((Flags & PREFIX_ZERO) != 0) {
    Buffer = BasePrintLibFillBuffer (Buffer, Width - Count, '0', Increment);
  }

  Digits = 3 - (Count % 3);
  for (Index = 0; Index < Count; Index++) {
    Buffer = BasePrintLibFillBuffer (Buffer, 1, ValueBuffer[Count - Index], Increment);
    if ((Flags & COMMA_TYPE) != 0) {
      Digits++;
      if (Digits == 3) {
        Digits = 0;
        if ((Index + 1) < Count) {
          Buffer = BasePrintLibFillBuffer (Buffer, 1, ',', Increment);
        }
      }
    }
  }

  Buffer = BasePrintLibFillBuffer (Buffer, 1, 0, Increment);

  return ((Buffer - OriginalBuffer) / Increment);
}


UINTN
UnicodeValueToString (
  IN OUT CHAR16  *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width
  )
{
  return BasePrintLibConvertValueToString ((CHAR8 *)Buffer, Flags, Value, Width, 2);
}

UINTN
AsciiValueToString (
  IN OUT CHAR8  *Buffer,
  IN UINTN      Flags,
  IN INT64      Value,
  IN UINTN      Width
  )
{
  return BasePrintLibConvertValueToString ((CHAR8 *)Buffer, Flags, Value, Width, 1);
}
