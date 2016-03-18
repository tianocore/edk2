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

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenSslSupport.h>

//
// -- Time Management Routines --
//

time_t time (time_t *timer)
{
  *timer = 0;
  return *timer;
}

struct tm * gmtime (const time_t *timer)
{
  return NULL;
}