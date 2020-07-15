/** @file
  Private definitions of the VirtIo 1.0 driver.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _VIRTIO_1_0_DXE_H_
#define _VIRTIO_1_0_DXE_H_

#include <Protocol/PciIo.h>
#include <Protocol/VirtioDevice.h>

#define VIRTIO_1_0_SIGNATURE SIGNATURE_32 ('V', 'I', 'O', '1')

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
  BOOLEAN             Exists;  // The device exposes this structure
  VIRTIO_1_0_BAR_TYPE BarType;
  UINT8               Bar;
  UINT32              Offset;  // Offset into BAR where structure starts
  UINT32              Length;  // Length of structure in BAR.
} VIRTIO_1_0_CONFIG;

typedef struct {
  UINT32                 Signature;
  VIRTIO_DEVICE_PROTOCOL VirtIo;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  UINT64                 OriginalPciAttributes;
  VIRTIO_1_0_CONFIG      CommonConfig;           // Common settings
  VIRTIO_1_0_CONFIG      NotifyConfig;           // Notifications
  UINT32                 NotifyOffsetMultiplier;
  VIRTIO_1_0_CONFIG      SpecificConfig;         // Device specific settings
} VIRTIO_1_0_DEV;

#define VIRTIO_1_0_FROM_VIRTIO_DEVICE(Device) \
          CR (Device, VIRTIO_1_0_DEV, VirtIo, VIRTIO_1_0_SIGNATURE)

#endif // _VIRTIO_1_0_DXE_H_
