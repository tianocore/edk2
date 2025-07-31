/** @file
  C Run-Time Libraries (CRT) Wrapper Implementation for MbedTLS-based
  Cryptographic Library.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <stdio.h>

/**dummy mbedtls_printf function. **/
int
mbedtls_printf (
  char const  *fmt,
  ...
  )
{
  ASSERT (FALSE);
  return 0;
}

/**dummy mbedtls_vsnprintf function. **/
int
mbedtls_vsnprintf (
  char        *str,
  size_t      size,
  const char  *format,
  ...
  )
{
  ASSERT (FALSE);
  return 0;
}

/**strchr function. **/
char *
strchr (
  const char  *str,
  int         ch
  )
{
  return ScanMem8 (str, AsciiStrSize (str), (char)ch);
}

char *
strncpy (
  char        *strDest,
  const char  *strSource,
  size_t      count
  )
{
  UINTN  DestMax = MAX_STRING_SIZE;

  if (count < MAX_STRING_SIZE) {
    DestMax = count + 1;
  } else {
    count = MAX_STRING_SIZE-1;
  }

  AsciiStrnCpyS (strDest, DestMax, strSource, (UINTN)count);

  return strDest;
}

/**strcmp function. **/
int
strcmp (
  const char  *s1,
  const char  *s2
  )
{
  return (int)AsciiStrCmp (s1, s2);
}

/**strpbrk function. **/
char *
strpbrk (
  const char  *s,
  const char  *accept
  )
{
  int  i;

  for ( ; *s != '\0'; s++) {
    for (i = 0; accept[i] != '\0'; i++) {
      if (*s == accept[i]) {
        return (char *)s;
      }
    }
  }

  return NULL;
}
