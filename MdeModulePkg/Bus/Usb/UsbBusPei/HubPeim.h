/** @file
Constants definitions for Usb Hub Peim

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_HUB_PEIM_H_
#define _PEI_HUB_PEIM_H_

//
// Hub feature numbers
//
#define C_HUB_LOCAL_POWER   0
#define C_HUB_OVER_CURRENT  1

//
// Hub class code & sub class code
//
#define CLASS_CODE_HUB      0x09
#define SUB_CLASS_CODE_HUB  0

//
// Hub Status & Hub Change bit masks
//
#define HUB_STATUS_LOCAL_POWER  0x0001
#define HUB_STATUS_OVERCURRENT  0x0002

#define HUB_CHANGE_LOCAL_POWER  0x0001
#define HUB_CHANGE_OVERCURRENT  0x0002

//
// Hub Characteristics
//
#define HUB_CHAR_LPSM      0x0003
#define HUB_CHAR_COMPOUND  0x0004
#define HUB_CHAR_OCPM      0x0018

//
// Standard hub request and request type
// By [Spec-USB20/Chapter-11.24]
//
#define USB_HUB_CLEAR_FEATURE           0x01
#define USB_HUB_CLEAR_FEATURE_REQ_TYPE  0x20

#define USB_HUB_CLEAR_FEATURE_PORT           0x01
#define USB_HUB_CLEAR_FEATURE_PORT_REQ_TYPE  0x23

#define USB_HUB_GET_BUS_STATE           0x02
#define USB_HUB_GET_BUS_STATE_REQ_TYPE  0xA3

#define USB_HUB_GET_DESCRIPTOR           0x06
#define USB_HUB_GET_DESCRIPTOR_REQ_TYPE  0xA0

#define USB_HUB_GET_HUB_STATUS           0x00
#define USB_HUB_GET_HUB_STATUS_REQ_TYPE  0xA0

#define USB_HUB_GET_PORT_STATUS           0x00
#define USB_HUB_GET_PORT_STATUS_REQ_TYPE  0xA3

#define USB_HUB_SET_DESCRIPTOR           0x07
#define USB_HUB_SET_DESCRIPTOR_REQ_TYPE  0x20

#define USB_HUB_SET_HUB_FEATURE           0x03
#define USB_HUB_SET_HUB_FEATURE_REQ_TYPE  0x20

#define USB_HUB_SET_PORT_FEATURE           0x03
#define USB_HUB_SET_PORT_FEATURE_REQ_TYPE  0x23

#define USB_RT_HUB   (USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT  (USB_TYPE_CLASS | USB_RECIP_OTHER)

#define USB_HUB_REQ_SET_DEPTH  12

#define MAXBYTES  8
#pragma pack(1)
//
// Hub descriptor, the last two fields are of variable length.
//
typedef struct {
  UINT8    Length;
  UINT8    DescriptorType;
  UINT8    NbrPorts;
  UINT8    HubCharacteristics[2];
  UINT8    PwrOn2PwrGood;
  UINT8    HubContrCurrent;
  UINT8    Filler[MAXBYTES];
} EFI_USB_HUB_DESCRIPTOR;

typedef struct {
  UINT16    HubStatus;
  UINT16    HubChangeStatus;
} EFI_USB_HUB_STATUS;

#pragma pack()

/**
  Get a given hub port status.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Port          Usb hub port number (starting from 1).
  @param  PortStatus    Current Hub port status and change status.

  @retval EFI_SUCCESS       Port status is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the port status due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubGetPortStatus (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN  UINT8            Port,
  OUT UINT32           *PortStatus
  );

/**
  Set specified feature to a given hub port.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Port          Usb hub port number (starting from 1).
  @param  Value         New feature value.

  @retval EFI_SUCCESS       Port feature is set successfully.
  @retval EFI_DEVICE_ERROR  Cannot set the port feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubSetPortFeature (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN  UINT8            Port,
  IN  UINT8            Value
  );

/**
  Get a given hub status.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  HubStatus     Current Hub status and change status.

  @retval EFI_SUCCESS       Hub status is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the hub status due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubGetHubStatus (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  OUT UINT32           *HubStatus
  );

/**
  Clear specified feature on a given hub port.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Port          Usb hub port number (starting from 1).
  @param  Value         Feature value that will be cleared from the hub port.

  @retval EFI_SUCCESS       Port feature is cleared successfully.
  @retval EFI_DEVICE_ERROR  Cannot clear the port feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubClearPortFeature (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN  UINT8            Port,
  IN  UINT8            Value
  );

/**
  Clear specified feature on a given hub.

  @param  PeiServices   General-purpose services that are available to every PEIM.
  @param  UsbIoPpi      Indicates the PEI_USB_IO_PPI instance.
  @param  Value         Feature value that will be cleared from the hub port.

  @retval EFI_SUCCESS       Hub feature is cleared successfully.
  @retval EFI_DEVICE_ERROR  Cannot clear the hub feature due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiHubClearHubFeature (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN  UINT8            Value
  );

/**
  Get a given hub descriptor.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  PeiUsbDevice   Indicates the hub controller device.
  @param  UsbIoPpi       Indicates the PEI_USB_IO_PPI instance.
  @param  DescriptorSize The length of Hub Descriptor buffer.
  @param  HubDescriptor  Caller allocated buffer to store the hub descriptor if
                         successfully returned.

  @retval EFI_SUCCESS       Hub descriptor is obtained successfully.
  @retval EFI_DEVICE_ERROR  Cannot get the hub descriptor due to a hardware error.
  @retval Others            Other failure occurs.

**/
EFI_STATUS
PeiGetHubDescriptor (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN PEI_USB_DEVICE           *PeiUsbDevice,
  IN PEI_USB_IO_PPI           *UsbIoPpi,
  IN UINTN                    DescriptorSize,
  OUT EFI_USB_HUB_DESCRIPTOR  *HubDescriptor
  );

/**
  Configure a given hub.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  PeiUsbDevice   Indicating the hub controller device that will be configured

  @retval EFI_SUCCESS       Hub configuration is done successfully.
  @retval EFI_DEVICE_ERROR  Cannot configure the hub due to a hardware error.

**/
EFI_STATUS
PeiDoHubConfig (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_DEVICE    *PeiUsbDevice
  );

/**
  Send reset signal over the given root hub port.

  @param  PeiServices    General-purpose services that are available to every PEIM.
  @param  UsbIoPpi       Indicates the PEI_USB_IO_PPI instance.
  @param  PortNum        Usb hub port number (starting from 1).

**/
VOID
PeiResetHubPort (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_USB_IO_PPI    *UsbIoPpi,
  IN UINT8             PortNum
  );

#endif
