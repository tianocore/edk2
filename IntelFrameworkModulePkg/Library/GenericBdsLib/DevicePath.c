/** @file
  BDS internal function define the default device path string, it can be
  replaced by platform device path.

Copyright (c) 2004 - 2013, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "InternalBdsLib.h"

/**
  This function converts an input device structure to a Unicode string.

  @param DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  )
{
  return ConvertDevicePathToText (DevPath, TRUE, TRUE);
}
