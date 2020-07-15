/** @file
  The SioBusDxe driver is used to create child devices on the ISA bus and
  installs the Super I/O protocols on them.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SioBusDxe.h"

//
// SioBus Driver Binding Protocol
//
EFI_DRIVER_BINDING_PROTOCOL gSioBusDriverBinding = {
  SioBusDriverBindingSupported,
  SioBusDriverBindingStart,
  SioBusDriverBindingStop,
  0x10,
  NULL,
  NULL
};


/**
  Tests to see if this driver supports a given controller. If a child device is
  provided, it further tests to see if this driver supports creating a handle
  for the specified child device.

  This function checks to see if the driver specified by This supports the
  device specified by ControllerHandle. Drivers will typically use the device
  path attached to ControllerHandle and/or the services from the bus I/O
  abstraction attached to ControllerHandle to determine if the driver supports
  ControllerHandle. This function may be called many times during platform
  initialization. In order to reduce boot times, the tests performed by this
  function must be very small, and take as little time as possible to execute.
  This function must not change the state of any hardware devices, and this
  function must be aware that the device specified by ControllerHandle may
  already be managed by the same driver or a different driver. This function
  must match its calls to AllocatePages() with FreePages(), AllocatePool() with
  FreePool(), and OpenProtocol() with CloseProtocol(). Since ControllerHandle
  may have been previously started by the same driver, if a protocol is already
  in the opened state, then it must not be closed with CloseProtocol(). This is
  required to guarantee the state of ControllerHandle is not modified by this
  function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This
                                   handle must support a protocol interface
                                   that supplies an I/O abstraction to the
                                   driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter
                                   is not NULL, then the bus driver must
                                   determine if the bus controller specified by
                                   ControllerHandle and the child controller
                                   specified by RemainingDevicePath are both
                                   supported by this bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the
                                   driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by the driver specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed
                                   by a different driver or an application that
                                   requires exclusive access.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the
                                   driver specified by This.

**/
EFI_STATUS
EFIAPI
SioBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  EFI_PCI_IO_PROTOCOL    *PciIo;
  PCI_TYPE00             Pci;
  UINTN                  SegmentNumber;
  UINTN                  BusNumber;
  UINTN                  DeviceNumber;
  UINTN                  FunctionNumber;

  //
  // Get PciIo protocol instance
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = PciIo->Pci.Read (
                    PciIo,
                    EfiPciIoWidthUint32,
                    0,
                    sizeof(Pci) / sizeof(UINT32),
                    &Pci);

  if (!EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
    if ((Pci.Hdr.Command & 0x03) == 0x03) {
      if (Pci.Hdr.ClassCode[2] == PCI_CLASS_BRIDGE) {
        //
        // See if this is a standard PCI to ISA Bridge from the Base Code and
        // Class Code
        //
        if (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA) {
          Status = EFI_SUCCESS;
        }

        //
        // See if this is an Intel PCI to ISA bridge in Positive Decode Mode
        //
        if (Pci.Hdr.ClassCode[1] == PCI_CLASS_BRIDGE_ISA_PDECODE &&
            Pci.Hdr.VendorId     == 0x8086) {
          //
          // See if this is on Function #0 to avoid false positives on
          // PCI_CLASS_BRIDGE_OTHER that has the same value as
          // PCI_CLASS_BRIDGE_ISA_PDECODE
          //
          Status = PciIo->GetLocation (
                            PciIo,
                            &SegmentNumber,
                            &BusNumber,
                            &DeviceNumber,
                            &FunctionNumber
                            );
          if (!EFI_ERROR (Status) && FunctionNumber == 0) {
            Status = EFI_SUCCESS;
          } else {
            Status = EFI_UNSUPPORTED;
          }
        }
      }
    }
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service
  ConnectController(). As a result, much of the error checking on the
  parameters to Start() has been moved into this common boot service. It is
  legal to call Start() from other locations, but the following calling
  restrictions must be followed or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a
     naturally aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver
     specified by This must have been called with the same calling parameters,
     and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                   instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This
                                   handle must support a protocol interface
                                   that supplies an I/O abstraction to the
                                   driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a
                                   device path.  This parameter is ignored by
                                   device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter
                                   is NULL, then handles for all the children
                                   of Controller are created by this driver. If
                                   this parameter is not NULL and the first
                                   Device Path Node is not the End of Device
                                   Path Node, then only the handle for the
                                   child device specified by the first Device
                                   Path Node of RemainingDevicePath is created
                                   by this driver. If the first Device Path
                                   Node of RemainingDevicePath is the End of
                                   Device Path Node, no child handle is created
                                   by this driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a
                                   device error.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a
                                   lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
SioBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                     Status;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_DEVICE_PATH_PROTOCOL       *ParentDevicePath;
  UINT64                         Supports;
  UINT64                         OriginalAttributes;
  UINT64                         Attributes;
  BOOLEAN                        Enabled;
  SIO_BUS_DRIVER_PRIVATE_DATA    *Private;
  UINT32                         ChildDeviceNumber;

  Enabled            = FALSE;
  Supports           = 0;
  OriginalAttributes = 0;
  Private            = NULL;

  //
  // Open the PCI I/O Protocol Interface
  //
  PciIo = NULL;
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID**) &PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
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
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return Status;
  }

  //
  // Get supported PCI attributes
  //
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationSupported,
                    0,
                    &Supports
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Supports &= (UINT64) (EFI_PCI_IO_ATTRIBUTE_ISA_IO |
                        EFI_PCI_IO_ATTRIBUTE_ISA_IO_16);
  if (Supports == 0 ||
      Supports == (EFI_PCI_IO_ATTRIBUTE_ISA_IO |
                   EFI_PCI_IO_ATTRIBUTE_ISA_IO_16)) {
    Status = EFI_UNSUPPORTED;
    goto Done;
  }

  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationGet,
                    0,
                    &OriginalAttributes
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Attributes = EFI_PCI_DEVICE_ENABLE |
               Supports |
               EFI_PCI_IO_ATTRIBUTE_ISA_MOTHERBOARD_IO;
  Status = PciIo->Attributes (
                    PciIo,
                    EfiPciIoAttributeOperationEnable,
                    Attributes,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Enabled = TRUE;

  //
  // Store the OriginalAttributes for the restore in BindingStop()
  //
  Private = AllocateZeroPool (sizeof (SIO_BUS_DRIVER_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  Private->PciIo              = PciIo;
  Private->OriginalAttributes = OriginalAttributes;

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiCallerIdGuid,
                  EFI_NATIVE_INTERFACE,
                  Private
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Report status code for the start of general controller initialization
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_LPC | EFI_IOB_PC_INIT),
    ParentDevicePath
    );

  //
  // Report status code for the start of enabling devices on the bus
  //
  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_IO_BUS_LPC | EFI_IOB_PC_ENABLE),
    ParentDevicePath
    );

  //
  // Create all the children upon the first entrance
  //
  ChildDeviceNumber = SioCreateAllChildDevices (
                        This,
                        Controller,
                        PciIo,
                        ParentDevicePath
                        );
  if (ChildDeviceNumber == 0) {
    Status = EFI_DEVICE_ERROR;
  }

