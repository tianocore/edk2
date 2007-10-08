/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UsbMassCbi.h

Abstract:

  Defination for the USB mass storage Control/Bulk/Interrupt transpor.

Revision History


**/

#ifndef _EFI_USBMASS_CBI_H_
#define _EFI_USBMASS_CBI_H_

enum {
  USB_CBI_MAX_PACKET_NUM        = 16,
  USB_CBI_RESET_CMD_LEN         = 12,

  //
  // Usb Cbi retry C/B/I transport times, set by experience
  //
  USB_CBI_MAX_RETRY             = 3,

  //
  // Usb Cbi wait device reset complete, set by experience
  //  
  USB_CBI_RESET_DEVICE_STALL    = 50 * USB_MASS_1_MILLISECOND,

  //
  // Usb Cbi transport timeout, set by experience
  //
  USB_CBI_RESET_DEVICE_TIMEOUT  = 1 * USB_MASS_1_SECOND
};

//
// Put Interface at the first field is to make it easy to get by Context, which
// could be BOT/CBI Protocol instance
//
typedef struct {
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   *BulkInEndpoint;
  EFI_USB_ENDPOINT_DESCRIPTOR   *BulkOutEndpoint;
  EFI_USB_ENDPOINT_DESCRIPTOR   *InterruptEndpoint;
  EFI_USB_IO_PROTOCOL           *UsbIo;
} USB_CBI_PROTOCOL;

#pragma pack(1)
typedef struct {
  UINT8               Type;
  UINT8               Value;
} USB_CBI_STATUS;
#pragma pack()

extern USB_MASS_TRANSPORT mUsbCbi0Transport;
extern USB_MASS_TRANSPORT mUsbCbi1Transport;
#endif
