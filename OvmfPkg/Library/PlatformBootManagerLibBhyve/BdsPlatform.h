/** @file
  Platform BDS customizations include file.

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2006 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  BdsPlatform.h

Abstract:

  Head file for BDS Platform specific code

**/

#ifndef _PLATFORM_SPECIFIC_BDS_PLATFORM_H_
#define _PLATFORM_SPECIFIC_BDS_PLATFORM_H_

#include <PiDxe.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/SmBios.h>
#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/Virtio095.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/BootLogoLib.h>
#include <Library/HobLib.h>
#include <Library/UefiLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/NvVarsFileLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>
#include <Library/QemuBootOrderLib.h>

#include <Protocol/Decompress.h>
#include <Protocol/PciIo.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/S3SaveState.h>
#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/LoadedImage.h>

#include <Guid/Acpi.h>
#include <Guid/SmBios.h>
#include <Guid/HobList.h>
#include <Guid/GlobalVariable.h>
#include <Guid/EventGroup.h>
#include <Guid/DebugAgentGuid.h>

#include <OvmfPlatforms.h>

extern EFI_DEVICE_PATH_PROTOCOL  *gPlatformConnectSequence[];
extern ACPI_HID_DEVICE_PATH      gPnpPs2KeyboardDeviceNode;
extern ACPI_HID_DEVICE_PATH      gPnp16550ComPortDeviceNode;
extern UART_DEVICE_PATH          gUartDeviceNode;
extern VENDOR_DEVICE_PATH        gTerminalTypeDeviceNode;

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

#define gPciIsaBridge \
  PCI_DEVICE_PATH_NODE(0, 0x1f)

#define gP2PBridge \
  PCI_DEVICE_PATH_NODE(0, 0x1e)

#define gPnpPs2Keyboard \
  PNPID_DEVICE_PATH_NODE(0x0303)

#define gPnp16550ComPort \
  PNPID_DEVICE_PATH_NODE(0x0501)

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

#define PCI_CLASS_SCC        0x07
#define PCI_SUBCLASS_SERIAL  0x00
#define PCI_IF_16550         0x02
#define IS_PCI_16550SERIAL(_p)  IS_CLASS3 (_p, PCI_CLASS_SCC, PCI_SUBCLASS_SERIAL, PCI_IF_16550)
#define IS_PCI_ISA_PDECODE(_p)  IS_CLASS3 (_p, PCI_CLASS_BRIDGE, PCI_CLASS_BRIDGE_ISA_PDECODE, 0)

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  UINTN                       ConnectType;
} PLATFORM_CONSOLE_CONNECT_ENTRY;

#define CONSOLE_OUT  BIT0
#define CONSOLE_IN   BIT1
#define STD_ERROR    BIT2
extern PLATFORM_CONSOLE_CONNECT_ENTRY  gPlatformConsole[];

//
// Platform BDS Functions
//

VOID
PlatformInitializeConsole (
  IN PLATFORM_CONSOLE_CONNECT_ENTRY  *PlatformConsole
  );

/**
  Loads and boots UEFI Linux via the FwCfg interface.

  @retval    EFI_NOT_FOUND - The Linux kernel was not found

**/
EFI_STATUS
TryRunningQemuKernel (
  VOID
  );

#endif // _PLATFORM_SPECIFIC_BDS_PLATFORM_H_
