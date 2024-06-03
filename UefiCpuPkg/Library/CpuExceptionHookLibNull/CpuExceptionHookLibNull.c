/** @file
  Library which provides a hook that is called when a cpu exception occurs

  Copyright (c) Microsoft Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>
#include <Uefi.h>
#include <Library/CpuExceptionHookLib.h>

/**
  Exception Handler hook library. Called when an exception occurs to allow
  platforms to perform additional actions.

  @param[in] ExceptionType       Cpu Exception Type which was triggered
  @param[in] SystemContext       Pointer the the CPU Context when the exception was triggered. Hook library
                                 is responsible for determining the correct cpu architecture type.
**/
VOID
EFIAPI
CpuExceptionHookLib (
  IN EFI_EXCEPTION_TYPE  ExceptionType,
  IN EFI_SYSTEM_CONTEXT  SystemContext
  )
{
  return;
}
