/*++

Copyright (c) 2006 , Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  x64math.c

Abstract:

  Math routines for x64.

--*/

UINT64
LeftShiftU64 (
  IN UINT64   Operand,
  IN UINT64   Count
  )
/*++

Routine Description:
  
  Left-shift a 64 bit value.

Arguments:

  Operand - 64-bit value to shift
  Count   - shift count

Returns:

  Operand << Count

--*/
{
  if (Count > 63) {
    return 0;
  }

  return Operand << Count;
}

UINT64
RightShiftU64 (
  IN UINT64   Operand,
  IN UINT64   Count
  )
/*++

Routine Description:
  
  Right-shift a 64 bit value.

Arguments:

  Operand - 64-bit value to shift
  Count   - shift count

Returns:

  Operand >> Count

--*/
{
  if (Count > 63) {
    return 0;
  }

  return Operand >> Count;
}

INT64
ARightShift64 (
  IN INT64   Operand,
  IN UINT64  Count
  )
/*++

Routine Description:
  
  Right-shift a 64 bit signed value.

Arguments:

  Operand - 64-bit value to shift
  Count   - shift count

Returns:

  Operand >> Count

--*/
{
  if (Count > 63) {

    if (Operand & 0x8000000000000000ULL) {
      return (INT64)~0;
    }

    return 0;
  }

  return Operand >> Count;
}

#if 0
//
// The compiler generates true assembly for these, so we don't need them.
//
INT32
ARightShift32 (
  IN INT32   Operand,
  IN UINTN   Count
  )
/*++

Routine Description:
  
  Right shift a 32-bit value

Arguments:

  Operand - value to shift
  Count   - shift count

Returns:

  Operand >> Count

--*/
{
  return Operand >> (Count & 0x1f);
}

INT32
MulS32x32 (
  INT32 Value1,
  INT32 Value2,
  INT32 *ResultHigh
  )
/*++

Routine Description:
  
  Multiply two signed 32-bit numbers.

Arguments:

  Value1      - first value to multiply
  Value2      - value to multiply Value1 by
  ResultHigh  - overflow

Returns:

  Value1 * Value2

Notes:

  The 64-bit result is the concatenation of *ResultHigh and the return value

  The product fits in 32 bits if
     (*ResultHigh == 0x00000000 AND *ResultLow_bit31 == 0)
                                     OR
     (*ResultHigh == 0xffffffff AND *ResultLow_bit31 == 1)

--*/
{
  INT64 Rres64;
  INT32 Result;

  Res64       = (INT64) Value1 * (INT64) Value2;
  *ResultHigh = (Res64 >> 32) & 0xffffffff;
  Result      = Res64 & 0xffffffff;
  return Result;
}

UINT32
MulU32x32 (
  UINT32 Value1,
  UINT32 Value2,
  UINT32 *ResultHigh
  )
/*++

Routine Description:
  
  Multiply two unsigned 32-bit values.

Arguments:

  Value1      - first number
  Value2      - number to multiply by Value1 
  ResultHigh  - overflow

Returns:

  Value1 * Value2

Notes:

  The 64-bit result is the concatenation of *ResultHigh and the return value.
  The product fits in 32 bits if *ResultHigh == 0x00000000

--*/
{
  UINT64  Res64;
  UINT32  Result;

  Res64       = (INT64) Value1 * (INT64) Value2;
  *ResultHigh = (Res64 >> 32) & 0xffffffff;
  Result      = Res64 & 0xffffffff;
  return Result;
}

INT32
DivS32x32 (
  INT32 Value1,
  INT32 Value2,
  INT32 *Remainder,
  UINTN *error
  )
//
// signed 32-bit by signed 32-bit divide; the 32-bit remainder is
// in *Remainder and the quotient is the return value; *error = 1 if the
// divisor is 0, and it is 1 otherwise
//
{
  INT32 Result;

  *error = 0;

  if (Value2 == 0x0) {
    *error      = 1;
    Result      = 0x80000000;
    *Remainder  = 0x80000000;
  } else {
    Result      = Value1 / Value2;
    *Remainder  = Value1 - Result * Value2;
  }

  return Result;
}

