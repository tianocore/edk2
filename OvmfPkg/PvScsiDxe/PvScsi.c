/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  pvscsi devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/PvScsi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Uefi/UefiSpec.h>

#include "PvScsi.h"

//
// Higher versions will be used before lower, 0x10-0xffffffef is the version
// range for IHV (Indie Hardware Vendors)
//
#define PVSCSI_BINDING_VERSION      0x10

//
// Ext SCSI Pass Thru utilities
//

/**
  Check if Target argument to EXT_SCSI_PASS_THRU.GetNextTarget() and
  EXT_SCSI_PASS_THRU.GetNextTargetLun() is initialized
**/
STATIC
BOOLEAN
IsTargetInitialized (
  IN UINT8                                          *Target
  )
{
  UINTN Idx;

  for (Idx = 0; Idx < TARGET_MAX_BYTES; ++Idx) {
    if (Target[Idx] != 0xFF) {
      return TRUE;
    }
  }
  return FALSE;
}

//
// Ext SCSI Pass Thru
//

STATIC
EFI_STATUS
EFIAPI
PvScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN EFI_EVENT                                      Event    OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target,
  IN OUT UINT64                                     *Lun
  )
{
  UINT8      *TargetPtr;
  UINT8      LastTarget;
  PVSCSI_DEV *Dev;

  if (Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The Target input parameter is unnecessarily a pointer-to-pointer
  //
  TargetPtr = *Target;

  //
  // If target not initialized, return first target & LUN
  //
  if (!IsTargetInitialized (TargetPtr)) {
    ZeroMem (TargetPtr, TARGET_MAX_BYTES);
    *Lun = 0;
    return EFI_SUCCESS;
  }

  //
  // We only use first byte of target identifer
  //
  LastTarget = *TargetPtr;

  //
  // Increment (target, LUN) pair if valid on input
  //
  Dev = PVSCSI_FROM_PASS_THRU (This);
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
    *TargetPtr = LastTarget;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                   **DevicePath
  )
{
  UINT8             TargetValue;
  PVSCSI_DEV        *Dev;
  SCSI_DEVICE_PATH  *ScsiDevicePath;

  if (DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // We only use first byte of target identifer
  //
  TargetValue = *Target;

  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (TargetValue > Dev->MaxTarget || Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  ScsiDevicePath = AllocatePool (sizeof (*ScsiDevicePath));
  if (ScsiDevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ScsiDevicePath->Header.Type      = MESSAGING_DEVICE_PATH;
  ScsiDevicePath->Header.SubType   = MSG_SCSI_DP;
  ScsiDevicePath->Header.Length[0] = (UINT8)sizeof (*ScsiDevicePath);
  ScsiDevicePath->Header.Length[1] = (UINT8)(sizeof (*ScsiDevicePath) >> 8);
  ScsiDevicePath->Pun              = TargetValue;
  ScsiDevicePath->Lun              = (UINT16)Lun;

  *DevicePath = &ScsiDevicePath->Header;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN EFI_DEVICE_PATH_PROTOCOL                       *DevicePath,
  OUT UINT8                                         **Target,
  OUT UINT64                                        *Lun
  )
{
  SCSI_DEVICE_PATH *ScsiDevicePath;
  PVSCSI_DEV       *Dev;

  if (DevicePath == NULL || Target == NULL || *Target == NULL || Lun == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (DevicePath->Type    != MESSAGING_DEVICE_PATH ||
      DevicePath->SubType != MSG_SCSI_DP) {
    return EFI_UNSUPPORTED;
  }

  ScsiDevicePath = (SCSI_DEVICE_PATH *)DevicePath;
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (ScsiDevicePath->Pun > Dev->MaxTarget ||
      ScsiDevicePath->Lun > Dev->MaxLun) {
    return EFI_NOT_FOUND;
  }

  //
  // We only use first byte of target identifer
  //
  **Target = (UINT8)ScsiDevicePath->Pun;
  ZeroMem (*Target + 1, TARGET_MAX_BYTES - 1);
  *Lun = ScsiDevicePath->Lun;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target
  )
{
  UINT8      *TargetPtr;
  UINT8      LastTarget;
  PVSCSI_DEV *Dev;

  if (Target == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The Target input parameter is unnecessarily a pointer-to-pointer
  //
  TargetPtr = *Target;

  //
  // If target not initialized, return first target
  //
  if (!IsTargetInitialized (TargetPtr)) {
    ZeroMem (TargetPtr, TARGET_MAX_BYTES);
    return EFI_SUCCESS;
  }

  //
  // We only use first byte of target identifer
  //
  LastTarget = *TargetPtr;

  //
  // Increment target if valid on input
  //
  Dev = PVSCSI_FROM_PASS_THRU (This);
  if (LastTarget > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  }

  if (LastTarget < Dev->MaxTarget) {
    ++LastTarget;
    *TargetPtr = LastTarget;
    return EFI_SUCCESS;
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
PvScsiSetPciAttributes (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  //
  // Backup original PCI Attributes
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationGet,
                         0,
                         &Dev->OriginalPciAttributes
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Enable MMIO-Space & Bus-Mastering
  //
  Status = Dev->PciIo->Attributes (
                         Dev->PciIo,
                         EfiPciIoAttributeOperationEnable,
                         (EFI_PCI_IO_ATTRIBUTE_MEMORY |
                          EFI_PCI_IO_ATTRIBUTE_BUS_MASTER),
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
PvScsiRestorePciAttributes (
  IN PVSCSI_DEV *Dev
  )
{
  Dev->PciIo->Attributes (
                Dev->PciIo,
                EfiPciIoAttributeOperationSet,
                Dev->OriginalPciAttributes,
                NULL
                );
}

STATIC
EFI_STATUS
PvScsiInit (
  IN OUT PVSCSI_DEV *Dev
  )
{
  EFI_STATUS Status;

  //
  // Init configuration
  //
  Dev->MaxTarget = PcdGet8 (PcdPvScsiMaxTargetLimit);
  Dev->MaxLun = PcdGet8 (PcdPvScsiMaxLunLimit);

  //
  // Set PCI Attributes
  //
  Status = PvScsiSetPciAttributes (Dev);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Populate the exported interface's attributes
  //
  Dev->PassThru.Mode             = &Dev->PassThruMode;
  Dev->PassThru.PassThru         = &PvScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &PvScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath  = &PvScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun     = &PvScsiGetTargetLun;
  Dev->PassThru.ResetChannel     = &PvScsiResetChannel;
  Dev->PassThru.ResetTargetLun   = &PvScsiResetTargetLun;
  Dev->PassThru.GetNextTarget    = &PvScsiGetNextTarget;

  //
  // AdapterId is a target for which no handle will be created during bus scan.
  // Prevent any conflict with real devices.
  //
  Dev->PassThruMode.AdapterId = MAX_UINT32;

  //
  // Set both physical and logical attributes for non-RAID SCSI channel
  //
  Dev->PassThruMode.Attributes = EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
                                 EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  //
  // No restriction on transfer buffer alignment
  //
  Dev->PassThruMode.IoAlign = 0;

  return EFI_SUCCESS;
}

STATIC
VOID
PvScsiUninit (
  IN OUT PVSCSI_DEV *Dev
  )
{
  PvScsiRestorePciAttributes (Dev);
}

//
// Driver Binding
//

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingSupported (
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

  if ((Pci.Hdr.VendorId != PCI_VENDOR_ID_VMWARE) ||
      (Pci.Hdr.DeviceId != PCI_DEVICE_ID_VMWARE_PVSCSI)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = EFI_SUCCESS;

Done:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  PVSCSI_DEV *Dev;
  EFI_STATUS Status;

  Dev = (PVSCSI_DEV *) AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&Dev->PciIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreePvScsi;
  }

  Status = PvScsiInit (Dev);
  if (EFI_ERROR (Status)) {
    goto ClosePciIo;
  }

  //
  // Setup complete, attempt to export the driver instance's PassThru interface
  //
  Dev->Signature = PVSCSI_SIG;
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  return EFI_SUCCESS;

UninitDev:
  PvScsiUninit (Dev);

ClosePciIo:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

FreePvScsi:
  FreePool (Dev);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
PvScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  PVSCSI_DEV                      *Dev;

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

  Dev = PVSCSI_FROM_PASS_THRU (PassThru);

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiExtScsiPassThruProtocolGuid,
                  &Dev->PassThru
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PvScsiUninit (Dev);

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  FreePool (Dev);

  return EFI_SUCCESS;
}

STATIC EFI_DRIVER_BINDING_PROTOCOL mPvScsiDriverBinding = {
  &PvScsiDriverBindingSupported,
  &PvScsiDriverBindingStart,
  &PvScsiDriverBindingStop,
  PVSCSI_BINDING_VERSION,
  NULL, // ImageHandle, filled by EfiLibInstallDriverBindingComponentName2()
  NULL  // DriverBindingHandle, filled as well
};

//
// Component Name
//

STATIC EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"PVSCSI Host Driver" },
  { NULL,     NULL                  }
};

STATIC EFI_COMPONENT_NAME_PROTOCOL mComponentName;

STATIC
EFI_STATUS
EFIAPI
PvScsiGetDriverName (
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
           (BOOLEAN)(This == &mComponentName) // Iso639Language
           );
}

STATIC
EFI_STATUS
EFIAPI
PvScsiGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC EFI_COMPONENT_NAME_PROTOCOL mComponentName = {
  &PvScsiGetDriverName,
  &PvScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC EFI_COMPONENT_NAME2_PROTOCOL mComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &PvScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &PvScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry Point
//

EFI_STATUS
EFIAPI
PvScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mPvScsiDriverBinding,
           ImageHandle,
           &mComponentName,
           &mComponentName2
           );
}
