/** @file
  Defines the stack cookie variable for GCC and Clang compilers.

  Copyright (c) Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/StackCheckLib.h>

__UINTPTR_TYPE__  __stack_chk_guard = 0;

/**
  This function gets called when a gcc/clang generated stack cookie fails.

  This function must not return.  GCC/Clang emit the call to it as if it were
  declared __attribute__((noreturn)): no instruction at all is generated after
  the call site.  If this function returns, execution therefore falls through
  into whatever the linker happened to place next in the section, silently
  running an unrelated function on a stack frame that was never unwound.  Spin
  here instead, so that a stack cookie failure stops at the point of detection
  instead of turning into a corrupt-stack crash somewhere else entirely.

**/
VOID
EFIAPI
__stack_chk_fail (
  VOID
  )
{
  volatile BOOLEAN  DeadLoop;

  for (DeadLoop = TRUE; DeadLoop; ) {
  }
}
