/** @file
  Standalone Mm Core Platform Hook Library. This library class defines a set of platform
  hooks called by the Standalone Mm Core.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __STANDALONE_MM_CORE_PLATFORM_HOOK_LIB__
#define __STANDALONE_MM_CORE_PLATFORM_HOOK_LIB__

/**
  Performs platform specific tasks before invoking registered MMI handlers.

  This function performs platform specific tasks before invoking registered MMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The paltform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookBeforeMmDispatch (
  VOID
  );

/**
  Performs platform specific tasks after invoking registered MMI handlers.

  This function performs platform specific tasks after invoking registered MMI handlers.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The paltform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookAfterMmDispatch (
  VOID
  );

#endif
