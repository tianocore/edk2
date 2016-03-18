/** @file
Defined the Ioh device path which will be used by
platform Bbd to perform the platform policy connect.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IohBds.h"

//
// Predefined platform root bridge
//
PLATFORM_ROOT_BRIDGE_DEVICE_PATH gPlatformRootBridge0 = {
  gPciRootBridge,
  gEndEntire
};

EFI_DEVICE_PATH_PROTOCOL* gPlatformRootBridges [] = {
  (EFI_DEVICE_PATH_PROTOCOL*)&gPlatformRootBridge0,
  NULL
};

//
// Ioh USB EHCI controller device path
//
IOH_PCI_USB_DEVICE_PATH gIohUsbDevicePath0 = {
  gPciRootBridge,
  PCI_DEVICE_PATH_NODE(IOH_EHCI_FUNCTION_NUMBER, IOH_USB_EHCI_DEVICE_NUMBER),
  gEndEntire
};

//
// Ioh predefined device connecting option
//
EFI_DEVICE_PATH_PROTOCOL* gDeviceConnectOption [] = {
  //  (EFI_DEVICE_PATH_PROTOCOL*)&gIohUsbDevicePath0,
  NULL
};

