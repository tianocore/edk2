/** @file
  Provides the required functionality for handling stack
  cookie check failures in GCC.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi/UefiBaseType.h>

#include <Library/BaseLib.h>
#include <Library/StackCheckLib.h>

/**
  Triggers an interrupt using the stack cookie exception vector.
**/
VOID
EFIAPI
TriggerStackCookieInterrupt (
  EFI_PHYSICAL_ADDRESS  ExceptionAddress
  );

VOID  *__stack_chk_guard = (VOID *)(UINTN)STACK_COOKIE_VALUE;

/**
  This function gets called when a gcc/clang generated stack cookie fails. This implementation calls into a platform
  failure hook lib and then triggers the stack cookie interrupt.

**/
VOID
EFIAPI
__stack_chk_fail (
  VOID
  )
{
  TriggerStackCookieInterrupt ((EFI_PHYSICAL_ADDRESS)(UINTN)RETURN_ADDRESS (0));
}
