/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UsbMassBot.h

Abstract:

  Defination for the USB mass storage Bulk-Only Transport protocol.
  This implementation is based on the "Universal Serial Bus Mass
  Storage Class Bulk-Only Transport" Revision 1.0, September 31, 1999.

Revision History


**/

#ifndef _EFI_USBMASS_BOT_H_
#define _EFI_USBMASS_BOT_H_

enum {
  //
  // Usb Bulk-Only class specfic request
  //
  USB_BOT_RESET_REQUEST    = 0xFF,       // Bulk-Only Mass Storage Reset
  USB_BOT_GETLUN_REQUEST   = 0xFE,       // Get Max Lun
  USB_BOT_CBW_SIGNATURE    = 0x43425355, // dCBWSignature, tag the packet as CBW
  USB_BOT_CSW_SIGNATURE    = 0x53425355, // dCSWSignature, tag the packet as CSW
  USB_BOT_MAX_LUN          = 0x0F,       // Lun number is from 0 to 15
  USB_BOT_MAX_CMDLEN       = 16,         // Maxium number of command from command set

  //
  // Usb BOT command block status values
  //
  USB_BOT_COMMAND_OK       = 0x00, // Command passed, good status
  USB_BOT_COMMAND_FAILED   = 0x01, // Command failed
  USB_BOT_COMMAND_ERROR    = 0x02, // Phase error, need to reset the device

  //
  // Usb Bot retry to get CSW, refers to specification[BOT10-5.3, it says 2 times]
  //
  USB_BOT_RECV_CSW_RETRY       = 3,

  //
  // Usb Bot wait device reset complete, set by experience
  //  
  USB_BOT_RESET_DEVICE_STALL   = 100 * USB_MASS_1_MILLISECOND,
  
  //
  // Usb Bot transport timeout, set by experience
  //
  USB_BOT_SEND_CBW_TIMEOUT     = 3 * USB_MASS_1_SECOND,
  USB_BOT_RECV_CSW_TIMEOUT     = 3 * USB_MASS_1_SECOND,
  USB_BOT_RESET_DEVICE_TIMEOUT = 3 * USB_MASS_1_SECOND
};

//
// The CBW (Command Block Wrapper) and CSW (Command Status Wrapper)
// structures used by the Usb BOT protocol.
//
#pragma pack(1)
typedef struct {
  UINT32              Signature;
  UINT32              Tag;
  UINT32              DataLen;  // Length of data between CBW and CSW
  UINT8               Flag;     // Bit 7, 0 ~ Data-Out, 1 ~ Data-In
  UINT8               Lun;      // Lun number. Bits 0~3 are used
  UINT8               CmdLen;   // Length of the command. Bits 0~4 are used
  UINT8               CmdBlock[USB_BOT_MAX_CMDLEN];
} USB_BOT_CBW;

typedef struct {
  UINT32              Signature;
  UINT32              Tag;
  UINT32              DataResidue;
  UINT8               CmdStatus;
} USB_BOT_CSW;
#pragma pack()

//
// Put Interface at the first field is to make it easy to get by Context, which
// could be BOT/CBI Protocol instance
//
typedef struct {
  EFI_USB_INTERFACE_DESCRIPTOR  Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR   *BulkInEndpoint;
  EFI_USB_ENDPOINT_DESCRIPTOR   *BulkOutEndpoint;
  UINT32                        CbwTag;
  EFI_USB_IO_PROTOCOL           *UsbIo;
} USB_BOT_PROTOCOL;

extern USB_MASS_TRANSPORT mUsbBotTransport;
#endif
