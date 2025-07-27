/** @file

    Usb Bus Driver Binding and Bus IO Protocol.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_USB_BUS_H_
#define _EFI_USB_BUS_H_

#include <Uefi.h>

#include <Protocol/Usb2HostController.h>
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
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Usb.h>

typedef struct _USB_DEVICE     USB_DEVICE;
typedef struct _USB_INTERFACE  USB_INTERFACE;
typedef struct _USB_BUS        USB_BUS;
typedef struct _USB_HUB_API    USB_HUB_API;

#include "UsbUtility.h"
#include "UsbDesc.h"
#include "UsbHub.h"
#include "UsbEnumer.h"

//
// USB bus timeout experience values
//

#define USB_MAX_LANG_ID    16
#define USB_MAX_INTERFACE  16
#define USB_MAX_DEVICES    128

#define USB_BUS_1_MILLISECOND  1000

//
// Roothub and hub's polling interval, set by experience,
// The unit of roothub is 100us, means 100ms as interval, and
// the unit of hub is 1ms, means 64ms as interval.
//
#define USB_ROOTHUB_POLL_INTERVAL  (100 * 10000U)
#define USB_HUB_POLL_INTERVAL      64

//
// Wait for port stable to work, refers to specification
// [USB20-9.1.2]
//
#define USB_WAIT_PORT_STABLE_STALL  (100 * USB_BUS_1_MILLISECOND)

//
// Wait for port statue reg change, set by experience
//
#define USB_WAIT_PORT_STS_CHANGE_STALL  (100)

//
// Wait for set device address, refers to specification
// [USB20-9.2.6.3, it says 2ms]
//
#define USB_SET_DEVICE_ADDRESS_STALL  (2 * USB_BUS_1_MILLISECOND)

//
// Wait for retry max packet size, set by experience
//
#define USB_RETRY_MAX_PACK_SIZE_STALL  (100 * USB_BUS_1_MILLISECOND)

//
// Wait for hub port power-on, refers to specification
// [USB20-11.23.2]
//
#define USB_SET_PORT_POWER_STALL  (2 * USB_BUS_1_MILLISECOND)

//
// Wait for port reset, refers to specification
// [USB20-7.1.7.5, it says 10ms for hub and 50ms for
// root hub]
//
// According to USB2.0, Chapter 11.5.1.5 Resetting,
// the worst case for TDRST is 20ms
//
#define USB_SET_PORT_RESET_STALL       (20 * USB_BUS_1_MILLISECOND)
#define USB_SET_ROOT_PORT_RESET_STALL  (50 * USB_BUS_1_MILLISECOND)

//
// Wait for port recovery to accept SetAddress, refers to specification
// [USB20-7.1.7.5, it says 10 ms for TRSTRCY]
//
#define USB_SET_PORT_RECOVERY_STALL  (10 * USB_BUS_1_MILLISECOND)

//
// Wait for clear roothub port reset, set by experience
//
#define USB_CLR_ROOT_PORT_RESET_STALL  (20 * USB_BUS_1_MILLISECOND)

//
// Wait for set roothub port enable, set by experience
//
#define USB_SET_ROOT_PORT_ENABLE_STALL  (20 * USB_BUS_1_MILLISECOND)

//
// Send general device request timeout.
//
// The USB Specification 2.0, section 11.24.1 recommends a value of
// 50 milliseconds.  We use a value of 500 milliseconds to work
// around slower hubs and devices.
//
#define USB_GENERAL_DEVICE_REQUEST_TIMEOUT  500

//
// Send clear feature request timeout, set by experience
//
#define USB_CLEAR_FEATURE_REQUEST_TIMEOUT  10000    // EDK uses 10.  Change for AMD

//
// Bus raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define  USB_BUS_TPL  TPL_NOTIFY

#define  USB_INTERFACE_SIGNATURE  SIGNATURE_32 ('U', 'S', 'B', 'I')
#define  USB_BUS_SIGNATURE        SIGNATURE_32 ('U', 'S', 'B', 'B')

#define USB_BIT(a)                 ((UINTN)(1 << (a)))
#define USB_BIT_IS_SET(Data, Bit)  ((BOOLEAN)(((Data) & (Bit)) == (Bit)))

#define USB_INTERFACE_FROM_USBIO(a) \
          CR(a, USB_INTERFACE, UsbIo, USB_INTERFACE_SIGNATURE)

#define USB_BUS_FROM_THIS(a) \
          CR(a, USB_BUS, BusId, USB_BUS_SIGNATURE)

#define USB_CONFIG_DESC_DEF_ALLOC_LEN  255

typedef enum {
  UsbEnumScriptEdk2    = 0,         // 0   :EDK2 flow
  UsbEnumScriptRsrv    = 1,         // 1   :Reserved flow - EDK2
  UsbEnumScriptLinux   = 2,         // 2   :Linux flow
  UsbEnumScriptWin     = 3,         // 3   :Window flow
  UsbEnumScriptUnknown = 0xFF,      // 0xff:Unknow flow
} USB_ENUM_SCRIPT_TYPE;

//
// Used to locate USB_BUS
// UsbBusProtocol is the private protocol.
// gEfiCallerIdGuid will be used as its protocol guid.
//
typedef struct _EFI_USB_BUS_PROTOCOL {
  UINT64    Reserved;
} EFI_USB_BUS_PROTOCOL;

//
// Stands for the real USB device. Each device may
// has several separately working interfaces.
//
struct _USB_DEVICE {
  USB_BUS                               *Bus;

  //
  // Configuration information
  //
  UINT8                                 Speed;
  UINT8                                 Address;
  UINT32                                MaxPacket0;

  //
  // The device's descriptors and its configuration
  //
  USB_DEVICE_DESC                       *DevDesc;
  USB_CONFIG_DESC                       *ActiveConfig;

  UINT16                                LangId[USB_MAX_LANG_ID];
  UINT16                                TotalLangId;

  UINT8                                 NumOfInterface;
  USB_INTERFACE                         *Interfaces[USB_MAX_INTERFACE];

  //
  // Parent child relationship
  //
  EFI_USB2_HC_TRANSACTION_TRANSLATOR    Translator;

  UINT8                                 ParentAddr;
  USB_INTERFACE                         *ParentIf;
  UINT8                                 ParentPort; // Start at 0
  UINT8                                 Tier;
  BOOLEAN                               DisconnectFail;
  BOOLEAN                               IsSSDev;
  UINT8                                 EnumScript;
};

//
// Stands for different functions of USB device
//
struct _USB_INTERFACE {
  UINTN                       Signature;
  USB_DEVICE                  *Device;
  USB_INTERFACE_DESC          *IfDesc;
  USB_INTERFACE_SETTING       *IfSetting;

  //
  // Handles and protocols
  //
  EFI_HANDLE                  Handle;
  EFI_USB_IO_PROTOCOL         UsbIo;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  BOOLEAN                     IsManaged;

  //
  // Hub device special data
  //
  BOOLEAN                     IsHub;
  USB_HUB_API                 *HubApi;
  UINT8                       NumOfPort;
  EFI_EVENT                   HubNotify;

  //
  // Data used only by normal hub devices
  //
  USB_ENDPOINT_DESC           *HubEp;
  UINT8                       *ChangeMap;

  //
  // Data used only by root hub to hand over device to
  // companion UHCI driver if low/full speed devices are
  // connected to EHCI.
  //
  UINT8                       MaxSpeed;
};

//
// Stands for the current USB Bus
//
struct _USB_BUS {
  UINTN                       Signature;
  EFI_USB_BUS_PROTOCOL        BusId;

  //
  // Managed USB host controller
  //
  EFI_HANDLE                  HostHandle;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  EFI_USB2_HC_PROTOCOL        *Usb2Hc;

  //
  // Recorded the max supported usb devices.
  // XHCI can support up to 255 devices.
  // EHCI/UHCI/OHCI supports up to 127 devices.
  //
  UINT32                      MaxDevices;
  //
  // An array of device that is on the bus. Devices[0] is
  // for root hub. Device with address i is at Devices[i].
  //
  USB_DEVICE                  *Devices[256];

  //
  // USB Bus driver need to control the recursive connect policy of the bus, only those wanted
  // usb child device will be recursively connected.
  //
  // WantedUsbIoDPList tracks the Usb child devices which user want to recursively fully connecte,
  // every wanted child device is stored in a item of the WantedUsbIoDPList, whose structure is
  // DEVICE_PATH_LIST_ITEM
  //
  LIST_ENTRY    WantedUsbIoDPList;
};

//
// USB Hub Api
//
struct _USB_HUB_API {
  USB_HUB_INIT                  Init;
  USB_HUB_GET_PORT_STATUS       GetPortStatus;
  USB_HUB_CLEAR_PORT_CHANGE     ClearPortChange;
  USB_HUB_SET_PORT_FEATURE      SetPortFeature;
  USB_HUB_CLEAR_PORT_FEATURE    ClearPortFeature;
  USB_HUB_RESET_PORT            ResetPort;
  USB_HUB_RELEASE               Release;
};

#define USB_US_LANG_ID  0x0409

#define DEVICE_PATH_LIST_ITEM_SIGNATURE  SIGNATURE_32('d','p','l','i')
typedef struct _DEVICE_PATH_LIST_ITEM {
  UINTN                       Signature;
  LIST_ENTRY                  Link;
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
} DEVICE_PATH_LIST_ITEM;

typedef struct {
  USB_CLASS_DEVICE_PATH       UsbClass;
  EFI_DEVICE_PATH_PROTOCOL    End;
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
  IN     LIST_ENTRY  *UsbIoDPList
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
  IN EFI_USB_BUS_PROTOCOL      *UsbBusId,
  IN EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath
  );

/**
  Check whether a usb child device is the wanted device in a bus.

  @param  Bus     The Usb bus's private data pointer.
  @param  UsbIf   The usb child device interface.

  @retval True    If a usb child device is the wanted device in a bus.
  @retval False   If a usb child device is *NOT* the wanted device in a bus.

**/
BOOLEAN
EFIAPI
UsbBusIsWantedUsbIO (
  IN USB_BUS        *Bus,
  IN USB_INTERFACE  *UsbIf
  );

