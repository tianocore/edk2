/** @file
    Comparison Functions for <string.h>.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/BaseMemoryLib.h>

#include  <LibConfig.h>

#include  <ctype.h>
#include  <string.h>

/** The memcmp function compares the first n characters of the object pointed
    to by s1 to the first n characters of the object pointed to by s2.

    @return   The memcmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the object pointed to by s1 is
              greater than, equal to, or less than the object pointed to by s2.
**/
int       memcmp(const void *s1, const void *s2, size_t n)
{
  return (int)CompareMem( s1, s2, n);
}

/** The strcmp function compares the string pointed to by s1 to the string
    pointed to by s2.

    @return   The strcmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the string pointed to by s1 is
              greater than, equal to, or less than the string pointed to by s2.
**/
int       strcmp(const char *s1, const char *s2)
{
  return (int)AsciiStriCmp( s1, s2);
}

/** The strcoll function compares the string pointed to by s1 to the string
    pointed to by s2, both interpreted as appropriate to the LC_COLLATE
    category of the current locale.

    @return   The strcoll function returns an integer greater than, equal to,
              or less than zero, accordingly as the string pointed to by s1 is
              greater than, equal to, or less than the string pointed to by s2
              when both are interpreted as appropriate to the current locale.
**/
int       strcoll(const char *s1, const char *s2)
{
  /* LC_COLLATE is unimplemented, hence always "C" */
  return (strcmp(s1, s2));
}

/** The strncmp function compares not more than n characters (characters that
    follow a null character are not compared) from the array pointed to by s1
    to the array pointed to by s2.

    @return   The strncmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the possibly null-terminated
              array pointed to by s1 is greater than, equal to, or less than
              the possibly null-terminated array pointed to by s2.
**/
int       strncmp(const char *s1, const char *s2, size_t n)
{
  return (int)AsciiStrnCmp( s1, s2, n);
}

/** The strxfrm function transforms the string pointed to by s2 and places the
    resulting string into the array pointed to by s1. The transformation is
    such that if the strcmp function is applied to two transformed strings, it
    returns a value greater than, equal to, or less than zero, corresponding to
    the result of the strcoll function applied to the same two original
    strings. No more than n characters are placed into the resulting array
    pointed to by s1, including the terminating null character. If n is zero,
    s1 is permitted to be a null pointer. If copying takes place between
    objects that overlap, the behavior is undefined.

    @return   The strxfrm function returns the length of the transformed string
              (not including the terminating null character). If the value
              returned is n or more, the contents of the array pointed to by s1
              are indeterminate.
**/
size_t    strxfrm(char * __restrict s1, const char * __restrict s2, size_t n)
{
  size_t srclen, copysize;

  /*
  * Since locales are unimplemented, this is just a copy.
  */
  srclen = strlen(s2);
  if (n != 0) {
    copysize = srclen < n ? srclen : n - 1;
    (void)memcpy(s1, s2, copysize);
    s1[copysize] = 0;
  }
  return (srclen);
}

/** Case agnostic string comparison for NetBSD compatibility. **/
int
strcasecmp(const char *s1, const char *s2)
{
  const unsigned char *us1 = (const unsigned char *)s1,
  *us2 = (const unsigned char *)s2;
  int Difference;

  while ( 0 == ( Difference = tolower(*us1) - tolower(*us2))) {
    if (*us1 == 0) {
    return (0);
    }
    us1 += 1;
    us2 += 1;
  }
  return Difference;
}

