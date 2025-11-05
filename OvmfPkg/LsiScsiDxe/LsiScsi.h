/** @file

  Internal definitions for the LSI 53C895A SCSI driver, which produces
  Extended SCSI Pass Thru Protocol instances for LSI 53C895A SCSI devices.

  Copyright (C) 2020, SUSE LLC.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LSI_SCSI_DXE_H_
#define _LSI_SCSI_DXE_H_

typedef struct {
  //
  // Allocate 32 UINT32 entries for the script and it's sufficient for
  // 16 instructions.
  //
  UINT32    Script[32];
  //
  // The max size of CDB is 32.
  //
  UINT8     Cdb[32];
  //
  // Allocate 64KB for read/write buffer. It seems sufficient for the common
  // boot scenarios.
  //
  // NOTE: The number of bytes for data transmission is bounded by DMA Byte
  //       Count (DBC), a 24-bit register, so the maximum is 0xFFFFFF (16MB-1).
  //
  UINT8     Data[SIZE_64KB];
  //
  // For SCSI Message In phase
  //
  UINT8     MsgIn[2];
  //
  // For SCSI Message Out phase
  //
  UINT8     MsgOut;
  //
  // For SCSI Status phase
  //
  UINT8     Status;
} LSI_SCSI_DMA_BUFFER;

typedef struct {
  UINT32                             Signature;
  UINT64                             OrigPciAttrs;
  EFI_EVENT                          ExitBoot;
  EFI_PCI_IO_PROTOCOL                *PciIo;
  UINT8                              MaxTarget;
  UINT8                              MaxLun;
  UINT32                             StallPerPollUsec;
  LSI_SCSI_DMA_BUFFER                *Dma;
  EFI_PHYSICAL_ADDRESS               DmaPhysical;
  VOID                               *DmaMapping;
  EFI_EXT_SCSI_PASS_THRU_MODE        PassThruMode;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    PassThru;
} LSI_SCSI_DEV;

#define LSI_SCSI_DEV_SIGNATURE  SIGNATURE_32 ('L','S','I','S')

#define LSI_SCSI_FROM_PASS_THRU(PassThruPtr) \
  CR (PassThruPtr, LSI_SCSI_DEV, PassThru, LSI_SCSI_DEV_SIGNATURE)

#define LSI_SCSI_DMA_ADDR(Dev, MemberName) \
  ((UINT32)(Dev->DmaPhysical + OFFSET_OF (LSI_SCSI_DMA_BUFFER, MemberName)))

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
LsiScsiControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
LsiScsiControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

EFI_STATUS
EFIAPI
LsiScsiControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

//
// The next seven functions implement EFI_EXT_SCSI_PASS_THRU_PROTOCOL
// for the LSI 53C895A SCSI Controller. Refer to UEFI Spec 2.3.1 + Errata C,
// sections
// - 14.1 SCSI Driver Model Overview,
// - 14.7 Extended SCSI Pass Thru Protocol.
//

EFI_STATUS
EFIAPI
LsiScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                 *This,
  IN UINT8                                           *Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN EFI_EVENT                                       Event     OPTIONAL
  );

EFI_STATUS
EFIAPI
LsiScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **TargetPointer,
  IN OUT UINT64                       *Lun
  );

EFI_STATUS
EFIAPI
LsiScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePath
  );

EFI_STATUS
EFIAPI
LsiScsiGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                            **TargetPointer,
  OUT UINT64                           *Lun
  );

EFI_STATUS
EFIAPI
LsiScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  );

EFI_STATUS
EFIAPI
LsiScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  );

EFI_STATUS
EFIAPI
LsiScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **TargetPointer
  );

//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
// Device type names ("LSI 53C895A SCSI Controller") are not formatted because
// the driver supports only that device type. Therefore the driver name
// suffices for unambiguous identification.
//

EFI_STATUS
EFIAPI
LsiScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
LsiScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

#endif // _LSI_SCSI_DXE_H_
