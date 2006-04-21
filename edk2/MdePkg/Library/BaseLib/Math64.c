/** @file
  Leaf math worker functions that require 64-bit arithmetic support from the
  compiler.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  Math64.c

**/

UINT64
EFIAPI
InternalMathLShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  return Operand << Count;
}

UINT64
EFIAPI
InternalMathRShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  return Operand >> Count;
}

UINT64
EFIAPI
InternalMathARShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  //
  // Test if this compiler supports arithmetic shift
  //
  if ((((-1) << (sizeof (-1) * 8 - 1)) >> (sizeof (-1) * 8 - 1)) == -1) {
    //
    // Arithmetic shift is supported
    //
    return (UINT64)((INT64)Operand >> Count);
  }

  //
  // Arithmetic is not supported
  //
  return (Operand >> Count) |
         ((INTN)Operand < 0 ? ~((UINTN)-1 >> Count) : 0);
}

UINT64
EFIAPI
InternalMathLRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  return (Operand << Count) | (Operand >> (64 - Count));
}

UINT64
EFIAPI
InternalMathRRotU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  return (Operand >> Count) | (Operand << (64 - Count));
}

UINT64
EFIAPI
InternalMathSwapBytes64 (
  IN      UINT64                    Operand
  )
{
  return (UINT64)(
           ((UINT64)SwapBytes32 ((UINT32)Operand) << 32) |
           ((UINT64)SwapBytes32 ((UINT32)(Operand >> 32)))
           );
}

UINT64
EFIAPI
InternalMathMultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  )
{
  return Multiplicand * Multiplier;
}

UINT64
EFIAPI
InternalMathMultU64x64 (
  IN      UINT64                    Multiplicand,
  IN      UINT64                    Multiplier
  )
{
  return Multiplicand * Multiplier;
}

UINT64
EFIAPI
InternalMathDivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  return Dividend / Divisor;
}

UINT64
EFIAPI
InternalMathModU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  return Dividend % Divisor;
}

UINT64
EFIAPI
InternalMathDivRemU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder
  )
{
  if (Remainder != NULL) {
    *Remainder = (UINT32)(Dividend % Divisor);
  }
  return Dividend / Divisor;
}

UINT64
EFIAPI
InternalMathDivRemU64x64 (
  IN      UINT64                    Dividend,
  IN      UINT64                    Divisor,
  OUT     UINT64                    *Remainder
  )
{
  if (Remainder != NULL) {
    *Remainder = Dividend % Divisor;
  }
  return Dividend / Divisor;
}

INT64
EFIAPI
InternalMathDivRemS64x64 (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder
  )
{
  if (Remainder != NULL) {
    *Remainder = Dividend % Divisor;
  }
  return Dividend / Divisor;
}
