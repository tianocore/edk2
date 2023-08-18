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

typedef int time_t;

//
// Structures Definitions
//
struct tm {
  int     tm_sec;    /* seconds after the minute [0-60] */
  int     tm_min;    /* minutes after the hour [0-59] */
  int     tm_hour;   /* hours since midnight [0-23] */
  int     tm_mday;   /* day of the month [1-31] */
  int     tm_mon;    /* months since January [0-11] */
  int     tm_year;   /* years since 1900 */
  int     tm_wday;   /* days since Sunday [0-6] */
  int     tm_yday;   /* days since January 1 [0-365] */
  int     tm_isdst;  /* Daylight Savings Time flag */
  long    tm_gmtoff; /* offset from CUT in seconds */
  char    *tm_zone;  /* timezone abbreviation */
};

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

/**_time64 function. **/
time_t
_time64 (
  time_t  *t
  )
{
  return time (t);
}
