/** @file
  Integer Arithmetic Run-time support functions for GCC.
  The integer arithmetic routines are used on platforms that don't provide
  hardware support for arithmetic operations on some modes..

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include <Uefi.h>
#include <Library/DebugLib.h>
#include <sys/EfiCdefs.h>

#include <Library/BaseLib.h>

// Shift Datum left by Count bits.
// ===========================================================================
int __ashlsi3(int Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (int) LShiftU64 ((UINT64)Datum, (UINTN)Count);
}

long __ashldi3(long Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long) LShiftU64 ((UINT64)Datum, (UINTN)Count);
}

long long __ashlti3(long long Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long long) LShiftU64 ((UINT64)Datum, (UINTN)Count);
}

// Arithmetically shift Datum right by Count bits.
// ===========================================================================
int __ashrsi3(int Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (int) ARShiftU64 ((UINT64) Datum, (UINTN)Count);
}

long __ashrdi3(long Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long) ARShiftU64 ((UINT64) Datum, (UINTN)Count);
}

long long __ashrti3(long long Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long long) ARShiftU64 ((UINT64) Datum, (UINTN)Count);
}

// Return the quotient of the signed division of Dividend and Divisor
// ===========================================================================
int __divsi3(int Dividend, int Divisor)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (int) DivS64x64Remainder ((INT64) Dividend, (INT64) Divisor, NULL);
}

INT64 __divdi3(INT64 Dividend, INT64 Divisor)
{
  INT64 Quotient;

  Quotient = DivS64x64Remainder ((INT64) Dividend, (INT64) Divisor, NULL);
  DEBUG((DEBUG_INFO, "%a: %Ld / %Ld = %Ld\n", __func__, Dividend, Divisor, Quotient));

  return Quotient;
}

long long __divti3(long long Dividend, long long Divisor)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long long) DivS64x64Remainder ((INT64) Dividend, (INT64) Divisor, NULL);
}

// Logically shift Datum right by Count bits
// ===========================================================================
int __lshrsi3(int Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (int) RShiftU64 ((UINT64) Datum, (UINTN)Count);
}

long __lshrdi3(int Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long) RShiftU64 ((UINT64) Datum, (UINTN)Count);
}

long long __lshrti3(int Datum, int Count)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long long) RShiftU64 ((UINT64) Datum, (UINTN)Count);
}

// Return the remainder of the signed division of Dividend and Divisor
// ===========================================================================
int __modsi3(int Dividend, int Divisor)
{
  INT64 Remainder;

  (void) DivS64x64Remainder ((INT64) Dividend, (INT64) Divisor, &Remainder);
  DEBUG((DEBUG_INFO, "modsi3: %d %% %d = %d\n", Dividend, Divisor, (int)Remainder));

  return (int) Remainder;
}

INT64 __moddi3(INT64 Dividend, INT64 Divisor)
{
  INT64 Remainder;

  (void) DivS64x64Remainder ((INT64) Dividend, (INT64) Divisor, &Remainder);
  DEBUG((DEBUG_INFO, "moddi3: %Ld %% %Ld = %Ld\n", (INT64)Dividend, (INT64)Divisor, Remainder));

  return Remainder;
}

long long __modti3(long long Dividend, long long Divisor)
{
  INT64 Remainder;

  (void) DivS64x64Remainder ((INT64) Dividend, (INT64) Divisor, &Remainder);
  DEBUG((DEBUG_INFO, "modti3: %Ld %% %Ld = %Ld\n", (INT64)Dividend, (INT64)Divisor, Remainder));

  return (long long) Remainder;
}

// These functions return the product of the Multiplicand and Multiplier.
// ===========================================================================
long long __multi3(long long Multiplicand, long long Multiplier)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long long) MultS64x64 ((INT64)Multiplicand, (INT64)Multiplier);
}

// Return the quotient of the unsigned division of a and b.
// ===========================================================================
unsigned int __udivsi3(unsigned int Dividend, unsigned int Divisor)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (int) DivU64x64Remainder ((UINT64) Dividend, (UINT64) Divisor, NULL);
}

unsigned long __udivdi3(unsigned long Dividend, unsigned long Divisor)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long) DivU64x64Remainder ((UINT64) Dividend, (UINT64) Divisor, NULL);
}

unsigned long long __udivti3(unsigned long long Dividend, unsigned long long Divisor)
{
  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  return (long long) DivU64x64Remainder ((UINT64) Dividend, (UINT64) Divisor, NULL);
}

// ===========================================================================
unsigned int __umodsi3(unsigned int Dividend, unsigned int Divisor)
{
  UINT64 Remainder;

  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  (void) DivU64x64Remainder ((UINT64) Dividend, (UINT64) Divisor, &Remainder);

  return (unsigned int) Remainder;
}

unsigned long __umoddi3(unsigned long Dividend, unsigned long Divisor)
{
  UINT64 Remainder;

  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  (void) DivU64x64Remainder ((UINT64) Dividend, (UINT64) Divisor, &Remainder);

  return (unsigned long) Remainder;
}

unsigned long long __umodti3(unsigned long long Dividend, unsigned long long Divisor)
{
  UINT64 Remainder;

  DEBUG((DEBUG_INFO, "%a:\n", __func__));
  (void) DivU64x64Remainder ((UINT64) Dividend, (UINT64) Divisor, &Remainder);

  return (unsigned long long) Remainder;
}
