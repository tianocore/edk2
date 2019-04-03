/** @file
  Defination for the USB mass storage Control/Bulk/Interrupt (CBI) transport,
  according to USB Mass Storage Class Control/Bulk/Interrupt (CBI) Transport, Revision 1.1.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_USBMASS_CBI_H_
#define _EFI_USBMASS_CBI_H_

extern USB_MASS_TRANSPORT mUsbCbi0Transport;
extern USB_MASS_TRANSPORT mUsbCbi1Transport;

#define USB_CBI_MAX_PACKET_NUM        16
#define USB_CBI_RESET_CMD_LEN         12
//
// USB CBI retry C/B/I transport times, set by experience
//
#define USB_CBI_MAX_RETRY             3
//
// Time to wait for USB CBI reset to complete, set by experience
//
#define USB_CBI_RESET_DEVICE_STALL    (50 * USB_MASS_1_MILLISECOND)
//
// USB CBI transport timeout, set by experience
//
#define USB_CBI_RESET_DEVICE_TIMEOUT  (1 * USB_MASS_1_SECOND)

typedef struct {
  //
  // Put Interface at the first field to make it easy to distinguish BOT/CBI Protocol instance
  //
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

/**
  Initializes USB CBI protocol.

  This function initializes the USB mass storage class CBI protocol.
  It will save its context which is a USB_CBI_PROTOCOL structure
  in the Context if Context isn't NULL.

  @param  UsbIo                 The USB I/O Protocol instance
  @param  Context               The buffer to save the context to

  @retval EFI_SUCCESS           The device is successfully initialized.
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval Other                 The USB CBI initialization fails.

**/
EFI_STATUS
UsbCbiInit (
  IN  EFI_USB_IO_PROTOCOL   *UsbIo,
  OUT VOID                  **Context       OPTIONAL
  );

/**
  Execute USB mass storage command through the CBI0/CBI1 transport protocol.

  @param  Context               The USB CBI Protocol.
  @param  Cmd                   The command to transfer to device
  @param  CmdLen                The length of the command
  @param  DataDir               The direction of data transfer
  @param  Data                  The buffer to hold the data
  @param  DataLen               The length of the buffer
  @param  Lun                   Should be 0, this field for bot only
  @param  Timeout               The time to wait
  @param  CmdStatus             The result of the command execution

  @retval EFI_SUCCESS           The command is executed successfully.
  @retval Other                 Failed to execute the command

**/
EFI_STATUS
UsbCbiExecCommand (
  IN  VOID                    *Context,
  IN  VOID                    *Cmd,
  IN  UINT8                   CmdLen,
  IN  EFI_USB_DATA_DIRECTION  DataDir,
  IN  VOID                    *Data,
  IN  UINT32                  DataLen,
  IN  UINT8                   Lun,
  IN  UINT32                  Timeout,
  OUT UINT32                  *CmdStatus
  );

/**
  Reset the USB mass storage device by CBI protocol.

  This function resets the USB mass storage device by CBI protocol.
  The reset is defined as a non-data command. Don't use UsbCbiExecCommand
  to send the command to device because that may introduce recursive loop.

  @param  Context               The USB CBI protocol
  @param  ExtendedVerification  The flag controlling the rule of reset.
                                Not used here.

  @retval EFI_SUCCESS           The device is reset.
  @retval Others                Failed to reset the device.

**/
EFI_STATUS
UsbCbiResetDevice (
  IN  VOID                    *Context,
  IN  BOOLEAN                  ExtendedVerification
  );

/**
  Clean up the CBI protocol's resource.

  @param  Context               The instance of CBI protocol.

  @retval EFI_SUCCESS           The resource is cleaned up.

**/
EFI_STATUS
UsbCbiCleanUp (
  IN  VOID                   *Context
  );

#endif