/**
  Recursively connect every wanted usb child device to ensure they all fully connected.
  Check all the child Usb IO handles in this bus, recursively connecte if it is wanted usb child device.

  @param  UsbBusId                  Point to EFI_USB_BUS_PROTOCOL interface.

  @retval EFI_SUCCESS               Connect is done successfully.
  @retval EFI_INVALID_PARAMETER     The parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbBusRecursivelyConnectWantedUsbIo (
  IN EFI_USB_BUS_PROTOCOL  *UsbBusId
  );

/**
  USB_IO function to execute a control transfer. This
  function will execute the USB transfer. If transfer
  successes, it will sync the internal state of USB bus
  with device state.

  @param  This                   The USB_IO instance
  @param  Request                The control transfer request
  @param  Direction              Direction for data stage
  @param  Timeout                The time to wait before timeout
  @param  Data                   The buffer holding the data
  @param  DataLength             Then length of the data
  @param  UsbStatus              USB result

  @retval EFI_INVALID_PARAMETER  The parameters are invalid
  @retval EFI_SUCCESS            The control transfer succeded.
  @retval Others                 Failed to execute the transfer

**/
EFI_STATUS
EFIAPI
UsbIoControlTransfer (
  IN  EFI_USB_IO_PROTOCOL     *This,
  IN  EFI_USB_DEVICE_REQUEST  *Request,
  IN  EFI_USB_DATA_DIRECTION  Direction,
  IN  UINT32                  Timeout,
  IN  OUT VOID                *Data       OPTIONAL,
  IN  UINTN                   DataLength  OPTIONAL,
  OUT UINT32                  *UsbStatus
  );

