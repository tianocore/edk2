/** @file
  Integer division worker functions for Ia32.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  DivS64x64Remainder.c

**/

#include "../BaseLibInternals.h"

/**
  Worker function that Divides a 64-bit signed integer by a 64-bit signed integer and 
  generates a  64-bit signed result and a optional 64-bit signed remainder.

  This function divides the 64-bit unsigned value Dividend by the 64-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. If Remainder
  is not NULL, then the 64-bit unsigned remainder is returned in Remainder.
  This function returns the 64-bit unsigned quotient.

  @param  Dividend  A 64-bit signed value.
  @param  Divisor   A 64-bit signed value.
  @param  Remainder A pointer to a 64-bit signed value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
INT64
InternalMathDivRemS64x64 (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder  OPTIONAL
  )
{
  INT64                             Quot;

  Quot = InternalMathDivRemU64x64 (
           Dividend >= 0 ? Dividend : -Dividend,
           Divisor >= 0 ? Divisor : -Divisor,
           (UINT64 *) Remainder
           );
  if (Remainder != NULL && Dividend < 0) {
    *Remainder = -*Remainder;
  }
  return (Dividend ^ Divisor) >= 0 ? Quot : -Quot;
}
