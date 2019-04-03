/** @file
Defined the platform specific device path which will be filled to
ConIn/ConOut variables.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBootManager.h"

///
/// the short form device path for Usb keyboard
///
#define CLASS_HID           3
#define SUBCLASS_BOOT       1
#define PROTOCOL_KEYBOARD   1

///
/// PcdDefaultTerminalType values
///
#define PCANSITYPE                0
#define VT100TYPE                 1
#define VT100PLUSTYPE             2
#define VTUTF8TYPE                3
#define TTYTERMTYPE               4

//
// Below is the platform console device path
//
typedef struct {
  ACPI_HID_DEVICE_PATH            PciRootBridge;
  PCI_DEVICE_PATH                 PciUart;
  UART_DEVICE_PATH                Uart;
  VENDOR_DEVICE_PATH              TerminalType;
  EFI_DEVICE_PATH_PROTOCOL        End;
} PCI_UART_DEVICE_PATH;

typedef struct {
  VENDOR_DEVICE_PATH        VendorHardware;
  UART_DEVICE_PATH          Uart;
  VENDOR_DEVICE_PATH        TerminalType;
  EFI_DEVICE_PATH_PROTOCOL  End;
} VENDOR_UART_DEVICE_PATH;

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;

#define PNPID_DEVICE_PATH_NODE(PnpId) \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      } \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
  }

#define PCI_DEVICE_PATH_NODE(Func, Dev) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    (Func), \
    (Dev) \
  }

#define gEndEntire \
  { \
    END_DEVICE_PATH_TYPE, \
    END_ENTIRE_DEVICE_PATH_SUBTYPE, \
    { \
      END_DEVICE_PATH_LENGTH, \
      0 \
    } \
  }

//
// Platform specific serial device path
//
PCI_UART_DEVICE_PATH   gPciUartDevicePath0 = {
  PNPID_DEVICE_PATH_NODE(0x0A03),
  PCI_DEVICE_PATH_NODE(1, 20),
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,         // Reserved
    921600,    // BaudRate
    8,         // DataBits
    1,         // Parity
    1          // StopBits
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      },
    },
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

PCI_UART_DEVICE_PATH   gPciUartDevicePath1 = {
  PNPID_DEVICE_PATH_NODE(0x0A03),
  PCI_DEVICE_PATH_NODE(5, 20),
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_UART_DP,
      {
        (UINT8)(sizeof (UART_DEVICE_PATH)),
        (UINT8)((sizeof (UART_DEVICE_PATH)) >> 8)
      }
    },
    0,         // Reserved
    921600,    // BaudRate
    8,         // DataBits
    1,         // Parity
    1          // StopBits
  },
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

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
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_VENDOR_DP,
      {
        (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
        (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    DEVICE_PATH_MESSAGING_PC_ANSI
  },
  gEndEntire
};

USB_CLASS_FORMAT_DEVICE_PATH gUsbClassKeyboardDevicePath = {
  {
    {
      MESSAGING_DEVICE_PATH,
      MSG_USB_CLASS_DP,
      {
        (UINT8)(sizeof (USB_CLASS_DEVICE_PATH)),
        (UINT8)((sizeof (USB_CLASS_DEVICE_PATH)) >> 8)
      }
    },
    0xffff,              // VendorId  - Match any vendor
    0xffff,              // ProductId - Match any product
    CLASS_HID,           // DeviceClass
    SUBCLASS_BOOT,       // DeviceSubClass
    PROTOCOL_KEYBOARD    // DeviceProtocol
  },
  gEndEntire
};

//
// Predefined platform default console device path
//
PLATFORM_CONSOLE_CONNECT_ENTRY   gPlatformConsole[] = {
  { (EFI_DEVICE_PATH_PROTOCOL *) &gPciUartDevicePath0,         (CONSOLE_OUT | CONSOLE_IN) },
  { (EFI_DEVICE_PATH_PROTOCOL *) &gPciUartDevicePath1,         (CONSOLE_OUT | CONSOLE_IN) },
  { (EFI_DEVICE_PATH_PROTOCOL *) &gDebugAgentUartDevicePath,   (CONSOLE_OUT | CONSOLE_IN) },
  { (EFI_DEVICE_PATH_PROTOCOL *) &gUsbClassKeyboardDevicePath, (CONSOLE_IN)               },
  { NULL, 0 }
};

EFI_STATUS
EFIAPI
InitializePlatformBootManagerLib (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_GUID  *TerminalTypeGuid;

  //
  // Update UART device path nodes based on UART PCD settings
  //
  gPciUartDevicePath0.Uart.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  gPciUartDevicePath0.Uart.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  gPciUartDevicePath0.Uart.Parity   = PcdGet8 (PcdUartDefaultParity);
  gPciUartDevicePath0.Uart.StopBits = PcdGet8 (PcdUartDefaultStopBits);
  gPciUartDevicePath1.Uart.BaudRate = PcdGet64 (PcdUartDefaultBaudRate);
  gPciUartDevicePath1.Uart.DataBits = PcdGet8 (PcdUartDefaultDataBits);
  gPciUartDevicePath1.Uart.Parity   = PcdGet8 (PcdUartDefaultParity);
  gPciUartDevicePath1.Uart.StopBits = PcdGet8 (PcdUartDefaultStopBits);

  //
  // Update Vendor device path nodes based on terminal type PCD settings
  //
  switch (PcdGet8 (PcdDefaultTerminalType)) {
  case PCANSITYPE:
    TerminalTypeGuid = &gEfiPcAnsiGuid;
    break;
  case VT100TYPE:
    TerminalTypeGuid = &gEfiVT100Guid;
    break;
  case VT100PLUSTYPE:
    TerminalTypeGuid = &gEfiVT100PlusGuid;
    break;
  case VTUTF8TYPE:
    TerminalTypeGuid = &gEfiVTUTF8Guid;
    break;
  case TTYTERMTYPE:
    TerminalTypeGuid = &gEfiTtyTermGuid;
    break;
  default:
    TerminalTypeGuid = &gEfiPcAnsiGuid;
    break;
  }
  CopyGuid (&gPciUartDevicePath0.TerminalType.Guid, TerminalTypeGuid);
  CopyGuid (&gPciUartDevicePath1.TerminalType.Guid, TerminalTypeGuid);

  return EFI_SUCCESS;
}
