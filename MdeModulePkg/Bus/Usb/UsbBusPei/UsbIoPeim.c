/** @file
The module is used to implement Usb Io PPI interfaces.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved. <BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbPeim.h"
#include "PeiUsbLib.h"

/**
  Submits control transfer to a target USB device.

  @param  PeiServices            The pointer of EFI_PEI_SERVICES.
  @param  This                   The pointer of PEI_USB_IO_PPI.
  @param  Request                USB device request to send.
  @param  Direction              Specifies the data direction for the data stage.
  @param  Timeout                Indicates the maximum timeout, in millisecond. If Timeout
                                 is 0, then the caller must wait for the function to be
                                 completed until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  @param  Data                   Data buffer to be transmitted or received from USB device.
  @param  DataLength             The size (in bytes) of the data buffer.

  @retval EFI_SUCCESS            Transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES   The transfer failed due to lack of resources.
  @retval EFI_INVALID_PARAMETER  Some parameters are invalid.
  @retval EFI_TIMEOUT            Transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR       Transfer failed due to host controller or device error.

**/
EFI_STATUS
EFIAPI
PeiUsbControlTransfer (
  IN     EFI_PEI_SERVICES          **PeiServices,
  IN     PEI_USB_IO_PPI            *This,
  IN     EFI_USB_DEVICE_REQUEST    *Request,
  IN     EFI_USB_DATA_DIRECTION    Direction,
  IN     UINT32                    Timeout,
  IN OUT VOID                      *Data,      OPTIONAL
  IN     UINTN                     DataLength  OPTIONAL
  )
{
  EFI_STATUS                  Status;
  PEI_USB_DEVICE              *PeiUsbDev;
  UINT32                      TransferResult;
  EFI_USB_ENDPOINT_DESCRIPTOR *EndpointDescriptor;
  UINT8                       EndpointIndex;

  PeiUsbDev = PEI_USB_DEVICE_FROM_THIS (This);

  EndpointDescriptor = NULL;
  EndpointIndex = 0;

  if ((Request->Request     == USB_REQ_CLEAR_FEATURE) &&
      (Request->RequestType == USB_DEV_CLEAR_FEATURE_REQ_TYPE_E) &&
      (Request->Value       == USB_FEATURE_ENDPOINT_HALT)) {
    //
    // Request->Index is the Endpoint Address, use it to get the Endpoint Index.
    //
    while (EndpointIndex < MAX_ENDPOINT) {
      Status = PeiUsbGetEndpointDescriptor (PeiServices, This, EndpointIndex, &EndpointDescriptor);
      if (EFI_ERROR (Status)) {
        return EFI_INVALID_PARAMETER;
      }

      if (EndpointDescriptor->EndpointAddress == Request->Index) {
        break;
      }

      EndpointIndex++;
    }

    if (EndpointIndex == MAX_ENDPOINT) {
      return EFI_INVALID_PARAMETER;
    }
  }

  if (PeiUsbDev->Usb2HcPpi != NULL) {
    Status = PeiUsbDev->Usb2HcPpi->ControlTransfer (
                        PeiServices,
                        PeiUsbDev->Usb2HcPpi,
                        PeiUsbDev->DeviceAddress,
                        PeiUsbDev->DeviceSpeed,
                        PeiUsbDev->MaxPacketSize0,
                        Request,
                        Direction,
                        Data,
                        &DataLength,
                        Timeout,
                        &(PeiUsbDev->Translator),
                        &TransferResult
                        );
  } else {
    Status = PeiUsbDev->UsbHcPpi->ControlTransfer (
                        PeiServices,
                        PeiUsbDev->UsbHcPpi,
                        PeiUsbDev->DeviceAddress,
                        PeiUsbDev->DeviceSpeed,
                        (UINT8) PeiUsbDev->MaxPacketSize0,
                        Request,
                        Direction,
                        Data,
                        &DataLength,
                        Timeout,
                        &TransferResult
                        );
  }

  //
  // Reset the endpoint toggle when endpoint stall is cleared
  //
  if ((Request->Request     == USB_REQ_CLEAR_FEATURE) &&
      (Request->RequestType == USB_DEV_CLEAR_FEATURE_REQ_TYPE_E) &&
      (Request->Value       == USB_FEATURE_ENDPOINT_HALT)) {
    if ((PeiUsbDev->DataToggle & (1 << EndpointIndex)) != 0) {
      PeiUsbDev->DataToggle = (UINT16) (PeiUsbDev->DataToggle ^ (1 << EndpointIndex));
    }
  }

  DEBUG ((EFI_D_INFO, "PeiUsbControlTransfer: %r\n", Status));
  return Status;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  PeiServices           The pointer of EFI_PEI_SERVICES.
  @param  This                  The pointer of PEI_USB_IO_PPI.
  @param  DeviceEndpoint        Endpoint number and its direction in bit 7.
  @param  Data                  A pointer to the buffer of data to transmit
                                from or receive into.
  @param  DataLength            The lenght of the data buffer.
  @param  Timeout               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete. If Timeout is 0, then
                                the caller must wait for the function to be completed
                                until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
PeiUsbBulkTransfer (
  IN     EFI_PEI_SERVICES    **PeiServices,
  IN     PEI_USB_IO_PPI      *This,
  IN     UINT8               DeviceEndpoint,
  IN OUT VOID                *Data,
  IN OUT UINTN               *DataLength,
  IN     UINTN               Timeout
  )
{
  EFI_STATUS                  Status;
  PEI_USB_DEVICE              *PeiUsbDev;
  UINT32                      TransferResult;
  UINTN                       MaxPacketLength;
  UINT8                       DataToggle;
  UINT8                       OldToggle;
  EFI_USB_ENDPOINT_DESCRIPTOR *EndpointDescriptor;
  UINT8                       EndpointIndex;
  VOID                        *Data2[EFI_USB_MAX_BULK_BUFFER_NUM];

  PeiUsbDev     = PEI_USB_DEVICE_FROM_THIS (This);

  EndpointDescriptor = NULL;
  EndpointIndex = 0;
  Data2[0] = Data;
  Data2[1] = NULL;

  while (EndpointIndex < MAX_ENDPOINT) {
    Status = PeiUsbGetEndpointDescriptor (PeiServices, This, EndpointIndex, &EndpointDescriptor);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }

    if (EndpointDescriptor->EndpointAddress == DeviceEndpoint) {
      break;
    }

    EndpointIndex++;
  }

  if (EndpointIndex == MAX_ENDPOINT) {
    return EFI_INVALID_PARAMETER;
  }

  MaxPacketLength = PeiUsbDev->EndpointDesc[EndpointIndex]->MaxPacketSize;
  if ((PeiUsbDev->DataToggle & (1 << EndpointIndex)) != 0) {
    DataToggle = 1;
  } else {
    DataToggle = 0;
  }

  OldToggle = DataToggle;

  if (PeiUsbDev->Usb2HcPpi != NULL) {
    Status = PeiUsbDev->Usb2HcPpi->BulkTransfer (
                        PeiServices,
                        PeiUsbDev->Usb2HcPpi,
                        PeiUsbDev->DeviceAddress,
                        DeviceEndpoint,
                        PeiUsbDev->DeviceSpeed,
                        MaxPacketLength,
                        Data2,
                        DataLength,
                        &DataToggle,
                        Timeout,
                        &(PeiUsbDev->Translator),
                        &TransferResult
                        );
  } else {
    Status = PeiUsbDev->UsbHcPpi->BulkTransfer (
                        PeiServices,
                        PeiUsbDev->UsbHcPpi,
                        PeiUsbDev->DeviceAddress,
                        DeviceEndpoint,
                        (UINT8) MaxPacketLength,
                        Data,
                        DataLength,
                        &DataToggle,
                        Timeout,
                        &TransferResult
                        );
  }

  if (OldToggle != DataToggle) {
    PeiUsbDev->DataToggle = (UINT16) (PeiUsbDev->DataToggle ^ (1 << EndpointIndex));
  }

  DEBUG ((EFI_D_INFO, "PeiUsbBulkTransfer: %r\n", Status));
  return Status;
}

