/** @file
  C Run-Time Libraries (CRT) Time Management Routines Wrapper Implementation
  for OpenSSL-based Cryptographic Library.

  This C file implements constant time value for time() and NULL for gmtime()
  thus should not be used in library instances which require functionality
  of following APIs which need system time support:
  1)  RsaGenerateKey
  2)  RsaCheckKey
  3)  RsaPkcs1Sign
  4)  Pkcs7Sign
  5)  DhGenerateParameter
  6)  DhGenerateKey

Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <CrtLibSupport.h>

//
// -- Time Management Routines --
//

time_t time (time_t *timer)
{
  if (timer != NULL) {
    *timer = 0;
  }
  return 0;
}

struct tm * gmtime (const time_t *timer)
{
  return NULL;
}
