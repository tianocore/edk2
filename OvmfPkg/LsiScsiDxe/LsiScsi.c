/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI 53C895A SCSI devices.

  Copyright (C) 2020, SUSE LLC.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/LsiScsi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Uefi/UefiSpec.h>

#include "LsiScsi.h"

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
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN EFI_EVENT                                      Event     OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LsiScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN OUT UINT8                       **TargetPointer,
  IN OUT UINT64                      *Lun
  )
{
  LSI_SCSI_DEV *Dev;
  UINTN        Idx;
  UINT8        *Target;
  UINT16       LastTarget;

  //
  // the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  //
  Target = *TargetPointer;

  //
  // Search for first non-0xFF byte. If not found, return first target & LUN.
  //
  for (Idx = 0; Idx < TARGET_MAX_BYTES && Target[Idx] == 0xFF; ++Idx)
    ;
  if (Idx == TARGET_MAX_BYTES) {
    SetMem (Target, TARGET_MAX_BYTES, 0x00);
    *Lun = 0;
    return EFI_SUCCESS;
  }

  CopyMem (&LastTarget, Target, sizeof LastTarget);

  //
  // increment (target, LUN) pair if valid on input
  //
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget || *Lun > Dev->MaxLun) {
    return EFI_INVALID_PARAMETER;
  }

  if (*Lun < Dev->MaxLun) {
    ++*Lun;
    return EFI_SUCCESS;
  }

  if (LastTarget < Dev->MaxTarget) {
    *Lun = 0;
    ++LastTarget;
    CopyMem (Target, &LastTarget, sizeof LastTarget);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
EFIAPI
LsiScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN UINT8                           *Target,
  IN UINT64                          Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  )
{
  UINT16           TargetValue;
  LSI_SCSI_DEV     *Dev;
  SCSI_DEVICE_PATH *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CopyMem (&TargetValue, Target, sizeof TargetValue);
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if (TargetValue > Dev->MaxTarget || Lun > Dev->MaxLun || Lun > 0xFFFF) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocatePool (sizeof *ScsiDevicePath);
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)  sizeof *ScsiDevicePath;
  ScsiDevicePath->Header.Length[1] = (UINT8) (sizeof *ScsiDevicePath >> 8);
  ScsiDevicePath->Pun              = TargetValue;
  ScsiDevicePath->Lun              = (UINT16) Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LsiScsiGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN  EFI_DEVICE_PATH_PROTOCOL        *DevicePath,
  OUT UINT8                           **TargetPointer,
  OUT UINT64                          *Lun
  )
{
  SCSI_DEVICE_PATH *ScsiDevicePath;
  LSI_SCSI_DEV     *Dev;
  UINT8            *Target;

  if (DevicePath == NULL || TargetPointer == NULL || *TargetPointer == NULL ||
      Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type    != MESSAGING_DEVICE_PATH ||
      DevicePath->SubType != MSG_SCSI_DP) {
    return EFI_UNSUPPORTED;
  }

  ScsiDevicePath = (SCSI_DEVICE_PATH *) DevicePath;
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if (ScsiDevicePath->Pun > Dev->MaxTarget ||
      ScsiDevicePath->Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  Target = *TargetPointer;
  ZeroMem (Target, TARGET_MAX_BYTES);
  CopyMem (Target, &ScsiDevicePath->Pun, sizeof ScsiDevicePath->Pun);
  *Lun = ScsiDevicePath->Lun;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LsiScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LsiScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN UINT8                           *Target,
  IN UINT64                          Lun
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LsiScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL *This,
  IN OUT UINT8                       **TargetPointer
  )
{
  LSI_SCSI_DEV *Dev;
  UINTN        Idx;
  UINT8        *Target;
  UINT16       LastTarget;

  //
  // the TargetPointer input parameter is unnecessarily a pointer-to-pointer
  //
  Target = *TargetPointer;

  //
  // Search for first non-0xFF byte. If not found, return first target.
  //
  for (Idx = 0; Idx < TARGET_MAX_BYTES && Target[Idx] == 0xFF; ++Idx)
    ;
  if (Idx == TARGET_MAX_BYTES) {
    SetMem (Target, TARGET_MAX_BYTES, 0x00);
    return EFI_SUCCESS;
  }

  CopyMem (&LastTarget, Target, sizeof LastTarget);

  //
  // increment target if valid on input
  //
  Dev = LSI_SCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  }

  if (LastTarget < Dev->MaxTarget) {
    ++LastTarget;
    CopyMem (Target, &LastTarget, sizeof LastTarget);
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

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
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint32,
                        0,
                        sizeof (Pci) / sizeof (UINT32),
                        &Pci
                        );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  if (Pci.Hdr.VendorId == LSI_LOGIC_PCI_VENDOR_ID &&
      Pci.Hdr.DeviceId == LSI_53C895A_PCI_DEVICE_ID) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

Done:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

EFI_STATUS
EFIAPI
LsiScsiControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS           Status;
  LSI_SCSI_DEV         *Dev;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = LSI_SCSI_DEV_SIGNATURE;

  STATIC_ASSERT (
    FixedPcdGet8 (PcdLsiScsiMaxTargetLimit) < 8,
    "LSI 53C895A supports targets [0..7]"
    );
  STATIC_ASSERT (
    FixedPcdGet8 (PcdLsiScsiMaxLunLimit) < 128,
    "LSI 53C895A supports LUNs [0..127]"
    );
  Dev->MaxTarget = PcdGet8 (PcdLsiScsiMaxTargetLimit);
  Dev->MaxLun = PcdGet8 (PcdLsiScsiMaxLunLimit);

  //
  // Host adapter channel, doesn't exist
  //
  Dev->PassThruMode.AdapterId = MAX_UINT32;
  Dev->PassThruMode.Attributes =
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  Dev->PassThru.Mode = &Dev->PassThruMode;
  Dev->PassThru.PassThru = &LsiScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &LsiScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath = &LsiScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun = &LsiScsiGetTargetLun;
  Dev->PassThru.ResetChannel = &LsiScsiResetChannel;
  Dev->PassThru.ResetTargetLun = &LsiScsiResetTargetLun;
  Dev->PassThru.GetNextTarget = &LsiScsiGetNextTarget;

  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    goto FreePool;
  }

  return EFI_SUCCESS;

FreePool:
  FreePool (Dev);

  return Status;
}

EFI_STATUS
EFIAPI
LsiScsiControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  LSI_SCSI_DEV                    *Dev;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  (VOID **)&PassThru,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // Lookup only
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = LSI_SCSI_FROM_PASS_THRU (PassThru);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FreePool (Dev);

  return Status;
}

//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC
EFI_DRIVER_BINDING_PROTOCOL gDriverBinding = {
  &LsiScsiControllerSupported,
  &LsiScsiControllerStart,
  &LsiScsiControllerStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in LsiScsiEntryPoint()
  NULL  // DriverBindingHandle, ditto
};


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

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"LSI 53C895A SCSI Controller Driver" },
  { NULL,     NULL                                  }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL gComponentName;

EFI_STATUS
EFIAPI
LsiScsiGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

EFI_STATUS
EFIAPI
LsiScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL gComponentName = {
  &LsiScsiGetDriverName,
  &LsiScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &LsiScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &LsiScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry point of this driver
//
EFI_STATUS
EFIAPI
LsiScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle, // The handle to install onto
           &gComponentName,
           &gComponentName2
           );
}
