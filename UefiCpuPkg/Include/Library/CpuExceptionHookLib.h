/** @file
  Library which provides a hook that is called when a cpu exception occurs

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CPU_EXCEPTION_HOOK_LIB_H_
#define CPU_EXCEPTION_HOOK_LIB_H_

#include <Uefi.h>
#include <Protocol/DebugSupport.h>

/**
  Hook function called when an exception has occured. The exception context is passed
  to allow the add in functionality to use the exception context to perform platform
  specific tasks.

  @param[in] ExceptionType       Cpu Exception Type which was triggered
  @param[in] SystemContext       Pointer to the CPU Context when the exception was triggered. Hook library
                                 is responsible for determining the correct cpu architecture type.
**/
VOID
EFIAPI
CpuExceptionHookLib (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  );

#endif
