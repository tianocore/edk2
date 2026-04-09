/** @file

  Copyright (C) 2016, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "NonDiscoverablePciDeviceIo.h"

#include <Protocol/DriverBinding.h>

#define MAX_NON_DISCOVERABLE_PCI_DEVICE_ID  (32 * 256)

STATIC UINTN           mUniqueIdCounter = 0;
EFI_CPU_ARCH_PROTOCOL  *mCpu;

//
// We only support the following device types
//
STATIC
CONST EFI_GUID *CONST
SupportedNonDiscoverableDevices[] = {
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

/**
  Supported function of Driver Binding protocol for this driver.
  Test to see if this driver supports ControllerHandle.

  @param This                   Protocol instance pointer.
  @param DeviceHandle           Handle of device to test.
  @param RemainingDevicePath    A pointer to the device path.
                                it should be ignored by device driver.

  @retval EFI_SUCCESS           This driver supports this device.
  @retval other                 This driver does not support this device.

**/
STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  NON_DISCOVERABLE_DEVICE            *Device;
  EFI_STATUS                         Status;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Desc;
  INTN                               Idx;

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid,
                  (VOID **)&Device,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_UNSUPPORTED;
  for (Idx = 0; Idx < ARRAY_SIZE (SupportedNonDiscoverableDevices); Idx++) {
    if (CompareGuid (Device->Type, SupportedNonDiscoverableDevices[Idx])) {
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
       Desc = (VOID *)((UINT8 *)Desc + Desc->Len + 3))
  {
    if ((Desc->Desc != ACPI_ADDRESS_SPACE_DESCRIPTOR) ||
        (Desc->ResType != ACPI_ADDRESS_SPACE_TYPE_MEM))
    {
      Status = EFI_UNSUPPORTED;
      break;
    }
  }

CloseProtocol:
  gBS->CloseProtocol (
         DeviceHandle,
         &gEdkiiNonDiscoverableDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  return Status;
}

/**
  This routine is called right after the .Supported() called and
  Start this driver on ControllerHandle.

  @param This                   Protocol instance pointer.
  @param DeviceHandle           Handle of device to bind driver to.
  @param RemainingDevicePath    A pointer to the device path.
                                it should be ignored by device driver.

  @retval EFI_SUCCESS           This driver is added to this device.
  @retval other                 Some error occurs when binding this driver to this device.

**/
STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;
  EFI_STATUS                   Status;

  ASSERT (mUniqueIdCounter < MAX_NON_DISCOVERABLE_PCI_DEVICE_ID);
  if (mUniqueIdCounter >= MAX_NON_DISCOVERABLE_PCI_DEVICE_ID) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev = AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEdkiiNonDiscoverableDeviceProtocolGuid,
                  (VOID **)&Dev->Device,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto FreeDev;
  }

  InitializePciIoProtocol (Dev);

  //
  // Setup complete, attempt to export the driver instance's
  // EFI_PCI_IO_PROTOCOL interface.
  //
  Dev->Signature = NON_DISCOVERABLE_PCI_DEVICE_SIG;
  Status         = gBS->InstallProtocolInterface (
                          &DeviceHandle,
                          &gEfiPciIoProtocolGuid,
                          EFI_NATIVE_INTERFACE,
                          &Dev->PciIo
                          );
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  Dev->UniqueId = mUniqueIdCounter++;

  return EFI_SUCCESS;

CloseProtocol:
  gBS->CloseProtocol (
         DeviceHandle,
         &gEdkiiNonDiscoverableDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

FreeDev:
  FreePool (Dev);

  return Status;
}

/**
  Stop this driver on ControllerHandle.

  @param This               Protocol instance pointer.
  @param DeviceHandle       Handle of device to stop driver on.
  @param NumberOfChildren   Not used.
  @param ChildHandleBuffer  Not used.

  @retval EFI_SUCCESS   This driver is removed from this device.
  @retval other         Some error occurs when removing this driver from this device.

**/
STATIC
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                   Status;
  EFI_PCI_IO_PROTOCOL          *PciIo;
  NON_DISCOVERABLE_PCI_DEVICE  *Dev;

  Status = gBS->OpenProtocol (
                  DeviceHandle,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  DeviceHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = NON_DISCOVERABLE_PCI_DEVICE_FROM_PCI_IO (PciIo);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallProtocolInterface (
                  DeviceHandle,
                  &gEfiPciIoProtocolGuid,
                  &Dev->PciIo
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         DeviceHandle,
         &gEdkiiNonDiscoverableDeviceProtocolGuid,
         This->DriverBindingHandle,
         DeviceHandle
         );

  FreePool (Dev);

  return EFI_SUCCESS;
}

//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL  gDriverBinding = {
  &NonDiscoverablePciDeviceSupported,
  &NonDiscoverablePciDeviceStart,
  &NonDiscoverablePciDeviceStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL,
  NULL
};

/**
  Entry point of this driver.

  @param  ImageHandle     Image handle this driver.
  @param  SystemTable     Pointer to the System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval other           Some error occurred when executing this entry point.

**/
EFI_STATUS
EFIAPI
NonDiscoverablePciDeviceDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  ASSERT_EFI_ERROR (Status);

  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}
