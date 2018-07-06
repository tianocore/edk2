//------------------------------------------------------------------------------
//
// Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
//
// This program and the accompanying materials are licensed and made
// available under the terms and conditions of the BSD License which
// accompanies this distribution.  The full text of the license may be
// found at http://opensource.org/licenses/bsd-license.php
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR
// IMPLIED.
//
//------------------------------------------------------------------------------

typedef __SIZE_TYPE__ size_t;

static void __memcpy(void *dest, const void *src, size_t n)
{
  unsigned char *d = dest;
  unsigned char const *s = src;

  while (n--)
    *d++ = *s++;
}

void *memcpy(void *dest, const void *src, size_t n)
{
  __memcpy(dest, src, n);
  return dest;
}

#ifdef __arm__

__attribute__((__alias__("__memcpy")))
void __aeabi_memcpy(void *dest, const void *src, size_t n);

__attribute__((__alias__("__memcpy")))
void __aeabi_memcpy4(void *dest, const void *src, size_t n);

__attribute__((__alias__("__memcpy")))
void __aeabi_memcpy8(void *dest, const void *src, size_t n);

#endif
