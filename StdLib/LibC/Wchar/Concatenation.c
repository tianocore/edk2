/** @file
    Concatenation Functions for <wchar.h>.

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

/** The wcscat function appends a copy of the wide string pointed to by s2
    (including the terminating null wide character) to the end of the wide
    string pointed to by s1. The initial wide character of s2 overwrites the
    null wide character at the end of s1.

    @return   The wcscat function returns the value of s1.
**/
wchar_t *wcscat(wchar_t * __restrict s1, const wchar_t * __restrict s2)
{
  return (wchar_t *)StrCat( (CHAR16 *)s1, (CONST CHAR16 *)s2);
}

/** The wcsncat function appends not more than n wide characters (a null wide
    character and those that follow it are not appended) from the array pointed
    to by s2 to the end of the wide string pointed to by s1. The initial wide
    character of s2 overwrites the null wide character at the end of s1.
    A terminating null wide character is always appended to the result.

    @return   The wcsncat function returns the value of s1.
**/
wchar_t *wcsncat(wchar_t * __restrict s1, const wchar_t * __restrict s2, size_t n)
{
  return (wchar_t *)StrnCat( (CHAR16 *)s1, (CONST CHAR16 *)s2, (UINTN)n);
}
