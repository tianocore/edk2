//------------------------------------------------------------------------------
//
// Copyright (c) 2019, Pete Batard. All rights reserved.
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

#if defined(_M_ARM64)
typedef unsigned __int64  size_t;
#else
typedef unsigned __int32  size_t;
#endif

void* memmove(void *, const void *, size_t);
#pragma intrinsic(memmove)
#pragma function(memmove)
void* memmove(void *dest, const void *src, size_t n)
{
  unsigned char *d = dest;
  unsigned char const *s = src;

  if (d < s) {
    while (n--)
      *d++ = *s++;
  } else {
    d += n;
    s += n;
    while (n--)
      *--d = *--s;
  }

  return dest;
}
