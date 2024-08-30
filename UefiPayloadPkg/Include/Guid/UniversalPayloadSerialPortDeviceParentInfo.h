/** @file
  Universal Payload serial port parent device information definitions.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO_
#define UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO_

extern GUID  gUniversalPayloadSerialPortParentDeviceInfoGuid;

// IsIsaCompatible
// TRUE: the serial port device is under an ISA compatible bus, which means the parent device is ISA/LPC/eSPI bus controller.
// FALSE: the serial port device is native PCI device under PCI bridge.
#pragma pack(1)
typedef struct {
  UINT32     Revision;
  BOOLEAN    IsIsaCompatible;
  UINT8      Reserved[3];
  UINT64     ParentDevicePcieBaseAddress;
} UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO;
#pragma pack()

#define UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO_REVISION  1

#endif // UNIVERSAL_PAYLOAD_SERIAL_PORT_PARENT_DEVICE_INFO_
