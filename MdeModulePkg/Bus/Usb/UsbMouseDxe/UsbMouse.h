/** @file

  Helper routine and corrsponding data struct used by USB Mouse Driver.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
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
#include <Library/PcdLib.h>
#include <Library/UsbLib.h>

#include <IndustryStandard/Usb.h>

#define CLASS_HID               3
#define SUBCLASS_BOOT           1
#define PROTOCOL_MOUSE          2

#define BOOT_PROTOCOL           0
#define REPORT_PROTOCOL         1

#define USB_MOUSE_DEV_SIGNATURE EFI_SIGNATURE_32 ('u', 'm', 'o', 'u')

typedef struct {
  BOOLEAN ButtonDetected;
  UINT8   ButtonMinIndex;
  UINT8   ButtonMaxIndex;
  UINT8   Reserved;
} PRIVATE_DATA;

typedef struct {
  UINTN                         Signature;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_EVENT                     DelayedRecoveryEvent;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   *IntEndpointDescriptor;
  UINT8                         NumberOfButtons;
  INT32                         XLogicMax;
  INT32                         XLogicMin;
  INT32                         YLogicMax;
  INT32                         YLogicMin;
  EFI_SIMPLE_POINTER_PROTOCOL   SimplePointerProtocol;
  EFI_SIMPLE_POINTER_STATE      State;
  EFI_SIMPLE_POINTER_MODE       Mode;
  BOOLEAN                       StateChanged;
  PRIVATE_DATA                  PrivateData;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;
} USB_MOUSE_DEV;

#define USB_MOUSE_DEV_FROM_MOUSE_PROTOCOL(a) \
    CR(a, USB_MOUSE_DEV, SimplePointerProtocol, USB_MOUSE_DEV_SIGNATURE)


/**
  Timer handler for Delayed Recovery timer.

  @param  Event                 The Delayed Recovery event.
  @param  Context               Points to the USB_KB_DEV instance.


**/
VOID
EFIAPI
USBMouseRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  );

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gUsbMouseDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbMouseComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbMouseComponentName2;
extern EFI_GUID                      gEfiUsbMouseDriverGuid;


/**
  Report Status Code in Usb Bot Driver.

  @param  DevicePath            Use this to get Device Path
  @param  CodeType              Status Code Type
  @param  CodeValue             Status Code Value

  @return None

**/
VOID
MouseReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  );

#endif
