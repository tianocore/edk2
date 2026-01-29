/** @file

  Internal definitions for the VirtIo PCI Device driver

  Copyright (C) 2013, ARM Ltd
  Copyright (c) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_PCI_DEVICE_DXE_H_
#define _VIRTIO_PCI_DEVICE_DXE_H_

#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/VirtioDevice.h>

#include <IndustryStandard/Virtio.h>

#define VIRTIO_PCI_DEVICE_SIGNATURE  SIGNATURE_32 ('V', 'P', 'C', 'I')

typedef struct {
  UINT32                    Signature;
  VIRTIO_DEVICE_PROTOCOL    VirtioDevice;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINT64                    OriginalPciAttributes;
  UINT32                    DeviceSpecificConfigurationOffset;
} VIRTIO_PCI_DEVICE;

#define VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE(Device) \
    CR (Device, VIRTIO_PCI_DEVICE, VirtioDevice, VIRTIO_PCI_DEVICE_SIGNATURE)

EFI_STATUS
EFIAPI
VirtioPciIoRead (
  IN  VIRTIO_PCI_DEVICE  *Dev,
  IN  UINTN              FieldOffset,
  IN  UINTN              FieldSize,
  IN  UINTN              BufferSize,
  OUT VOID               *Buffer
  );

EFI_STATUS
EFIAPI
VirtioPciIoWrite (
  IN  VIRTIO_PCI_DEVICE  *Dev,
  IN UINTN               FieldOffset,
  IN UINTN               FieldSize,
  IN UINT64              Value
  );

/********************************************
 * PCI Functions for VIRTIO_DEVICE_PROTOCOL
 *******************************************/
EFI_STATUS
EFIAPI
VirtioPciDeviceRead (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   FieldOffset,
  IN  UINTN                   FieldSize,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  );

EFI_STATUS
EFIAPI
VirtioPciDeviceWrite (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINTN                   FieldOffset,
  IN UINTN                   FieldSize,
  IN UINT64                  Value
  );

EFI_STATUS
EFIAPI
VirtioPciGetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT64                 *DeviceFeatures
  );

EFI_STATUS
EFIAPI
VirtioPciGetQueueSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueAlignment (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINT32                  Alignment
  );

EFI_STATUS
EFIAPI
VirtioPciSetPageSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINT32                  PageSize
  );

EFI_STATUS
EFIAPI
VirtioPciGetDeviceStatus (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT8                   *DeviceStatus
  );

EFI_STATUS
EFIAPI
VirtioPciSetGuestFeatures (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT64                  Features
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueAddress (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN VRING                   *Ring,
  IN UINT64                  RingBaseShift
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueSel (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINT16                  Sel
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueNotify (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINT16                  Index
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINT16                  Size
  );

EFI_STATUS
EFIAPI
VirtioPciSetDeviceStatus (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINT8                   DeviceStatus
  );

EFI_STATUS
EFIAPI
VirtioPciAllocateSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   NumPages,
  OUT VOID                    **HostAddress
  );

VOID
EFIAPI
VirtioPciFreeSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   NumPages,
  IN  VOID                    *HostAddress
  );

EFI_STATUS
EFIAPI
VirtioPciMapSharedBuffer (
  IN      VIRTIO_DEVICE_PROTOCOL  *This,
  IN      VIRTIO_MAP_OPERATION    Operation,
  IN      VOID                    *HostAddress,
  IN OUT  UINTN                   *NumberOfBytes,
  OUT     EFI_PHYSICAL_ADDRESS    *DeviceAddress,
  OUT     VOID                    **Mapping
  );

EFI_STATUS
EFIAPI
VirtioPciUnmapSharedBuffer (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  VOID                    *Mapping
  );

#endif // _VIRTIO_PCI_DEVICE_DXE_H_
