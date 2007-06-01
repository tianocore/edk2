/** @file
  Support for USB 1.1 standard.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Usb.h

**/

#ifndef __USB_H__
#define __USB_H__

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

//
// Request target types.
//
#define USB_RT_DEVICE     0x00
#define USB_RT_INTERFACE  0x01
#define USB_RT_ENDPOINT   0x02
#define USB_RT_HUB        (USB_TYPE_CLASS | USB_RECIP_DEVICE)
#define USB_RT_PORT       (USB_TYPE_CLASS | USB_RECIP_OTHER)

//
// USB Transfer Results
//
#define EFI_USB_NOERROR         0x00
#define EFI_USB_ERR_NOTEXECUTE  0x01
#define EFI_USB_ERR_STALL       0x02
#define EFI_USB_ERR_BUFFER      0x04
#define EFI_USB_ERR_BABBLE      0x08
#define EFI_USB_ERR_NAK         0x10
#define EFI_USB_ERR_CRC         0x20
#define EFI_USB_ERR_TIMEOUT     0x40
#define EFI_USB_ERR_BITSTUFF    0x80
#define EFI_USB_ERR_SYSTEM      0x100

//
//Use 200 ms to increase the error handling response time
//
#define EFI_USB_INTERRUPT_DELAY 2000000

//
// USB transation direction
//
typedef enum {
  EfiUsbDataIn,
  EfiUsbDataOut,
  EfiUsbNoData
} EFI_USB_DATA_DIRECTION;

//
// Usb Data recipient type
//
typedef enum {
  EfiUsbDevice,
  EfiUsbInterface,
  EfiUsbEndpoint
} EFI_USB_RECIPIENT;

typedef enum {
  EfiUsbEndpointHalt,
  EfiUsbDeviceRemoteWakeup
} EFI_USB_STANDARD_FEATURE_SELECTOR;

#pragma pack(1)
//
// Usb device request structure
//
typedef struct {
  UINT8   RequestType;
  UINT8   Request;
  UINT16  Value;
  UINT16  Index;
  UINT16  Length;
} EFI_USB_DEVICE_REQUEST;

//
// Standard USB request
//
#define USB_DEV_GET_STATUS                  0x00

#define USB_DEV_CLEAR_FEATURE               0x01

#define USB_DEV_SET_FEATURE                 0x03

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
// Device descriptor. refer USB1.1
//
typedef struct usb_device_descriptor {
  UINT8   Length;
  UINT8   DescriptorType;
  UINT16  BcdUSB;
  UINT8   DeviceClass;
  UINT8   DeviceSubClass;
  UINT8   DeviceProtocol;
  UINT8   MaxPacketSize0;
  UINT16  IdVendor;
  UINT16  IdProduct;
  UINT16  BcdDevice;
  UINT8   StrManufacturer;
  UINT8   StrProduct;
  UINT8   StrSerialNumber;
  UINT8   NumConfigurations;
} EFI_USB_DEVICE_DESCRIPTOR;

//
// Endpoint descriptor
//
typedef struct {
  UINT8   Length;
  UINT8   DescriptorType;
  UINT8   EndpointAddress;
  UINT8   Attributes;
  UINT16  MaxPacketSize;
  UINT8   Interval;
} EFI_USB_ENDPOINT_DESCRIPTOR;

//
// Interface descriptor
//
typedef struct {
  UINT8 Length;
  UINT8 DescriptorType;
  UINT8 InterfaceNumber;
  UINT8 AlternateSetting;
  UINT8 NumEndpoints;
  UINT8 InterfaceClass;
  UINT8 InterfaceSubClass;
  UINT8 InterfaceProtocol;
  UINT8 Interface;
} EFI_USB_INTERFACE_DESCRIPTOR;

//
// USB alternate setting
//
typedef struct {
  EFI_USB_INTERFACE_DESCRIPTOR  *Interface;
} USB_ALT_SETTING;

//
// Configuration descriptor
//
typedef struct {
  UINT8   Length;
  UINT8   DescriptorType;
  UINT16  TotalLength;
  UINT8   NumInterfaces;
  UINT8   ConfigurationValue;
  UINT8   Configuration;
  UINT8   Attributes;
  UINT8   MaxPower;
} EFI_USB_CONFIG_DESCRIPTOR;

//
// Supported String Languages
//
typedef struct {
  UINT8   Length;
  UINT8   DescriptorType;
  UINT16  SupportedLanID[1];
} EFI_USB_SUPPORTED_LANGUAGES;

//
// String descriptor
//
typedef struct {
  UINT8   Length;
  UINT8   DescriptorType;
  CHAR16  String[1];
} EFI_USB_STRING_DESCRIPTOR;

//
// Hub descriptor
//
#define MAXBYTES  8
typedef struct {
  UINT8 Length;
  UINT8 DescriptorType;
  UINT8 NbrPorts;
  UINT8 HubCharacteristics[2];
  UINT8 PwrOn2PwrGood;
  UINT8 HubContrCurrent;
  UINT8 Filler[MAXBYTES];
} EFI_USB_HUB_DESCRIPTOR;

typedef struct {
  UINT16  PortStatus;
  UINT16  PortChangeStatus;
} EFI_USB_PORT_STATUS;

//
// Constant value for Port Status & Port Change Status
//
#define USB_PORT_STAT_CONNECTION    0x0001
#define USB_PORT_STAT_ENABLE        0x0002
#define USB_PORT_STAT_SUSPEND       0x0004
#define USB_PORT_STAT_OVERCURRENT   0x0008
#define USB_PORT_STAT_RESET         0x0010
#define USB_PORT_STAT_POWER         0x0100
#define USB_PORT_STAT_LOW_SPEED     0x0200
#define USB_PORT_STAT_HIGH_SPEED    0x0400
#define USB_PORT_STAT_OWNER         0x0800

#define USB_PORT_STAT_C_CONNECTION  0x0001
#define USB_PORT_STAT_C_ENABLE      0x0002
#define USB_PORT_STAT_C_SUSPEND     0x0004
#define USB_PORT_STAT_C_OVERCURRENT 0x0008
#define USB_PORT_STAT_C_RESET       0x0010

//
// Used for set/clear port feature request
//
typedef enum {
  EfiUsbPortEnable            = 1,
  EfiUsbPortSuspend           = 2,
  EfiUsbPortReset             = 4,
  EfiUsbPortPower             = 8,
  EfiUsbPortOwner             = 13,
  EfiUsbPortConnectChange     = 16,
  EfiUsbPortEnableChange      = 17,
  EfiUsbPortSuspendChange     = 18,
  EfiUsbPortOverCurrentChange = 19,
  EfiUsbPortResetChange       = 20
} EFI_USB_PORT_FEATURE;

#pragma pack()

#endif
