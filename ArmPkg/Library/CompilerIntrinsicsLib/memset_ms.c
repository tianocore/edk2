//------------------------------------------------------------------------------
//
// Copyright (c) 2017, Pete Batard. All rights reserved.<BR>
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

#if defined(_M_ARM64)
typedef unsigned __int64  size_t;
#else
typedef unsigned __int32  size_t;
#endif

void* memset(void *, int, size_t);
#pragma intrinsic(memset)
#pragma function(memset)
void *memset(void *s, int c, size_t n)
{
  unsigned char *d = s;

  while (n--)
    *d++ = (unsigned char)c;

  return s;
}
