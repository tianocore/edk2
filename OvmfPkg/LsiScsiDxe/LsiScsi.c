/** @file

  This driver produces Extended SCSI Pass Thru Protocol instances for
  LSI 53C895A SCSI devices.

  Copyright (C) 2020, SUSE LLC.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiLib.h>
#include <Uefi/UefiSpec.h>

#include "LsiScsi.h"

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
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
LsiScsiControllerStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  return EFI_SUCCESS;
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
  return EFI_SUCCESS;
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
