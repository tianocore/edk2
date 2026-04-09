/** @file
Common Library  for PEI USB.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UsbPeim.h"
#include "PeiUsbLib.h"

/**
  Clear a given usb feature.

  @param  PeiServices       General-purpose services that are available to every PEIM.
  @param  UsbIoPpi          Indicates the PEI_USB_IO_PPI instance.
  @param  Recipient         The recipient of ClearFeature Request, should be one of Device/Interface/Endpoint.
  @param  Value             Request Value.
  @param  Target            Request Index.

  @retval EFI_SUCCESS       Usb feature is cleared successfully.
  @retval EFI_DEVICE_ERROR  Cannot clear the usb feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiUsbClearDeviceFeature (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN PEI_USB_IO_PPI     *UsbIoPpi,
  IN EFI_USB_RECIPIENT  Recipient,
  IN UINT16             Value,
  IN UINT16             Target
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  ASSERT (UsbIoPpi != NULL);

  switch (Recipient) {
    case EfiUsbDevice:
      DevReq.RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_D;
      break;

    case EfiUsbInterface:
      DevReq.RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_I;
      break;

    case EfiUsbEndpoint:
      DevReq.RequestType = USB_DEV_CLEAR_FEATURE_REQ_TYPE_E;
      break;
  }

  DevReq.Request = USB_DEV_CLEAR_FEATURE;
  DevReq.Value   = Value;
  DevReq.Index   = Target;
  DevReq.Length  = 0;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DevReq,
                     EfiUsbNoData,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     NULL,
                     0
                     );
}

/**
  Clear Endpoint Halt.

  @param  PeiServices       General-purpose services that are available to every PEIM.
  @param  UsbIoPpi          Indicates the PEI_USB_IO_PPI instance.
  @param  EndpointAddress   The endpoint address.

  @retval EFI_SUCCESS       Endpoint halt is cleared successfully.
  @retval EFI_DEVICE_ERROR  Cannot clear the endpoint halt status due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiUsbClearEndpointHalt (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN UINT8             EndpointAddress
  )
{
  EFI_STATUS                    Status;
  EFI_USB_INTERFACE_DESCRIPTOR  *InterfaceDesc;
  EFI_USB_ENDPOINT_DESCRIPTOR   *EndpointDescriptor;
  UINT8                         EndpointIndex;

  //
  // Check its interface
  //
  Status = UsbIoPpi->UsbGetInterfaceDescriptor (
                       PeiServices,
                       UsbIoPpi,
                       &InterfaceDesc
                       );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (EndpointIndex = 0; EndpointIndex < InterfaceDesc->NumEndpoints; EndpointIndex++) {
    Status = UsbIoPpi->UsbGetEndpointDescriptor (PeiServices, UsbIoPpi, EndpointIndex, &EndpointDescriptor);
    if (EFI_ERROR (Status)) {
      return EFI_INVALID_PARAMETER;
    }

    if (EndpointDescriptor->EndpointAddress == EndpointAddress) {
      break;
    }
  }

  if (EndpointIndex == InterfaceDesc->NumEndpoints) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PeiUsbClearDeviceFeature (
             PeiServices,
             UsbIoPpi,
             EfiUsbEndpoint,
             EfiUsbEndpointHalt,
             EndpointAddress
             );

  return Status;
}
