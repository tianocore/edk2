/** @file
    Miscellaneous Functions for <string.h>.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
//#include  <sys/EfiCdefs.h>

#include  <Uefi.h>
#include  <Library/BaseLib.h>
#include  <Library/BaseMemoryLib.h>
#include  <Library/PcdLib.h>
#include  <Library/PrintLib.h>

#include  <LibConfig.h>

#include  <errno.h>
#include  <limits.h>
#include  <string.h>

extern char *sys_errlist[];

/** The memset function copies the value of c (converted to an unsigned char)
    into each of the first n characters of the object pointed to by s.

    @return   The memset function returns the value of s.
**/
void *
memset(void *s, int c, size_t n)
{
  return SetMem( s, (UINTN)n, (UINT8)c);
}

int
strerror_r(int errnum, char *buf, size_t buflen)
{
  const char   *estring;
  INTN          i;
  int           retval = 0;

  if( (errnum < 0) || (errnum >= EMAXERRORVAL)) {
    (void) AsciiSPrint( buf, ASCII_STRING_MAX, "Unknown Error: %d.", errnum);
    retval = EINVAL;
  }
  else {
    estring = sys_errlist[errnum];
    for( i = buflen; i > 0; --i) {
      if( (*buf++ = *estring++) == '\0') {
        break;
      }
    }
    if(i == 0) {
      retval = ERANGE;
    }
  }
  return retval;
}

/** The strerror function maps the number in errnum to a message string.
    Typically, the values for errnum come from errno, but strerror shall map
    any value of type int to a message.

    The implementation shall behave as if no library function calls the
    strerror function.

    @return   The strerror function returns a pointer to the string, the
              contents of which are locale specific.  The array pointed to
              shall not be modified by the program, but may be overwritten by
              a subsequent call to the strerror function.
**/
char *
strerror(int errnum)
{
  static char errorbuf[ASCII_STRING_MAX];
  int         status;

  status = strerror_r(errnum, errorbuf, sizeof(errorbuf));
  if(status != 0) {
    errno = status;
  }
  return errorbuf;
}

/** The strlen function computes the length of the string pointed to by s.

    @return   The strlen function returns the number of characters that
              precede the terminating null character.
**/
size_t
strlen(const char *s)
{
  return (size_t)AsciiStrLen( s);
}