/**
  Execute a bulk transfer to the device endpoint.

  @param  This                   The USB IO instance.
  @param  Endpoint               The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of the data to transfer.
  @param  Timeout                Time to wait before timeout.
  @param  UsbStatus              The result of USB transfer.

  @retval EFI_SUCCESS            The bulk transfer is OK.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval Others                 Failed to execute transfer, reason returned in
                                 UsbStatus.

**/
EFI_STATUS
EFIAPI
UsbIoBulkTransfer (
  IN  EFI_USB_IO_PROTOCOL  *This,
  IN  UINT8                Endpoint,
  IN  OUT VOID             *Data,
  IN  OUT UINTN            *DataLength,
  IN  UINTN                Timeout,
  OUT UINT32               *UsbStatus
  );

/**
  Execute a synchronous interrupt transfer.

  @param  This                   The USB IO instance.
  @param  Endpoint               The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of the data to transfer.
  @param  Timeout                Time to wait before timeout.
  @param  UsbStatus              The result of USB transfer.

  @retval EFI_SUCCESS            The synchronous interrupt transfer is OK.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval Others                 Failed to execute transfer, reason returned in
                                 UsbStatus.

**/
EFI_STATUS
EFIAPI
UsbIoSyncInterruptTransfer (
  IN  EFI_USB_IO_PROTOCOL  *This,
  IN  UINT8                Endpoint,
  IN  OUT VOID             *Data,
  IN  OUT UINTN            *DataLength,
  IN  UINTN                Timeout,
  OUT UINT32               *UsbStatus
  );

