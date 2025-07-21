/** @file
  This library allows platforms to customize MM loading and calling.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/MmIplPlatformHookLib.h>
#include <Library/PeCoffLib.h>

/**
  Performs platform specific tasks to alter how MM is loaded.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookBeforeMmLoad (
  IN PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Allows platforms to override how MM is called, rather than calling the entrypoint.

  @retval EFI_SUCCESS       The platform hook completes successfully.
  @retval Other values      The platform hook cannot complete due to some error.

**/
EFI_STATUS
EFIAPI
PlatformHookCallMmCore (
  IN     PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext,
  IN     VOID                          *Context,
  IN OUT EFI_STATUS                    *PiMmCoreStatus
  )
{
  return EFI_UNSUPPORTED;
}
