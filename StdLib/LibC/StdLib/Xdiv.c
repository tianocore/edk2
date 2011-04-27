/** @file
  The div, ldiv, and lldiv, functions compute numer / denom and
  numer % denom in a single operation.

  The div, ldiv, and lldiv functions return a structure of type div_t, ldiv_t,
  and lldiv_t, respectively, comprising both the quotient and the remainder.
  The structures shall contain (in either order) the members quot
  (the quotient) and rem (the remainder), each of which has the same type as
  the arguments numer and denom. If either part of the result cannot be
  represented, the behavior is undefined.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <LibConfig.h>
#include  <sys/EfiCdefs.h>

#include  <Library/BaseLib.h>
#include  <stdlib.h>          /* div_t, ldiv_t, lldiv_t */

/* DivS64x64Remainder will write the remainder as a 64-bit value, so we store
   it first into bigrem and then into r.rem.  This avoids writing the remainder
   beyond the end of the div_t structure.
*/
div_t
div(int num, int denom)
{
  div_t r;
  INT64 bigrem;

  r.quot = (int)DivS64x64Remainder( (INT64)num, (INT64)denom, &bigrem);
  r.rem  = (int)bigrem;

  return (r);
}

/* DivS64x64Remainder will write the remainder as a 64-bit value, so we store
   it first into bigrem and then into r.rem.  This avoids writing the remainder
   beyond the end of the div_t structure.
*/
ldiv_t
ldiv(long num, long denom)
{
  ldiv_t r;
  INT64 bigrem;

  r.quot = (long)DivS64x64Remainder( (INT64)num, (INT64)denom, &bigrem);
  r.rem  = (long)bigrem;

  return (r);
}

/* DivS64x64Remainder will write the remainder as a 64-bit value, so we store
   it first into bigrem and then into r.rem.  This avoids writing the remainder
   beyond the end of the div_t structure if r.rem is narrower than 64-bits.

   Even though most implementations make long long 64 bits wide, we still go
   through bigrem, just-in-case.
*/
lldiv_t
lldiv(long long num, long long denom)
{
  lldiv_t r;
  INT64 bigrem;

  r.quot = (long long)DivS64x64Remainder( (INT64)num, (INT64)denom, &bigrem);
  r.rem  = (long long)bigrem;

  return (r);
}