/**
  Queue a new asynchronous interrupt transfer, or remove the old
  request if (IsNewTransfer == FALSE).

  @param  This                   The USB_IO instance.
  @param  Endpoint               The device endpoint.
  @param  IsNewTransfer          Whether this is a new request, if it's old, remove
                                 the request.
  @param  PollInterval           The interval to poll the transfer result, (in ms).
  @param  DataLength             The length of perodic data transfer.
  @param  Callback               The function to call periodically when transfer is
                                 ready.
  @param  Context                The context to the callback.

  @retval EFI_SUCCESS            New transfer is queued or old request is removed.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval Others                 Failed to queue the new request or remove the old
                                 request.

**/
EFI_STATUS
EFIAPI
UsbIoAsyncInterruptTransfer (
  IN EFI_USB_IO_PROTOCOL              *This,
  IN UINT8                            Endpoint,
  IN BOOLEAN                          IsNewTransfer,
  IN UINTN                            PollInterval        OPTIONAL,
  IN UINTN                            DataLength          OPTIONAL,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback            OPTIONAL,
  IN VOID                             *Context            OPTIONAL
  );

/**
  Execute a synchronous isochronous transfer.

  @param  This                   The USB IO instance.
  @param  DeviceEndpoint         The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of the data to transfer.
  @param  UsbStatus              The result of USB transfer.

  @retval EFI_UNSUPPORTED        Currently isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
UsbIoIsochronousTransfer (
  IN  EFI_USB_IO_PROTOCOL  *This,
  IN  UINT8                DeviceEndpoint,
  IN  OUT VOID             *Data,
  IN  UINTN                DataLength,
  OUT UINT32               *Status
  );

/**
  Queue an asynchronous isochronous transfer.

  @param  This                   The USB_IO instance.
  @param  DeviceEndpoint         The device endpoint.
  @param  Data                   The data to transfer.
  @param  DataLength             The length of perodic data transfer.
  @param  IsochronousCallBack    The function to call periodically when transfer is
                                 ready.
  @param  Context                The context to the callback.

  @retval EFI_UNSUPPORTED        Currently isochronous transfer isn't supported.

**/
EFI_STATUS
EFIAPI
UsbIoAsyncIsochronousTransfer (
  IN EFI_USB_IO_PROTOCOL              *This,
  IN UINT8                            DeviceEndpoint,
  IN OUT VOID                         *Data,
  IN UINTN                            DataLength,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  IsochronousCallBack,
  IN VOID                             *Context              OPTIONAL
  );

/**
  Retrieve the device descriptor of the device.

  @param  This                   The USB IO instance.
  @param  Descriptor             The variable to receive the device descriptor.

  @retval EFI_SUCCESS            The device descriptor is returned.
  @retval EFI_INVALID_PARAMETER  The parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbIoGetDeviceDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *This,
  OUT EFI_USB_DEVICE_DESCRIPTOR  *Descriptor
  );

/**
  Return the configuration descriptor of the current active configuration.

  @param  This                   The USB IO instance.
  @param  Descriptor             The USB configuration descriptor.

  @retval EFI_SUCCESS            The active configuration descriptor is returned.
  @retval EFI_INVALID_PARAMETER  Some parameter is invalid.
  @retval EFI_NOT_FOUND          Currently no active configuration is selected.

**/
EFI_STATUS
EFIAPI
UsbIoGetActiveConfigDescriptor (
  IN  EFI_USB_IO_PROTOCOL        *This,
  OUT EFI_USB_CONFIG_DESCRIPTOR  *Descriptor
  );

