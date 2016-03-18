/** @file
    Copying Functions for <wchar.h>.

    Unless explicitly stated otherwise, if the execution of a function declared
    in this file causes copying to take place between objects that overlap, the
    behavior is undefined.

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

/** The wcscpy function copies the wide string pointed to by s2 (including the
    terminating null wide character) into the array pointed to by s1.

    @return   The wcscpy function returns the value of s1.
**/
wchar_t *wcscpy(wchar_t * __restrict s1, const wchar_t * __restrict s2)
{
  return (wchar_t *)StrCpy( (CHAR16 *)s1, (CONST CHAR16 *)s2);
}

/** The wcsncpy function copies not more than n wide characters (those that
    follow a null wide character are not copied) from the array pointed to by
    s2 to the array pointed to by s1.

    If the array pointed to by s2 is a wide string that is shorter than n wide
    characters, null wide characters are appended to the copy in the array
    pointed to by s1, until n wide characters in all have been written.

    @return   The wcsncpy function returns the value of s1.
**/
wchar_t *wcsncpy(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n)
{
  return (wchar_t *)StrnCpy( (CHAR16 *)s1, (CONST CHAR16 *)s2, (UINTN)n);
}

/** The wmemcpy function copies n wide characters from the object pointed to by
    s2 to the object pointed to by s1.

    Use this function if you know that s1 and s2 DO NOT Overlap.  Otherwise,
    use wmemmove.

    @return   The wmemcpy function returns the value of s1.
**/
wchar_t *wmemcpy(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n)
{
  return (wchar_t *)CopyMem( s1, s2, (UINTN)(n * sizeof(wchar_t)));
}

/** The wmemmove function copies n wide characters from the object pointed to by
    s2 to the object pointed to by s1. The objects pointed to by s1 and s2 are
    allowed to overlap.

    Because the UEFI BaseMemoryLib function CopyMem explicitly handles
    overlapping source and destination objects, this function and wmemcpy are
    implemented identically.

    For programming clarity, it is recommended that you use wmemcpy if you know
    that s1 and s2 DO NOT Overlap.  If s1 and s2 might possibly overlap, then
    use wmemmove.

    @return   The wmemmove function returns the value of s1.
**/
wchar_t *wmemmove(wchar_t *s1, const wchar_t *s2, size_t n)
{
  return (wchar_t *)CopyMem( s1, s2, (UINTN)(n * sizeof(wchar_t)));
}
