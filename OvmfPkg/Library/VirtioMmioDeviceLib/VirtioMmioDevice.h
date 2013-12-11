/** @file

  Internal definitions for the VirtIo MMIO Device driver

  Copyright (C) 2013, ARM Ltd

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_MMIO_DEVICE_INTERNAL_H_
#define _VIRTIO_MMIO_DEVICE_INTERNAL_H_

#include <Protocol/VirtioDevice.h>

#include <IndustryStandard/Virtio.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioMmioDeviceLib.h>

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
  OUT UINT32                *DeviceFeatures
  );

EFI_STATUS
EFIAPI
VirtioMmioGetQueueAddress (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                 *QueueAddress
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
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                  QueueSize
  );

EFI_STATUS
EFIAPI
VirtioMmioSetDeviceStatus (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT8                   DeviceStatus
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueNotify (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                  QueueNotify
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueSel (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                  Sel
  );

EFI_STATUS
VirtioMmioSetQueueAddress (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Address
  );

EFI_STATUS
EFIAPI
VirtioMmioSetQueueAlignment (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Alignment
  );

EFI_STATUS
EFIAPI
VirtioMmioSetPageSize (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  PageSize
  );

EFI_STATUS
EFIAPI
VirtioMmioSetGuestFeatures (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Features
  );

#endif // _VIRTIO_MMIO_DEVICE_INTERNAL_H_
