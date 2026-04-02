/** @file
  This file is empty to allow host applications
  to use the MSVC C runtime lib that provides
  stack cookie definitions without breaking the
  build.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

extern VOID  *__security_cookie;

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
