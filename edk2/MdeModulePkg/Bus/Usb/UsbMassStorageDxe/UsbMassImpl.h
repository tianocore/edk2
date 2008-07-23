/** @file

  The implementation of USB mass storage class device driver.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_USBMASS_IMPL_H_
#define _EFI_USBMASS_IMPL_H_

typedef struct _USB_MASS_DEVICE USB_MASS_DEVICE;

#include "UsbMass.h"
#include "UsbMassBot.h"
#include "UsbMassCbi.h"
#include "UsbMassBoot.h"


//
// MassStorage raises TPL to TPL_NOTIFY to serialize all its operations
// to protect shared data structures.
//
#define  USB_MASS_TPL          TPL_NOTIFY

#define  USB_MASS_SIGNATURE    EFI_SIGNATURE_32 ('U', 's', 'b', 'M')


typedef struct _USB_MASS_DEVICE {
  UINT32                    Signature;
  EFI_HANDLE                Controller;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_BLOCK_IO_PROTOCOL     BlockIo;
  EFI_BLOCK_IO_MEDIA        BlockIoMedia;
  BOOLEAN                   OpticalStorage;
  UINT8                     Lun;          // Logical Unit Number
  UINT8                     Pdt;          // Peripheral Device Type
  USB_MASS_TRANSPORT        *Transport;   // USB mass storage transport protocol
  VOID                      *Context;     // Opaque storage for mass transport
}USB_MASS_DEVICE;

#define USB_MASS_DEVICE_FROM_BLOCKIO(a) \
        CR (a, USB_MASS_DEVICE, BlockIo, USB_MASS_SIGNATURE)

extern EFI_COMPONENT_NAME_PROTOCOL   gUsbMassStorageComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbMassStorageComponentName2;

#endif
