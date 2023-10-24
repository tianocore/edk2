/** @file
  NULL instance of StandaloneMmCorePlatformHookLib.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/StandaloneMmCorePlatformHookLib.h>

/**
  Performs platform specific tasks before invoking registered SMI handlers.

  This function performs platform specific tasks before invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookBeforeMmDispatch (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Performs platform specific tasks after invoking registered SMI handlers.

  This function performs platform specific tasks after invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookAfterMmDispatch (
  VOID
  )
{
  return EFI_SUCCESS;
}
