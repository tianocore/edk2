/** @file

  Internal definitions for the VirtIo PCI Device driver

  Copyright (C) 2013, ARM Ltd

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_PCI_DEVICE_DXE_H_
#define _VIRTIO_PCI_DEVICE_DXE_H_

#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/PciIo.h>
#include <Protocol/VirtioDevice.h>

#include <IndustryStandard/Virtio.h>

#define VIRTIO_PCI_DEVICE_SIGNATURE   SIGNATURE_32 ('V', 'P', 'C', 'I')

typedef struct {
  UINT32                 Signature;
  VIRTIO_DEVICE_PROTOCOL VirtioDevice;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  UINT64                 OriginalPciAttributes;
  UINT32                 DeviceSpecificConfigurationOffset;
} VIRTIO_PCI_DEVICE;

#define VIRTIO_PCI_DEVICE_FROM_VIRTIO_DEVICE(Device) \
    CR (Device, VIRTIO_PCI_DEVICE, VirtioDevice, VIRTIO_PCI_DEVICE_SIGNATURE)


EFI_STATUS
EFIAPI
VirtioPciIoRead (
  IN  VIRTIO_PCI_DEVICE         *Dev,
  IN  UINTN                      FieldOffset,
  IN  UINTN                      FieldSize,
  IN  UINTN                      BufferSize,
  OUT VOID                       *Buffer
  );

EFI_STATUS
EFIAPI
VirtioPciIoWrite (
  IN  VIRTIO_PCI_DEVICE         *Dev,
  IN UINTN                       FieldOffset,
  IN UINTN                       FieldSize,
  IN UINT64                      Value
  );

/********************************************
 * PCI Functions for VIRTIO_DEVICE_PROTOCOL
 *******************************************/
EFI_STATUS
EFIAPI
VirtioPciDeviceRead (
  IN  VIRTIO_DEVICE_PROTOCOL     *This,
  IN  UINTN                      FieldOffset,
  IN  UINTN                      FieldSize,
  IN  UINTN                      BufferSize,
  OUT VOID                       *Buffer
  );

EFI_STATUS
EFIAPI
VirtioPciDeviceWrite (
  IN VIRTIO_DEVICE_PROTOCOL      *This,
  IN UINTN                       FieldOffset,
  IN UINTN                       FieldSize,
  IN UINT64                      Value
  );

EFI_STATUS
EFIAPI
VirtioPciGetDeviceFeatures (
  IN VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                *DeviceFeatures
  );

EFI_STATUS
EFIAPI
VirtioPciGetQueueAddress (
  IN  VIRTIO_DEVICE_PROTOCOL *This,
  OUT UINT32                 *QueueAddress
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
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT32                         Alignment
  );

EFI_STATUS
EFIAPI
VirtioPciSetPageSize (
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT32                         PageSize
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
  IN UINT32                   Features
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueAddress (
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT32                         Address
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueSel (
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT16                         Sel
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueNotify (
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT16                         Index
  );

EFI_STATUS
EFIAPI
VirtioPciSetQueueSize (
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT16                         Size
  );

EFI_STATUS
EFIAPI
VirtioPciSetDeviceStatus (
  VIRTIO_DEVICE_PROTOCOL         *This,
  UINT8                          DeviceStatus
  );

#endif // _VIRTIO_PCI_DEVICE_DXE_H_
