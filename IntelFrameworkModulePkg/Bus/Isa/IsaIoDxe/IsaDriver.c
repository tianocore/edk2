/** @file
  IsaIo UEFI driver.

  Produce an instance of the ISA I/O Protocol for every SIO controller.
  
Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IsaDriver.h"

//
// IsaIo Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gIsaIoDriver = {
  IsaIoDriverSupported,
  IsaIoDriverStart,
  IsaIoDriverStop,
  0xa,
  NULL,
  NULL
};

/**
  The main entry point for the IsaIo driver.

  @param[in] ImageHandle        The firmware allocated handle for the EFI image.  
  @param[in] SystemTable        A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory in pool to install all the protocols.
**/
EFI_STATUS
EFIAPI
InitializeIsaIo (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIsaIoDriver,
             ImageHandle,
             &gIsaIoComponentName,
             &gIsaIoComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** 
  Tests to see if a controller can be managed by the IsaIo driver.

  @param[in] This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.  
  @param[in] Controller           The handle of the controller to test.
  @param[in] RemainingDevicePath  A pointer to the remaining portion of a device path.
  
  @retval EFI_SUCCESS             The device is supported by this driver.
  @retval EFI_ALREADY_STARTED     The device is already being managed by this driver.
  @retval EFI_ACCESS_DENIED       The device is already being managed by a different driver 
                                  or an application that requires exclusive access.
  @retval EFI_UNSUPPORTED         The device is is not supported by this driver.

**/
EFI_STATUS
EFIAPI
IsaIoDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_SIO_PROTOCOL          *Sio;
  EFI_HANDLE                PciHandle;

  //
  // Try to open EFI DEVICE PATH protocol on the controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Get the PciIo protocol from its parent controller.
    //
    Status = gBS->LocateDevicePath (&gEfiPciIoProtocolGuid, &DevicePath, &PciHandle);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Try to open the Super IO protocol on the controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiSioProtocolGuid,
                  (VOID **) &Sio,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiSioProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  return Status;
}

/**
  Start this driver on ControllerHandle. 
  
  The Start() function is designed to be invoked from the EFI boot service ConnectController(). 
  As a result, much of the error checking on the parameters to Start() has been moved into this 
  common boot service. It is legal to call Start() from other locations, but the following calling 
  restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.  

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.
**/
EFI_STATUS
EFIAPI
IsaIoDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                            Status;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_HANDLE                            PciHandle;
  EFI_SIO_PROTOCOL                      *Sio;
  ACPI_RESOURCE_HEADER_PTR              Resources;
  EFI_DEVICE_PATH_PROTOCOL              *TempDevicePath;
  ISA_IO_DEVICE                         *IsaIoDevice;

  PciIo = NULL;
  Sio   = NULL;

  //
  // Open Device Path Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the PciIo protocol from its parent controller.
  //
  TempDevicePath = DevicePath;
  Status = gBS->LocateDevicePath (&gEfiPciIoProtocolGuid, &TempDevicePath, &PciHandle);
  if (!EFI_ERROR (Status)) {
    Status = gBS->HandleProtocol (PciHandle, &gEfiPciIoProtocolGuid, (VOID **) &PciIo);
    ASSERT_EFI_ERROR (Status);

    //
    // Open Super IO Protocol
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiSioProtocolGuid,
                    (VOID **) &Sio,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
  }

  if (EFI_ERROR (Status)) {
    //
    // Fail due to LocateDevicePath(...) or OpenProtocol(Sio, BY_DRIVER)
    //
    return Status;
  }

  Status = Sio->GetResources (Sio, &Resources);
  ASSERT_EFI_ERROR (Status);

  IsaIoDevice = AllocatePool (sizeof (ISA_IO_DEVICE));
  ASSERT (IsaIoDevice != NULL);

  IsaIoDevice->Signature = ISA_IO_DEVICE_SIGNATURE;
  IsaIoDevice->PciIo     = PciIo;

  //
  // Initialize the ISA I/O instance structure
  //
  InitializeIsaIoInstance (IsaIoDevice, DevicePath, Resources);

  //
  // Install the ISA I/O protocol on the Controller handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiIsaIoProtocolGuid,
                  &IsaIoDevice->IsaIo,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Stop this driver on ControllerHandle. 
  
  The Stop() function is designed to be invoked from the EFI boot service DisconnectController(). 
  As a result, much of the error checking on the parameters to Stop() has been moved 
  into this common boot service. It is legal to call Stop() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  
  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must 
                                support a bus specific I/O protocol for the driver 
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.
**/
EFI_STATUS
EFIAPI
IsaIoDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   * ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                          Status;
  ISA_IO_DEVICE                       *IsaIoDevice;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaIoProtocolGuid,
                  (VOID **) &IsaIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (IsaIo);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Controller,
                  &gEfiIsaIoProtocolGuid,
                  &IsaIoDevice->IsaIo,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiSioProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    FreePool (IsaIoDevice->IsaIo.ResourceList);
    FreePool (IsaIoDevice);
  }

   return Status;
}