/**
  Get the usb interface descriptor.

  @param  PeiServices          General-purpose services that are available to every PEIM.
  @param  This                 Indicates the PEI_USB_IO_PPI instance.
  @param  InterfaceDescriptor  Request interface descriptor.


  @retval EFI_SUCCESS          Usb interface descriptor is obtained successfully.

**/
EFI_STATUS
EFIAPI
PeiUsbGetInterfaceDescriptor (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  PEI_USB_IO_PPI                 *This,
  OUT EFI_USB_INTERFACE_DESCRIPTOR   **InterfaceDescriptor
  )
{
  PEI_USB_DEVICE  *PeiUsbDev;
  PeiUsbDev             = PEI_USB_DEVICE_FROM_THIS (This);
  *InterfaceDescriptor  = PeiUsbDev->InterfaceDesc;
  return EFI_SUCCESS;
}

/**
  Get the usb endpoint descriptor.

  @param  PeiServices          General-purpose services that are available to every PEIM.
  @param  This                 Indicates the PEI_USB_IO_PPI instance.
  @param  EndpointIndex        The valid index of the specified endpoint.
  @param  EndpointDescriptor   Request endpoint descriptor.

  @retval EFI_SUCCESS       Usb endpoint descriptor is obtained successfully.
  @retval EFI_NOT_FOUND     Usb endpoint descriptor is NOT found.

**/
EFI_STATUS
EFIAPI
PeiUsbGetEndpointDescriptor (
  IN  EFI_PEI_SERVICES               **PeiServices,
  IN  PEI_USB_IO_PPI                 *This,
  IN  UINT8                          EndpointIndex,
  OUT EFI_USB_ENDPOINT_DESCRIPTOR    **EndpointDescriptor
  )
{
  PEI_USB_DEVICE  *PeiUsbDev;

  PeiUsbDev = PEI_USB_DEVICE_FROM_THIS (This);

  ASSERT (EndpointDescriptor != NULL);

  //
  // The valid range of EndpointIndex is 0..15
  // If EndpointIndex is lesser than 15 but larger than the number of interfaces,
  // a EFI_NOT_FOUND should be returned
  //
  ASSERT (EndpointIndex <= 15);

  if (EndpointIndex >= PeiUsbDev->InterfaceDesc->NumEndpoints) {
    return EFI_NOT_FOUND;
  }

  *EndpointDescriptor = PeiUsbDev->EndpointDesc[EndpointIndex];

  return EFI_SUCCESS;
}

/**
  Reset the port and re-configure the usb device.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  This           Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS    Usb device is reset and configured successfully.
  @retval Others         Other failure occurs.

**/
EFI_STATUS
EFIAPI
PeiUsbPortReset (
  IN EFI_PEI_SERVICES               **PeiServices,
  IN PEI_USB_IO_PPI                 *This
  )
{
  PEI_USB_DEVICE  *PeiUsbDev;
  EFI_STATUS      Status;
  UINT8           Address;

  PeiUsbDev = PEI_USB_DEVICE_FROM_THIS (This);

  ResetRootPort (
    PeiServices,
    PeiUsbDev->UsbHcPpi,
    PeiUsbDev->Usb2HcPpi,
    PeiUsbDev->DeviceAddress,
    0
    );

  //
  // Set address
  //
  Address                   = PeiUsbDev->DeviceAddress;
  PeiUsbDev->DeviceAddress  = 0;

  Status = PeiUsbSetDeviceAddress (
            PeiServices,
            This,
            Address
            );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  PeiUsbDev->DeviceAddress = Address;

  //
  // Set default configuration
  //
  Status = PeiUsbSetConfiguration (
            PeiServices,
            This
            );

  return Status;
}
