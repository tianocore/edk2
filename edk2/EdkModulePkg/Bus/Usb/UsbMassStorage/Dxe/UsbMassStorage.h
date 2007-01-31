/*++
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    UsbMassStorage.h

Abstract:

    Header file for USB Mass Storage Driver's Data Structures

Revision History
--*/

#ifndef _USB_FLP_H
#define _USB_FLP_H


#include <IndustryStandard/Usb.h>
#include "UsbMassStorageData.h"

#define CLASS_MASSTORAGE          8
#define SUBCLASS_UFI              4
#define SUBCLASS_8070             5
#define PROTOCOL_BOT              0x50
#define PROTOCOL_CBI0             0
#define PROTOCOL_CBI1             1

#define USBFLOPPY                 1 // for those that use ReadCapacity(0x25) command to retrieve media capacity
#define USBFLOPPY2                2 // for those that use ReadFormatCapacity(0x23) command to retrieve media capacity
#define USBCDROM                  3

#define BLOCK_UNIT                128

#define USB_FLOPPY_DEV_SIGNATURE  EFI_SIGNATURE_32 ('u', 'f', 'l', 'p')

typedef struct {
  UINTN                   Signature;

  EFI_HANDLE              Handle;
  EFI_BLOCK_IO_PROTOCOL   BlkIo;
  EFI_BLOCK_IO_MEDIA      BlkMedia;
  EFI_USB_ATAPI_PROTOCOL  *AtapiProtocol;

  REQUEST_SENSE_DATA      *SenseData;
  UINT8                   SenseDataNumber;
  UINT8                   DeviceType;
} USB_FLOPPY_DEV;

#define USB_FLOPPY_DEV_FROM_THIS(a) \
    CR(a, USB_FLOPPY_DEV, BlkIo, USB_FLOPPY_DEV_SIGNATURE)

extern EFI_COMPONENT_NAME_PROTOCOL  gUsbMassStorageComponentName;

//
// Prototypes
// Driver model protocol interface
//
EFI_STATUS
EFIAPI
USBFloppyDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
USBFloppyDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
USBFloppyDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
EFI_STATUS
EFIAPI
UsbMassStorageComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
UsbMassStorageComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

#endif
