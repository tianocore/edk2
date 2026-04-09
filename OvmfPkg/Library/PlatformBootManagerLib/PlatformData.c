/** @file
  Defined the platform specific device path which will be used by
  platform Bbd to perform the platform policy connect.

  Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BdsPlatform.h"
#include <Guid/QemuRamfb.h>
#include <Guid/SerialPortLibVendor.h>

//
// Vendor UART Device Path structure
//
#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          VendorHardware;
  UART_DEVICE_PATH            Uart;
  VENDOR_DEVICE_PATH          TerminalType;
  EFI_DEVICE_PATH_PROTOCOL    End;
} VENDOR_UART_DEVICE_PATH;
#pragma pack ()

//
// USB Keyboard Device Path structure
//
#pragma pack (1)
typedef struct {
  USB_CLASS_DEVICE_PATH       Keyboard;
  EFI_DEVICE_PATH_PROTOCOL    End;
} USB_KEYBOARD_DEVICE_PATH;
#pragma pack ()

//
// QemuRamfb Device Path structure
//
#pragma pack (1)
typedef struct {
  VENDOR_DEVICE_PATH          Vendor;
  ACPI_ADR_DEVICE_PATH        AcpiAdr;
  EFI_DEVICE_PATH_PROTOCOL    End;
} VENDOR_RAMFB_DEVICE_PATH;
#pragma pack ()

ACPI_HID_DEVICE_PATH  gPnpPs2KeyboardDeviceNode  = gPnpPs2Keyboard;
ACPI_HID_DEVICE_PATH  gPnp16550ComPortDeviceNode = gPnp16550ComPort;
UART_DEVICE_PATH      gUartDeviceNode            = gUart;
VENDOR_DEVICE_PATH    gTerminalTypeDeviceNode    = gVtUtf8Terminal;

//
// Platform specific keyboard device path
//

//
// Debug Agent UART Device Path
//
VENDOR_UART_DEVICE_PATH  gDebugAgentUartDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EFI_DEBUG_AGENT_GUID,
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,  // Reserved
    0,  // BaudRate - Default
    0,  // DataBits - Default
    0,  // Parity   - Default
    0,  // StopBits - Default
  },
  gVtUtf8Terminal,
  gEndEntire
};

STATIC USB_KEYBOARD_DEVICE_PATH  gUsbKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8)sizeof (USB_CLASS_DEVICE_PATH),
        (UINT8)(sizeof (USB_CLASS_DEVICE_PATH) >> 8)
      }
    },
    0xFFFF, // VendorId: any
    0xFFFF, // ProductId: any
    3,      // DeviceClass: HID
    1,      // DeviceSubClass: boot
    1       // DeviceProtocol: keyboard
  },
  gEndEntire
};

STATIC VENDOR_RAMFB_DEVICE_PATH  gQemuRamfbDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    QEMU_RAMFB_GUID,
  },
  {
    {
      ACPI_DEVICE_PATH,
      ACPI_ADR_DP,
      {
        (UINT8)(sizeof (ACPI_ADR_DEVICE_PATH)),
        (UINT8)((sizeof (ACPI_ADR_DEVICE_PATH)) >> 8)
      }
    },
    ACPI_DISPLAY_ADR (
      1,                                       // DeviceIdScheme
      0,                                       // HeadId
      0,                                       // NonVgaOutput
      1,                                       // BiosCanDetect
      0,                                       // VendorInfo
      ACPI_ADR_DISPLAY_TYPE_EXTERNAL_DIGITAL,  // Type
      0,                                       // Port
      0                                        // Index
      ),
  },
  gEndEntire
};

STATIC VENDOR_UART_DEVICE_PATH  gXenConsoleDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    EDKII_SERIAL_PORT_LIB_VENDOR_GUID
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,
    FixedPcdGet64 (PcdUartDefaultBaudRate),
    FixedPcdGet8 (PcdUartDefaultDataBits),
    FixedPcdGet8 (PcdUartDefaultParity),
    FixedPcdGet8 (PcdUartDefaultStopBits),
  },
  gVtUtf8Terminal,
  gEndEntire
};

//
// Predefined platform default console device path
//
PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gDebugAgentUartDevicePath,
    (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gUsbKeyboardDevicePath,
    CONSOLE_IN
  },
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gQemuRamfbDevicePath,
    CONSOLE_OUT
  },
  {
    NULL,
    0
  }
};

PLATFORM_CONSOLE_CONNECT_ENTRY  gXenPlatformConsole[] = {
  {
    (EFI_DEVICE_PATH_PROTOCOL *)&gXenConsoleDevicePath,
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
EFI_DEVICE_PATH_PROTOCOL  *gPlatformConnectSequence[] = { NULL };