/**
  Retrieve the active interface setting descriptor for this USB IO instance.

  @param  This                   The USB IO instance.
  @param  Descriptor             The variable to receive active interface setting.

  @retval EFI_SUCCESS            The active interface setting is returned.
  @retval EFI_INVALID_PARAMETER  Some parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbIoGetInterfaceDescriptor (
  IN  EFI_USB_IO_PROTOCOL           *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR  *Descriptor
  );

/**
  Retrieve the endpoint descriptor from this interface setting.

  @param  This                   The USB IO instance.
  @param  Index                  The index (start from zero) of the endpoint to
                                 retrieve.
  @param  Descriptor             The variable to receive the descriptor.

  @retval EFI_SUCCESS            The endpoint descriptor is returned.
  @retval EFI_INVALID_PARAMETER  Some parameter is invalid.

**/
EFI_STATUS
EFIAPI
UsbIoGetEndpointDescriptor (
  IN  EFI_USB_IO_PROTOCOL          *This,
  IN  UINT8                        Index,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR  *Descriptor
  );

/**
  Retrieve the supported language ID table from the device.

  @param  This                   The USB IO instance.
  @param  LangIDTable            The table to return the language IDs.
  @param  TableSize              The size, in bytes, of the table LangIDTable.

  @retval EFI_SUCCESS            The language ID is return.

**/
EFI_STATUS
EFIAPI
UsbIoGetSupportedLanguages (
  IN  EFI_USB_IO_PROTOCOL  *This,
  OUT UINT16               **LangIDTable,
  OUT UINT16               *TableSize
  );

/**
  Retrieve an indexed string in the language of LangID.

  @param  This                   The USB IO instance.
  @param  LangID                 The language ID of the string to retrieve.
  @param  StringIndex            The index of the string.
  @param  String                 The variable to receive the string.

  @retval EFI_SUCCESS            The string is returned.
  @retval EFI_NOT_FOUND          No such string existed.

**/
EFI_STATUS
EFIAPI
UsbIoGetStringDescriptor (
  IN  EFI_USB_IO_PROTOCOL  *This,
  IN  UINT16               LangID,
  IN  UINT8                StringIndex,
  OUT CHAR16               **String
  );

/**
  Reset the device, then if that succeeds, reconfigure the
  device with its address and current active configuration.

  @param  This                   The USB IO instance.

  @retval EFI_SUCCESS            The device is reset and configured.
  @retval Others                 Failed to reset the device.

**/
EFI_STATUS
EFIAPI
UsbIoPortReset (
  IN EFI_USB_IO_PROTOCOL  *This
  );

/**
  Install Usb Bus Protocol on host controller, and start the Usb bus.

  @param This                    The USB bus driver binding instance.
  @param Controller              The controller to check.
  @param RemainingDevicePath     The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the usb bus.
  @retval EFI_ALREADY_STARTED    The controller is already controlled by the usb bus.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
EFIAPI
UsbBusBuildProtocol (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  The USB bus driver entry pointer.

  @param ImageHandle       The driver image handle.
  @param SystemTable       The system table.

  @return EFI_SUCCESS      The component name protocol is installed.
  @return Others           Failed to init the usb driver.

**/
EFI_STATUS
EFIAPI
UsbBusDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );

/**
  Check whether USB bus driver support this device.

  @param  This                   The USB bus driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The bus supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.

**/
EFI_STATUS
EFIAPI
UsbBusControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start to process the controller.

  @param  This                   The USB bus driver binding instance.
  @param  Controller             The controller to check.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the usb bus.
  @retval EFI_ALREADY_STARTED    The controller is already controlled by the usb
                                 bus.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
EFIAPI
UsbBusControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop handle the controller by this USB bus driver.

  @param  This                   The USB bus driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The child of USB bus that opened controller
                                 BY_CHILD.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The controller or children are stopped.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver.

**/
EFI_STATUS
EFIAPI
UsbBusControllerDriverStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

extern EFI_USB_IO_PROTOCOL           mUsbIoProtocol;
extern EFI_DRIVER_BINDING_PROTOCOL   mUsbBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   mUsbBusComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  mUsbBusComponentName2;

#endif
