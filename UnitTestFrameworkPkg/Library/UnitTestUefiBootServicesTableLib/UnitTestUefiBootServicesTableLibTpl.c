/** @file
  Implementation of Task Priority Level (TPL) related services in the UEFI Boot Services table for use in unit tests.

Copyright (c) Microsoft Corporation
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestUefiBootServicesTableLib.h"

/**
  Raise the task priority level to the new level.
  High level is implemented by disabling processor interrupts.

  @param  NewTpl  New task priority level

  @return The previous task priority level

**/
EFI_TPL
EFIAPI
UnitTestRaiseTpl (
  IN EFI_TPL  NewTpl
  )
{
  return TPL_APPLICATION;
}

/**
  Lowers the task priority to the previous value.   If the new
  priority unmasks events at a higher priority, they are dispatched.

  @param  NewTpl  New, lower, task priority

**/
VOID
EFIAPI
UnitTestRestoreTpl (
  IN EFI_TPL  NewTpl
  )
{
  return;
}
