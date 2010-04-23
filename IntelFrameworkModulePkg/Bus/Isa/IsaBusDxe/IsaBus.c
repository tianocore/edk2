/** @file
  ISA Bus UEFI driver.

  Discovers all the ISA Controllers and their resources by using the ISA ACPI 
  Protocol, produces an instance of the ISA I/O Protocol for every ISA 
  Controller found. This driver is designed to manage a PCI-to-ISA bridge Device
  such as LPC bridge.
  
Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "InternalIsaBus.h"

//
// ISA Bus Driver Global Variables
//
EFI_DRIVER_BINDING_PROTOCOL gIsaBusControllerDriver = {
  IsaBusControllerDriverSupported,
  IsaBusControllerDriverStart,
  IsaBusControllerDriverStop,
  0xa,
  NULL,
  NULL
};

/**
  The main entry point for the ISA Bus driver.

  @param[in] ImageHandle        The firmware allocated handle for the EFI image.  
  @param[in] SystemTable        A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS           The entry point is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  There was not enough memory in pool to install all the protocols.
**/
EFI_STATUS
EFIAPI
InitializeIsaBus(
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
             &gIsaBusControllerDriver,
             ImageHandle,
             &gIsaBusComponentName,
             &gIsaBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** 
  Tests to see if a controller can be managed by the ISA Bus Driver. If a child device is provided, 
  it further tests to see if this driver supports creating a handle for the specified child device.

  Note that the ISA Bus driver always creates all of its child handles on the first call to Start().
  How the Start() function of a driver is implemented can affect how the Supported() function is implemented.

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
IsaBusControllerDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_ISA_ACPI_PROTOCOL     *IsaAcpi;

  //
  // If RemainingDevicePath is not NULL, it should verify that the first device
  // path node in RemainingDevicePath is an ACPI Device path node which is a 
  // legal Device Path Node for this bus driver's children.
  //
  if (RemainingDevicePath != NULL) {
    if (RemainingDevicePath->Type != ACPI_DEVICE_PATH) {
      return EFI_UNSUPPORTED;
    } else if (RemainingDevicePath->SubType == ACPI_DP) {
      if (DevicePathNodeLength (RemainingDevicePath) != sizeof (ACPI_HID_DEVICE_PATH)) {
        return EFI_UNSUPPORTED;
      }
    } else if (RemainingDevicePath->SubType == ACPI_EXTENDED_DP) {
      if (DevicePathNodeLength (RemainingDevicePath) != sizeof (ACPI_EXTENDED_HID_DEVICE_PATH)) {
        return EFI_UNSUPPORTED;
      }
    } else {
      return EFI_UNSUPPORTED;
    }
  }
  //
  // Try to open EFI DEVICE PATH protocol on the controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  //
  // Although this driver creates all child handles at one time,
  // but because all child handles may be not stopped at one time in EFI Driver Binding.Stop(),
  // So it is allowed to create child handles again in successive calls to EFI Driver Binding.Start().
  //
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  //
  // Try to get Pci IO Protocol because it is assumed
  // to have been opened by ISA ACPI driver
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Try to open the Isa Acpi protocol on the controller
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaAcpiProtocolGuid,
                  (VOID **) &IsaAcpi,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add more check to see if the child device is valid by calling IsaAcpi->DeviceEnumerate?
  //

  gBS->CloseProtocol (
         Controller,
         &gEfiIsaAcpiProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Start this driver on ControllerHandle. 
  
  Note that the ISA Bus driver always creates all of its child handles on the first call to Start().
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
IsaBusControllerDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                            Status;
  EFI_PCI_IO_PROTOCOL                   *PciIo;
  EFI_DEVICE_PATH_PROTOCOL              *ParentDevicePath;
  EFI_ISA_ACPI_PROTOCOL                 *IsaAcpi;
  EFI_ISA_ACPI_DEVICE_ID                *IsaDevice;
  EFI_ISA_ACPI_RESOURCE_LIST            *ResourceList;
  EFI_GENERIC_MEMORY_TEST_PROTOCOL      *GenMemoryTest;

  //
  // Local variables declaration for StatusCode reporting
  //
  EFI_DEVICE_PATH_PROTOCOL              *DevicePathData;

  //
  // Get Pci IO Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open Device Path Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    return Status;
  }

  //
  // Open ISA Acpi Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaAcpiProtocolGuid,
                  (VOID **) &IsaAcpi,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && Status != EFI_ALREADY_STARTED) {
    //
    // Close opened protocol
    //
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return Status;
  }
  //
  // The IsaBus driver will use memory below 16M, which is not tested yet,
  // so call CompatibleRangeTest to test them. Since memory below 1M should
  // be reserved to CSM, and 15M~16M might be reserved for Isa hole, test 1M
  // ~15M here
  //
  Status = gBS->LocateProtocol (
                  &gEfiGenericMemTestProtocolGuid,
                  NULL,
                  (VOID **) &GenMemoryTest
                  );

  if (!EFI_ERROR (Status)) {
    Status = GenMemoryTest->CompatibleRangeTest (
                              GenMemoryTest,
                              0x100000,
                              0xE00000
                              );
  }
  //
  // Report Status Code here since we will initialize the host controller
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_LPC | EFI_IOB_PC_INIT),
    ParentDevicePath
    );

  //
  // first init ISA interface
  //
  IsaAcpi->InterfaceInit (IsaAcpi);

  //
  // Report Status Code here since we will enable the host controller
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_LPC | EFI_IOB_PC_ENABLE),
    ParentDevicePath
    );

  //
  // Create each ISA device handle in this ISA bus
  //
  IsaDevice = NULL;
  do {
    Status = IsaAcpi->DeviceEnumerate (IsaAcpi, &IsaDevice);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Get current resource of this ISA device
    //
    ResourceList  = NULL;
    Status        = IsaAcpi->GetCurResource (IsaAcpi, IsaDevice, &ResourceList);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Create handle for this ISA device
    //
    // If any child device handle was created in previous call to Start() and not stopped
    // in previous call to Stop(), it will not be created again because the
    // InstallMultipleProtocolInterfaces() boot service will reject same device path.
    //
    Status = IsaCreateDevice (
               This,
               Controller,
               PciIo,
               ParentDevicePath,
               ResourceList,
               &DevicePathData
               );

    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // Initialize ISA device
    //
    IsaAcpi->InitDevice (IsaAcpi, IsaDevice);

    //
    // Set resources for this ISA device
    //
    Status = IsaAcpi->SetResource (IsaAcpi, IsaDevice, ResourceList);

    //
    // Report Status Code here when failed to resource conflicts
    //
    if (EFI_ERROR (Status) && (Status != EFI_UNSUPPORTED)) {
      //
      // It's hard to tell which resource conflicts
      //
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
         EFI_ERROR_CODE,
         (EFI_IO_BUS_LPC | EFI_IOB_EC_RESOURCE_CONFLICT),
         DevicePathData
         );

    }
    //
    // Set power for this ISA device
    //
    IsaAcpi->SetPower (IsaAcpi, IsaDevice, TRUE);

    //
    // Enable this ISA device
    //
    IsaAcpi->EnableDevice (IsaAcpi, IsaDevice, TRUE);

  } while (TRUE);

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
IsaBusControllerDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   * ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  BOOLEAN                             AllChildrenStopped;
  ISA_IO_DEVICE                       *IsaIoDevice;
  EFI_ISA_IO_PROTOCOL                 *IsaIo;
  EFI_PCI_IO_PROTOCOL                 *PciIo;

  if (NumberOfChildren == 0) {
    //
    // Close the bus driver
    //
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiDevicePathProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->CloseProtocol (
                    Controller,
                    &gEfiIsaAcpiProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    return EFI_SUCCESS;
  }
  //
  // Complete all outstanding transactions to Controller.
  // Don't allow any new transaction to Controller to be started.
  //
  //
  // Stop all the children
  // Find all the ISA devices that were discovered on this PCI to ISA Bridge
  // with the Start() function.
  //
  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {

    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiIsaIoProtocolGuid,
                    (VOID **) &IsaIo,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {

      IsaIoDevice = ISA_IO_DEVICE_FROM_ISA_IO_THIS (IsaIo);

      //
      // Close the child handle
      //

      Status = gBS->CloseProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      This->DriverBindingHandle,
                      ChildHandleBuffer[Index]
                      );
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      ChildHandleBuffer[Index],
                      &gEfiDevicePathProtocolGuid,
                      IsaIoDevice->DevicePath,
                      &gEfiIsaIoProtocolGuid,
                      &IsaIoDevice->IsaIo,
                      NULL
                      );

      if (!EFI_ERROR (Status)) {
        FreePool (IsaIoDevice->DevicePath);
        FreePool (IsaIoDevice);
      } else {
        //
        // Re-open PCI IO Protocol on behalf of the child device
        // because of failure of destroying the child device handle
        //
        gBS->OpenProtocol (
               Controller,
               &gEfiPciIoProtocolGuid,
               (VOID **) &PciIo,
               This->DriverBindingHandle,
               ChildHandleBuffer[Index],
               EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
               );     
      }
    }

    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

