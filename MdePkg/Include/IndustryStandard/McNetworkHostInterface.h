/** @file

  Copyright (c) 2026, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference:
    - DSP0270 version 1.3.1 (https://www.dmtf.org/dsp/DSP0270)

**/

#pragma once

#define MCHI_NETWORK_DEVICE_CHARACTERISTIC_CREDENTIAL_VIA_IPMI_SUPPORT  (BIT0)
#define MCHI_NETWORK_DEVICE_CHARACTERISTIC_RESERVED_BITS                (~(BIT0))

/** Network Device type value.

    See DSP0270 7.3.1 Table 3: Device type values.
*/
typedef enum MchiNetworkDeviceType {
  MchiNetworkDeviceTypeReserved00 = 0x00,
  MchiNetworkDeviceTypeReserved01 = 0x01,
  MchiNetworkDeviceTypeUsb        = 0x02,
  MchiNetworkDeviceTypePci        = 0x03,
  MchiNetworkDeviceTypeUsbv2      = 0x04,
  MchiNetworkDeviceTypePciv2      = 0x05,
  MchiNetworkDeviceTypeOemStart   = 0x80,
  MchiNetworkDeviceTypeOemEnd     = 0xff,
  MchiNetworkDeviceTypeMax,
} MCHI_NETWORK_DEVICE_TYPE;

#pragma pack(1)

/** Interface-specific data for interface type 40h (Network)

    See DSP0270 7.3 Table 2: Interface-specific data for interface type 40h.
*/
typedef struct MchiNetworkSpecificData {
  /// Device Type (REDFISH_INTERFACE_DEVICE_TYPE)
  UINT8    DeviceType;

  /// Followed by Device Data. See the REDFISH_HOST_INTERFACE_*_DATA.
} MCHI_NETWORK_SPECIFIC_DATA;

/** Interface-specific USB data for interface type 40h (Network)

    See DSP0270 7.3.2 Table 4: Device descriptor data for device type 02h,
    USB network interface.
*/
typedef struct MchiNetworkDeviceDescUsbData {
  /// Vendor Id.
  UINT16    VendorId;

  /// Product Id.
  UINT16    ProductId;

  /// Serial number descriptor Length. minimum is 0x02.
  UINT8     SerialNumberDescLength;

  /// Serial number descriptor Type. always 0x03 (unicode)
  UINT8     SerialNumberDescType;

  /// followed by SerialNumber Unicode string without NULL terminator.
  /// UINT16 SerialNumber[]
} MCHI_NETWORK_DEVICE_DESC_USB_DATA;

/** Interface-specific PCI data for interface type 40h (Network)

    See DSP0270 7.3.3 Table 5: Device descriptor data for device type 03h,
    PCI/PCIe network interface.

*/
typedef struct MchiNetworkDeviceDescPciData {
  /// Vendor Id.
  UINT16    VendorId;

  /// Device Id.
  UINT16    DeviceId;

  /// Subsystem Vendor Id.
  UINT16    SubSystemVendorId;

  /// Subsystem Id.
  UINT16    SubSystemId;
} MCHI_NETWORK_DEVICE_DESC_PCI_DATA;

/** Interface-specific USB v2 data for interface type 40h (Network)

    See DSP0270 7.3.4 Table 6: Device descriptor data for device type 04h,
    USB network interface v2.
*/
typedef struct MchiNetworkDeviceDescUsbv2Data {
  /// Length of this structure.
  UINT8     Length;

  /// Vendor Id.
  UINT16    VendorId;

  /// Product Id.
  UINT16    ProductId;

  /// Serial number reference in string table.
  UINT8     SerialNumberRef;

  /// Mac address of the USB.
  UINT8     MacAddress[6];

  /// Device Characteristics (version 2 only).
  UINT16    Characteristic;

  /// Credential bootstrapping with IPMI interface (Type 38) otherwise 0xFFFF.
  UINT16    CredentialBootStrappingHandle;
} MCHI_NETWORK_DEVICE_DESC_USB_V2_DATA;

