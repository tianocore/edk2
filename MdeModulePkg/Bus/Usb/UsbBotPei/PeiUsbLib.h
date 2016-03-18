/** @file
Common Libarary  for PEI USB.

Copyright (c) 1999 - 2010, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_USB_LIB_H_
#define _PEI_USB_LIB_H_
//
// Standard device request and request type
// By [Spec-USB20/Chapter-9.4]
//
#define USB_DEV_GET_STATUS                  0x00
#define USB_DEV_GET_STATUS_REQ_TYPE_D       0x80 // Receiver : Device
#define USB_DEV_GET_STATUS_REQ_TYPE_I       0x81 // Receiver : Interface
#define USB_DEV_GET_STATUS_REQ_TYPE_E       0x82 // Receiver : Endpoint

#define USB_DEV_CLEAR_FEATURE               0x01
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_D    0x00 // Receiver : Device
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_I    0x01 // Receiver : Interface
#define USB_DEV_CLEAR_FEATURE_REQ_TYPE_E    0x02 // Receiver : Endpoint

#define USB_DEV_SET_FEATURE                 0x03
#define USB_DEV_SET_FEATURE_REQ_TYPE_D      0x00 // Receiver : Device
#define USB_DEV_SET_FEATURE_REQ_TYPE_I      0x01 // Receiver : Interface
#define USB_DEV_SET_FEATURE_REQ_TYPE_E      0x02 // Receiver : Endpoint
 
#define USB_DEV_SET_ADDRESS                 0x05
#define USB_DEV_SET_ADDRESS_REQ_TYPE        0x00

#define USB_DEV_GET_DESCRIPTOR              0x06
#define USB_DEV_GET_DESCRIPTOR_REQ_TYPE     0x80

#define USB_DEV_SET_DESCRIPTOR              0x07
#define USB_DEV_SET_DESCRIPTOR_REQ_TYPE     0x00

#define USB_DEV_GET_CONFIGURATION           0x08
#define USB_DEV_GET_CONFIGURATION_REQ_TYPE  0x80

#define USB_DEV_SET_CONFIGURATION           0x09
#define USB_DEV_SET_CONFIGURATION_REQ_TYPE  0x00

#define USB_DEV_GET_INTERFACE               0x0A
#define USB_DEV_GET_INTERFACE_REQ_TYPE      0x81

#define USB_DEV_SET_INTERFACE               0x0B
#define USB_DEV_SET_INTERFACE_REQ_TYPE      0x01

#define USB_DEV_SYNCH_FRAME                 0x0C
#define USB_DEV_SYNCH_FRAME_REQ_TYPE        0x82

//
// USB Descriptor types
//
#define USB_DT_DEVICE     0x01
#define USB_DT_CONFIG     0x02
#define USB_DT_STRING     0x03
#define USB_DT_INTERFACE  0x04
#define USB_DT_ENDPOINT   0x05
#define USB_DT_HUB        0x29
#define USB_DT_HID        0x21

//
// USB request type
//
#define USB_TYPE_STANDARD (0x00 << 5)
#define USB_TYPE_CLASS    (0x01 << 5)
#define USB_TYPE_VENDOR   (0x02 << 5)
#define USB_TYPE_RESERVED (0x03 << 5)

//
// USB request targer device
//
#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02
#define USB_RECIP_OTHER     0x03

typedef enum {
  EfiUsbEndpointHalt,
  EfiUsbDeviceRemoteWakeup
} EFI_USB_STANDARD_FEATURE_SELECTOR;

//
// Usb Data recipient type
//
typedef enum {
  EfiUsbDevice,
  EfiUsbInterface,
  EfiUsbEndpoint
} EFI_USB_RECIPIENT;

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
  );

/**
  Set a usb device with a specified address.

  @param  PeiServices        General-purpose services that are available to every PEIM.
  @param  UsbIoPpi           Indicates the PEI_USB_IO_PPI instance.
  @param  AddressValue       The address to assign.

  @retval EFI_SUCCESS        Usb device address is set successfully.
  @retval EFI_DEVICE_ERROR   Cannot set the usb address due to a hardware error.
  @retval Others             Other failure occurs.

**/
EFI_STATUS
PeiUsbSetDeviceAddress (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_IO_PPI           *UsbIoPpi,
  IN UINT16                   AddressValue
  );

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
  );

/**
  Configure a usb device to Configuration 1.

  @param  PeiServices        General-purpose services that are available to every PEIM.
  @param  UsbIoPpi           Indicates the PEI_USB_IO_PPI instance.

  @retval EFI_SUCCESS        Usb device is set to use Configuration 1 successfully.
  @retval EFI_DEVICE_ERROR   Cannot set the usb device due to a hardware error.
  @retval Others             Other failure occurs.

**/
EFI_STATUS
PeiUsbSetConfiguration (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_IO_PPI           *UsbIoPpi
  );

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
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_IO_PPI           *UsbIoPpi,
  IN UINT8                    EndpointAddress
  );

/**
  Judge if the port is connected with a usb device or not.

  @param  PortStatus  The usb port status gotten.

  @retval TRUE        A usb device is connected with the port.
  @retval FALSE       No usb device is connected with the port.

**/
BOOLEAN
IsPortConnect (
  IN UINT16  PortStatus
  );

/**
  Judge if the port is connected with a low-speed usb device or not.

  @param  PortStatus  The usb port status gotten.

  @retval TRUE        A low-speed usb device is connected with the port.
  @retval FALSE       No low-speed usb device is connected with the port.

**/
BOOLEAN
IsPortLowSpeedDeviceAttached (
  IN UINT16  PortStatus
  );

/**
  Judge if the port is in "connection change" status or not.

  @param  PortChangeStatus  The usb port change status gotten.

  @retval TRUE              The port is in "connection change" status.
  @retval FALSE             The port is NOT in "connection change" status.

**/
BOOLEAN
IsPortConnectChange (
  IN UINT16  PortChangeStatus
  );
#endif
