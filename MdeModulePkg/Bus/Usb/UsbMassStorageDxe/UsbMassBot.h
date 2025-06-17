/** @file
  Definition for the USB mass storage Bulk-Only Transport protocol,
  based on the "Universal Serial Bus Mass Storage Class Bulk-Only
  Transport" Revision 1.0, September 31, 1999.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_USBMASS_BOT_H_
#define _EFI_USBMASS_BOT_H_

extern USB_MASS_TRANSPORT  mUsbBotTransport;

//
// Usb Bulk-Only class specific request
//
#define USB_BOT_RESET_REQUEST   0xFF        ///< Bulk-Only Mass Storage Reset
#define USB_BOT_GETLUN_REQUEST  0xFE        ///< Get Max Lun
#define USB_BOT_CBW_SIGNATURE   0x43425355  ///< dCBWSignature, tag the packet as CBW
#define USB_BOT_CSW_SIGNATURE   0x53425355  ///< dCSWSignature, tag the packet as CSW
#define USB_BOT_MAX_LUN         0x0F        ///< Lun number is from 0 to 15
#define USB_BOT_MAX_CMDLEN      16          ///< Maximum number of command from command set

//
// Usb BOT command block status values
//
#define USB_BOT_COMMAND_OK      0x00  ///< Command passed, good status
#define USB_BOT_COMMAND_FAILED  0x01  ///< Command failed
#define USB_BOT_COMMAND_ERROR   0x02  ///< Phase error, need to reset the device

//
// Usb Bot retry to get CSW, refers to specification[BOT10-5.3, it says 2 times]
//
#define USB_BOT_RECV_CSW_RETRY  3

//
// Usb Bot wait device reset complete, set by experience
//
#define USB_BOT_RESET_DEVICE_STALL  (100 * USB_MASS_1_MILLISECOND)
#define USB_BOT_CLR_STALL_EP_STALL  (10 * USB_MASS_1_MILLISECOND)

//
// Usb Bot transport timeout, set by experience
//
#define USB_BOT_SEND_CBW_TIMEOUT      (3 * USB_MASS_1_SECOND)
#define USB_BOT_RECV_CSW_TIMEOUT      (3 * USB_MASS_1_SECOND)
#define USB_BOT_RESET_DEVICE_TIMEOUT  (3 * USB_MASS_1_SECOND)

#pragma pack(1)
///
/// The CBW (Command Block Wrapper) structures used by the USB BOT protocol.
///
typedef struct {
  UINT32    Signature;
  UINT32    Tag;
  UINT32    DataLen;            ///< Length of data between CBW and CSW
  UINT8     Flag;               ///< Bit 7, 0 ~ Data-Out, 1 ~ Data-In
  UINT8     Lun;                ///< Lun number. Bits 0~3 are used
  UINT8     CmdLen;             ///< Length of the command. Bits 0~4 are used
  UINT8     CmdBlock[USB_BOT_MAX_CMDLEN];
} USB_BOT_CBW;

///
/// The and CSW (Command Status Wrapper) structures used by the USB BOT protocol.
///
typedef struct {
  UINT32    Signature;
  UINT32    Tag;
  UINT32    DataResidue;
  UINT8     CmdStatus;
} USB_BOT_CSW;
#pragma pack()

typedef struct {
  //
  // Put Interface at the first field to make it easy to distinguish BOT/CBI Protocol instance
  //
  EFI_USB_INTERFACE_DESCRIPTOR    Interface;
  EFI_USB_ENDPOINT_DESCRIPTOR     *BulkInEndpoint;
  EFI_USB_ENDPOINT_DESCRIPTOR     *BulkOutEndpoint;
  UINT32                          CbwTag;
  EFI_USB_IO_PROTOCOL             *UsbIo;
} USB_BOT_PROTOCOL;

/**
  Initializes USB BOT protocol.

  This function initializes the USB mass storage class BOT protocol.
  It will save its context which is a USB_BOT_PROTOCOL structure
  in the Context if Context isn't NULL.

  @param  UsbIo                 The USB I/O Protocol instance
  @param  Context               The buffer to save the context to

  @retval EFI_SUCCESS           The device is successfully initialized.
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval Other                 The USB BOT initialization fails.

**/
EFI_STATUS
UsbBotInit (
  IN  EFI_USB_IO_PROTOCOL  *UsbIo,
  OUT VOID                 **Context OPTIONAL
  );

/**
  Call the USB Mass Storage Class BOT protocol to issue
  the command/data/status circle to execute the commands.

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL
  @param  Cmd                   The high level command
  @param  CmdLen                The command length
  @param  DataDir               The direction of the data transfer
  @param  Data                  The buffer to hold data
  @param  DataLen               The length of the data
  @param  Lun                   The number of logic unit
  @param  Timeout               The time to wait command
  @param  CmdStatus             The result of high level command execution

  @retval EFI_SUCCESS           The command is executed successfully.
  @retval Other                 Failed to execute command

**/
EFI_STATUS
UsbBotExecCommand (
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
  Reset the USB mass storage device by BOT protocol.

  @param  Context               The context of the BOT protocol, that is,
                                USB_BOT_PROTOCOL.
  @param  ExtendedVerification  If FALSE, just issue Bulk-Only Mass Storage Reset request.
                                If TRUE, additionally reset parent hub port.

  @retval EFI_SUCCESS           The device is reset.
  @retval Others                Failed to reset the device..

**/
EFI_STATUS
UsbBotResetDevice (
  IN  VOID     *Context,
  IN  BOOLEAN  ExtendedVerification
  );

/**
  Get the max LUN (Logical Unit Number) of USB mass storage device.

  @param  Context          The context of the BOT protocol, that is, USB_BOT_PROTOCOL
  @param  MaxLun           Return pointer to the max number of LUN. (e.g. MaxLun=1 means LUN0 and
                           LUN1 in all.)

  @retval EFI_SUCCESS      Max LUN is got successfully.
  @retval Others           Fail to execute this request.

**/
EFI_STATUS
UsbBotGetMaxLun (
  IN  VOID   *Context,
  OUT UINT8  *MaxLun
  );

/**
  Clean up the resource used by this BOT protocol.

  @param  Context         The context of the BOT protocol, that is, USB_BOT_PROTOCOL.

  @retval EFI_SUCCESS     The resource is cleaned up.

**/
EFI_STATUS
UsbBotCleanUp (
  IN  VOID  *Context
  );

#endif
