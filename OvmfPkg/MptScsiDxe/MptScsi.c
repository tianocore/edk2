/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI Fusion MPT SCSI devices.

  Copyright (C) 2020, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Uefi/UefiSpec.h>

//
// Higher versions will be used before lower, 0x10-0xffffffef is the version
// range for IVH (Indie Hardware Vendors)
//
#define MPT_SCSI_BINDING_VERSION 0x10

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
  return EFI_UNSUPPORTED;
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
  return EFI_UNSUPPORTED;
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
  return EFI_UNSUPPORTED;
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
