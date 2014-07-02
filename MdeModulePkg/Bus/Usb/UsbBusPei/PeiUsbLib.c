/** @file
Common Libarary for PEI USB

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved. <BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UsbPeim.h"
#include "PeiUsbLib.h"

/**
  Get a given usb descriptor.

  @param  PeiServices        General-purpose services that are available to every PEIM.
  @param  UsbIoPpi           Indicates the PEI_USB_IO_PPI instance.
  @param  Value              Request Value.
  @param  Index              Request Index.
  @param  DescriptorLength   Request descriptor Length.
  @param  Descriptor         Request descriptor.


  @retval EFI_SUCCESS       Usb descriptor is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the usb descriptor due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiUsbGetDescriptor (
  IN  EFI_PEI_SERVICES         **PeiServices,
  IN  PEI_USB_IO_PPI           *UsbIoPpi,
  IN  UINT16                   Value,
  IN  UINT16                   Index,
  IN  UINT16                   DescriptorLength,
  OUT VOID                     *Descriptor
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  ASSERT (UsbIoPpi != NULL);

  DevReq.RequestType  = USB_DEV_GET_DESCRIPTOR_REQ_TYPE;
  DevReq.Request      = USB_DEV_GET_DESCRIPTOR;
  DevReq.Value        = Value;
  DevReq.Index        = Index;
  DevReq.Length       = DescriptorLength;

  return UsbIoPpi->UsbControlTransfer (
                     PeiServices,
                     UsbIoPpi,
                     &DevReq,
                     EfiUsbDataIn,
                     PcdGet32 (PcdUsbTransferTimeoutValue),
                     Descriptor,
                     DescriptorLength
                     );
}

/**
  Set a usb device with a specified address.

  @param  PeiServices        General-purpose services that are available to every PEIM.
  @param  UsbIoPpi           Indicates the PEI_USB_IO_PPI instance.
  @param  AddressValue       The address to assign.

  @retval EFI_SUCCESS       Usb device address is set successfully.
  @retval EFI_DEVICE_ERROR  Cannot set the usb address due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiUsbSetDeviceAddress (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_IO_PPI           *UsbIoPpi,
  IN UINT16                   AddressValue
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;

  ASSERT (UsbIoPpi != NULL);

  DevReq.RequestType  = USB_DEV_SET_ADDRESS_REQ_TYPE;
  DevReq.Request      = USB_DEV_SET_ADDRESS;
  DevReq.Value        = AddressValue;
  DevReq.Index        = 0;
  DevReq.Length       = 0;

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
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_IO_PPI           *UsbIoPpi,
  IN EFI_USB_RECIPIENT        Recipient,
  IN UINT16                   Value,
  IN UINT16                   Target
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

  DevReq.Request      = USB_DEV_CLEAR_FEATURE;
  DevReq.Value        = Value;
  DevReq.Index        = Target;
  DevReq.Length       = 0;

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
  Configure a usb device to Configuration 1.

  @param  PeiServices        General-purpose services that are available to every PEIM.
  @param  UsbIoPpi           Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS       Usb device is set to use Configuration 1 successfully.
  @retval EFI_DEVICE_ERROR  Cannot set the usb device due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiUsbSetConfiguration (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_IO_PPI           *UsbIoPpi
  )
{
  EFI_USB_DEVICE_REQUEST  DevReq;
  ZeroMem (&DevReq, sizeof (EFI_USB_DEVICE_REQUEST));

  DevReq.RequestType  = USB_DEV_SET_CONFIGURATION_REQ_TYPE;
  DevReq.Request      = USB_DEV_SET_CONFIGURATION;
  DevReq.Value        = 1;

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
  Judge if the port is connected with a usb device or not.

  @param  PortStatus  The usb port status gotten.

  @retval TRUE        A usb device is connected with the port.
  @retval FALSE       No usb device is connected with the port.

**/
BOOLEAN
IsPortConnect (
  IN UINT16  PortStatus
  )
{
  //
  // return the bit 0 value of PortStatus
  //
  if ((PortStatus & USB_PORT_STAT_CONNECTION) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Get device speed according to port status.

  @param    PortStatus  The usb port status gotten.

  @return   Device speed value.

**/
UINTN
PeiUsbGetDeviceSpeed (
  IN UINT16 PortStatus
  )
{
  if ((PortStatus & USB_PORT_STAT_LOW_SPEED) != 0) {
    return EFI_USB_SPEED_LOW;
  } else if ((PortStatus & USB_PORT_STAT_HIGH_SPEED) != 0){
    return EFI_USB_SPEED_HIGH;
  } else if ((PortStatus & USB_PORT_STAT_SUPER_SPEED) != 0) {
    return EFI_USB_SPEED_SUPER;
  } else {
    return EFI_USB_SPEED_FULL;
  }
}

/**
  Judge if the port is in "connection change" status or not.

  @param  PortChangeStatus  The usb port change status gotten.

  @retval TRUE              The port is in "connection change" status.
  @retval FALSE             The port is NOT in "connection change" status.

**/
BOOLEAN
IsPortConnectChange (
  IN UINT16  PortChangeStatus
  )
{
  //
  // return the bit 0 value of PortChangeStatus
  //
  if ((PortChangeStatus & USB_PORT_STAT_C_CONNECTION) != 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}
