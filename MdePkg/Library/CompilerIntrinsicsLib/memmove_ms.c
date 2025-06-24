// ------------------------------------------------------------------------------
//
// Copyright (c) 2019, Pete Batard. All rights reserved.
// Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// ------------------------------------------------------------------------------

//
// Starting from VS2019, the compiler recognizes memmove() as an intrinsic function,
// so we need to use _MSC_VER to control the following logic.
//

#if defined (_MSC_VER) && (_MSC_VER >= 1920)

typedef UINTN size_t;

void *
memmove (
  void *,
  const void *,
  size_t
  );

#pragma intrinsic(memmove)
#pragma function(memmove)
void *
memmove (
  void        *dest,
  const void  *src,
  size_t      n
  )
{
  unsigned char        *d;
  unsigned char const  *s;

  d = dest;
  s = src;

  if (d < s) {
    while (n-- != 0) {
      *d++ = *s++;
    }
  } else {
    d += n;
    s += n;
    while (n-- != 0) {
      *--d = *--s;
    }
  }

  return dest;
}

#endif
