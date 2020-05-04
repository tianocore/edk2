/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI Fusion MPT SCSI devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/FusionMptScsi.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciIo.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Uefi/UefiSpec.h>

//
// Higher versions will be used before lower, 0x10-0xffffffef is the version
// range for IVH (Indie Hardware Vendors)
//
#define MPT_SCSI_BINDING_VERSION 0x10

//
// Runtime Structures
//

#define MPT_SCSI_DEV_SIGNATURE SIGNATURE_32 ('M','P','T','S')
typedef struct {
  UINT32                          Signature;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL PassThru;
  EFI_EXT_SCSI_PASS_THRU_MODE     PassThruMode;
  UINT8                           MaxTarget;
} MPT_SCSI_DEV;

#define MPT_SCSI_FROM_PASS_THRU(PassThruPtr) \
  CR (PassThruPtr, MPT_SCSI_DEV, PassThru, MPT_SCSI_DEV_SIGNATURE)

//
// Ext SCSI Pass Thru
//

STATIC
EFI_STATUS
EFIAPI
MptScsiPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN UINT8                                          *Target,
  IN UINT64                                         Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET *Packet,
  IN EFI_EVENT                                      Event     OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

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

STATIC
EFI_STATUS
EFIAPI
MptScsiGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                *This,
  IN OUT UINT8                                      **Target,
  IN OUT UINT64                                     *Lun
  )
{
  MPT_SCSI_DEV *Dev;

  Dev = MPT_SCSI_FROM_PASS_THRU (This);
  //
  // Currently support only LUN 0, so hardcode it
  //
  if (!IsTargetInitialized (*Target)) {
    ZeroMem (*Target, TARGET_MAX_BYTES);
    *Lun = 0;
  } else if (**Target > Dev->MaxTarget || *Lun > 0) {
    return EFI_INVALID_PARAMETER;
  } else if (**Target < Dev->MaxTarget) {
    //
    // This device interface support 256 targets only, so it's enough to
    // increment the LSB of Target, as it will never overflow.
    //
    **Target += 1;
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN OUT UINT8                                     **Target
  )
{
  MPT_SCSI_DEV *Dev;

  Dev = MPT_SCSI_FROM_PASS_THRU (This);
  if (!IsTargetInitialized (*Target)) {
    ZeroMem (*Target, TARGET_MAX_BYTES);
  } else if (**Target > Dev->MaxTarget) {
    return EFI_INVALID_PARAMETER;
  } else if (**Target < Dev->MaxTarget) {
    //
    // This device interface support 256 targets only, so it's enough to
    // increment the LSB of Target, as it will never overflow.
    //
    **Target += 1;
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN UINT8                                         *Target,
  IN UINT64                                        Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                  **DevicePath
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN EFI_DEVICE_PATH_PROTOCOL                      *DevicePath,
  OUT UINT8                                        **Target,
  OUT UINT64                                       *Lun
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
EFIAPI
MptScsiResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL               *This,
  IN UINT8                                         *Target,
  IN UINT64                                        Lun
  )
{
  return EFI_UNSUPPORTED;
}

//
// Driver Binding
//

STATIC
EFI_STATUS
EFIAPI
MptScsiControllerSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
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
      (Pci.Hdr.DeviceId == LSI_53C1030_PCI_DEVICE_ID ||
       Pci.Hdr.DeviceId == LSI_SAS1068_PCI_DEVICE_ID ||
       Pci.Hdr.DeviceId == LSI_SAS1068E_PCI_DEVICE_ID)) {
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

STATIC
EFI_STATUS
EFIAPI
MptScsiControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS           Status;
  MPT_SCSI_DEV         *Dev;

  Dev = AllocateZeroPool (sizeof (*Dev));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = MPT_SCSI_DEV_SIGNATURE;

  Dev->MaxTarget = PcdGet8 (PcdMptScsiMaxTargetLimit);

  //
  // Host adapter channel, doesn't exist
  //
  Dev->PassThruMode.AdapterId = MAX_UINT32;
  Dev->PassThruMode.Attributes =
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_PHYSICAL |
    EFI_EXT_SCSI_PASS_THRU_ATTRIBUTES_LOGICAL;

  Dev->PassThru.Mode = &Dev->PassThruMode;
  Dev->PassThru.PassThru = &MptScsiPassThru;
  Dev->PassThru.GetNextTargetLun = &MptScsiGetNextTargetLun;
  Dev->PassThru.BuildDevicePath = &MptScsiBuildDevicePath;
  Dev->PassThru.GetTargetLun = &MptScsiGetTargetLun;
  Dev->PassThru.ResetChannel = &MptScsiResetChannel;
  Dev->PassThru.ResetTargetLun = &MptScsiResetTargetLun;
  Dev->PassThru.GetNextTarget = &MptScsiGetNextTarget;

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

STATIC
EFI_STATUS
EFIAPI
MptScsiControllerStop (
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN  EFI_HANDLE                            ControllerHandle,
  IN  UINTN                                 NumberOfChildren,
  IN  EFI_HANDLE                            *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  MPT_SCSI_DEV                    *Dev;

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

  Dev = MPT_SCSI_FROM_PASS_THRU (PassThru);

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

STATIC
EFI_DRIVER_BINDING_PROTOCOL mMptScsiDriverBinding = {
  &MptScsiControllerSupported,
  &MptScsiControllerStart,
  &MptScsiControllerStop,
  MPT_SCSI_BINDING_VERSION,
  NULL, // ImageHandle, filled by EfiLibInstallDriverBindingComponentName2
  NULL, // DriverBindingHandle, filled as well
};

//
// Component Name
//

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"LSI Fusion MPT SCSI Driver" },
  { NULL,     NULL                   }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL mComponentName;

EFI_STATUS
EFIAPI
MptScsiGetDriverName (
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

EFI_STATUS
EFIAPI
MptScsiGetDeviceName (
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
EFI_COMPONENT_NAME_PROTOCOL mComponentName = {
  &MptScsiGetDriverName,
  &MptScsiGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL mComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &MptScsiGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &MptScsiGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};

//
// Entry Point
//

EFI_STATUS
EFIAPI
MptScsiEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &mMptScsiDriverBinding,
           ImageHandle, // The handle to install onto
           &mComponentName,
           &mComponentName2
           );
}
