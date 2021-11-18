/*++ @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBm.h"

EMU_PLATFORM_UGA_DEVICE_PATH  gGopDevicePath = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EMU_THUNK_PROTOCOL_GUID
    },
    0
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        },
      },
      EMU_GRAPHICS_WINDOW_PROTOCOL_GUID,
    },
    0
  },
  gEndEntire
};

EMU_PLATFORM_UGA_DEVICE_PATH  gGopDevicePath2 = {
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        }
      },
      EMU_THUNK_PROTOCOL_GUID
    },
    0
  },
  {
    {
      {
        HARDWARE_DEVICE_PATH,
        HW_VENDOR_DP,
        {
          (UINT8)(sizeof (EMU_VENDOR_DEVICE_PATH_NODE)),
          (UINT8)((sizeof (EMU_VENDOR_DEVICE_PATH_NODE)) >> 8)
        },
      },
      EMU_GRAPHICS_WINDOW_PROTOCOL_GUID,
    },
    1
  },
  gEndEntire
};

//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY  gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gGopDevicePath,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gGopDevicePath2,
    (CONSOLE_OUT | CONSOLE_IN)
  },
  {
    NULL,
    0
  }
};
