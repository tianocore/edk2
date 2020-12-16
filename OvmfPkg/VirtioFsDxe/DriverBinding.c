/** @file
  Provide EFI_SIMPLE_FILE_SYSTEM_PROTOCOL instances on virtio-fs devices.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>                  // AsciiStrCmp()
#include <Library/UefiBootServicesTableLib.h> // gBS
#include <Protocol/ComponentName2.h>          // EFI_COMPONENT_NAME2_PROTOCOL
#include <Protocol/DriverBinding.h>           // EFI_DRIVER_BINDING_PROTOCOL

//
// UEFI Driver Model protocol instances.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL  mDriverBinding;
STATIC EFI_COMPONENT_NAME2_PROTOCOL mComponentName2;

//
// UEFI Driver Model protocol member functions.
//
EFI_STATUS
EFIAPI
VirtioFsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
VirtioFsBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath OPTIONAL
  )
{
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
VirtioFsBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  ControllerHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer OPTIONAL
  )
{
  return EFI_DEVICE_ERROR;
}

EFI_STATUS
EFIAPI
VirtioFsGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  if (AsciiStrCmp (Language, "en") != 0) {
    return EFI_UNSUPPORTED;
  }
  *DriverName = L"Virtio Filesystem Driver";
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioFsGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
VirtioFsEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS Status;

  mDriverBinding.Supported           = VirtioFsBindingSupported;
  mDriverBinding.Start               = VirtioFsBindingStart;
  mDriverBinding.Stop                = VirtioFsBindingStop;
  mDriverBinding.Version             = 0x10;
  mDriverBinding.ImageHandle         = ImageHandle;
  mDriverBinding.DriverBindingHandle = ImageHandle;

  mComponentName2.GetDriverName      = VirtioFsGetDriverName;
  mComponentName2.GetControllerName  = VirtioFsGetControllerName;
  mComponentName2.SupportedLanguages = "en";

  Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
                  &gEfiDriverBindingProtocolGuid, &mDriverBinding,
                  &gEfiComponentName2ProtocolGuid, &mComponentName2, NULL);
  return Status;
}
