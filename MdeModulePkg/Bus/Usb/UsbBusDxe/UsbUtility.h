/** @file

    Manage Usb Port/Hc/Etc.

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_USB_UTILITY_H_
#define _EFI_USB_UTILITY_H_

/**
  Get the capability of the host controller.

  @param  UsbBus           The usb driver.
  @param  MaxSpeed         The maximum speed this host controller supports.
  @param  NumOfPort        The number of the root hub port.
  @param  Is64BitCapable   Whether this controller support 64 bit addressing.

  @retval EFI_SUCCESS      The host controller capability is returned.
  @retval Others           Failed to retrieve the host controller capability.

**/
EFI_STATUS
UsbHcGetCapability (
  IN  USB_BUS  *UsbBus,
  OUT UINT8    *MaxSpeed,
  OUT UINT8    *NumOfPort,
  OUT UINT8    *Is64BitCapable
  );

/**
  Get the root hub port state.

  @param  UsbBus           The USB bus driver.
  @param  PortIndex        The index of port.
  @param  PortStatus       The variable to save port state.

  @retval EFI_SUCCESS      The root port state is returned in.
  @retval Others           Failed to get the root hub port state.

**/
EFI_STATUS
UsbHcGetRootHubPortStatus (
  IN  USB_BUS              *UsbBus,
  IN  UINT8                PortIndex,
  OUT EFI_USB_PORT_STATUS  *PortStatus
  );

/**
  Set the root hub port feature.

  @param  UsbBus           The USB bus driver.
  @param  PortIndex        The port index.
  @param  Feature          The port feature to set.

  @retval EFI_SUCCESS      The port feature is set.
  @retval Others           Failed to set port feature.

**/
EFI_STATUS
UsbHcSetRootHubPortFeature (
  IN USB_BUS               *UsbBus,
  IN UINT8                 PortIndex,
  IN EFI_USB_PORT_FEATURE  Feature
  );

/**
  Clear the root hub port feature.

  @param  UsbBus           The USB bus driver.
  @param  PortIndex        The port index.
  @param  Feature          The port feature to clear.

  @retval EFI_SUCCESS      The port feature is clear.
  @retval Others           Failed to clear port feature.

**/
EFI_STATUS
UsbHcClearRootHubPortFeature (
  IN USB_BUS               *UsbBus,
  IN UINT8                 PortIndex,
  IN EFI_USB_PORT_FEATURE  Feature
  );

/**
  Execute a control transfer to the device.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The device address.
  @param  DevSpeed         The device speed.
  @param  MaxPacket        Maximum packet size of endpoint 0.
  @param  Request          The control transfer request.
  @param  Direction        The direction of data stage.
  @param  Data             The buffer holding data.
  @param  DataLength       The length of the data.
  @param  TimeOut          Timeout (in ms) to wait until timeout.
  @param  Translator       The transaction translator for low/full speed device.
  @param  UsbResult        The result of transfer.

  @retval EFI_SUCCESS      The control transfer finished without error.
  @retval Others           The control transfer failed, reason returned in UsbResult.

**/
EFI_STATUS
UsbHcControlTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  EFI_USB_DEVICE_REQUEST              *Request,
  IN  EFI_USB_DATA_DIRECTION              Direction,
  IN  OUT VOID                            *Data,
  IN  OUT UINTN                           *DataLength,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  );

/**
  Execute a bulk transfer to the device's endpoint.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The target device address.
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7.
  @param  DevSpeed         The device's speed.
  @param  MaxPacket        The endpoint's max packet size.
  @param  BufferNum        The number of data buffer.
  @param  Data             Array of pointers to data buffer.
  @param  DataLength       The length of data buffer.
  @param  DataToggle       On input, the initial data toggle to use, also  return
                           the next toggle on output.
  @param  TimeOut          The time to wait until timeout.
  @param  Translator       The transaction translator for low/full speed device.
  @param  UsbResult        The result of USB execution.

  @retval EFI_SUCCESS      The bulk transfer is finished without error.
  @retval Others           Failed to execute bulk transfer, result in UsbResult.

**/
EFI_STATUS
UsbHcBulkTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  UINT8                               BufferNum,
  IN  OUT VOID                            *Data[],
  IN  OUT UINTN                           *DataLength,
  IN  OUT UINT8                           *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  );

/**
  Queue or cancel an asynchronous interrupt transfer.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The target device address.
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7.
  @param  DevSpeed         The device's speed.
  @param  MaxPacket        The endpoint's max packet size.
  @param  IsNewTransfer    Whether this is a new request. If not, cancel the old
                           request.
  @param  DataToggle       Data toggle to use on input, next toggle on output.
  @param  PollingInterval  The interval to poll the interrupt transfer (in ms).
  @param  DataLength       The length of periodical data receive.
  @param  Translator       The transaction translator for low/full speed device.
  @param  Callback         Function to call when data is received.
  @param  Context          The context to the callback.

  @retval EFI_SUCCESS      The asynchronous transfer is queued.
  @retval Others           Failed to queue the transfer.

**/
EFI_STATUS
UsbHcAsyncInterruptTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN  BOOLEAN                             IsNewTransfer,
  IN OUT UINT8                            *DataToggle,
  IN  UINTN                               PollingInterval,
  IN  UINTN                               DataLength,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  IN  EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
  IN  VOID                                *Context OPTIONAL
  );

/**
  Execute a synchronous interrupt transfer to the target endpoint.

  @param  UsbBus           The USB bus driver.
  @param  DevAddr          The target device address.
  @param  EpAddr           The target endpoint address, with direction encoded in
                           bit 7.
  @param  DevSpeed         The device's speed.
  @param  MaxPacket        The endpoint's max packet size.
  @param  Data             Pointer to data buffer.
  @param  DataLength       The length of data buffer.
  @param  DataToggle       On input, the initial data toggle to use, also  return
                           the next toggle on output.
  @param  TimeOut          The time to wait until timeout.
  @param  Translator       The transaction translator for low/full speed device.
  @param  UsbResult        The result of USB execution.

  @retval EFI_SUCCESS      The synchronous interrupt transfer is OK.
  @retval Others           Failed to execute the synchronous interrupt transfer.

**/
EFI_STATUS
UsbHcSyncInterruptTransfer (
  IN  USB_BUS                             *UsbBus,
  IN  UINT8                               DevAddr,
  IN  UINT8                               EpAddr,
  IN  UINT8                               DevSpeed,
  IN  UINTN                               MaxPacket,
  IN OUT VOID                             *Data,
  IN OUT UINTN                            *DataLength,
  IN OUT UINT8                            *DataToggle,
  IN  UINTN                               TimeOut,
  IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
  OUT UINT32                              *UsbResult
  );

/**
  Open the USB host controller protocol BY_CHILD.

  @param  Bus              The USB bus driver.
  @param  Child            The child handle.

  @return The open protocol return.

**/
EFI_STATUS
UsbOpenHostProtoByChild (
  IN USB_BUS     *Bus,
  IN EFI_HANDLE  Child
  );

/**
  Close the USB host controller protocol BY_CHILD.

  @param  Bus              The USB bus driver.
  @param  Child            The child handle.

  @return None.

**/
VOID
UsbCloseHostProtoByChild (
  IN USB_BUS     *Bus,
  IN EFI_HANDLE  Child
  );

/**
  return the current TPL, copied from the EDKII glue lib.

  @param  VOID.

  @return Current TPL.

**/
EFI_TPL
UsbGetCurrentTpl (
  VOID
  );

#endif
