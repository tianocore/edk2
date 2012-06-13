/** @file
  Helper routine and corresponding data struct used by USB Mouse Driver.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_USB_MOUSE_H_
#define _EFI_USB_MOUSE_H_


#include <Uefi.h>

#include <Protocol/SimplePointer.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>

#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiUsbLib.h>
#include <Library/DebugLib.h>

#include <IndustryStandard/Usb.h>

#define CLASS_HID               3
#define SUBCLASS_BOOT           1
#define PROTOCOL_MOUSE          2

#define BOOT_PROTOCOL           0
#define REPORT_PROTOCOL         1

#define USB_MOUSE_DEV_SIGNATURE SIGNATURE_32 ('u', 'm', 'o', 'u')

//
// A common header for usb standard descriptor.
// Each stand descriptor has a length and type.
//
#pragma pack(1)
typedef struct {
  UINT8                   Len;
  UINT8                   Type;
} USB_DESC_HEAD;
#pragma pack()

///
/// Button range and status
///
typedef struct {
  BOOLEAN ButtonDetected;
  UINT8   ButtonMinIndex;
  UINT8   ButtonMaxIndex;
  UINT8   Reserved;
} USB_MOUSE_BUTTON_DATA;

///
/// Device instance of USB mouse.
///
typedef struct {
  UINTN                         Signature;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_EVENT                     DelayedRecoveryEvent;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   IntEndpointDescriptor;
  UINT8                         NumberOfButtons;
  INT32                         XLogicMax;
  INT32                         XLogicMin;
  INT32                         YLogicMax;
  INT32                         YLogicMin;
  EFI_SIMPLE_POINTER_PROTOCOL   SimplePointerProtocol;
  EFI_SIMPLE_POINTER_STATE      State;
  EFI_SIMPLE_POINTER_MODE       Mode;
  BOOLEAN                       StateChanged;
  USB_MOUSE_BUTTON_DATA         PrivateData;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;
} USB_MOUSE_DEV;

///
/// General HID Item structure
///

typedef union {
  UINT8   Uint8;
  UINT16  Uint16;
  UINT32  Uint32;
  INT8    Int8;
  INT16   Int16;
  INT32   Int32;
  UINT8   *LongData;
} HID_DATA;

typedef struct {
  UINT16    Format;
  UINT8     Size;
  UINT8     Type;
  UINT8     Tag;
  HID_DATA  Data;
} HID_ITEM;

#define USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL(a) \
    CR(a, USB_MOUSE_DEV, SimplePointerProtocol, USB_MOUSE_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gUsbMouseDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbMouseComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbMouseComponentName2;

//
// Functions of Driver Binding Protocol
//

/**
  Check whether USB mouse driver supports this device.

  @param  This                   The USB mouse driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
USBMouseDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Starts the mouse device with this driver.

  This function consumes USB I/O Portocol, intializes USB mouse device,
  installs Simple Pointer Protocol, and submits Asynchronous Interrupt
  Transfer to manage the USB mouse device.

  @param  This                  The USB mouse driver binding instance.
  @param  Controller            Handle of device to bind driver to.
  @param  RemainingDevicePath   Optional parameter use to pick a specific child
                                device to start.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval EFI_UNSUPPORTED       This driver does not support this device.
  @retval EFI_DEVICE_ERROR      This driver cannot be started due to device Error.
  @retval EFI_OUT_OF_RESOURCES  Can't allocate memory resources.
  @retval EFI_ALREADY_STARTED   This driver has been started.

**/
EFI_STATUS
EFIAPI
USBMouseDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Stop the USB mouse device handled by this driver.

  @param  This                   The USB mouse driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Simple Pointer Protocol is not installed on Controller.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
USBMouseDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbMouseComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.
  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbMouseComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

//
// Functions of EFI_SIMPLE_POINTER_PROTOCOL
//

