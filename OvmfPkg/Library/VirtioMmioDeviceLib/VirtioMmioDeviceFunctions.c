/** @file

  This driver produces Virtio Device Protocol instances for Virtio MMIO devices.

  Copyright (C) 2012, Red Hat, Inc.
  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, ARM Ltd.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "VirtioMmioDevice.h"

EFI_STATUS
EFIAPI
VirtioMmioGetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                *DeviceFeatures
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  if (DeviceFeatures == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  *DeviceFeatures = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_HOST_FEATURES);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioGetQueueAddress (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                 *QueueAddress
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  if (QueueAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  *QueueAddress = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_QUEUE_PFN);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioGetQueueSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  if (QueueNumMax == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  *QueueNumMax = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_QUEUE_NUM_MAX) & 0xFFFF;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioGetDeviceStatus (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT8                   *DeviceStatus
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  if (DeviceStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  *DeviceStatus = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_STATUS) & 0xFF;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueSize (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                  QueueSize
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_NUM, QueueSize);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetDeviceStatus (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT8                   DeviceStatus
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_STATUS, DeviceStatus);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueNotify (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                  QueueNotify
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_NOTIFY, QueueNotify);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueAlignment (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Alignment
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_ALIGN, Alignment);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetPageSize (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  PageSize
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  if (PageSize != EFI_PAGE_SIZE) {
    return EFI_UNSUPPORTED;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_GUEST_PAGE_SIZE, PageSize);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueSel (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                  Sel
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_SEL, Sel);

  return EFI_SUCCESS;
}

EFI_STATUS
VirtioMmioSetQueueAddress (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Address
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_PFN, Address);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetGuestFeatures (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Features
  )
{
  VIRTIO_MMIO_DEVICE *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_GUEST_FEATURES, Features);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioDeviceWrite (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINTN                  FieldOffset,
  IN UINTN                  FieldSize,
  IN UINT64                 Value
  )
{
  UINTN                     DstBaseAddress;
  VIRTIO_MMIO_DEVICE       *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  //
  // Double-check fieldsize
  //
  if ((FieldSize != 1) && (FieldSize != 2) &&
      (FieldSize != 4) && (FieldSize != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Compute base address
  //
  DstBaseAddress = Device->BaseAddress +
      VIRTIO_DEVICE_SPECIFIC_CONFIGURATION_OFFSET_MMIO + FieldOffset;

  //
  // The device-specific memory area of Virtio-MMIO can only be written in
  // byte accesses. This is not currently in the Virtio spec.
  //
  MmioWriteBuffer8 (DstBaseAddress, FieldSize, (UINT8*)&Value);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioDeviceRead (
  IN  VIRTIO_DEVICE_PROTOCOL    *This,
  IN  UINTN                     FieldOffset,
  IN  UINTN                     FieldSize,
  IN  UINTN                     BufferSize,
  OUT VOID                      *Buffer
  )
{
  UINTN                     SrcBaseAddress;
  VIRTIO_MMIO_DEVICE       *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  //
  // Parameter validation
  //
  ASSERT (FieldSize == BufferSize);

  //
  // Double-check fieldsize
  //
  if ((FieldSize != 1) && (FieldSize != 2) &&
      (FieldSize != 4) && (FieldSize != 8)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Compute base address
  //
  SrcBaseAddress = Device->BaseAddress +
      VIRTIO_DEVICE_SPECIFIC_CONFIGURATION_OFFSET_MMIO + FieldOffset;

  //
  // The device-specific memory area of Virtio-MMIO can only be read in
  // byte reads. This is not currently in the Virtio spec.
  //
  MmioReadBuffer8 (SrcBaseAddress, BufferSize, Buffer);

  return EFI_SUCCESS;
}
