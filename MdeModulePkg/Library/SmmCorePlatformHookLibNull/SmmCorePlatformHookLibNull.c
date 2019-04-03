/** @file
  Null instance of SmmCorePlatformHookLibNull.

  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/SmmCorePlatformHookLib.h>

/**
  Performs platform specific tasks before invoking registered SMI handlers.

  This function performs platform specific tasks before invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The paltform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookBeforeSmmDispatch (
  VOID
  )
{
  return EFI_SUCCESS;
}


/**
  Performs platform specific tasks after invoking registered SMI handlers.

  This function performs platform specific tasks after invoking registered SMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The paltform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookAfterSmmDispatch (
  VOID
  )
{
  return EFI_SUCCESS;
}
