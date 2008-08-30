/** @file

  Defination for the USB mass storage class driver. The USB mass storage
  class is specified in two layers: the bottom layer is the transportation
  protocol. The top layer is the command set. The transportation layer
  provides the transportation of the command, data and result. The command
  set defines what the command, data and result. The Bulk-Only-Transport and
  Control/Bulk/Interrupt transport are two transportation protocol. USB mass
  storage class adopts various industrial standard as its command set.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EFI_USBMASS_H_
#define _EFI_USBMASS_H_


#include <Uefi.h>

#include <Protocol/BlockIo.h>
#include <Protocol/UsbIo.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>

#define USB_IS_IN_ENDPOINT(EndPointAddr)      (((EndPointAddr) & 0x80) == 0x80)
#define USB_IS_OUT_ENDPOINT(EndPointAddr)     (((EndPointAddr) & 0x80) == 0)
#define USB_IS_BULK_ENDPOINT(Attribute)       (((Attribute) & 0x03) == 0x02)
#define USB_IS_INTERRUPT_ENDPOINT(Attribute)  (((Attribute) & 0x03) == 0x03)
#define USB_IS_ERROR(Result, Error)           (((Result) & (Error)) != 0)

typedef enum {
  //
  // Usb mass storage class code
  //
  USB_MASS_STORE_CLASS    = 0x08,

  //
  // Usb mass storage subclass code, specify the command set used.
  //
  USB_MASS_STORE_RBC      = 0x01, // Reduced Block Commands
  USB_MASS_STORE_8020I    = 0x02, // SFF-8020i, typically a CD/DVD device
  USB_MASS_STORE_QIC      = 0x03, // Typically a tape device
  USB_MASS_STORE_UFI      = 0x04, // Typically a floppy disk driver device
  USB_MASS_STORE_8070I    = 0x05, // SFF-8070i, typically a floppy disk driver device.
  USB_MASS_STORE_SCSI     = 0x06, // SCSI transparent command set

  //
  // Usb mass storage protocol code, specify the transport protocol
  //
  USB_MASS_STORE_CBI0     = 0x00, // CBI protocol with command completion interrupt
  USB_MASS_STORE_CBI1     = 0x01, // CBI protocol without command completion interrupt
  USB_MASS_STORE_BOT      = 0x50, // Bulk-Only Transport

  USB_MASS_1_MILLISECOND  = 1000,
  USB_MASS_1_SECOND       = 1000 * USB_MASS_1_MILLISECOND,

  USB_MASS_CMD_SUCCESS    = 0,
  USB_MASS_CMD_FAIL,
  USB_MASS_CMD_PERSISTENT
}USB_MASS_DEV_CLASS_AND_VALUE;

typedef
EFI_STATUS
(*USB_MASS_INIT_TRANSPORT) (
  IN  EFI_USB_IO_PROTOCOL     *Usb,
  OUT VOID                    **Context    OPTIONAL
  );

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

typedef
EFI_STATUS
(*USB_MASS_RESET) (
  IN  VOID                    *Context,
  IN  BOOLEAN                 ExtendedVerification
  );

typedef
EFI_STATUS
(*USB_MASS_GET_MAX_LUN) (
  IN  VOID                    *Context,
  IN  UINT8                   *MaxLun
  );

typedef
EFI_STATUS
(*USB_MASS_FINI) (
  IN  VOID                    *Context
  );

//
// This structure contains information necessary to select the
// proper transport protocol. The mass storage class defines
// two transport protocols. One is the CBI, and the other is BOT.
// CBI is being obseleted. The design is made modular by this
// structure so that the CBI protocol can be easily removed when
// it is no longer necessary.
//
typedef struct {
  UINT8                   Protocol;
  USB_MASS_INIT_TRANSPORT Init;        // Initialize the mass storage transport protocol
  USB_MASS_EXEC_COMMAND   ExecCommand; // Transport command to the device then get result
  USB_MASS_RESET          Reset;       // Reset the device
  USB_MASS_GET_MAX_LUN    GetMaxLun;   // Get max lun, only for bot
  USB_MASS_FINI           Fini;        // Clean up the resources.
} USB_MASS_TRANSPORT;


/**
  Use the USB clear feature control transfer to clear the endpoint
  stall condition.

  @param  UsbIo                  The USB IO protocol to use
  @param  EndpointAddr           The endpoint to clear stall for

  @retval EFI_SUCCESS            The endpoint stall condtion is clear
  @retval Others                 Failed to clear the endpoint stall condtion

**/
EFI_STATUS
UsbClearEndpointStall (
  IN EFI_USB_IO_PROTOCOL      *UsbIo,
  IN UINT8                    EndpointAddr
  );

#endif
