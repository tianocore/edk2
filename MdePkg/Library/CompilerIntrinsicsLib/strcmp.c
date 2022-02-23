/** @file
  Intrinsic Memory Routines Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>

typedef UINTN size_t;

#if defined (__GNUC__) || defined (__clang__)
#define GLOBAL_USED  __attribute__((used))
#else
#define GLOBAL_USED
#endif

/* OpenSSL will use floating point support, and C compiler produces the _fltused
   symbol by default. Simply define this symbol here to satisfy the linker. */
int  GLOBAL_USED  _fltused = 1;

int
strcmp (
  const char  *s1,
  const char  *s2
  )
{
  return (int)AsciiStrCmp (s1, s2);
}
