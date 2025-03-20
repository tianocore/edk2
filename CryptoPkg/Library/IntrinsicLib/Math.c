/** @file
  Intrinsic Math Routines Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>

long long
__ashlti3 (
  long long  a,
  int        b
  )
{
  return LShiftU64 (a, b);
}

long
__lshrdi3 (
  long  a,
  int   b
  )
{
  return a >> b;
}
