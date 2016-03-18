/** @file
    Search Functions for <wchar.h>.

  Unless explicitly stated otherwise, the functions defined in this file order
  two wide characters the same way as two integers of the underlying integer
  type designated by wchar_t.

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
#include  <Library/MemoryAllocationLib.h>

#include  <LibConfig.h>

#include  <wchar.h>

/* Data initialized by the library constructor */
extern  UINT8  *__wchar_bitmap;
extern  UINTN   __wchar_bitmap_size;
extern  UINTN   __wchar_bitmap_64;

/** The wcschr function locates the first occurrence of c in the wide string
    pointed to by s.  The terminating null wide character is considered to be
    part of the wide string.

    @return   The wcschr function returns a pointer to the located wide
              character, or a null pointer if the wide character does not occur
              in the wide string.
**/
wchar_t *wcschr(const wchar_t *s, wchar_t c)
{
  do {
    if( *s == c) {
      return (wchar_t *)s;
    }
  } while(*s++ != 0);
  return NULL;
}

static UINT8  BitMask[] = {
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
  };

#define WHICH8(c)     ((unsigned short)(c) >> 3)
#define WHICH_BIT(c)  (BitMask[((c) & 0x7)])
#define BITMAP64      ((UINT64 *)bitmap)

static
void
BuildBitmap(unsigned char * bitmap, const wchar_t *s2, UINTN n)
{
  UINT8 bit;
  UINTN index;

  //// Initialize bitmap.  Bit 0 is always 1 which corresponds to '\0'
  //for (BITMAP64[0] = index = 1; index < n; index++)
  //  BITMAP64[index] = 0;
  (void)wmemset( (wchar_t *)bitmap, 0, n / sizeof(wchar_t));
  bitmap[0] = 1;

  // Set bits in bitmap corresponding to the characters in s2
  for (; *s2 != 0; ++s2) {
    index = WHICH8(*s2);
    bit = WHICH_BIT(*s2);
    bitmap[index] |= bit;
  }
}

/** The wcscspn function computes the length of the maximum initial segment of
    the wide string pointed to by s1 which consists entirely of wide characters
    not from the wide string pointed to by s2.

    @return   The wcscspn function returns the length of the segment.
**/
size_t wcscspn(const wchar_t *s1, const wchar_t *s2)
{
  const wchar_t *str;
  UINT8 bit;
  int index;
  size_t s1len;

  if(*s1 == 0)   return 0;
  s1len = wcslen(s1);

  BuildBitmap( __wchar_bitmap, s2, __wchar_bitmap_size);

  for(str = s1; str < &s1[s1len] ; str++) {
    index = WHICH8(*str);
    bit = WHICH_BIT(*str);
    if ((__wchar_bitmap[index] & bit) != 0)
      break;
  }
  return (str - s1);
}

/** The wcspbrk function locates the first occurrence in the wide string
    pointed to by s1 of any wide character from the wide string
    pointed to by s2.

    @return   The wcspbrk function returns a pointer to the wide character
              in s1, or a null pointer if no wide character from s2 occurs
              in s1.
**/
wchar_t *wcspbrk(const wchar_t *s1, const wchar_t *s2)
{
  UINT8 bit;
  int index;

  BuildBitmap( __wchar_bitmap, s2, __wchar_bitmap_size);

  for( ; *s1 != 0; ++s1) {
    index = WHICH8(*s1);
    bit = WHICH_BIT(*s1);
    if( (__wchar_bitmap[index] & bit) != 0) {
      return (wchar_t *)s1;
    }
  }
  return NULL;
}

/** The wcsrchr function locates the last occurrence of c in the wide string
    pointed to by s. The terminating null wide character is considered to be
    part of the wide string.

    @return   The wcsrchr function returns a pointer to the wide character,
              or a null pointer if c does not occur in the wide string.
**/
wchar_t *wcsrchr(const wchar_t *s, wchar_t c)
{
  wchar_t  *found  = NULL;

  do {
    if( *s == c)  found = (wchar_t *)s;
  } while( *s++ != 0);

  return found;
}

