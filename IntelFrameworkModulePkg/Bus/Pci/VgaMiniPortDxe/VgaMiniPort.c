/** @file
  Implements EFI Driver Binding Protocol and VGA Mini Port Protocol for VGA Mini Port Driver.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VgaMiniPort.h"

//
// EFI Driver Binding Protocol Instance
//
//   This driver has a version value of 0x00000000.  This is the
//   lowest possible priority for a driver.  This is done on purpose to help
//   the developers of UGA drivers.  This driver can bind if no UGA driver
//   is present, so a console is available.  Then, when a UGA driver is loaded
//   this driver can be disconnected, and the UGA driver can be connected.
//   As long as the UGA driver has a version value greater than 0x00000000, it
//   will be connected first and will block this driver from connecting.
//
EFI_DRIVER_BINDING_PROTOCOL gPciVgaMiniPortDriverBinding = {
  PciVgaMiniPortDriverBindingSupported,
  PciVgaMiniPortDriverBindingStart,
  PciVgaMiniPortDriverBindingStop,
  0x00000000,
  NULL,
  NULL
};

/**
  Entrypoint of VGA Mini Port Driver.

  This function is the entrypoint of UVGA Mini Port Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gPciVgaMiniPortDriverBinding,
             ImageHandle,
             &gPciVgaMiniPortComponentName,
             &gPciVgaMiniPortComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


/**
  Check whether VGA Mini Port driver supports this device.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval EFI_UNSUPPORTED        This device isn't supported.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  EFI_PCI_IO_PROTOCOL *PciIo;
  PCI_TYPE00          Pci;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // See if this is a PCI VGA Controller by looking at the Command register and
  // Class Code Register
  //
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

  Status = EFI_UNSUPPORTED;
  //
  // See if the device is an enabled VGA device.
  // Most systems can only have on VGA device on at a time.
  //
  if (((Pci.Hdr.Command & 0x03) == 0x03) && IS_PCI_VGA (&Pci)) {
    Status = EFI_SUCCESS;
  }

Done:
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}


/**
  Starts the VGA device with this driver.

  This function consumes PCI I/O Protocol, and installs VGA Mini Port Protocol
  onto the VGA device handle.

  @param  This                   The driver binding instance.
  @param  Controller             The controller to check.
  @param  RemainingDevicePath    The remaining device patch.

  @retval EFI_SUCCESS            The controller is controlled by the driver.
  @retval EFI_ALREADY_STARTED    The controller is already controlled by the driver.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS            Status;
  EFI_PCI_IO_PROTOCOL   *PciIo;
  PCI_VGA_MINI_PORT_DEV *PciVgaMiniPortPrivate;

  PciVgaMiniPortPrivate = NULL;
  PciIo                 = NULL;
  //
  // Open the IO Abstraction(s) needed
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Allocate the private device structure
  //
  PciVgaMiniPortPrivate = AllocateZeroPool (sizeof (PCI_VGA_MINI_PORT_DEV));
  ASSERT (PciVgaMiniPortPrivate != NULL);

  //
  // Initialize the private device structure
  //
  PciVgaMiniPortPrivate->Signature = PCI_VGA_MINI_PORT_DEV_SIGNATURE;
  PciVgaMiniPortPrivate->Handle = Controller;
  PciVgaMiniPortPrivate->PciIo = PciIo;

  PciVgaMiniPortPrivate->VgaMiniPort.SetMode = PciVgaMiniPortSetMode;
  PciVgaMiniPortPrivate->VgaMiniPort.VgaMemoryOffset = 0xb8000;
  PciVgaMiniPortPrivate->VgaMiniPort.CrtcAddressRegisterOffset = 0x3d4;
  PciVgaMiniPortPrivate->VgaMiniPort.CrtcDataRegisterOffset = 0x3d5;
  PciVgaMiniPortPrivate->VgaMiniPort.VgaMemoryBar = EFI_PCI_IO_PASS_THROUGH_BAR;
  PciVgaMiniPortPrivate->VgaMiniPort.CrtcAddressRegisterBar = EFI_PCI_IO_PASS_THROUGH_BAR;
  PciVgaMiniPortPrivate->VgaMiniPort.CrtcDataRegisterBar = EFI_PCI_IO_PASS_THROUGH_BAR;
  PciVgaMiniPortPrivate->VgaMiniPort.MaxMode = 1;

  //
  // Install VGA Mini Port Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiVgaMiniPortProtocolGuid,
                  &PciVgaMiniPortPrivate->VgaMiniPort,
                  NULL
                  );
Done:
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    if (PciVgaMiniPortPrivate != NULL) {
      FreePool (PciVgaMiniPortPrivate);
    }
  }

  return Status;
}


/**
  Stop the VGA device with this driver.

  This function uninstalls VGA Mini Port Protocol from the VGA device handle,
  and closes PCI I/O Protocol.

  @param  This                   The driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The child number that opened controller
                                 BY_CHILD.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The controller or children are stopped.
  @retval EFI_DEVICE_ERROR       Failed to stop the driver.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;
  EFI_VGA_MINI_PORT_PROTOCOL  *VgaMiniPort;
  PCI_VGA_MINI_PORT_DEV       *PciVgaMiniPortPrivate;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiVgaMiniPortProtocolGuid,
                  (VOID **) &VgaMiniPort,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  PciVgaMiniPortPrivate = PCI_VGA_MINI_PORT_DEV_FROM_THIS (VgaMiniPort);

  Status = gBS->UninstallProtocolInterface (
                  Controller,
                  &gEfiVgaMiniPortProtocolGuid,
                  &PciVgaMiniPortPrivate->VgaMiniPort
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  FreePool (PciVgaMiniPortPrivate);

  return EFI_SUCCESS;
}
//
// VGA Mini Port Protocol Functions
//

/**
  Sets the text display mode of a VGA controller.

  This function implements EFI_VGA_MINI_PORT_PROTOCOL.SetMode().
  If ModeNumber exceeds the valid range, then EFI_UNSUPPORTED is returned.
  Otherwise, EFI_SUCCESS is directly returned without real operation.

  @param This                 Protocol instance pointer.
  @param ModeNumber           Mode number.  0 - 80x25   1-80x50

  @retval EFI_SUCCESS         The mode was set
  @retval EFI_UNSUPPORTED     ModeNumber is not supported.
  @retval EFI_DEVICE_ERROR    The device is not functioning properly.

**/
EFI_STATUS
EFIAPI
PciVgaMiniPortSetMode (
  IN  EFI_VGA_MINI_PORT_PROTOCOL  *This,
  IN  UINTN                       ModeNumber
  )
{
  if (ModeNumber > This->MaxMode) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

