/** @file

  Internal definitions for the VirtIo MMIO Device driver

  Copyright (C) 2013, ARM Ltd
  Copyright (C) 2017, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_MMIO_DEVICE_INTERNAL_H_
#define _VIRTIO_MMIO_DEVICE_INTERNAL_H_

#include <Protocol/VirtioDevice.h>

#include <IndustryStandard/Virtio.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioMmioDeviceLib.h>
#include <Library/MemoryAllocationLib.h>

#define VIRTIO_MMIO_DEVICE_SIGNATURE  SIGNATURE_32 ('V', 'M', 'I', 'O')

typedef struct {
  UINT32                 Signature;
  VIRTIO_DEVICE_PROTOCOL VirtioDevice;
  PHYSICAL_ADDRESS       BaseAddress;
} VIRTIO_MMIO_DEVICE;

#define VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE(Device) \
    CR (Device, VIRTIO_MMIO_DEVICE, VirtioDevice, VIRTIO_MMIO_DEVICE_SIGNATURE)

#define VIRTIO_CFG_WRITE(Device, Offset, Val) \
    (MmioWrite32 (Device->BaseAddress + (Offset), Val))
#define VIRTIO_CFG_READ(Device, Offset)       \
    (MmioRead32  (Device->BaseAddress + (Offset)))

EFI_STATUS
EFIAPI
VirtioMmioDeviceRead (
  IN  VIRTIO_DEVICE_PROTOCOL    *This,
  IN  UINTN                     FieldOFfset,
  IN  UINTN                     FieldSize,
  IN  UINTN                     BufferSize,
  OUT VOID*                     Buffer
  );

EFI_STATUS
EFIAPI
VirtioMmioDeviceWrite (
  IN  VIRTIO_DEVICE_PROTOCOL    *This,
  IN  UINTN                     FieldOffset,
  IN  UINTN                     FieldSize,
  IN  UINT64                    Value
  );

EFI_STATUS
EFIAPI
VirtioMmioGetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT64                *DeviceFeatures
  );

EFI_STATUS
EFIAPI
VirtioMmioGetQueueSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  );

EFI_STATUS
EFIAPI
VirtioMmioGetDeviceStatus (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT8                   *DeviceStatus
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueSize (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT16                  QueueSize
  );

EFI_STATUS
EFIAPI
VirtioMmioSetDeviceStatus (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT8                   DeviceStatus
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueNotify (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT16                  QueueNotify
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueSel (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT16                  Sel
  );

EFI_STATUS
VirtioMmioSetQueueAddress (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN VRING                   *Ring,
  IN UINT64                  RingBaseShift
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueAlignment (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT32                  Alignment
  );

EFI_STATUS
EFIAPI
VirtioMmioSetPageSize (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT32                  PageSize
  );

EFI_STATUS
EFIAPI
VirtioMmioSetGuestFeatures (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINT64                  Features
  );

EFI_STATUS
EFIAPI
VirtioMmioAllocateSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL        *This,
  IN  UINTN                         NumPages,
  OUT VOID                          **HostAddress
  );

VOID
EFIAPI
VirtioMmioFreeSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL        *This,
  IN  UINTN                         NumPages,
  IN  VOID                          *HostAddress
  );

EFI_STATUS
EFIAPI
VirtioMmioMapSharedBuffer (
  IN      VIRTIO_DEVICE_PROTOCOL        *This,
  IN      VIRTIO_MAP_OPERATION          Operation,
  IN      VOID                          *HostAddress,
  IN OUT  UINTN                         *NumberOfBytes,
  OUT     EFI_PHYSICAL_ADDRESS          *DeviceAddress,
  OUT     VOID                          **Mapping
  );

EFI_STATUS
EFIAPI
VirtioMmioUnmapSharedBuffer (
  IN  VIRTIO_DEVICE_PROTOCOL        *This,
  IN  VOID                          *Mapping
  );

#endif // _VIRTIO_MMIO_DEVICE_INTERNAL_H_
