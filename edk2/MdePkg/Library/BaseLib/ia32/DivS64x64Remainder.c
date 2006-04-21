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

INT64
EFIAPI
InternalMathDivRemS64x64 (
  IN      INT64                     Dividend,
  IN      INT64                     Divisor,
  OUT     INT64                     *Remainder
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
