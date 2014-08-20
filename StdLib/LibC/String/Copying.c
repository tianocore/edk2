/** @file
    Copying Functions for <string.h>.

    Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
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

#include  <stdlib.h>
#include  <string.h>

/** Do not define memcpy for IPF+GCC or ARM+GCC builds.
    For IPF, using a GCC compiler, the memcpy function is converted to
    CopyMem by objcpy during build.
    For ARM, the memcpy function is provided by the CompilerIntrinsics library.
**/
#if !((defined(MDE_CPU_IPF) || defined(MDE_CPU_ARM)) && defined(__GNUC__))
/** The memcpy function copies n characters from the object pointed to by s2
    into the object pointed to by s1.

    The implementation is reentrant and handles the case where s2 overlaps s1.

    @return   The memcpy function returns the value of s1.
**/
void *
memcpy(void * __restrict s1, const void * __restrict s2, size_t n)
{
  return CopyMem( s1, s2, n);
}
#endif  /* !(defined(MDE_CPU_IPF) && defined(__GCC)) */

/** The memmove function copies n characters from the object pointed to by s2
    into the object pointed to by s1. Copying takes place as if the n
    characters from the object pointed to by s2 are first copied into a
    temporary array of n characters that does not overlap the objects pointed
    to by s1 and s2, and then the n characters from the temporary array are
    copied into the object pointed to by s1.

    This is a version of memcpy that is guaranteed to work when s1 and s2
    overlap.  Since our implementation of memcpy already handles overlap,
    memmove can be identical to memcpy.

    @return   The memmove function returns the value of s1.
**/
void *
memmove(void *s1, const void *s2, size_t n)
{
  return CopyMem( s1, s2, n);
}

/** The strcpy function copies the string pointed to by s2 (including the
    terminating null character) into the array pointed to by s1. If copying
    takes place between objects that overlap, the behavior is undefined.

    @return   The strcpy function returns the value of s1.
**/
char *
strcpy(char * __restrict s1, const char * __restrict s2)
{
  //char *s1ret = s1;

  //while ( *s1++ = *s2++)  /* Empty Body */;
  //return(s1ret);
  return AsciiStrCpy( s1, s2);
}

/** The strncpy function copies not more than n characters (characters that
    follow a null character are not copied) from the array pointed to by s2 to
    the array pointed to by s1. If copying takes place between objects that
    overlap, the behavior is undefined.

    If the array pointed to by s2 is a string that is shorter than n
    characters, null characters are appended to the copy in the array pointed
    to by s1, until n characters in all have been written.

    @return   The strncpy function returns the value of s1.
**/
char     *strncpy(char * __restrict s1, const char * __restrict s2, size_t n)
{
  return AsciiStrnCpy( s1, s2, n);
  //char *dest = s1;

  //while(n != 0) {
  //  --n;
  //  if((*dest++ = *s2++) == '\0')  break;
  //}
  //while(n != 0) {
  //  *dest++ = '\0';
  //  --n;
  //}
  //return (s1);
}

/** The strncpyX function copies not more than n-1 characters (characters that
    follow a null character are not copied) from the array pointed to by s2 to
    the array pointed to by s1. Array s1 is guaranteed to be NULL terminated.
    If copying takes place between objects that overlap,
    the behavior is undefined.

    strncpyX exists because normal strncpy does not indicate if the copy was
    terminated because of exhausting the buffer or reaching the end of s2.

    @return   The strncpyX function returns 0 if the copy operation was
              terminated because it reached the end of s1.  Otherwise,
              a non-zero value is returned indicating how many characters
              remain in s1.
**/
int strncpyX(char * __restrict s1, const char * __restrict s2, size_t n)
{
  int NumLeft;

  for( ; n != 0; --n) {
    if((*s1++ = *s2++) == '\0')  break;
  }
  NumLeft = (int)n;

  for( --s1; n != 0; --n) {
    *s1++ = '\0';
  }

  return NumLeft;   // Zero if we ran out of buffer ( strlen(s1) < strlen(s2) )
}

/** NetBSD Compatibility Function strdup creates a duplicate copy of a string. **/
char *
strdup(const char *str)
{
  size_t len;
  char *copy;

  len = strlen(str) + 1;
  if ((copy = malloc(len)) == NULL)
    return (NULL);
  memcpy(copy, str, len);
  return (copy);
}