/** The wcsspn function computes the length of the maximum initial segment of
    the wide string pointed to by s1 which consists entirely of wide characters
    from the wide string pointed to by s2.

    @return   The wcsspn function returns the length of the segment.
**/
size_t wcsspn(const wchar_t *s1, const wchar_t *s2)
{
  size_t  length = 0;
  int     index;
  UINT8   bit;

  BuildBitmap( __wchar_bitmap, s2, __wchar_bitmap_size);

  for( ; *s1 != 0; ++s1) {
    index = WHICH8(*s1);
    bit = WHICH_BIT(*s1);
    if( (__wchar_bitmap[index] & bit) == 0)   break;
    ++length;
  }
  return length;
}

/** The wcsstr function locates the first occurrence in the wide string pointed
    to by s1 of the sequence of wide characters (excluding the terminating null
    wide character) in the wide string pointed to by s2.

    @return   The wcsstr function returns a pointer to the located wide string,
              or a null pointer if the wide string is not found. If s2 points
              to a wide string with zero length, the function returns s1.
**/
wchar_t *wcsstr(const wchar_t *s1, const wchar_t *s2)
{
  return (wchar_t *)StrStr( (CONST CHAR16 *)s1, (CONST CHAR16 *)s2);
}

/** A sequence of calls to the wcstok function breaks the wide string pointed
    to by s1 into a sequence of tokens, each of which is delimited by a wide
    character from the wide string pointed to by s2. The third argument points
    to a caller-provided wchar_t pointer into which the wcstok function stores
    information necessary for it to continue scanning the same wide string.

    The first call in a sequence has a non-null first argument and stores an
    initial value in the object pointed to by ptr. Subsequent calls in the
    sequence have a null first argument and the object pointed to by ptr is
    required to have the value stored by the previous call in the sequence,
    which is then updated. The separator wide string pointed to by s2 may be
    different from call to call.

    The first call in the sequence searches the wide string pointed to by s1
    for the first wide character that is not contained in the current separator
    wide string pointed to by s2. If no such wide character is found, then
    there are no tokens in the wide string pointed to by s1 and the wcstok
    function returns a null pointer. If such a wide character is found, it is
    the start of the first token.

    The wcstok function then searches from there for a wide character that is
    contained in the current separator wide string. If no such wide character
    is found, the current token extends to the end of the wide string pointed
    to by s1, and subsequent searches in the same wide string for a token
    return a null pointer. If such a wide character is found, it is overwritten
    by a null wide character, which terminates the current token.

    In all cases, the wcstok function stores sufficient information in the
    pointer pointed to by ptr so that subsequent calls, with a null pointer for
    s1 and the unmodified pointer value for ptr, shall start searching just
    past the element overwritten by a null wide character (if any).

    @return   The wcstok function returns a pointer to the first wide character
              of a token, or a null pointer if there is no token.
**/
wchar_t *wcstok(wchar_t * __restrict s1, const wchar_t * __restrict s2, wchar_t ** __restrict ptr)
{
  wchar_t        *Token = NULL;
  int             index;
  UINT8           bit;

  if(     (s1 == NULL)
      &&  ((s1 = *ptr) == NULL))
  {
    return  NULL;
  }

  // s2 can be different on each call, so build the bitmap each time.
  BuildBitmap( __wchar_bitmap, s2, __wchar_bitmap_size);

  // skip leading delimiters: all chars in s2
  for( ; *s1 != 0; ++s1) {
    index = WHICH8(*s1);
    bit = WHICH_BIT(*s1);
    if( (__wchar_bitmap[index] & bit) == 0)   break;
  }
  if( *s1 != 0)
  {
    // Remember this point, it is the start of the token
    Token = s1++;

    // find the next delimiter and replace it with a '\0'
    for( ; *s1 != 0; ++s1) {
      index = WHICH8(*s1);
      bit = WHICH_BIT(*s1);
      if( (__wchar_bitmap[index] & bit) != 0) {
        *s1++ = 0;
        *ptr = s1;
        return Token;
      }
    }
  }
  *ptr = NULL;
  return Token;
}

/** The wmemchr function locates the first occurrence of c in the initial n
    wide characters of the object pointed to by s.

    @return   The wmemchr function returns a pointer to the located wide
              character, or a null pointer if the wide character does not occur
              in the object.
**/
wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
  return (wchar_t *)ScanMem16( s, (UINTN)(n * sizeof(wchar_t)), (UINT16)c);
}
