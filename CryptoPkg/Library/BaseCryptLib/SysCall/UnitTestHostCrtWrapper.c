/** @file
  C Run-Time Libraries (CRT) Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2009 - 2017, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <stdio.h>

#include <Base.h>
#include <Library/DebugLib.h>

/* Convert character to lowercase */
#ifdef _MSC_VER
//
// Workaround for building NOOPT on Windows systems. Due to disabled
// optimization, the MSVC compiler cannot hide this function
// implementation from the linker.
//
int
tolower_noos (
  int  c
  )
  #pragma comment(linker, "/alternatename:tolower=tolower_noos")
#else
int
tolower (
  int  c
  )
#endif
{
  if (('A' <= (c)) && ((c) <= 'Z')) {
    return (c - ('A' - 'a'));
  }

  return (c);
}

/* Compare first n bytes of string s1 with string s2, ignoring case */
int
strncasecmp (
  const char  *s1,
  const char  *s2,
  size_t      n
  )
{
  int  Val;

  ASSERT (s1 != NULL);
  ASSERT (s2 != NULL);

  if (n != 0) {
    do {
      Val = tolower (*s1) - tolower (*s2);
      if (Val != 0) {
        return Val;
      }

      ++s1;
      ++s2;
      if (*s1 == '\0') {
        break;
      }
    } while (--n != 0);
  }

  return 0;
}

/* Read formatted data from a string */
int
sscanf (
  const char  *buffer,
  const char  *format,
  ...
  )
{
  //
  // Null sscanf() function implementation to satisfy the linker, since
  // no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

uid_t
getuid (
  void
  )
{
  return 0;
}

uid_t
geteuid (
  void
  )
{
  return 0;
}

gid_t
getgid (
  void
  )
{
  return 0;
}

gid_t
getegid (
  void
  )
{
  return 0;
}

unsigned int
sleep (
  unsigned int  seconds
  )
{
  return 0;
}

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

int   errno = 0;
long  timezone;
