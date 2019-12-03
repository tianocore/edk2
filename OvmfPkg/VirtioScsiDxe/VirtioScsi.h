/** @file

  Internal definitions for the virtio-scsi driver, which produces Extended SCSI
  Pass Thru Protocol instances for virtio-scsi devices.

  Copyright (C) 2012, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_SCSI_DXE_H_
#define _VIRTIO_SCSI_DXE_H_

#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/ScsiPassThruExt.h>

#include <IndustryStandard/Virtio.h>


//
// This driver supports 2-byte target identifiers and 4-byte LUN identifiers.
//
// EFI_EXT_SCSI_PASS_THRU_PROTOCOL provides TARGET_MAX_BYTES bytes for target
// identification, and 8 bytes for LUN identification.
//
// EFI_EXT_SCSI_PASS_THRU_MODE.AdapterId is also a target identifier,
// consisting of 4 bytes. Make sure TARGET_MAX_BYTES can accommodate both
// AdapterId and our target identifiers.
//
#if TARGET_MAX_BYTES < 4
#  error "virtio-scsi requires TARGET_MAX_BYTES >= 4"
#endif


#define VSCSI_SIG SIGNATURE_32 ('V', 'S', 'C', 'S')

typedef struct {
  //
  // Parts of this structure are initialized / torn down in various functions
  // at various call depths. The table to the right should make it easier to
  // track them.
  //
  //                              field              init function       init depth
  //                              ----------------   ------------------  ----------
  UINT32                          Signature;      // DriverBindingStart  0
  VIRTIO_DEVICE_PROTOCOL          *VirtIo;        // DriverBindingStart  0
  EFI_EVENT                       ExitBoot;       // DriverBindingStart  0
  BOOLEAN                         InOutSupported; // VirtioScsiInit      1
  UINT16                          MaxTarget;      // VirtioScsiInit      1
  UINT32                          MaxLun;         // VirtioScsiInit      1
  UINT32                          MaxSectors;     // VirtioScsiInit      1
  VRING                           Ring;           // VirtioRingInit      2
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL PassThru;       // VirtioScsiInit      1
  EFI_EXT_SCSI_PASS_THRU_MODE     PassThruMode;   // VirtioScsiInit      1
  VOID                            *RingMap;       // VirtioRingMap       2
} VSCSI_DEV;

#define VIRTIO_SCSI_FROM_PASS_THRU(PassThruPointer) \
        CR (PassThruPointer, VSCSI_DEV, PassThru, VSCSI_SIG)


//
// Probe, start and stop functions of this driver, called by the DXE core for
// specific devices.
//
// The following specifications document these interfaces:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01, 9 Driver Binding Protocol
// - UEFI Spec 2.3.1 + Errata C, 10.1 EFI Driver Binding Protocol
//

EFI_STATUS
EFIAPI
VirtioScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );


EFI_STATUS
EFIAPI
VirtioScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  );


EFI_STATUS
EFIAPI
VirtioScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  );


//
// The next seven functions implement EFI_EXT_SCSI_PASS_THRU_PROTOCOL
// for the virtio-scsi HBA. Refer to UEFI Spec 2.3.1 + Errata C, sections
// - 14.1 SCSI Driver Model Overview,
// - 14.7 Extended SCSI Pass Thru Protocol.
//

EFI_STATUS
EFIAPI
VirtioScsiPassThru (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL            *This,
  IN     UINT8                                      *Target,
  IN     UINT64                                     Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN     EFI_EVENT                                  Event   OPTIONAL
  );


EFI_STATUS
EFIAPI
VirtioScsiGetNextTargetLun (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN OUT UINT8                           **Target,
  IN OUT UINT64                          *Lun
  );


EFI_STATUS
EFIAPI
VirtioScsiBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN     UINT8                           *Target,
  IN     UINT64                          Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL        **DevicePath
  );


EFI_STATUS
EFIAPI
VirtioScsiGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  OUT UINT8                           **Target,
  OUT UINT64                          *Lun
  );


EFI_STATUS
EFIAPI
VirtioScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This
  );


EFI_STATUS
EFIAPI
VirtioScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN UINT8                           *Target,
  IN UINT64                          Lun
  );


EFI_STATUS
EFIAPI
VirtioScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN OUT UINT8                       **Target
  );


//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
// Device type names ("Virtio SCSI Host Device") are not formatted because the
// driver supports only that device type. Therefore the driver name suffices
// for unambiguous identification.
//

EFI_STATUS
EFIAPI
VirtioScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  );


EFI_STATUS
EFIAPI
VirtioScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  );

#endif // _VIRTIO_SCSI_DXE_H_
