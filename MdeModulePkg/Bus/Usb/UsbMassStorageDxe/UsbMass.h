/** @file
  Definition of USB Mass Storage Class and its value, USB Mass Transport Protocol,
  and other common definitions.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_USBMASS_H_
#define _EFI_USBMASS_H_


#include <Uefi.h>
#include <IndustryStandard/Scsi.h>
#include <Protocol/BlockIo.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DiskInfo.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

typedef struct _USB_MASS_TRANSPORT USB_MASS_TRANSPORT;
typedef struct _USB_MASS_DEVICE    USB_MASS_DEVICE;

#include "UsbMassBot.h"
#include "UsbMassCbi.h"
#include "UsbMassBoot.h"
#include "UsbMassDiskInfo.h"
#include "UsbMassImpl.h"

#define USB_IS_IN_ENDPOINT(EndPointAddr)      (((EndPointAddr) & BIT7) == BIT7)
#define USB_IS_OUT_ENDPOINT(EndPointAddr)     (((EndPointAddr) & BIT7) == 0)
#define USB_IS_BULK_ENDPOINT(Attribute)       (((Attribute) & (BIT0 | BIT1)) == USB_ENDPOINT_BULK)
#define USB_IS_INTERRUPT_ENDPOINT(Attribute)  (((Attribute) & (BIT0 | BIT1)) == USB_ENDPOINT_INTERRUPT)
#define USB_IS_ERROR(Result, Error)           (((Result) & (Error)) != 0)

#define USB_MASS_1_MILLISECOND  1000
#define USB_MASS_1_SECOND       (1000 * USB_MASS_1_MILLISECOND)

#define USB_MASS_CMD_SUCCESS    0
#define USB_MASS_CMD_FAIL       1
#define USB_MASS_CMD_PERSISTENT 2

/**
  Initializes USB transport protocol.

  This function initializes the USB mass storage class transport protocol.
  It will save its context in the Context if Context isn't NULL.

  @param  UsbIo                 The USB I/O Protocol instance
  @param  Context               The buffer to save the context to

  @retval EFI_SUCCESS           The device is successfully initialized.
  @retval EFI_UNSUPPORTED       The transport protocol doesn't support the device.
  @retval Other                 The USB transport initialization fails.

**/
typedef
EFI_STATUS
(*USB_MASS_INIT_TRANSPORT) (
  IN  EFI_USB_IO_PROTOCOL     *Usb,
  OUT VOID                    **Context    OPTIONAL
  );

/**
  Execute USB mass storage command through the transport protocol.

  @param  Context               The USB Transport Protocol.
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
typedef
EFI_STATUS
(*USB_MASS_EXEC_COMMAND) (
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
  Reset the USB mass storage device by Transport protocol.

  @param  Context               The USB Transport Protocol
  @param  ExtendedVerification  The flag controlling the rule of reset.
                                Not used here.

  @retval EFI_SUCCESS           The device is reset.
  @retval Others                Failed to reset the device.

**/
typedef
EFI_STATUS
(*USB_MASS_RESET) (
  IN  VOID                    *Context,
  IN  BOOLEAN                 ExtendedVerification
  );

/**
  Get the max LUN (Logical Unit Number) of USB mass storage device.

  @param  Context          The context of the transport protocol.
  @param  MaxLun           Return pointer to the max number of LUN. (e.g. MaxLun=1 means LUN0 and
                           LUN1 in all.)

  @retval EFI_SUCCESS      Max LUN is got successfully.
  @retval Others           Fail to execute this request.

**/
typedef
EFI_STATUS
(*USB_MASS_GET_MAX_LUN) (
  IN  VOID                    *Context,
  IN  UINT8                   *MaxLun
  );

/**
  Clean up the transport protocol's resource.

  @param  Context               The instance of transport protocol.

  @retval EFI_SUCCESS           The resource is cleaned up.

**/
typedef
EFI_STATUS
(*USB_MASS_CLEAN_UP) (
  IN  VOID                    *Context
  );

///
/// This structure contains information necessary to select the
/// proper transport protocol. The mass storage class defines
/// two transport protocols. One is the CBI, and the other is BOT.
/// CBI is being obseleted. The design is made modular by this
/// structure so that the CBI protocol can be easily removed when
/// it is no longer necessary.
///
struct _USB_MASS_TRANSPORT {
  UINT8                   Protocol;
  USB_MASS_INIT_TRANSPORT Init;        ///< Initialize the mass storage transport protocol
  USB_MASS_EXEC_COMMAND   ExecCommand; ///< Transport command to the device then get result
  USB_MASS_RESET          Reset;       ///< Reset the device
  USB_MASS_GET_MAX_LUN    GetMaxLun;   ///< Get max lun, only for bot
  USB_MASS_CLEAN_UP       CleanUp;     ///< Clean up the resources.
};

struct _USB_MASS_DEVICE {
  UINT32                    Signature;
  EFI_HANDLE                Controller;
  EFI_USB_IO_PROTOCOL       *UsbIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_BLOCK_IO_PROTOCOL     BlockIo;
  EFI_BLOCK_IO_MEDIA        BlockIoMedia;
  BOOLEAN                   OpticalStorage;
  UINT8                     Lun;          ///< Logical Unit Number
  UINT8                     Pdt;          ///< Peripheral Device Type
  USB_MASS_TRANSPORT        *Transport;   ///< USB mass storage transport protocol
  VOID                      *Context;
  EFI_DISK_INFO_PROTOCOL    DiskInfo;
  USB_BOOT_INQUIRY_DATA     InquiryData;
  BOOLEAN                   Cdb16Byte;
};

#endif
