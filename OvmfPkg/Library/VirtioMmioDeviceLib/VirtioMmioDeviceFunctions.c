/** @file

  This driver produces Virtio Device Protocol instances for Virtio MMIO devices.

  Copyright (C) 2012, Red Hat, Inc.
  Copyright (c) 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (C) 2013, ARM Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VirtioMmioDevice.h"

EFI_STATUS
EFIAPI
VirtioMmioGetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT64                 *DeviceFeatures
  )
{
  VIRTIO_MMIO_DEVICE  *Device;
  UINT32              LowBits, HighBits;

  if (DeviceFeatures == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    *DeviceFeatures = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_HOST_FEATURES);
  } else {
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_HOST_FEATURES_SEL, 0);
    LowBits = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_HOST_FEATURES);
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_HOST_FEATURES_SEL, 1);
    HighBits        = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_HOST_FEATURES);
    *DeviceFeatures = LShiftU64 (HighBits, 32) | LowBits;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioGetQueueSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

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
  VIRTIO_MMIO_DEVICE  *Device;

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
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                  QueueSize
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_NUM, QueueSize);
  } else {
    Device->QueueNum = QueueSize;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetDeviceStatus (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT8                   DeviceStatus
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_STATUS, DeviceStatus);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueNotify (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                  QueueNotify
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_NOTIFY, QueueNotify);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueAlignment (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                  Alignment
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_ALIGN, Alignment);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetPageSize (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                  PageSize
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  if (PageSize != EFI_PAGE_SIZE) {
    return EFI_UNSUPPORTED;
  }

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_GUEST_PAGE_SIZE, PageSize);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueSel (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT16                  Sel
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_SEL, Sel);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    Device->QueueNum = VIRTIO_CFG_READ (Device, VIRTIO_MMIO_OFFSET_QUEUE_NUM_MAX) & 0xFFFF;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetQueueAddress (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN VRING                   *Ring,
  IN UINT64                  RingBaseShift
  )
{
  VIRTIO_MMIO_DEVICE  *Device;
  UINT64              Address;

  ASSERT (RingBaseShift == 0);

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_PFN,
      (UINT32)((UINTN)Ring->Base >> EFI_PAGE_SHIFT)
      );
  } else {
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_NUM, Device->QueueNum);

    Address = (UINTN)Ring->Base;
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_DESC_LO,
      (UINT32)Address
      );
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_DESC_HI,
      (UINT32)RShiftU64 (Address, 32)
      );

    Address = (UINTN)Ring->Avail.Flags;
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_AVAIL_LO,
      (UINT32)Address
      );
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_AVAIL_HI,
      (UINT32)RShiftU64 (Address, 32)
      );

    Address = (UINTN)Ring->Used.Flags;
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_USED_LO,
      (UINT32)Address
      );
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_QUEUE_USED_HI,
      (UINT32)RShiftU64 (Address, 32)
      );

    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_QUEUE_READY, 1);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioSetGuestFeatures (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT64                  Features
  )
{
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  if (Device->Version == VIRTIO_MMIO_DEVICE_VERSION_0_95) {
    if (Features > MAX_UINT32) {
      return EFI_UNSUPPORTED;
    }

    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_GUEST_FEATURES,
      (UINT32)Features
      );
  } else {
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_GUEST_FEATURES_SEL, 0);
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_GUEST_FEATURES,
      (UINT32)Features
      );
    VIRTIO_CFG_WRITE (Device, VIRTIO_MMIO_OFFSET_GUEST_FEATURES_SEL, 1);
    VIRTIO_CFG_WRITE (
      Device,
      VIRTIO_MMIO_OFFSET_GUEST_FEATURES,
      (UINT32)RShiftU64 (Features, 32)
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioDeviceWrite (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINTN                   FieldOffset,
  IN UINTN                   FieldSize,
  IN UINT64                  Value
  )
{
  UINTN               DstBaseAddress;
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  //
  // Double-check fieldsize
  //
  if ((FieldSize != 1) && (FieldSize != 2) &&
      (FieldSize != 4) && (FieldSize != 8))
  {
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
  MmioWriteBuffer8 (DstBaseAddress, FieldSize, (UINT8 *)&Value);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioDeviceRead (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   FieldOffset,
  IN  UINTN                   FieldSize,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  )
{
  UINTN               SrcBaseAddress;
  VIRTIO_MMIO_DEVICE  *Device;

  Device = VIRTIO_MMIO_DEVICE_FROM_VIRTIO_DEVICE (This);

  //
  // Parameter validation
  //
  ASSERT (FieldSize == BufferSize);

  //
  // Double-check fieldsize
  //
  if ((FieldSize != 1) && (FieldSize != 2) &&
      (FieldSize != 4) && (FieldSize != 8))
  {
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

EFI_STATUS
EFIAPI
VirtioMmioAllocateSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   NumPages,
  OUT VOID                    **HostAddress
  )
{
  VOID  *Buffer;

  Buffer = AllocatePages (NumPages);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *HostAddress = Buffer;
  return EFI_SUCCESS;
}

VOID
EFIAPI
VirtioMmioFreeSharedPages (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  IN  UINTN                   NumPages,
  IN  VOID                    *HostAddress
  )
{
  FreePages (HostAddress, NumPages);
}

EFI_STATUS
EFIAPI
VirtioMmioMapSharedBuffer (
  IN      VIRTIO_DEVICE_PROTOCOL  *This,
  IN      VIRTIO_MAP_OPERATION    Operation,
  IN      VOID                    *HostAddress,
  IN OUT  UINTN                   *NumberOfBytes,
  OUT     EFI_PHYSICAL_ADDRESS    *DeviceAddress,
  OUT     VOID                    **Mapping
  )
{
  *DeviceAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
  *Mapping       = NULL;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioMmioUnmapSharedBuffer (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN VOID                    *Mapping
  )
{
  return EFI_SUCCESS;
}
