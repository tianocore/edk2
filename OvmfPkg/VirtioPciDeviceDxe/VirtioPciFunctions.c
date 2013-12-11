/** @file

  This driver produces Virtio Device Protocol instances for Virtio PCI devices.

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
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include "VirtioPciDevice.h"

/**

  Read a word from Region 0 of the device specified by VirtIo Device protocol.

  The function implements the ReadDevice protocol member of
  VIRTIO_DEVICE_PROTOCOL.

  @param[in] This         VirtIo Device protocol.

  @param[in] FieldOffset  Source offset.

  @param[in] FieldSize    Source field size, must be in { 1, 2, 4, 8 }.

  @param[in] BufferSize   Number of bytes available in the target buffer. Must
                          equal FieldSize.

  @param[out] Buffer      Target buffer.


  @return  Status code returned by PciIo->Io.Read().

**/
EFI_STATUS
EFIAPI
VirtioPciDeviceRead (
  IN  VIRTIO_DEVICE_PROTOCOL    *This,
  IN  UINTN                     FieldOffset,
  IN  UINTN                     FieldSize,
  IN  UINTN                     BufferSize,
  OUT VOID                      *Buffer
  )
{
  VIRTIO_PCI_DEVICE         *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoRead (Dev,
      Dev->DeviceSpecificConfigurationOffset + FieldOffset,
      FieldSize, BufferSize, Buffer);
}

/**

  Write a word into Region 0 of the device specified by VirtIo Device protocol.

  @param[in] This         VirtIo Device protocol.

  @param[in] FieldOffset  Destination offset.

  @param[in] FieldSize    Destination field size, must be in { 1, 2, 4, 8 }.

  @param[in] Value        Little endian value to write, converted to UINT64.
                          The least significant FieldSize bytes will be used.


  @return  Status code returned by PciIo->Io.Write().

**/
EFI_STATUS
EFIAPI
VirtioPciDeviceWrite (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  IN UINTN                  FieldOffset,
  IN UINTN                  FieldSize,
  IN UINT64                 Value
  )
{
  VIRTIO_PCI_DEVICE         *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoWrite (Dev,
      Dev->DeviceSpecificConfigurationOffset + FieldOffset, FieldSize, Value);
}

EFI_STATUS
EFIAPI
VirtioPciGetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                *DeviceFeatures
  )
{
  VIRTIO_PCI_DEVICE         *Dev;

  if (DeviceFeatures == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoRead (Dev, VIRTIO_PCI_OFFSET_DEVICE_FEATURES, sizeof (UINT32),
      sizeof (UINT32), DeviceFeatures);
}

EFI_STATUS
EFIAPI
VirtioPciGetQueueAddress (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                 *QueueAddress
  )
{
  VIRTIO_PCI_DEVICE         *Dev;

  if (QueueAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoRead (Dev, VIRTIO_PCI_OFFSET_QUEUE_ADDRESS, sizeof (UINT32),
      sizeof (UINT32), QueueAddress);
}

EFI_STATUS
EFIAPI
VirtioPciGetQueueSize (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT16                  *QueueNumMax
  )
{
  VIRTIO_PCI_DEVICE         *Dev;

  if (QueueNumMax == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoRead (Dev, VIRTIO_PCI_OFFSET_QUEUE_SIZE, sizeof (UINT16),
      sizeof (UINT16), QueueNumMax);
}

EFI_STATUS
EFIAPI
VirtioPciGetDeviceStatus (
  IN  VIRTIO_DEVICE_PROTOCOL  *This,
  OUT UINT8                   *DeviceStatus
  )
{
  VIRTIO_PCI_DEVICE         *Dev;

  if (DeviceStatus == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoRead (Dev, VIRTIO_PCI_OFFSET_QUEUE_DEVICE_STATUS,
      sizeof (UINT8), sizeof (UINT8), DeviceStatus);
}

EFI_STATUS
EFIAPI
VirtioPciSetGuestFeatures (
  IN VIRTIO_DEVICE_PROTOCOL  *This,
  IN UINT32                   Features
  )
{
  VIRTIO_PCI_DEVICE *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoWrite (Dev, VIRTIO_PCI_OFFSET_GUEST_FEATURES,
      sizeof (UINT32), Features);
}

EFI_STATUS
EFIAPI
VirtioPciSetQueueAddress (
  VIRTIO_DEVICE_PROTOCOL    *This,
  UINT32                    Address
  )
{
  VIRTIO_PCI_DEVICE *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoWrite (Dev, VIRTIO_PCI_OFFSET_QUEUE_ADDRESS, sizeof (UINT32),
      Address);
}

EFI_STATUS
EFIAPI
VirtioPciSetQueueSel (
  VIRTIO_DEVICE_PROTOCOL    *This,
  UINT16                    Sel
  )
{
  VIRTIO_PCI_DEVICE *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoWrite (Dev, VIRTIO_PCI_OFFSET_QUEUE_SELECT, sizeof (UINT16),
      Sel);
}

EFI_STATUS
EFIAPI
VirtioPciSetQueueAlignment (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  Alignment
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioPciSetPageSize (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT32                  PageSize
  )
{
  return (PageSize == EFI_PAGE_SIZE) ? EFI_SUCCESS : EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
VirtioPciSetQueueNotify (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                 Index
  )
{
  VIRTIO_PCI_DEVICE *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoWrite (Dev, VIRTIO_PCI_OFFSET_QUEUE_NOTIFY, sizeof (UINT16),
      Index);
}

EFI_STATUS
EFIAPI
VirtioPciSetQueueSize (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT16                 Size
  )
{
  //
  // This function is only applicable in Virtio-MMIO.
  // (The QueueSize field is read-only in Virtio proper (PCI))
  //
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioPciSetDeviceStatus (
  VIRTIO_DEVICE_PROTOCOL *This,
  UINT8                  DeviceStatus
  )
{
  VIRTIO_PCI_DEVICE *Dev;

  Dev = VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE (This);

  return VirtioPciIoWrite (Dev, VIRTIO_PCI_OFFSET_QUEUE_DEVICE_STATUS,
      sizeof (UINT8), DeviceStatus);
}
