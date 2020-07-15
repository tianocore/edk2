//------------------------------------------------------------------------------
//
// Copyright (c) 2017, Pete Batard. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
//------------------------------------------------------------------------------

#if defined(_M_ARM64)
typedef unsigned __int64  size_t;
#else
typedef unsigned __int32  size_t;
#endif

void* memcpy(void *, const void *, size_t);
#pragma intrinsic(memcpy)
#pragma function(memcpy)
void* memcpy(void *dest, const void *src, size_t n)
{
  unsigned char *d = dest;
  unsigned char const *s = src;

  while (n--)
    *d++ = *s++;

  return dest;
}
