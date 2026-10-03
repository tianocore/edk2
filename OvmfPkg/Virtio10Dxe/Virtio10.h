/** @file
  Private definitions of the VirtIo 1.0 driver.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Protocol/PciIo.h>
#include <Protocol/VirtioDevice.h>

#define VIRTIO_1_0_SIGNATURE  SIGNATURE_32 ('V', 'I', 'O', '1')

//
// Type of the PCI BAR that contains a VirtIo 1.0 config structure.
//
typedef enum {
  Virtio10BarTypeMem,
  Virtio10BarTypeIo
} VIRTIO_1_0_BAR_TYPE;

//
// The type below defines the access to a VirtIo 1.0 config structure.
//
typedef struct {
  BOOLEAN                Exists; // The device exposes this structure
  VIRTIO_1_0_BAR_TYPE    BarType;
  UINT8                  Bar;    // EFI_PCI_IO_PROTOCOL BAR index
  UINT32                 Offset; // Offset into BAR where structure starts
  UINT32                 Length; // Length of structure in BAR.
} VIRTIO_1_0_CONFIG;

//
// Maximum number of VIRTIO_PCI_CAP_SHARED_MEMORY_CFG regions tracked per
// device. Regions beyond this limit are ignored (and left mapped uncached).
//
#define VIRTIO_1_0_MAX_SHM_REGIONS  8

//
// A shared memory region exposed by the device through a
// VIRTIO_PCI_CAP_SHARED_MEMORY_CFG capability. Such regions are not read
// sensitive, and may therefore be mapped with write-combining semantics.
//
typedef struct {
  UINT8                   Id;               // Shared memory region ID
  UINT8                   Bar;              // EFI_PCI_IO_PROTOCOL BAR index
  UINT64                  Offset;           // Offset into BAR where region starts
  UINT64                  Length;           // Length of region in BAR
  EFI_PHYSICAL_ADDRESS    HostAddress;      // CPU view of the region's base
  BOOLEAN                 Remapped;         // GCD attributes were changed to WC
  UINT64                  OrigAttributes;   // GCD attributes before remapping
  UINT64                  OrigCapabilities; // GCD capabilities before remapping
} VIRTIO_1_0_SHM_REGION;

typedef struct {
  UINT32                    Signature;
  VIRTIO_DEVICE_PROTOCOL    VirtIo;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    OriginalPciAttributes;
  VIRTIO_1_0_CONFIG         CommonConfig;        // Common settings
  VIRTIO_1_0_CONFIG         NotifyConfig;        // Notifications
  UINT32                    NotifyOffsetMultiplier;
  VIRTIO_1_0_CONFIG         SpecificConfig;      // Device specific settings
  UINTN                     ShmRegionCount;
  VIRTIO_1_0_SHM_REGION     ShmRegions[VIRTIO_1_0_MAX_SHM_REGIONS];
} VIRTIO_1_0_DEV;

#define VIRTIO_1_0_FROM_VIRTIO_DEVICE(Device) \
          CR (Device, VIRTIO_1_0_DEV, VirtIo, VIRTIO_1_0_SIGNATURE)