/** Interface-specific PCI/PCIe v2 data for interface type 40h (Network)

    See DSP0270 7.3.5 Table 7: Device descriptor data for device type 04h,
    PCI/PCIe network interface v2.
*/
typedef struct MchiNetworkDeviceDescPciv2Data {
  /// Length of this structure.
  UINT8     Length;

  /// Vendor Id.
  UINT16    VendorId;

  /// Device Id.
  UINT16    DeviceId;

  /// Subsystem Vendor Id.
  UINT16    SubSystemVendorId;

  /// Subsystem Id.
  UINT16    SubSystemId;

  /// MAC address (v2 only).
  UINT8     MacAddress[6];

  /// Segment group number
  UINT16    Segment;

  /// Bus
  UINT8     Bus;

  /*
   * Function (device) number (v2 only):
   *   Bits 7:3 - Device Number
   *   Bits 2:0 - Function Number
   */
  UINT8     Function;

  /// Device Characteristics (version 2 only).
  UINT16    Characteristic;

  /// Credential bootstrapping with IPMI interface (Type 38) otherwise 0xFFFF.
  UINT16    CredentialBootStrappingHandle;
} MCHI_NETWORK_DEVICE_DESC_PCI_V2_DATA;

/** Interface-specific OEM data for interface type 40h (Network)

    See DSP0270 7.3.6 Table 8: Device descriptor data for device type 80h-FFh,
    OEM device.
*/
typedef struct MchiNetworkDeviceDescOemData {
  /// Vendor IANA
  UINT32    Iana;

  /// Followed by OEM defined data.
} MCHI_NETWORK_DEVICE_DESC_OEM_DATA;

/** Assignment and discovery type values

    See DSP0270 7.4.2 Table 12
*/
typedef enum RedfishIpType {
  RedfishIpTypeUnknown       = 0x00,
  RedfishIpTypeStatic        = 0x01,
  RedfishIpTypeDhcp          = 0x02,
  RedfishIpTypeAutoConfigure = 0x03,
  RedfishIpTypeHostSelected  = 0x04,
  RedfishIpTypeMax,
} REDFISH_IP_TYPE;

/** IP address format values

    See DSP0270 7.4.3 Table 13
*/
typedef enum RedfishIpAddressFormat {
  RedfishIpFormatUnknown = 0x00,
  RedfishIpFormatIpv4    = 0x01,
  RedfishIpFormatIpv6    = 0x02,
  RedfishIpFormatMax,
} REDFISH_IP_ADDRESS_FORMAT;

/** Redfish over IP protocol-specific record data

    See DSP0270 7.4.1 Table 11
*/
typedef struct RedfishProtocolOverIpData {
  /// Service UUID.
  EFI_GUID    ServiceUuid;

  /// Host Ip assignment type.
  UINT8       HostIpAssignType;

  /// Host Ip address format.
  UINT8       HostIpAddressFormat;

  /// Host Ip address.
  UINT8       HostIpAddress[16];

  /// Host Ip mask.
  UINT8       HostIpMask[16];

  /// Service Ip discovery type.
  UINT8       ServiceIpDiscoveryType;

  /// Service Ip address format.
  UINT8       ServiceIpAddressFormat;

  /// Service Ip address.
  UINT8       ServiceIpAddress[16];

  /// Service Ip Mask.
  UINT8       ServiceIpMask[16];

  /// Service Ip Port.
  UINT16      ServiceIpPort;

  /// Service Vlan Id.
  UINT32      ServiceVlanId;

  /// Length of Hostname
  UINT8       HostnameLength;

  /// followed by Hostname
} REDFISH_PROTOCOL_OVER_IP_DATA;

#pragma pack()

/* Note. GetSizeof should get the number of string in the string table. */
