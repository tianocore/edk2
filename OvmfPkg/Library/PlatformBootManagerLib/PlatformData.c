/** @file
  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

  Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsPlatform.h"

//
// Debug Agent UART Device Path structure
//
#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH        VendorHardware;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} VENDOR_UART_DEVICE_PATH;
#pragma pack()

ACPI_HID_DEVICE_PATH       gPnpPs2KeyboardDeviceNode  = gPnpPs2Keyboard;
ACPI_HID_DEVICE_PATH       gPnp16550ComPortDeviceNode = gPnp16550ComPort;
UART_DEVICE_PATH           gUartDeviceNode            = gUart;
VENDOR_DEVICE_PATH         gTerminalTypeDeviceNode    = gPcAnsiTerminal;

//
// Platform specific keyboard device path
//


//
// Debug Agent UART Device Path
//
VENDOR_UART_DEVICE_PATH gDebugAgentUartDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_DEBUG_AGENT_GUID,
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8) (sizeof (UART_DEVICE_PATH)),
        (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,  // Reserved
    0,  // BaudRate - Default
    0,  // DataBits - Default
    0,  // Parity   - Default
    0,  // StopBits - Default
  },
  gPcAnsiTerminal,
  gEndEntire
};


//
// Predefined platform default console device path
//
PLATFORM_CONSOLE_CONNECT_ENTRY   gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *) &gDebugAgentUartDevicePath,
    (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)
  },
  {
    NULL,
    0
  }
};

//
// Predefined platform connect sequence
//
EFI_DEVICE_PATH_PROTOCOL    *gPlatformConnectSequence[] = { NULL };