UINT32
DivU32x32 (
  UINT32  Value1,
  UINT32  Value2,
  UINT32  *Remainder,
  UINTN   *Error
  )
//
// unsigned 32-bit by unsigned 32-bit divide; the 32-bit remainder is
// in *Remainder and the quotient is the return value; *error = 1 if the
// divisor is 0, and it is 1 otherwise
//
{
  UINT32  Result;

  *Error = 0;

  if (Value2 == 0x0) {
    *Error      = 1;
    Result      = 0x80000000;
    *Remainder  = 0x80000000;
  } else {
    Result      = Value1 / Value2;
    *Remainder  = Value1 - Result * Value2;
  }

  return Result;
}

#endif

INT64
MulS64x64 (
  INT64 Value1,
  INT64 Value2,
  INT64 *ResultHigh
  )
/*++

Routine Description:
  
  Multiply two signed 32-bit numbers.

Arguments:

  Value1      - first value to multiply
  Value2      - value to multiply Value1 by
  ResultHigh  - overflow

Returns:

  Value1 * Value2

Notes:

  The 64-bit result is the concatenation of *ResultHigh and the return value

  The product fits in 32 bits if
     (*ResultHigh == 0x00000000 AND *ResultLow_bit31 == 0)
                                     OR
     (*ResultHigh == 0xffffffff AND *ResultLow_bit31 == 1)

--*/
{
  INT64 Result;
  
  Result  = Value1 * Value2;

  return Result;
}

UINT64
MulU64x64 (
  UINT64 Value1,
  UINT64 Value2,
  UINT64 *ResultHigh
  )
/*++

Routine Description:
  
  Multiply two unsigned 32-bit values.

Arguments:

  Value1      - first number
  Value2      - number to multiply by Value1 
  ResultHigh  - overflow

Returns:

  Value1 * Value2

Notes:

  The 64-bit result is the concatenation of *ResultHigh and the return value.
  The product fits in 32 bits if *ResultHigh == 0x00000000

--*/
{
  UINT64  Result;

  Result  = Value1 * Value2;

  return Result;
}

INT64
DivS64x64 (
  INT64 Value1,
  INT64 Value2,
  INT64 *Remainder,
  UINTN *Error
  )
/*++

Routine Description:
  
  Divide two 64-bit signed values.

Arguments:

  Value1    - dividend
  Value2    - divisor
  Remainder - remainder of Value1/Value2
  Error     - to flag errors (divide-by-0)

Returns:

  Value1 / Valu2

Note:

  The 64-bit remainder is in *Remainder and the quotient is the return value.
  *Error = 1 if the divisor is 0, and it is 1 otherwise

--*/
{
  INT64 Result;

  *Error = 0;

  if (Value2 == 0x0) {
    *Error      = 1;
    Result      = 0x8000000000000000;
    *Remainder  = 0x8000000000000000;
  } else {
    Result      = Value1 / Value2;
    *Remainder  = Value1 - Result * Value2;
  }

  return Result;
}

UINT64
DivU64x64 (
  UINT64 Value1,
  UINT64 Value2,
  UINT64 *Remainder,
  UINTN  *Error
  )
/*++

Routine Description:
  
  Divide two 64-bit unsigned values.

Arguments:

  Value1    - dividend
  Value2    - divisor
  Remainder - remainder of Value1/Value2
  Error     - to flag errors (divide-by-0)

Returns:

  Value1 / Valu2

Note:

  The 64-bit remainder is in *Remainder and the quotient is the return value.
  *Error = 1 if the divisor is 0, and it is 1 otherwise

--*/
{
  UINT64  Result;

  *Error = 0;

  if (Value2 == 0x0) {
    *Error      = 1;
    Result      = 0x8000000000000000;
    *Remainder  = 0x8000000000000000;
  } else {
    Result      = Value1 / Value2;
    *Remainder  = Value1 - Result * Value2;
  }

  return Result;
}
