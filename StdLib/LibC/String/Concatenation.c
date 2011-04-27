/** @file
    Concatenation Functions for <string.h>.

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

#include  <LibConfig.h>

#include  <string.h>

/** The strcat function appends a copy of the string pointed to by s2
    (including the terminating null character) to the end of the string pointed
    to by s1. The initial character of s2 overwrites the null character at the
    end of s1. If copying takes place between objects that overlap, the
    behavior is undefined.

    @return   The strcat function returns the value of s1.
**/
char *
strcat(char * __restrict s1, const char * __restrict s2)
{
  return AsciiStrCat( s1, s2);
}

/** The strncat function appends not more than n characters (a null character
    and characters that follow it are not appended) from the array pointed to
    by s2 to the end of the string pointed to by s1. The initial character of
    s2 overwrites the null character at the end of s1. A terminating null
    character is always appended to the result. If copying takes place
    between objects that overlap, the behavior is undefined.

    @return   The strncat function returns the value of s1.
**/
char *
strncat(char * __restrict s1, const char * __restrict s2, size_t n)
{
  return AsciiStrnCat( s1, s2, n);
}

/** The strncatX function appends not more than n characters (a null character
    and characters that follow it are not appended) from the array pointed to
    by s2 to the end of the string pointed to by s1. The initial character of
    s2 overwrites the null character at the end of s1. The result is always
    terminated with a null character. If copying takes place between objects
    that overlap, the behavior is undefined.

    strncatX exists because normal strncat does not indicate if the operation
    was terminated because of exhausting n or reaching the end of s2.

    @return   The strncatX function returns 0 if the operation was terminated
              because it reached the end of s1.  Otherwise, a non-zero value is
              returned indicating how many characters remain in s1.
**/
int
strncatX(char * __restrict s1, const char * __restrict s2, size_t n)
{
  int NumLeft;

  // Find s1's terminating NUL
  for( ; n != 0; --n) {
    if( *s1++ == '\0')  break;
  }

  // Now copy *s2 into s1, overwriting s1's terminating NUL
  for( --s1; n != 0; --n) {
    if((*s1++ = *s2++) == '\0')  break;
  }
  NumLeft = (int)n;

  // Guarantee that s1 is NUL terminated.
  *--s1 = '\0';

  return NumLeft;   // Zero if we ran out of buffer ( strlen(s1) < strlen(s2) )
}
