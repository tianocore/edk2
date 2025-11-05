/** @file

  This file contains the definination for host controller schedule routines.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_EHCI_SCHED_H_
#define _EFI_EHCI_SCHED_H_

/**
  Initialize the schedule data structure such as frame list.

  @param Ehc                    The EHCI device to init schedule data for.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to init schedule data.
  @retval EFI_SUCCESS           The schedule data is initialized.

**/
EFI_STATUS
EhcInitSched (
  IN USB2_HC_DEV  *Ehc
  );

/**
  Free the schedule data. It may be partially initialized.

  @param  Ehc            The EHCI device.

**/
VOID
EhcFreeSched (
  IN USB2_HC_DEV  *Ehc
  );

/**
  Link the queue head to the asynchronous schedule list.
  UEFI only supports one CTRL/BULK transfer at a time
  due to its interfaces. This simplifies the AsynList
  management: A reclamation header is always linked to
  the AsyncListAddr, the only active QH is appended to it.

  @param  Ehc            The EHCI device.
  @param  Qh             The queue head to link.

**/
VOID
EhcLinkQhToAsync (
  IN USB2_HC_DEV  *Ehc,
  IN EHC_QH       *Qh
  );

/**
  Unlink a queue head from the asynchronous schedule list.
  Need to synchronize with hardware.

  @param  Ehc            The EHCI device.
  @param  Qh             The queue head to unlink.

**/
VOID
EhcUnlinkQhFromAsync (
  IN USB2_HC_DEV  *Ehc,
  IN EHC_QH       *Qh
  );

/**
  Link a queue head for interrupt transfer to the periodic
  schedule frame list. This code is very much the same as
  that in UHCI.

  @param  Ehc            The EHCI device.
  @param  Qh             The queue head to link.

**/
VOID
EhcLinkQhToPeriod (
  IN USB2_HC_DEV  *Ehc,
  IN EHC_QH       *Qh
  );

/**
  Unlink an interrupt queue head from the periodic
  schedule frame list.

  @param  Ehc            The EHCI device.
  @param  Qh             The queue head to unlink.

**/
VOID
EhcUnlinkQhFromPeriod (
  IN USB2_HC_DEV  *Ehc,
  IN EHC_QH       *Qh
  );

/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Ehc               The EHCI device.
  @param  Urb               The URB to execute.
  @param  TimeOut           The time to wait before abort, in millisecond.

  @retval EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @retval EFI_TIMEOUT       The transfer failed due to time out.
  @retval EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
EhcExecTransfer (
  IN  USB2_HC_DEV  *Ehc,
  IN  URB          *Urb,
  IN  UINTN        TimeOut
  );

/**
  Delete a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Ehc            The EHCI device.
  @param  DevAddr        The address of the target device.
  @param  EpNum          The endpoint of the target.
  @param  DataToggle     Return the next data toggle to use.

  @retval EFI_SUCCESS    An asynchronous transfer is removed.
  @retval EFI_NOT_FOUND  No transfer for the device is found.

**/
EFI_STATUS
EhciDelAsyncIntTransfer (
  IN  USB2_HC_DEV  *Ehc,
  IN  UINT8        DevAddr,
  IN  UINT8        EpNum,
  OUT UINT8        *DataToggle
  );

/**
  Remove all the asynchronous interrutp transfers.

  @param  Ehc            The EHCI device.

**/
VOID
EhciDelAllAsyncIntTransfers (
  IN USB2_HC_DEV  *Ehc
  );

/**
  Insert a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Ehc               The EHCI device.
  @param  DevAddr           The device address.
  @param  EpAddr            Endpoint addrress & its direction.
  @param  DevSpeed          The device speed.
  @param  Toggle            Initial data toggle to use.
  @param  MaxPacket         The max packet length of the endpoint.
  @param  Hub               The transaction translator to use.
  @param  DataLen           The length of data buffer.
  @param  Callback          The function to call when data is transferred.
  @param  Context           The context to the callback.
  @param  Interval          The interval for interrupt transfer.

  @return Created URB or NULL.

**/
URB *
EhciInsertAsyncIntTransfer (
  IN USB2_HC_DEV                         *Ehc,
  IN UINT8                               DevAddr,
  IN UINT8                               EpAddr,
  IN UINT8                               DevSpeed,
  IN UINT8                               Toggle,
  IN UINTN                               MaxPacket,
  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Hub,
  IN UINTN                               DataLen,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
  IN VOID                                *Context,
  IN UINTN                               Interval
  );

/**
  Interrupt transfer periodic check handler.

  @param  Event          Interrupt event.
  @param  Context        Pointer to USB2_HC_DEV.

**/
VOID
EFIAPI
EhcMonitorAsyncRequests (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

#endif
