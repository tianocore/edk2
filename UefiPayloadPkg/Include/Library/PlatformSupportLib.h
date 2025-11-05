/** @file
  Bootloader Platform Support library. Platform can provide an implementation of this
  library class to provide hooks that may be required for some type of
  platform features.

Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BOOTLOADER_PLATFORM_SUPPORT_LIB_
#define BOOTLOADER_PLATFORM_SUPPORT_LIB_

/**
  Parse platform specific information from bootloader

  @retval RETURN_SUCCESS       The platform specific coreboot support succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific coreboot support could not be completed.

**/
EFI_STATUS
EFIAPI
ParsePlatformInfo (
  VOID
  );

#endif // __BOOTLOADER_PLATFORM_SUPPORT_LIB__