Done:
  if (EFI_ERROR (Status)) {
    if (PciIo != NULL && Enabled) {
      PciIo->Attributes (
               PciIo,
               EfiPciIoAttributeOperationSet,
               OriginalAttributes,
               NULL
               );
    }

    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiPciIoProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    if (Private != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             Controller,
             &gEfiCallerIdGuid,
             Private,
             NULL
             );
      FreePool (Private);
    }

    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service
  DisconnectController(). As a result, much of the error checking on the
  parameters to Stop() has been moved into this common boot service. It is
  legal to call Stop() from other locations, but the following calling
  restrictions must be followed or the system behavior will not be
  deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous
     call to this same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a
     valid EFI_HANDLE. In addition, all of these handles must have been created
     in this driver's Start() function, and the Start() function must have
     called OpenProtocol() on ControllerHandle with an Attribute of
     EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This               A pointer to the EFI_DRIVER_BINDING_PROTOCOL
                                 instance.
  @param[in]  ControllerHandle   A handle to the device being stopped. The
                                 handle must support a bus specific I/O
                                 protocol for the driver to use to stop the
                                 device.
  @param[in]  NumberOfChildren   The number of child device handles in
                                 ChildHandleBuffer.
  @param[in]  ChildHandleBuffer  An array of child handles to be freed. May be
                                 NULL if NumberOfChildren is 0.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a
                                 device error.

**/
EFI_STATUS
EFIAPI
SioBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                     Status;
  SIO_BUS_DRIVER_PRIVATE_DATA    *Private;
  UINTN                          Index;
  BOOLEAN                        AllChildrenStopped;
  EFI_SIO_PROTOCOL               *Sio;
  SIO_DEV                        *SioDevice;
  EFI_PCI_IO_PROTOCOL            *PciIo;

  if (NumberOfChildren == 0) {
    //
    // Restore PCI attributes
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **) &Private,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = Private->PciIo->Attributes (
                               Private->PciIo,
                               EfiPciIoAttributeOperationSet,
                               Private->OriginalAttributes,
                               NULL
                               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    gBS->UninstallProtocolInterface (
          Controller,
          &gEfiCallerIdGuid,
          Private
          );
    FreePool (Private);

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
                    &gEfiPciIoProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    return EFI_SUCCESS;
  }

  //
  // Stop all the children
  //
  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gEfiSioProtocolGuid,
                    (VOID **) &Sio,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      SioDevice = SIO_DEV_FROM_SIO (Sio);

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
                      SioDevice->DevicePath,
                      &gEfiSioProtocolGuid,
                      &SioDevice->Sio,
                      NULL
                      );

      if (!EFI_ERROR (Status)) {
        FreePool (SioDevice->DevicePath);
        FreePool (SioDevice);
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

/**
  The entry point for the SioBusDxe driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
SioBusDxeDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  //
  // Install driver model protocol(s).
  //
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gSioBusDriverBinding,
           ImageHandle,
           &gSioBusComponentName,
           &gSioBusComponentName2
           );
}
