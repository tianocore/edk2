/** @file

    Usb Bus Driver Binding and Bus IO Protocol.

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_USB_BUS_H_
#define _EFI_USB_BUS_H_


#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
#include <Protocol/UsbHostController.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
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

typedef enum {
  USB_MAX_LANG_ID           = 16,
  USB_MAX_INTERFACE         = 16,
  USB_MAX_DEVICES           = 128,

  USB_BUS_1_MILLISECOND     = 1000,

  //
  // Roothub and hub's polling interval, set by experience,
  // The unit of roothub is 100us, means 1s as interval, and
  // the unit of hub is 1ms, means 64ms as interval.
  //
  USB_ROOTHUB_POLL_INTERVAL = 1000 * 10000U,
  USB_HUB_POLL_INTERVAL     = 64,

  //
  // Wait for port stable to work, refers to specification
  // [USB20-9.1.2]
  //
  USB_WAIT_PORT_STABLE_STALL     = 100 * USB_BUS_1_MILLISECOND,

  //
  // Wait for port statue reg change, set by experience
  //
  USB_WAIT_PORT_STS_CHANGE_STALL = 5 * USB_BUS_1_MILLISECOND,

  //
  // Wait for set device address, refers to specification
  // [USB20-9.2.6.3, it says 2ms]
  //
  USB_SET_DEVICE_ADDRESS_STALL   = 20 * USB_BUS_1_MILLISECOND,

  //
  // Wait for retry max packet size, set by experience
  //
  USB_RETRY_MAX_PACK_SIZE_STALL  = 100 * USB_BUS_1_MILLISECOND,

  //
  // Wait for hub port power-on, refers to specification
  // [USB20-11.23.2]
  //
  USB_SET_PORT_POWER_STALL       = 2 * USB_BUS_1_MILLISECOND,

  //
  // Wait for port reset, refers to specification
  // [USB20-7.1.7.5, it says 10ms for hub and 50ms for
  // root hub]
  //
  USB_SET_PORT_RESET_STALL       = 20 * USB_BUS_1_MILLISECOND,
  USB_SET_ROOT_PORT_RESET_STALL  = 50 * USB_BUS_1_MILLISECOND,

  //
  // Wait for clear roothub port reset, set by experience
  //
  USB_CLR_ROOT_PORT_RESET_STALL  = 1 * USB_BUS_1_MILLISECOND,

  //
  // Wait for set roothub port enable, set by experience
  //
  USB_SET_ROOT_PORT_ENABLE_STALL = 20 * USB_BUS_1_MILLISECOND,

  //
  // Send general device request timeout, refers to
  // specification[USB20-11.24.1]
  //
  USB_GENERAL_DEVICE_REQUEST_TIMEOUT = 50 * USB_BUS_1_MILLISECOND,

  //
  // Send clear feature request timeout, set by experience
  //
  USB_CLEAR_FEATURE_REQUEST_TIMEOUT  = 10 * USB_BUS_1_MILLISECOND
}USB_BUS_TIMEOUT_EXPERIENCE_VALUE;

//
// Bus raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define  USB_BUS_TPL               TPL_NOTIFY

#define  USB_INTERFACE_SIGNATURE   EFI_SIGNATURE_32 ('U', 'S', 'B', 'I')
#define  USB_BUS_SIGNATURE         EFI_SIGNATURE_32 ('U', 'S', 'B', 'B')

#define USB_BIT(a)                  ((UINTN)(1 << (a)))
#define USB_BIT_IS_SET(Data, Bit)   ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define EFI_USB_BUS_PROTOCOL_GUID \
          {0x2B2F68CC, 0x0CD2, 0x44cf, {0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75}}

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

  //
  // USB Bus driver need to control the recursive connect policy of the bus, only those wanted
  // usb child device will be recursively connected.
  //
  // WantedUsbIoDPList tracks the Usb child devices which user want to recursivly fully connecte,
  // every wanted child device is stored in a item of the WantedUsbIoDPList, whose structrure is
  // DEVICE_PATH_LIST_ITEM
  //
  LIST_ENTRY                WantedUsbIoDPList;

};

//
// USB Hub Api
//
struct _USB_HUB_API{
  USB_HUB_INIT                Init;
  USB_HUB_GET_PORT_STATUS     GetPortStatus;
  USB_HUB_CLEAR_PORT_CHANGE   ClearPortChange;
  USB_HUB_SET_PORT_FEATURE    SetPortFeature;
  USB_HUB_CLEAR_PORT_FEATURE  ClearPortFeature;
  USB_HUB_RESET_PORT          ResetPort;
  USB_HUB_RELEASE             Release;
};

#define USB_US_LAND_ID   0x0409

#define DEVICE_PATH_LIST_ITEM_SIGNATURE     EFI_SIGNATURE_32('d','p','l','i')
typedef struct _DEVICE_PATH_LIST_ITEM{
  UINTN                                 Signature;
  LIST_ENTRY                            Link;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
} DEVICE_PATH_LIST_ITEM;

typedef struct {
  USB_CLASS_DEVICE_PATH           UsbClass;
  EFI_DEVICE_PATH_PROTOCOL        End;
} USB_CLASS_FORMAT_DEVICE_PATH;

/**
  Free a DEVICE_PATH_LIST_ITEM list.

  @param  UsbIoDPList            a DEVICE_PATH_LIST_ITEM list pointer.

  @retval EFI_INVALID_PARAMETER  If parameters are invalid, return this value.
  @retval EFI_SUCCESS            If free operation is successful, return this value.

**/
EFI_STATUS
EFIAPI
UsbBusFreeUsbDPList (
  IN     LIST_ENTRY                                 *UsbIoDPList
  );

/**
  Store a wanted usb child device info (its Usb part of device path) which is indicated by
  RemainingDevicePath in a Usb bus which  is indicated by UsbBusId.

  @param  UsbBusId               Point to EFI_USB_BUS_PROTOCOL interface.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            Add operation is successful.
  @retval EFI_INVALID_PARAMETER  The parameters are invalid.

**/
EFI_STATUS
EFIAPI
UsbBusAddWantedUsbIoDP (
  IN EFI_USB_BUS_PROTOCOL         *UsbBusId,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Check whether a usb child device is the wanted device in a bus.

  @param  Bus     The Usb bus's private data pointer.
  @param  UsbIf   The usb child device inferface.

  @retval True    If a usb child device is the wanted device in a bus.
  @retval False   If a usb child device is *NOT* the wanted device in a bus.

**/
BOOLEAN
EFIAPI
UsbBusIsWantedUsbIO (
  IN USB_BUS                 *Bus,
  IN USB_INTERFACE           *UsbIf
  );

/**
  Recursively connnect every wanted usb child device to ensure they all fully connected.
  Check all the child Usb IO handles in this bus, recursively connecte if it is wanted usb child device.

  @param  UsbBusId                  Point to EFI_USB_BUS_PROTOCOL interface.

  @retval EFI_SUCCESS               Connect is done successfully.
  @retval EFI_INVALID_PARAMETER     The parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbBusRecursivelyConnectWantedUsbIo (
  IN EFI_USB_BUS_PROTOCOL         *UsbBusId
  );

extern EFI_USB_IO_PROTOCOL            mUsbIoProtocol;
extern EFI_DRIVER_BINDING_PROTOCOL    mUsbBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL    mUsbBusComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL   mUsbBusComponentName2;

#endif
