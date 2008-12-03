/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:

    UsbMouseAbsolutePointer.h

  Abstract:


**/

#ifndef _USB_MOUSE_ABSOLUTE_POINTER_H
#define _USB_MOUSE_ABSOLUTE_POINTER_H


#include <Uefi.h>

#include <Protocol/AbsolutePointer.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>

#include <Library/ReportStatusCodeLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiUsbLib.h>

#include <IndustryStandard/Usb.h>

#define CLASS_HID               3
#define SUBCLASS_BOOT           1
#define PROTOCOL_MOUSE          2

#define BOOT_PROTOCOL           0
#define REPORT_PROTOCOL         1

#define USB_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE EFI_SIGNATURE_32 ('u', 'm', 's', 't')

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

  EFI_ABSOLUTE_POINTER_PROTOCOL AbsolutePointerProtocol;
  EFI_ABSOLUTE_POINTER_STATE	AbsolutePointerState;
  EFI_ABSOLUTE_POINTER_MODE		AbsolutePointerMode;
  BOOLEAN						AbsolutePointerStateChanged;

  PRIVATE_DATA                  PrivateData;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;
} USB_MOUSE_ABSOLUTE_POINTER_DEV;

#define USB_MOUSE_ABSOLUTE_POINTER_DEV_FROM_MOUSE_PROTOCOL(a) \
    CR(a, USB_MOUSE_ABSOLUTE_POINTER_DEV, AbsolutePointerProtocol, USB_MOUSE_ABSOLUTE_POINTER_DEV_SIGNATURE)

VOID
EFIAPI
USBMouseAbsolutePointerRecoveryHandler (
  IN    EFI_EVENT    Event,
  IN    VOID         *Context
  );

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gUsbMouseAbsolutePointerDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbMouseAbsolutePointerComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbMouseAbsolutePointerComponentName2;
extern EFI_GUID                      gEfiUsbMouseAbsolutePointerDriverGuid;

VOID
MouseAbsolutePointerReportStatusCode (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_STATUS_CODE_TYPE      CodeType,
  IN EFI_STATUS_CODE_VALUE     Value
  );

//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  );

EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
USBMouseAbsolutePointerDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Controller,
  IN  UINTN                         NumberOfChildren,
  IN  EFI_HANDLE                    *ChildHandleBuffer
  );


#endif
