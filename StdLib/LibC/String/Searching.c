/** @file
    Search Functions for <string.h>.

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
#include  <limits.h>
#include  <string.h>

/** The memchr function locates the first occurrence of c (converted to an
    unsigned char) in the initial n characters (each interpreted as
    unsigned char) of the object pointed to by s.

    @return   The memchr function returns a pointer to the located character,
              or a null pointer if the character does not occur in the object.
**/
void *
memchr(const void *s, int c, size_t n)
{
  return ScanMem8( s, (UINTN)n, (UINT8)c);
}

/** The strchr function locates the first occurrence of c (converted to a char)
    in the string pointed to by s. The terminating null character is considered
    to be part of the string.

    @return   The strchr function returns a pointer to the located character,
              or a null pointer if the character does not occur in the string.
**/
char *
strchr(const char *s, int c)
{
  char  tgt = (char)c;

  do {
    if( *s == tgt) {
      return (char *)s;
    }
  } while(*s++ != '\0');
  return NULL;
}

static UINT8  BitMask[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
  };

#define WHICH8(c)     ((unsigned char)(c) >> 3)
#define WHICH_BIT(c)  (BitMask[((c) & 0x7)])
#define BITMAP64      ((UINT64 *)bitmap)

static
void
BuildBitmap(unsigned char * bitmap, const char *s2, int n)
{
  unsigned char bit;
  int           index;

  // Initialize bitmap.  Bit 0 is always 1 which corresponds to '\0'
  for (BITMAP64[0] = index = 1; index < n; index++)
    BITMAP64[index] = 0;

  // Set bits in bitmap corresponding to the characters in s2
  for (; *s2 != '\0'; s2++) {
    index = WHICH8(*s2);
    bit = WHICH_BIT(*s2);
    bitmap[index] = bitmap[index] | bit;
  }
}

/** The strcspn function computes the length of the maximum initial segment of
    the string pointed to by s1 which consists entirely of characters not from
    the string pointed to by s2.

    @return   The strcspn function returns the length of the segment.
**/
size_t
strcspn(const char *s1, const char *s2)
{
  UINT8 bitmap[ (((UCHAR_MAX + 1) / CHAR_BIT) + (CHAR_BIT - 1)) & ~7U];
  const char *str;
  UINT8 bit;
  int index;

  if(*s1 == '\0')   return 0;

  BuildBitmap( bitmap, s2, sizeof(bitmap) / sizeof(UINT64));

  for(str = s1; ; str++) {
    index = WHICH8(*str);
    bit = WHICH_BIT(*str);
    if ((bitmap[index] & bit) != 0)
      break;
  }
  return (str - s1);
}

/** The strpbrk function locates the first occurrence in the string pointed to
    by s1 of any character from the string pointed to by s2.

    @return   The strpbrk function returns a pointer to the character, or a
              null pointer if no character from s2 occurs in s1.
**/
char *
strpbrk(const char *s1, const char *s2)
{
  UINT8 bitmap[ (((UCHAR_MAX + 1) / CHAR_BIT) + (CHAR_BIT - 1)) & ~7U];
  UINT8 bit;
  int index;

  BuildBitmap( bitmap, s2, sizeof(bitmap) / sizeof(UINT64));

  for( ; *s1 != '\0'; ++s1) {
    index = WHICH8(*s1);
    bit = WHICH_BIT(*s1);
    if( (bitmap[index] & bit) != 0) {
      return (char *)s1;
    }
  }
  return NULL;
}

/** The strrchr function locates the last occurrence of c (converted to a char)
    in the string pointed to by s. The terminating null character is considered
    to be part of the string.

    @return   The strrchr function returns a pointer to the character, or a
              null pointer if c does not occur in the string.
**/
char *
strrchr(const char *s, int c)
{
  char  *found  = NULL;
  char  tgt     = (char)c;

  do {
    if( *s == tgt)  found = (char *)s;
  } while( *s++ != '\0');

  return found;
}

/** The strspn function computes the length of the maximum initial segment of
    the string pointed to by s1 which consists entirely of characters from the
    string pointed to by s2.

    @return   The strspn function returns the length of the segment.
**/
size_t
strspn(const char *s1 , const char *s2)
{
  UINT8 bitmap[ (((UCHAR_MAX + 1) / CHAR_BIT) + (CHAR_BIT - 1)) & ~7U];
  size_t  length = 0;
  int     index;
  UINT8   bit;

  BuildBitmap( bitmap, s2, sizeof(bitmap) / sizeof(UINT64));

  for( ; *s1 != '\0'; ++s1) {
    index = WHICH8(*s1);
    bit = WHICH_BIT(*s1);
    if( (bitmap[index] & bit) == 0)   break;
    ++length;
  }
  return length;
}

/** The strstr function locates the first occurrence in the string pointed to
    by s1 of the sequence of characters (excluding the terminating null
    character) in the string pointed to by s2.

    @return   The strstr function returns a pointer to the located string, or a
              null pointer if the string is not found. If s2 points to a string
              with zero length, the function returns s1.
**/
char *
strstr(const char *s1 , const char *s2)
{
  return  AsciiStrStr( s1, s2);
}

/** A sequence of calls to the strtok function breaks the string pointed to by
    s1 into a sequence of tokens, each of which is delimited by a character
    from the string pointed to by s2. The first call in the sequence has a
    non-null first argument; subsequent calls in the sequence have a null first
    argument. The separator string pointed to by s2 may be different from call
    to call.

    The first call in the sequence searches the string pointed to by s1 for the
    first character that is not contained in the current separator string
    pointed to by s2. If no such character is found, then there are no tokens
    in the string pointed to by s1 and the strtok function returns a null
    pointer. If such a character is found, it is the start of the first token.

    The strtok function then searches from there for a character that is
    contained in the current separator string. If no such character is found,
    the current token extends to the end of the string pointed to by s1, and
    subsequent searches for a token will return a null pointer. If such a
    character is found, it is overwritten by a null character, which terminates
    the current token. The strtok function saves a pointer to the following
    character, from which the next search for a token will start.

    Each subsequent call, with a null pointer as the value of the first
    argument, starts searching from the saved pointer and behaves as
    described above.

    @return   The strtok function returns a pointer to the first character of a
              token, or a null pointer if there is no token.
**/
char *
strtok(char * __restrict s1, const char * __restrict s2)
{
  static char  *Next  = NULL;
  UINT8         bitmap[ (((UCHAR_MAX + 1) / CHAR_BIT) + (CHAR_BIT - 1)) & ~7U];
  char         *Token = NULL;
  int           index;
  UINT8         bit;

  if(     (s1 == NULL)
      &&  ((s1 = Next) == NULL))
  {
    return  NULL;
  }

  // s2 can be different on each call, so build the bitmap each time.
  BuildBitmap( bitmap, s2, sizeof(bitmap) / sizeof(UINT64));

  // skip leading delimiters: all chars in s2
  for( ; *s1 != '\0'; ++s1) {
    index = WHICH8(*s1);
    bit = WHICH_BIT(*s1);
    if( (bitmap[index] & bit) == 0)   break;
  }
  if( *s1 != 0)
  {
    // Remember this point, it is the start of the token
    Token = s1++;

    // find the next delimiter and replace it with a '\0'
    for( ; *s1 != '\0'; ++s1) {
      index = WHICH8(*s1);
      bit = WHICH_BIT(*s1);
      if( (bitmap[index] & bit) != 0) {
        *s1++ = '\0';
        Next = s1;
        return Token;
      }
    }
  }
  Next = NULL;
  return Token;
}
