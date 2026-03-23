/** @file
  Defines the stack cookie variable for GCC, Clang and MSVC compilers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/StackCheckLib.h>

VOID  *__security_cookie = (VOID *)(UINTN)0x0;

/**
  This function gets called when an MSVC generated stack cookie fails. This implementation calls into a platform
  failure hook lib and then triggers the stack cookie interrupt.

  @param[in] ActualCookieValue  The value that was written onto the stack, corrupting the stack cookie.

**/
VOID
EFIAPI
StackCheckFailure (
  VOID  *ActualCookieValue
  )
{
}
