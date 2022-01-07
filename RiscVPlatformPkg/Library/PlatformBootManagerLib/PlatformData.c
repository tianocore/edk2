/** @file
  Defined the platform specific device path which will be filled to
  ConIn/ConOut variables.

Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBootManager.h"

//
// Platform specific serial device path
//
SERIAL_CONSOLE_DEVICE_PATH  gSerialConsoleDevicePath0 = {
  {
    { HARDWARE_DEVICE_PATH,  HW_VENDOR_DP,                   { sizeof (VENDOR_DEVICE_PATH),       0 }
    },
    EFI_SERIAL_DXE_GUID  // Use the driver's GUID
  },
  {
    { MESSAGING_DEVICE_PATH, MSG_UART_DP,                    { sizeof (UART_DEVICE_PATH),         0 }
    },
    0,                  // Reserved
    115200,             // BaudRate
    8,                  // DataBits
    1,                  // Parity
    1                   // StopBits
  },
  {
    { MESSAGING_DEVICE_PATH, MSG_VENDOR_DP,                  { sizeof (VENDOR_DEVICE_PATH),       0 }
    },
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  { END_DEVICE_PATH_TYPE,  END_ENTIRE_DEVICE_PATH_SUBTYPE, { sizeof (EFI_DEVICE_PATH_PROTOCOL), 0 }
  }
};

//
// Predefined platform default console device path
//
PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gSerialConsoleDevicePath0,
    CONSOLE_OUT | CONSOLE_IN
  },
  {
    NULL,
    0
  }
};
