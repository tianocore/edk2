/** @file
  Internal function defines the default device path string for SecureBoot configuration module.

Copyright (c) 2012 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecureBootConfigImpl.h"


/**
  This function converts an input device structure to a Unicode string.

  @param[in] DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  return ConvertDevicePathToText (
           DevPath,
           FALSE,
           TRUE
           );
}

