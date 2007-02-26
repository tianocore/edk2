/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  BOT.h

Abstract:

--*/

#ifndef _BOT_H
#define _BOT_H


#include <IndustryStandard/Usb.h>
extern UINT32     gBOTDebugLevel;
extern UINT32     gBOTErrorLevel;
#define MASS_STORAGE_CLASS   0x08

#pragma pack(1)
//
// Bulk Only device protocol
//
typedef struct {
  UINT32  dCBWSignature;
  UINT32  dCBWTag;
  UINT32  dCBWDataTransferLength;
  UINT8   bmCBWFlags;
  UINT8   bCBWLUN;
  UINT8   bCBWCBLength;
  UINT8   CBWCB[16];
} CBW;

typedef struct {
  UINT32  dCSWSignature;
  UINT32  dCSWTag;
  UINT32  dCSWDataResidue;
  UINT8   bCSWStatus;
} CSW;

#pragma pack()

#define USB_BOT_DEVICE_SIGNATURE  EFI_SIGNATURE_32 ('u', 'b', 'o', 't')

typedef struct {
  UINTN                         Signature;
  EFI_DEVICE_PATH_PROTOCOL      *DevicePath;
  EFI_USB_ATAPI_PROTOCOL        UsbAtapiProtocol;
  EFI_USB_IO_PROTOCOL           *UsbIo;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   *BulkInEndpointDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR   *BulkOutEndpointDescriptor;
  EFI_UNICODE_STRING_TABLE      *ControllerNameTable;
} USB_BOT_DEVICE;

#define USB_BOT_DEVICE_FROM_THIS(a) \
    CR(a, USB_BOT_DEVICE, UsbAtapiProtocol, USB_BOT_DEVICE_SIGNATURE)

//
// Status code, see Usb Bot device spec
//
#define CSWSIG  0x53425355
#define CBWSIG  0x43425355

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL  gUsbBotDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL  gUsbBotComponentName;

//
// Bot Driver Binding Protocol
//
EFI_STATUS
EFIAPI
BotDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
BotDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
BotDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
UsbBotComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
UsbBotComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

#endif
