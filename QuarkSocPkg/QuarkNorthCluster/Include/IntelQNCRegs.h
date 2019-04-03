/** @file
Registers definition for Intel QuarkNcSocId.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __INTEL_QNC_REGS_H__
#define __INTEL_QNC_REGS_H__

#include <QNCAccess.h>

//
// PCI HostBridge Segment number
//
#define QNC_PCI_HOST_BRIDGE_SEGMENT_NUMBER    0

//
// PCI RootBridge resource allocation's attribute
//
#define QNC_PCI_ROOT_BRIDGE_RESOURCE_ALLOCATION_ATTRIBUTE \
  EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM

//
// PCI HostBridge resource appeture
//
#define QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSBASE     0x0
#define QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSLIMIT    0xff
#define QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_TSEG_SIZE   0x10000000

//
// PCI RootBridge configure port
//
#define QNC_PCI_ROOT_BRIDGE_CONFIGURATION_ADDRESS_PORT  0xCF8
#define QNC_PCI_ROOT_BRIDGE_CONFIGURATION_DATA_PORT     0xCFC

//
// PCI Rootbridge's support feature
//
#define QNC_PCI_ROOT_BRIDGE_SUPPORTED                   (EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO | \
                                                         EFI_PCI_ATTRIBUTE_ISA_IO         | \
                                                         EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO | \
                                                         EFI_PCI_ATTRIBUTE_VGA_MEMORY     | \
                                                         EFI_PCI_ATTRIBUTE_VGA_IO)

#endif // __INTEL_QNC_REGS_H__