/**
  Retrieves the current state of a pointer device.
    
  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL instance.                                   
  @param  MouseState            A pointer to the state information on the pointer device.
                                
  @retval EFI_SUCCESS           The state of the pointer device was returned in State.
  @retval EFI_NOT_READY         The state of the pointer device has not changed since the last call to
                                GetState().                                                           
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to retrieve the pointer device's
                                current state.                                                           
  @retval EFI_INVALID_PARAMETER MouseState is NULL.                                                           

**/
EFI_STATUS
EFIAPI
GetMouseState (
  IN   EFI_SIMPLE_POINTER_PROTOCOL  *This,
  OUT  EFI_SIMPLE_POINTER_STATE     *MouseState
  );

/**                                                                 
  Resets the pointer device hardware.
  
  @param  This                  A pointer to the EFI_SIMPLE_POINTER_PROTOCOL instance.
  @param  ExtendedVerification  Indicates that the driver may perform a more exhaustive
                                verification operation of the device during reset.
                                
  @retval EFI_SUCCESS           The device was reset.
  @retval EFI_DEVICE_ERROR      The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
UsbMouseReset (
  IN EFI_SIMPLE_POINTER_PROTOCOL    *This,
  IN BOOLEAN                        ExtendedVerification
  );

/**
  Event notification function for SIMPLE_POINTER.WaitForInput event.

  @param  Event        Event to be signaled when there's input from mouse.
  @param  Context      Points to USB_MOUSE_DEV instance.
 
**/
VOID
EFIAPI
UsbMouseWaitForInput (
  IN  EFI_EVENT               Event,
  IN  VOID                    *Context
  );

//
// Internal worker functions
//

/**
  Uses USB I/O to check whether the device is a USB mouse device.

  @param  UsbIo    Pointer to a USB I/O protocol instance.

  @retval TRUE     Device is a USB mouse device.
  @retval FALSE    Device is a not USB mouse device.

**/
BOOLEAN
IsUsbMouse (
  IN  EFI_USB_IO_PROTOCOL     *UsbIo
  );

/**
  Initialize the USB mouse device.

  This function retrieves and parses HID report descriptor, and
  initializes state of USB_MOUSE_DEV. Then it sets indefinite idle
  rate for the device. Finally it creates event for delayed recovery,
  which deals with device error.

  @param  UsbMouseDev           Device instance to be initialized.

  @retval EFI_SUCCESS           USB mouse device successfully initialized..
  @retval EFI_UNSUPPORTED       HID descriptor type is not report descriptor.
  @retval Other                 USB mouse device was not initialized successfully.

**/
EFI_STATUS
InitializeUsbMouseDevice (
  IN OUT USB_MOUSE_DEV           *UsbMouseDev
  );

/**
  Handler function for USB mouse's asynchronous interrupt transfer.

  This function is the handler function for USB mouse's asynchronous interrupt transfer
  to manage the mouse. It parses data returned from asynchronous interrupt transfer, and
  get button and movement state.

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_KB_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
OnMouseInterruptComplete (
  IN  VOID        *Data,
  IN  UINTN       DataLength,
  IN  VOID        *Context,
  IN  UINT32      Result
  );

/**
  Handler for Delayed Recovery event.

  This function is the handler for Delayed Recovery event triggered
  by timer.
  After a device error occurs, the event would be triggered
  with interval of EFI_USB_INTERRUPT_DELAY. EFI_USB_INTERRUPT_DELAY
  is defined in USB standard for error handling.

  @param  Event              The Delayed Recovery event.
  @param  Context            Points to the USB_MOUSE_DEV instance.

**/
VOID
EFIAPI
USBMouseRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  );

/**
  Parse Mouse Report Descriptor.

  According to USB HID Specification, report descriptors are
  composed of pieces of information. Each piece of information
  is called an Item. This function retrieves each item from
  the report descriptor and updates USB_MOUSE_DEV.

  @param  UsbMouse          The instance of USB_MOUSE_DEV
  @param  ReportDescriptor  Report descriptor to parse
  @param  ReportSize        Report descriptor size

  @retval EFI_SUCCESS       Report descriptor successfully parsed.
  @retval EFI_UNSUPPORTED   Report descriptor contains long item.

**/
EFI_STATUS
ParseMouseReportDescriptor (
  OUT USB_MOUSE_DEV   *UsbMouse,
  IN  UINT8           *ReportDescriptor,
  IN  UINTN           ReportSize
  );

#endif
