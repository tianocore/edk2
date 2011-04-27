/** @file
    Miscelaneous Functions for <wchar.h>.

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

/** The wcslen function computes the length of the wide string pointed to by s.

    @return   The wcslen function returns the number of wide characters that
              precede the terminating null wide character.
**/
size_t wcslen(const wchar_t *s)
{
  return (size_t)StrLen( (CONST CHAR16 *)s);
}

/** The wmemset function copies the value of c into each of the first n wide
    characters of the object pointed to by s.

    @return   The wmemset function returns the value of s.
**/
wchar_t *wmemset(wchar_t *s, wchar_t c, size_t n)
{
  return (wchar_t *)SetMem16( s, (UINTN)(n * sizeof(wchar_t)), (UINT16)c);
}
