/** @file
    Comparison Functions for <wchar.h>.

    Unless explicitly stated otherwise, the functions defined in this file order
    two wide characters the same way as two integers of the underlying integer
    type designated by wchar_t.

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

#include  <wchar.h>

/** The wcscmp function compares the wide string pointed to by s1 to the wide
    string pointed to by s2.

    @return   The wcscmp function returns an integer greater than, equal to, or
              less than zero, accordingly as the wide string pointed to by s1
              is greater than, equal to, or less than the wide string
              pointed to by s2.
**/
int wcscmp(const wchar_t *s1, const wchar_t *s2)
{
  return (int)StrCmp( (CONST CHAR16 *)s1, (CONST CHAR16 *)s2);
}

/** The wcscoll function compares the wide string pointed to by s1 to the wide
    string pointed to by s2, both interpreted as appropriate to the LC_COLLATE
    category of the current locale.

    @return   The wcscoll function returns an integer greater than, equal to,
              or less than zero, accordingly as the wide string pointed to by
              s1 is greater than, equal to, or less than the wide string
              pointed to by s2 when both are interpreted as appropriate to
              the current locale.
**/
//int wcscoll(const wchar_t *s1, const wchar_t *s2)
//{
//  return -1;  // STUBB
//}

/** The wcsncmp function compares not more than n wide characters (those that
    follow a null wide character are not compared) from the array pointed to by
    s1 to the array pointed to by s2.

    @return   The wcsncmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the possibly null-terminated
              array pointed to by s1 is greater than, equal to, or less than
              the possibly null-terminated array pointed to by s2.
**/
int wcsncmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
  return (int)StrnCmp( (CONST CHAR16 *)s1, (CONST CHAR16 *)s2, (UINTN)n);
}

/** The wcsxfrm function transforms the wide string pointed to by s2 and places
    the resulting wide string into the array pointed to by s1. The
    transformation is such that if the wcscmp function is applied to two
    transformed wide strings, it returns a value greater than, equal to, or
    less than zero, corresponding to the result of the wcscoll function applied
    to the same two original wide strings. No more than n wide characters are
    placed into the resulting array pointed to by s1, including the terminating
    null wide character. If n is zero, s1 is permitted to be a null pointer.

    @return   The wcsxfrm function returns the length of the transformed wide
              string (not including the terminating null wide character). If
              the value returned is n or greater, the contents of the array
              pointed to by s1 are indeterminate.
**/
//size_t wcsxfrm(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n)
//{
//  return n;  // STUBB
//}

/** The wmemcmp function compares the first n wide characters of the object
    pointed to by s1 to the first n wide characters of the object pointed to
    by s2.

    @return   The wmemcmp function returns an integer greater than, equal to,
              or less than zero, accordingly as the object pointed to by s1 is
              greater than, equal to, or less than the object pointed to by s2.
**/
int wmemcmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
  return (int)CompareMem( s1, s2, (UINTN)(n * sizeof(wchar_t)));
}
