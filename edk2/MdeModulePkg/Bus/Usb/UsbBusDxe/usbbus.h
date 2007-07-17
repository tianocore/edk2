/** @file
Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbBus.h

  Abstract:

    Usb Bus Driver Binding and Bus IO Protocol

  Revision History


**/

#ifndef _EFI_USB_BUS_H_
#define _EFI_USB_BUS_H_

//
// The package level header files this module uses
//
#include <PiDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>


#include <IndustryStandard/Usb.h>

typedef struct _USB_DEVICE     USB_DEVICE;
typedef struct _USB_INTERFACE  USB_INTERFACE;
typedef struct _USB_BUS        USB_BUS;
typedef struct _USB_HUB_API    USB_HUB_API;


#include "UsbUtility.h"
#include "UsbDesc.h"
#include "UsbHub.h"
#include "UsbEnumer.h"

enum {
  //
  // Time definition
  //
  USB_STALL_1_MS            = 1000,
  TICKS_PER_MS              = 10000U,
  USB_ROOTHUB_POLL_INTERVAL = 1000 * TICKS_PER_MS,
  USB_HUB_POLL_INTERVAL     = 64,

  //
  // Maximum definition
  //
  USB_MAX_LANG_ID           = 16,
  USB_MAX_INTERFACE         = 16,
  USB_MAX_DEVICES           = 128,

  //
  // Bus raises TPL to TPL_NOTIFY to serialize all its operations
  // to protect shared data structures.
  //
  USB_BUS_TPL               = TPL_NOTIFY,

  USB_INTERFACE_SIGNATURE   = EFI_SIGNATURE_32 ('U', 'S', 'B', 'I'),
  USB_BUS_SIGNATURE         = EFI_SIGNATURE_32 ('U', 'S', 'B', 'B')
};

#define USB_BIT(a)                  ((UINTN)(1 << (a)))
#define USB_BIT_IS_SET(Data, Bit)   ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define EFI_USB_BUS_PROTOCOL_GUID \
          {0x2B2F68CC, 0x0CD2, 0x44cf, 0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75}

#define USB_INTERFACE_FROM_USBIO(a) \
          CR(a, USB_INTERFACE, UsbIo, USB_INTERFACE_SIGNATURE)

#define USB_BUS_FROM_THIS(a) \
          CR(a, USB_BUS, BusId, USB_BUS_SIGNATURE)

//
// Used to locate USB_BUS
//
typedef struct _EFI_USB_BUS_PROTOCOL {
  UINT64                    Reserved;
} EFI_USB_BUS_PROTOCOL;


//
// Stands for the real USB device. Each device may
// has several seperately working interfaces.
//
struct _USB_DEVICE {
  USB_BUS                   *Bus;

  //
  // Configuration information
  //
  UINT8                     Speed;
  UINT8                     Address;
  UINT8                     MaxPacket0;

  //
  // The device's descriptors and its configuration
  //
  USB_DEVICE_DESC           *DevDesc;
  USB_CONFIG_DESC           *ActiveConfig;

  UINT16                    LangId [USB_MAX_LANG_ID];
  UINT16                    TotalLangId;

  UINT8                     NumOfInterface;
  USB_INTERFACE             *Interfaces [USB_MAX_INTERFACE];

  //
  // Parent child relationship
  //
  EFI_USB2_HC_TRANSACTION_TRANSLATOR Translator;

  UINT8                     ParentAddr;
  USB_INTERFACE             *ParentIf;
  UINT8                     ParentPort;       // Start at 0
};

//
// Stands for different functions of USB device
//
struct _USB_INTERFACE {
  UINTN                     Signature;
  USB_DEVICE                *Device;
  USB_INTERFACE_DESC        *IfDesc;
  USB_INTERFACE_SETTING     *IfSetting;

  //
  // Handles and protocols
  //
  EFI_HANDLE                Handle;
  EFI_USB_IO_PROTOCOL       UsbIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  BOOLEAN                   IsManaged;

  //
  // Hub device special data
  //
  BOOLEAN                   IsHub;
  USB_HUB_API               *HubApi;
  UINT8                     NumOfPort;
  EFI_EVENT                 HubNotify;

  //
  // Data used only by normal hub devices
  //
  USB_ENDPOINT_DESC         *HubEp;
  UINT8                     *ChangeMap;

  //
  // Data used only by root hub to hand over device to
  // companion UHCI driver if low/full speed devices are
  // connected to EHCI.
  //
  UINT8                     MaxSpeed;
};

//
// Stands for the current USB Bus
//
struct _USB_BUS {
  UINTN                     Signature;
  EFI_USB_BUS_PROTOCOL      BusId;

  //
  // Managed USB host controller
  //
  EFI_HANDLE                HostHandle;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_USB2_HC_PROTOCOL      *Usb2Hc;
  EFI_USB_HC_PROTOCOL       *UsbHc;

  //
  // An array of device that is on the bus. Devices[0] is
  // for root hub. Device with address i is at Devices[i].
  //
  USB_DEVICE                *Devices[USB_MAX_DEVICES];
};

extern EFI_USB_IO_PROTOCOL           mUsbIoProtocol;
extern EFI_DRIVER_BINDING_PROTOCOL   mUsbBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   mUsbBusComponentName;

#endif
