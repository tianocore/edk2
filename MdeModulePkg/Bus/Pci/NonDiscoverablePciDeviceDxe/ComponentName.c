/** @file

  Copyright (C) 2016, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "NonDiscoverablePciDeviceIo.h"

//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"PCI I/O protocol emulation driver for non-discoverable devices" },
  { NULL,     NULL                   }
};

EFI_COMPONENT_NAME_PROTOCOL gComponentName;

STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciGetDriverName (
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

STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

EFI_COMPONENT_NAME_PROTOCOL gComponentName = {
  &NonDiscoverablePciGetDriverName,
  &NonDiscoverablePciGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

EFI_COMPONENT_NAME2_PROTOCOL gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &NonDiscoverablePciGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &NonDiscoverablePciGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};
