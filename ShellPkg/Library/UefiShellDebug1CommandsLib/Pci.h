/** @file
  Header file for Pci shell Debug1 function.

  Copyright (c) 2013 Hewlett-Packard Development Company, L.P.
  Copyright (c) 2005 - 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_SHELL_PCI_H_
#define _EFI_SHELL_PCI_H_

typedef enum {
  PciDevice,
  PciP2pBridge,
  PciCardBusBridge,
  PciUndefined
} PCI_HEADER_TYPE;

#define INDEX_OF(Field)                               ((UINT8 *) (Field) - (UINT8 *) mConfigSpace)

#define IS_PCIE_ENDPOINT(DevicePortType) \
    ((DevicePortType) == PCIE_DEVICE_PORT_TYPE_PCIE_ENDPOINT || \
     (DevicePortType) == PCIE_DEVICE_PORT_TYPE_LEGACY_PCIE_ENDPOINT || \
     (DevicePortType) == PCIE_DEVICE_PORT_TYPE_ROOT_COMPLEX_INTEGRATED_ENDPOINT)

#define IS_PCIE_SWITCH(DevicePortType) \
    ((DevicePortType == PCIE_DEVICE_PORT_TYPE_UPSTREAM_PORT) || \
     (DevicePortType == PCIE_DEVICE_PORT_TYPE_DOWNSTREAM_PORT))

#pragma pack(1)
//
// Data region after PCI configuration header(for cardbus bridge)
//
typedef struct {
  UINT16  SubVendorId;  // Subsystem Vendor ID
  UINT16  SubSystemId;  // Subsystem ID
  UINT32  LegacyBase;   // Optional 16-Bit PC Card Legacy
  // Mode Base Address
  //
  UINT32  Data[46];
} PCI_CARDBUS_DATA;

typedef union {
  PCI_DEVICE_HEADER_TYPE_REGION Device;
  PCI_BRIDGE_CONTROL_REGISTER   Bridge;
  PCI_CARDBUS_CONTROL_REGISTER  CardBus;
} NON_COMMON_UNION;

typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Common;
  NON_COMMON_UNION              NonCommon;
  UINT32                        Data[48];
} PCI_CONFIG_SPACE;

#pragma pack()

#endif // _PCI_H_
