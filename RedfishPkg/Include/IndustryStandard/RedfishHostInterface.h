/** @file
  This file defines the Redfish Interface Specific Data.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef REDFISH_HOST_INTERFACE_
#define REDFISH_HOST_INTERFACE_

#include <IndustryStandard/SmBios.h>

#define REDFISH_HOST_INTERFACE_DEVICE_TYPE_USB         0x02 // We don't support this type of interface.
                                                            // Use REDFISH_HOST_INTERFACE_DEVICE_TYPE_USB_V2 instead.
#define REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE    0x03 // We don't support this type of interface.
                                                            // Use REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE_V2 instead.
#define REDFISH_HOST_INTERFACE_DEVICE_TYPE_USB_V2      0x04
#define REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE_V2 0x05

#define REDFISH_HOST_INTERFACE_HOST_IP_ASSIGNMENT_TYPE_UNKNOWN            0x00
#define REDFISH_HOST_INTERFACE_HOST_IP_ASSIGNMENT_TYPE_STATIC             0x01
#define REDFISH_HOST_INTERFACE_HOST_IP_ASSIGNMENT_TYPE_DHCP               0x02
#define REDFISH_HOST_INTERFACE_HOST_IP_ASSIGNMENT_TYPE_AUTO_CONFIGURE     0x03
#define REDFISH_HOST_INTERFACE_HOST_IP_ASSIGNMENT_TYPE_HOST_SELECTED      0x04

#define REDFISH_HOST_INTERFACE_HOST_IP_ADDRESS_FORMAT_UNKNOWN      0x00
#define REDFISH_HOST_INTERFACE_HOST_IP_ADDRESS_FORMAT_IP4          0x01
#define REDFISH_HOST_INTERFACE_HOST_IP_ADDRESS_FORMAT_IP6          0x02

#pragma pack(1)
///
/// Structure definitions of Host Interface device type 04h (USB Network Interface V2)
///
typedef struct {
  UINT8                   Length;          ///< Length of the structure, including Device Type
                                           ///< and Length fields.
  UINT16                  IdVendor;        ///< The Vendor ID of the device, as read from the
                                           ///< idVendor field of the USB descriptor.
  UINT16                  IdProduct;       ///< The Product ID of the device, as read from the
                                           ///< idProduct field of the USB descriptor.
  UINT8                   SecialNumberStr; ///< The string number for the Serial Number of the
                                           ///< device. The string data is read from the
                                           ///< iSerialNumber.bDescriptorType field of the USB
                                           ///< descriptor, and is converted from Unicode to ASCII
                                           ///< and is NULL terminated.
  UINT8                   MacAddress [6];  ///< The MAC address of the PCI/PCIe network device.
} USB_INTERFACE_DEVICE_DESCRIPTOR_V2;

//
// Structure definitions of Host Interface device type 05h (PCI/PCIE V2)
//
typedef struct {
  UINT8                   Length;               ///< Length of the structure, including Device Type and Length fields.
  UINT16                  VendorId;             ///< The Vendor ID of the PCI/PCIe device.
  UINT16                  DeviceId;             ///< The Device ID of the PCI/PCIe device.
  UINT16                  SubsystemVendorId;    ///< The Subsystem Vendor ID of the PCI/PCIe device.
  UINT16                  SubsystemId;          ///< The Subsystem ID of the PCI/PCIe device.
  UINT8                   MacAddress [6];       ///< The MAC address of the PCI/PCIe network device.
  UINT16                  SegmemtGroupNumber;   ///< The Segment Group Number of the PCI/PCIe.
  UINT8                   BusNumber;            ///< The Bus Number of the PCI/PCIe device.
  UINT8                   DeviceFunctionNumber; ///< The Device/Function Number of the PCI/PCIe.
} PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2;

///
/// Structure definitions of Host Interface device type 80-FFh (OEM)
///
typedef struct {
  UINT32                  VendorIana;          ///< The IANA code for the vendor (MSB first).
  UINT8                   OemDefinedData[1];   ///< OEM defined data.
} OEM_DEVICE_DESCRIPTOR;

///
/// Define union for the Host Interface Device Descriptor
///
typedef union {
    USB_INTERFACE_DEVICE_DESCRIPTOR_V2          UsbDeviceV2;     ///< Device type USB V2 device discriptor.
    PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2  PciPcieDeviceV2; ///< Device type PCI/PCIe V2 device discriptor.
    OEM_DEVICE_DESCRIPTOR                       OemDevice;       ///< OEM type device discriptor.
} DEVICE_DESCRITOR; /// Device descriptor data formated based on Device Type.

///
///  Interface Specific Data starts at offset 06h of the SMBIOS Type 42 struct.
///  This table defines the Interface Specific data for Interface Type 40h. There
///  are 3 types of Device Descriptor3 defined , however only 1 may be used in
///  specific Tape 42 table.
///
typedef struct {
  UINT8            DeviceType;        ///< The Device Type of the interface.
  DEVICE_DESCRITOR DeviceDescriptor;  ///< The Device descriptor.
} REDFISH_INTERFACE_DATA;

//
//  the protocol-specific data for the "Redfish Over IP" protocol
//
typedef struct {
  EFI_GUID            ServiceUuid;  //same as Redfish Service UUID in Redfish Service Root resource

  //
  //  Unknown=00h,
  //  Static=01h,
  //  DHCP=02h,
  //  AutoConfigure=03h,
  //  HostSelected=04h,
  //  other values reserved
  //
  UINT8               HostIpAssignmentType;

  //
  //  Unknown=00h,
  //  Ipv4=01h,
  //  Ipv6=02h,
  //  other values reserved
  //
  UINT8               HostIpAddressFormat;

  //
  //  Used for Static and AutoConfigure.
  //  For IPV4, use the first 4 Bytes and zero fill the remaining bytes.
  //
  UINT8               HostIpAddress[16];

  //
  //  Used for Static and AutoConfigure.
  //  For IPV4, use the first 4 Bytes and zero fill the remaining bytes.
  //
  UINT8               HostIpMask[16];

  //
  //  Unknown=00h,
  //  Static=01h,
  //  DHCP=02h,
  //  AutoConfigure=03h,
  //  HostSelected=04h,
  //  other values reserved
  //
  UINT8               RedfishServiceIpDiscoveryType;

  //
  //  Unknown=00h,
  //  Ipv4=01h,
  //  Ipv6=02h,
  //  other values reserved
  //
  UINT8               RedfishServiceIpAddressFormat;

  //
  //  Used for Static and AutoConfigure.
  //  For IPV4, use the first 4 Bytes and zero fill the remaining bytes.
  //
  UINT8               RedfishServiceIpAddress[16];

  //
  //  Used for Static and AutoConfigure.
  //  For IPV4, use the first 4 Bytes and zero fill the remaining bytes.
  //
  UINT8               RedfishServiceIpMask[16];

  UINT16              RedfishServiceIpPort;  // Used for Static and AutoConfigure.
  UINT32              RedfishServiceVlanId;  // Used for Static and AutoConfigure.
  UINT8               RedfishServiceHostnameLength;   // length of the following hostname string
  UINT8               RedfishServiceHostname[1];  // hostname of Redfish Service
} REDFISH_OVER_IP_PROTOCOL_DATA;

#pragma pack()

#endif

