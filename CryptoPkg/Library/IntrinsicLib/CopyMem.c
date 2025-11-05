/** @file
  Intrinsic Memory Routines Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>

#if defined (__clang__) && !defined (__APPLE__)

/* Copies bytes between buffers */
static __attribute__ ((__used__))
void *
__memcpy (
  void          *dest,
  const void    *src,
  unsigned int  count
  )
{
  return CopyMem (dest, src, (UINTN)count);
}

__attribute__ ((__alias__ ("__memcpy")))
void *
memcpy (
  void          *dest,
  const void    *src,
  unsigned int  count
  );

#else
/* Copies bytes between buffers */
void *
memcpy (
  void          *dest,
  const void    *src,
  unsigned int  count
  )
{
  return CopyMem (dest, src, (UINTN)count);
}

#endif
