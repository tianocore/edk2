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

#include <Protocol/DriverBinding.h>

//
// We only support the following device types
//
STATIC
CONST EFI_GUID * CONST
SupportedNonDiscoverableDevices [] = {
  &gEdkiiNonDiscoverableAhciDeviceGuid,
  &gEdkiiNonDiscoverableEhciDeviceGuid,
  &gEdkiiNonDiscoverableNvmeDeviceGuid,
  &gEdkiiNonDiscoverableOhciDeviceGuid,
  &gEdkiiNonDiscoverableSdhciDeviceGuid,
  &gEdkiiNonDiscoverableUfsDeviceGuid,
  &gEdkiiNonDiscoverableUhciDeviceGuid,
  &gEdkiiNonDiscoverableXhciDeviceGuid,
};

//
// Probe, start and stop functions of this driver, called by the DXE core for
// specific devices.
//
// The following specifications document these interfaces:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01, 9 Driver Binding Protocol
// - UEFI Spec 2.3.1 + Errata C, 10.1 EFI Driver Binding Protocol
//
// The implementation follows:
// - Driver Writer's Guide for UEFI 2.3.1 v1.01
//   - 5.1.3.4 OpenProtocol() and CloseProtocol()
// - UEFI Spec 2.3.1 + Errata C
//   -  6.3 Protocol Handler Services
//

STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  NON_DISCOVERABLE_DEVICE             *Device;
  EFI_STATUS                          Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR   *Desc;
  INTN                                Idx;

  Status = gBS->OpenProtocol (DeviceHandle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid, (VOID **)&Device,
                  This->DriverBindingHandle, DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Restricted to DMA coherent for now
  //
  Status = EFI_UNSUPPORTED;
  if (Device->DmaType != NonDiscoverableDeviceDmaTypeCoherent) {
    goto CloseProtocol;
  }

  for (Idx = 0; Idx < ARRAY_SIZE (SupportedNonDiscoverableDevices); Idx++) {
    if (CompareGuid (Device->Type, SupportedNonDiscoverableDevices [Idx])) {
      Status = EFI_SUCCESS;
      break;
    }
  }

  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  //
  // We only support MMIO devices, so iterate over the resources to ensure
  // that they only describe things that we can handle
  //
  for (Desc = Device->Resources; Desc->Desc != ACPI_END_TAG_DESCRIPTOR;
       Desc = (VOID *)((UINT8 *)Desc + Desc->Len + 3)) {
    if (Desc->Desc != ACPI_ADDRESS_SPACE_DESCRIPTOR ||
        Desc->ResType != ACPI_ADDRESS_SPACE_TYPE_MEM) {
      Status = EFI_UNSUPPORTED;
      break;
    }
  }

CloseProtocol:
  gBS->CloseProtocol (DeviceHandle, &gEdkiiNonDiscoverableDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

  return Status;
}

STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  NON_DISCOVERABLE_PCI_DEVICE   *Dev;
  EFI_STATUS                    Status;

  Dev = AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (DeviceHandle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid,
                  (VOID **)&Dev->Device, This->DriverBindingHandle,
                  DeviceHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    goto FreeDev;
  }

  InitializePciIoProtocol (Dev);

  //
  // Setup complete, attempt to export the driver instance's
  // EFI_PCI_IO_PROTOCOL interface.
  //
  Dev->Signature = NON_DISCOVERABLE_PCI_DEVICE_SIG;
  Status = gBS->InstallProtocolInterface (&DeviceHandle, &gEfiPciIoProtocolGuid,
                  EFI_NATIVE_INTERFACE, &Dev->PciIo);
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  return EFI_SUCCESS;

CloseProtocol:
  gBS->CloseProtocol (DeviceHandle, &gEdkiiNonDiscoverableDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

FreeDev:
  FreePool (Dev);

  return Status;
}


STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS                      Status;
  EFI_PCI_IO_PROTOCOL             *PciIo;
  NON_DISCOVERABLE_PCI_DEVICE     *Dev;

  Status = gBS->OpenProtocol (DeviceHandle, &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo, This->DriverBindingHandle, DeviceHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (PciIo);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallProtocolInterface (DeviceHandle,
                  &gEfiPciIoProtocolGuid, &Dev->PciIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (DeviceHandle, &gEdkiiNonDiscoverableDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

  FreePool (Dev);

  return EFI_SUCCESS;
}


//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL gDriverBinding = {
  &NonDiscoverablePciDeviceSupported,
  &NonDiscoverablePciDeviceStart,
  &NonDiscoverablePciDeviceStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL,
  NULL
};

//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceDxeEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}