//
// Internal Function
//

/**
  Create EFI Handle for a ISA device found via ISA ACPI Protocol 

  @param[in] This                   The EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in] Controller             The handle of ISA bus controller(PCI to ISA bridge)
  @param[in] PciIo                  The Pointer to the PCI protocol 
  @param[in] ParentDevicePath       Device path of the ISA bus controller
  @param[in] IsaDeviceResourceList  The resource list of the ISA device
  @param[out] ChildDevicePath       The pointer to the child device.

  @retval EFI_SUCCESS               The handle for the child device was created.
  @retval EFI_OUT_OF_RESOURCES      The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR          The handle for the child device can not be created.
**/
EFI_STATUS
IsaCreateDevice (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_PCI_IO_PROTOCOL          *PciIo,
  IN EFI_DEVICE_PATH_PROTOCOL     *ParentDevicePath,
  IN EFI_ISA_ACPI_RESOURCE_LIST   *IsaDeviceResourceList,
  OUT EFI_DEVICE_PATH_PROTOCOL    **ChildDevicePath
  )
{
  EFI_STATUS    Status;
  ISA_IO_DEVICE *IsaIoDevice;
  EFI_DEV_PATH  Node;

  //
  // Initialize the PCI_IO_DEVICE structure
  //
  IsaIoDevice = AllocateZeroPool (sizeof (ISA_IO_DEVICE));
  if (IsaIoDevice == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IsaIoDevice->Signature  = ISA_IO_DEVICE_SIGNATURE;
  IsaIoDevice->Handle     = NULL;
  IsaIoDevice->PciIo      = PciIo;

  //
  // Initialize the ISA I/O instance structure
  //
  InitializeIsaIoInstance (IsaIoDevice, IsaDeviceResourceList);

  //
  // Build the child device path
  //
  Node.DevPath.Type     = ACPI_DEVICE_PATH;
  Node.DevPath.SubType  = ACPI_DP;
  SetDevicePathNodeLength (&Node.DevPath, sizeof (ACPI_HID_DEVICE_PATH));
  Node.Acpi.HID = IsaDeviceResourceList->Device.HID;
  Node.Acpi.UID = IsaDeviceResourceList->Device.UID;

  IsaIoDevice->DevicePath = AppendDevicePathNode (
                              ParentDevicePath,
                              &Node.DevPath
                              );

  if (IsaIoDevice->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  *ChildDevicePath = IsaIoDevice->DevicePath;

  //
  // Create a child handle and install Device Path and ISA I/O protocols
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &IsaIoDevice->Handle,
                  &gEfiDevicePathProtocolGuid,
                  IsaIoDevice->DevicePath,
                  &gEfiIsaIoProtocolGuid,
                  &IsaIoDevice->IsaIo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **) &PciIo,
                  This->DriverBindingHandle,
                  IsaIoDevice->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           IsaIoDevice->Handle,
           &gEfiDevicePathProtocolGuid,
           IsaIoDevice->DevicePath,
           &gEfiIsaIoProtocolGuid,
           &IsaIoDevice->IsaIo,
           NULL
           );
  }

Done:

  if (EFI_ERROR (Status)) {
    if (IsaIoDevice->DevicePath != NULL) {
      FreePool (IsaIoDevice->DevicePath);
    }

    FreePool (IsaIoDevice);
  }

  return Status;
}

