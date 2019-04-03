/** @file
Head file for BDS Platform specific code

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _IOH_BDS_H
#define _IOH_BDS_H

#include <Ioh.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>

extern EFI_DEVICE_PATH_PROTOCOL  *gDeviceConnectOption [];

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
      } \
    }, \
    EISA_PNP_ID((PnpId)), \
    0 \
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

#define gPciRootBridge \
  PNPID_DEVICE_PATH_NODE(0x0A03)


//
// Platform Root Bridge
//
typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  EFI_DEVICE_PATH_PROTOCOL  End;
} PLATFORM_ROOT_BRIDGE_DEVICE_PATH;


typedef struct {
  ACPI_HID_DEVICE_PATH      PciRootBridge;
  PCI_DEVICE_PATH           IohDevice;
  EFI_DEVICE_PATH_PROTOCOL  End;
} IOH_PCI_USB_DEVICE_PATH;

//
// Ioh BDS Functions
//


#endif // _IOH_BDS_H
