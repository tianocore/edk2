/** @file
  Macro and type definitions for QEMU's Red Hat vendor-specific PCI
  capabilities that provide various hints about PCI Bridges.

  Refer to "docs/pcie_pci_bridge.txt" in the QEMU source directory.

  Copyright (C) 2017, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __QEMU_PCI_BRIDGE_CAPABILITIES_H__
#define __QEMU_PCI_BRIDGE_CAPABILITIES_H__

#include <IndustryStandard/Pci23.h>

//
// The hints apply to PCI Bridges whose PCI_DEVICE_INDEPENDENT_REGION.VendorId
// equals the following value.
//
#define QEMU_PCI_BRIDGE_VENDOR_ID_REDHAT 0x1B36

//
// Common capability header for all hints.
//
#pragma pack (1)
typedef struct {
  EFI_PCI_CAPABILITY_VENDOR_HDR VendorHdr;
  UINT8                         Type;
} QEMU_PCI_BRIDGE_CAPABILITY_HDR;
#pragma pack ()

//
// Values defined for QEMU_PCI_BRIDGE_CAPABILITY_HDR.Type.
//
#define QEMU_PCI_BRIDGE_CAPABILITY_TYPE_RESOURCE_RESERVATION 0x01

//
// PCI Resource Reservation structure for when
// QEMU_PCI_BRIDGE_CAPABILITY_HDR.Type equals
// QEMU_PCI_BRIDGE_CAPABILITY_TYPE_RESOURCE_RESERVATION.
//
#pragma pack (1)
typedef struct {
  QEMU_PCI_BRIDGE_CAPABILITY_HDR BridgeHdr;
  UINT32                         BusNumbers;
  UINT64                         Io;
  UINT32                         NonPrefetchable32BitMmio;
  UINT32                         Prefetchable32BitMmio;
  UINT64                         Prefetchable64BitMmio;
} QEMU_PCI_BRIDGE_CAPABILITY_RESOURCE_RESERVATION;
#pragma pack ()

#endif
