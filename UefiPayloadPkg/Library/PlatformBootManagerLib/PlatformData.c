/** @file
  Defined the platform specific device path which will be filled to
  ConIn/ConOut variables.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "PlatformBootManager.h"

///
/// Predefined platform default console device path
///
GLOBAL_REMOVE_IF_UNREFERENCED PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[] = {
  {
    NULL,
    0
  }
};

GLOBAL_REMOVE_IF_UNREFERENCED USB_CLASS_FORMAT_DEVICE_PATH  gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8)(sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8)((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff,           // VendorId
    0xffff,           // ProductId
    CLASS_HID,        // DeviceClass
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },
  gEndEntire
};
