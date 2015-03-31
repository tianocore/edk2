/** @file
  Head file for BDS Platform specific code

Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PLATFORM_SPECIFIC_BDS_PLATFORM_H_
#define _PLATFORM_SPECIFIC_BDS_PLATFORM_H_

#include <PiDxe.h>
#include <IndustryStandard/Pci.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/GenericBdsLib.h>
#include <Library/PlatformBdsLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/PciIo.h>
#include <Protocol/SerialIo.h>

#include <Guid/GlobalVariable.h>
extern BDS_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformConnectSequence[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformDriverOption[];
extern EFI_DEVICE_PATH_PROTOCOL   *gPlatformRootBridges[];
extern ACPI_HID_DEVICE_PATH       gPnp16550ComPortDeviceNode;
extern UART_DEVICE_PATH           gUartDeviceNode;
extern VENDOR_DEVICE_PATH         gTerminalTypeDeviceNode;
extern VENDOR_DEVICE_PATH         gUartDeviceVenderNode;

//
//
//
#define VarConsoleInpDev        L"ConInDev"
#define VarConsoleInp           L"ConIn"
#define VarConsoleOutDev        L"ConOutDev"
#define VarConsoleOut           L"ConOut"
#define VarErrorOutDev          L"ErrOutDev"
#define VarErrorOut             L"ErrOut"

#define PCI_DEVICE_PATH_NODE(Func, Dev) \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_PCI_DP, \
      { \
        (UINT8) (sizeof (PCI_DEVICE_PATH)), \
        (UINT8) ((sizeof (PCI_DEVICE_PATH)) >> 8) \
      } \
    }, \
    (Func), \
    (Dev) \
  }

#define PNPID_DEVICE_PATH_NODE(PnpId) \
  { \
    { \
      ACPI_DEVICE_PATH, \
      ACPI_DP, \
      { \
        (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)), \
        (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8) \
      }, \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
  }

#define gPciRootBridge \
  PNPID_DEVICE_PATH_NODE(0x0A03)
#define gPnp16550ComPort \
  PNPID_DEVICE_PATH_NODE(0x0501)

#define gUartVender \
  { \
    { \
      HARDWARE_DEVICE_PATH, \
      HW_VENDOR_DP, \
      { \
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)), \
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8) \
      } \
    }, \
    EFI_SERIAL_IO_PROTOCOL_GUID \
  }
    
#define gUart \
  { \
    { \
      MESSAGING_DEVICE_PATH, \
      MSG_UART_DP, \
      { \
        (UINT8) (sizeof (UART_DEVICE_PATH)), \
        (UINT8) ((sizeof (UART_DEVICE_PATH)) >> 8) \
      } \
    }, \
    0, \
    115200, \
    8, \
    1, \
    1 \
  }

#define gPcAnsiTerminal \
  { \
    { \
      MESSAGING_DEVICE_PATH, \
      MSG_VENDOR_DP, \
      { \
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)), \
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8) \
      } \
    }, \
    DEVICE_PATH_MESSAGING_PC_ANSI \
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
#define PCI_CLASS_SCC          0x07
#define PCI_SUBCLASS_SERIAL    0x00
#define PCI_IF_16550           0x02
#define IS_PCI_16550SERIAL(_p)           IS_CLASS3 (_p, PCI_CLASS_SCC, PCI_SUBCLASS_SERIAL, PCI_IF_16550)

#define IS_PCI_ISA_PDECODE(_p)        IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE, 0)

//
// Platform Root Bridge
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ROOT_BRIDGE_DEVICE_PATH;
EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  );
VOID
PlatformBdsEnterFrontPage (
  IN UINT16                 TimeoutDefault,
  IN BOOLEAN                ConnectAllHappened
  );
#endif // _PLATFORM_SPECIFIC_BDS_PLATFORM_H_
