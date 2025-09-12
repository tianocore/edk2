/** @file
  This library allows platforms to customize MM loading and calling.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _MM_IPL_PLATFORM_HOOK_LIB_H_
#define _MM_IPL_PLATFORM_HOOK_LIB_H_

#include <PiPei.h>
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
  );

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
  );

#endif
