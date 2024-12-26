/** @file
  C Run-Time Libraries (CRT) Time Management Routines Wrapper Implementation
  for MbedTLS-based Cryptographic Library.

  This C file implements constant time value for time() and NULL for gmtime()
  thus should not be used in library instances which require functionality
  of following APIs which need system time support:
  1)  RsaGenerateKey
  2)  RsaCheckKey
  3)  RsaPkcs1Sign
  4)  Pkcs7Sign
  5)  DhGenerateParameter
  6)  DhGenerateKey

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>
#include <CrtLibSupport.h>

//
// -- Time Management Routines --
//

/**time function. **/
time_t
time (
  time_t  *timer
  )
{
  if (timer != NULL) {
    *timer = 0;
  }

  return 0;
}

/**gmtime function. **/
struct tm *
gmtime (
  const time_t  *timer
  )
{
  return NULL;
}

/**sleep function. **/
unsigned int
sleep (
  unsigned int  seconds
  )
{
  return 0;
}

/**_time64 function. **/
time_t
_time64 (
  time_t  *t
  )
{
  return time (t);
}

long  timezone;

int
gettimeofday (
  struct timeval   *tv,
  struct timezone  *tz
  )
{
  tv->tv_sec  = 0;
  tv->tv_usec = 0;
  return 0;
}
