/** @file
  Provide EFI_SIMPLE_FILE_SYSTEM_PROTOCOL instances on virtio-fs devices.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>                  // AsciiStrCmp()
#include <Library/MemoryAllocationLib.h>      // AllocatePool()
#include <Library/UefiBootServicesTableLib.h> // gBS
#include <Protocol/ComponentName2.h>          // EFI_COMPONENT_NAME2_PROTOCOL
#include <Protocol/DriverBinding.h>           // EFI_DRIVER_BINDING_PROTOCOL

#include "VirtioFsDxe.h"

//
// UEFI Driver Model protocol instances.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL   mDriverBinding;
STATIC EFI_COMPONENT_NAME2_PROTOCOL  mComponentName2;

//
// UEFI Driver Model protocol member functions.
//
EFI_STATUS
EFIAPI
VirtioFsBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS              Status;
  VIRTIO_DEVICE_PROTOCOL  *Virtio;
  EFI_STATUS              CloseStatus;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&Virtio,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Virtio->SubSystemDeviceId != VIRTIO_SUBSYSTEM_FILESYSTEM) {
    Status = EFI_UNSUPPORTED;
  }

  CloseStatus = gBS->CloseProtocol (
                       ControllerHandle,
                       &gVirtioDeviceProtocolGuid,
                       This->DriverBindingHandle,
                       ControllerHandle
                       );
  ASSERT_EFI_ERROR (CloseStatus);

  return Status;
}

EFI_STATUS
EFIAPI
VirtioFsBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  VIRTIO_FS   *VirtioFs;
  EFI_STATUS  Status;
  EFI_STATUS  CloseStatus;

  VirtioFs = AllocatePool (sizeof *VirtioFs);
  if (VirtioFs == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VirtioFs->Signature = VIRTIO_FS_SIG;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gVirtioDeviceProtocolGuid,
                  (VOID **)&VirtioFs->Virtio,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeVirtioFs;
  }

  Status = VirtioFsInit (VirtioFs);
  if (EFI_ERROR (Status)) {
    goto CloseVirtio;
  }

  Status = VirtioFsFuseInitSession (VirtioFs);
  if (EFI_ERROR (Status)) {
    goto UninitVirtioFs;
  }

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  VirtioFsExitBoot,
                  VirtioFs,
                  &VirtioFs->ExitBoot
                  );
  if (EFI_ERROR (Status)) {
    goto UninitVirtioFs;
  }

  InitializeListHead (&VirtioFs->OpenFiles);
  VirtioFs->SimpleFs.Revision   = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  VirtioFs->SimpleFs.OpenVolume = VirtioFsOpenVolume;

  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &VirtioFs->SimpleFs
                  );
  if (EFI_ERROR (Status)) {
    goto CloseExitBoot;
  }

  return EFI_SUCCESS;

CloseExitBoot:
  CloseStatus = gBS->CloseEvent (VirtioFs->ExitBoot);
  ASSERT_EFI_ERROR (CloseStatus);

UninitVirtioFs:
  VirtioFsUninit (VirtioFs);

CloseVirtio:
  CloseStatus = gBS->CloseProtocol (
                       ControllerHandle,
                       &gVirtioDeviceProtocolGuid,
                       This->DriverBindingHandle,
                       ControllerHandle
                       );
  ASSERT_EFI_ERROR (CloseStatus);

FreeVirtioFs:
  FreePool (VirtioFs);

  return Status;
}

EFI_STATUS
EFIAPI
VirtioFsBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL  *SimpleFs;
  VIRTIO_FS                        *VirtioFs;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **)&SimpleFs,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  VirtioFs = VIRTIO_FS_FROM_SIMPLE_FS (SimpleFs);

  if (!IsListEmpty (&VirtioFs->OpenFiles)) {
    return EFI_ACCESS_DENIED;
  }

  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  SimpleFs
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseEvent (VirtioFs->ExitBoot);
  ASSERT_EFI_ERROR (Status);

  VirtioFsUninit (VirtioFs);

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gVirtioDeviceProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );
  ASSERT_EFI_ERROR (Status);

  FreePool (VirtioFs);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioFsGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  )
{
  if ((Language == NULL) || (DriverName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (AsciiStrCmp (Language, "en") != 0) {
    return EFI_UNSUPPORTED;
  }

  *DriverName = L"Virtio Filesystem Driver";
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
VirtioFsGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mDriverBinding.Supported           = VirtioFsBindingSupported;
  mDriverBinding.Start               = VirtioFsBindingStart;
  mDriverBinding.Stop                = VirtioFsBindingStop;
  mDriverBinding.Version             = 0x10;
  mDriverBinding.ImageHandle         = ImageHandle;
  mDriverBinding.DriverBindingHandle = ImageHandle;

  mComponentName2.GetDriverName      = VirtioFsGetDriverName;
  mComponentName2.GetControllerName  = VirtioFsGetControllerName;
  mComponentName2.SupportedLanguages = "en";

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &mDriverBinding,
                  &gEfiComponentName2ProtocolGuid,
                  &mComponentName2,
                  NULL
                  );
  return Status;
}
