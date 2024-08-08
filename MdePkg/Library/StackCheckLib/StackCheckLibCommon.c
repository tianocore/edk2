/** @file
  Provides the required functionality for handling stack
  cookie check failures.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/StackCheckFailureHookLib.h>

/**
  Calls an interrupt using the vector specified by PcdStackCookieExceptionVector
**/
VOID
TriggerStackCookieInterrupt (
  VOID
  );

#if defined (__GNUC__) || defined (__clang__)

VOID  *__stack_chk_guard = (VOID *)(UINTN)STACK_COOKIE_VALUE;

/**
  This function gets called when a gcc/clang generated stack cookie fails. This implementation calls into a platform
  failure hook lib and then triggers the stack cookie interrupt.

**/
VOID
__stack_chk_fail (
  VOID
  )
{
  DEBUG ((DEBUG_ERROR, "Stack cookie check failed at address 0x%llx!\n", RETURN_ADDRESS (0)));
  StackCheckFailureHook (RETURN_ADDRESS (0));
  TriggerStackCookieInterrupt ();
}

#elif defined (_MSC_VER)
VOID  *__security_cookie = (VOID *)(UINTN)STACK_COOKIE_VALUE;

/**
  This function gets called when an MSVC generated stack cookie fails. This implementation calls into a platform
  failure hook lib and then triggers the stack cookie interrupt.

  @param[in] ActualCookieValue  The value that was written onto the stack, corrupting the stack cookie.

**/
VOID
StackCheckFailure (
  VOID  *ActualCookieValue
  )
{
  DEBUG ((DEBUG_ERROR, "Stack cookie check failed at address 0x%llx!\n", RETURN_ADDRESS (0)));
  StackCheckFailureHook (RETURN_ADDRESS (0));
  TriggerStackCookieInterrupt ();
}

#endif
