// ------------------------------------------------------------------------------
//
// Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
// Copyright (c) 2021, Arm Limited. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//
// ------------------------------------------------------------------------------

typedef __SIZE_TYPE__ size_t;

static __attribute__ ((__used__))
void *
__memset (
  void    *s,
  int     c,
  size_t  n
  )
{
  unsigned char  *d;

  d = s;

  while (n-- != 0) {
    *d++ = c;
  }

  return s;
}

//
// Other modules (such as CryptoPkg/IntrinsicLib) may provide another
// implementation of memset(), which may conflict with this one if this
// object was pulled into the link due to the definitions below. So make
// our memset() 'weak' to let the other implementation take precedence.
//
__attribute__ ((__weak__, __alias__ ("__memset")))
void *
memset (
  void    *dest,
  int     c,
  size_t  n
  );
