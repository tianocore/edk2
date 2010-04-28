/*++

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name: 

  PlatformData.c

Abstract:
  
  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

--*/

#include "BdsPlatform.h"

//
// Predefined platform default time out value
//
UINT16   gPlatformBootTimeOutDefault = 3;

ACPI_HID_DEVICE_PATH       gPnpPs2KeyboardDeviceNode  = gPnpPs2Keyboard;
ACPI_HID_DEVICE_PATH       gPnp16550ComPortDeviceNode = gPnp16550ComPort;
UART_DEVICE_PATH           gUartDeviceNode            = gUart;
VENDOR_DEVICE_PATH         gTerminalTypeDeviceNode    = gPcAnsiTerminal;

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

USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8) (sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8) ((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff,           // VendorId 
    0xffff,           // ProductId 
    CLASS_HID,        // DeviceClass 
    SUBCLASS_BOOT,    // DeviceSubClass
    PROTOCOL_KEYBOARD // DeviceProtocol
  },

  { 
    END_DEVICE_PATH_TYPE, 
    END_ENTIRE_DEVICE_PATH_SUBTYPE, 
    {
      END_DEVICE_PATH_LENGTH, 
      0
    }
  }
};

/*
//
// Platform specific Dummy ISA keyboard device path
//
PLATFORM_DUMMY_ISA_KEYBOARD_DEVICE_PATH     gDummyIsaKeyboardDevicePath = {
  gPciRootBridge,
  gPciIsaBridge,
  gPnpPs2Keyboard,
  gEndEntire
};

//
// Platform specific Dummy ISA serial device path
//
PLATFORM_DUMMY_ISA_SERIAL_DEVICE_PATH   gDummyIsaSerialDevicePath = {
  gPciRootBridge,
  gPciIsaBridge,
  gPnp16550ComPort,
  gUart,
  gPcAnsiTerminal,
  gEndEntire
};

//
// Platform specific Dummy PCI VGA device path
//
PLATFORM_DUMMY_PCI_VGA_DEVICE_PATH gDummyPciVgaDevicePath = {
  gPciRootBridge,
  PCI_DEVICE_PATH_NODE(0, 0x2),
  gEndEntire
};

//
// Platform specific Dummy PCI serial device path
//
PLATFORM_DUMMY_PCI_SERIAL_DEVICE_PATH gDummyPciSerialDevicePath = {
  gPciRootBridge,
  gP2PBridge,
  PCI_DEVICE_PATH_NODE(0, 0x0),
  gUart,
  gPcAnsiTerminal,
  gEndEntire
};
*/
//
// Predefined platform default console device path
//
BDS_CONSOLE_CONNECT_ENTRY         gPlatformConsole[] = {
  //
  // need update dynamically
  //
//  {
//    (EFI_DEVICE_PATH_PROTOCOL *) &gDummyIsaSerialDevicePath,
//    (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)
//  },
//  {
//    (EFI_DEVICE_PATH_PROTOCOL *) &gDummyIsaKeyboardDevicePath,
//    (CONSOLE_IN | STD_ERROR)
//  },
//  {
//    (EFI_DEVICE_PATH_PROTOCOL *) &gDummyPciVgaDevicePath,
//    CONSOLE_OUT
//  },
//  {
//    (EFI_DEVICE_PATH_PROTOCOL *) &gDummyPciSerialDevicePath,
//    (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)
//  },
  {
    (EFI_DEVICE_PATH_PROTOCOL*) &gUsbClassKeyboardDevicePath, 
    CONSOLE_IN
  },
  {
    NULL,
    0
  }
};

//
// Predefined platform specific driver option
//
EFI_DEVICE_PATH_PROTOCOL    *gPlatformDriverOption[] = { NULL };

//
// Predefined platform connect sequence
//
EFI_DEVICE_PATH_PROTOCOL    *gPlatformConnectSequence[] = { NULL };

