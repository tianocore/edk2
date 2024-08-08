/** @file
  Defines the stack cookie variable for GCC, Clang and MSVC compilers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>

#if defined (__GNUC__) || defined (__clang__)
VOID  *__stack_chk_guard = (VOID *)(UINTN)0x0;

/**
  This function gets called when a gcc/clang generated stack cookie fails. This implementation does nothing when
  a stack cookie failure occurs.

**/
VOID
EFIAPI
__stack_chk_fail (
  VOID
  )
{
}

#elif defined (_MSC_VER)
VOID  *__security_cookie = (VOID *)(UINTN)0x0;
#endif
