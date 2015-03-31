/** @file
  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BdsPlatform.h"

//
// Predefined platform default time out value
//
UINT16                      gPlatformBootTimeOutDefault = 5;

ACPI_HID_DEVICE_PATH       gPnp16550ComPortDeviceNode = gPnp16550ComPort;
UART_DEVICE_PATH           gUartDeviceNode            = gUart;
VENDOR_DEVICE_PATH         gTerminalTypeDeviceNode    = gPcAnsiTerminal;
VENDOR_DEVICE_PATH         gUartDeviceVenderNode      = gUartVender;
//
// Predefined platform root bridge
//
PLATFORM_ROOT_BRIDGE_DEVICE_PATH  gPlatformRootBridge0 = {
  gPciRootBridge,
  gEndEntire
};

EFI_DEVICE_PATH_PROTOCOL          *gPlatformRootBridges[] = {
  (EFI_DEVICE_PATH_PROTOCOL *) &gPlatformRootBridge0,
  NULL
};

//
// Platform specific keyboard device path
//

///
/// Predefined platform default console device path
///
BDS_CONSOLE_CONNECT_ENTRY   gPlatformConsole[] = {
  {
    NULL,
    0
  }
};

///
/// Predefined platform specific driver option
///
EFI_DEVICE_PATH_PROTOCOL    *gPlatformDriverOption[] = { NULL };

///
/// Predefined platform connect sequence
///
EFI_DEVICE_PATH_PROTOCOL    *gPlatformConnectSequence[] = { NULL };
