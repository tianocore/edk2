/** @file
  CxlDxe driver is used to discover CXL devices
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"

//
// CXL Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL  gCxlDriverBinding = {
  CxlDriverBindingSupported,
  CxlDriverBindingStart,
  CxlDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/**
  Tests to see if this driver supports a given controller.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
CxlDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS           Status;
  EFI_STATUS           RegStatus;
  EFI_PCI_IO_PROTOCOL  *PciIo;
  UINT8                ClassCode[3];
  UINT32               ExtCapOffset;
  UINT32               NextExtCapOffset;
  UINT32               PcieExtCapAndDvsecHeader[PcieDvsecHeaderMax];

  // Ensure driver won't be started multiple times
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiCallerIdGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return EFI_ALREADY_STARTED;
  }

  // Attempt to Open PCI I/O Protocol
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] PciGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = PciIo->Pci.Read (
                        PciIo,
                        EfiPciIoWidthUint8,
                        PCI_CLASSCODE_OFFSET,
                        sizeof (ClassCode),
                        ClassCode
                        );

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] CallerGuid OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if ((ClassCode[0] != CXL_MEMORY_PROGIF) || (ClassCode[1] != CXL_MEMORY_SUB_CLASS) || (ClassCode[2] != CXL_MEMORY_CLASS)) {
    DEBUG ((EFI_D_ERROR, "[%a]: UNSUPPORTED Class [CXL-%08X] \n", __func__, Controller));
    Status = EFI_UNSUPPORTED;
    goto EXIT;
  }

  DEBUG ((EFI_D_INFO, "[%a]: SUPPORTED ClassCode0 = %d\n", __func__, ClassCode[0]));
  DEBUG ((EFI_D_INFO, "[%a]: SUPPORTED ClassCode1 = %d\n", __func__, ClassCode[1]));
  DEBUG ((EFI_D_INFO, "[%a]: SUPPORTED ClassCode2 = %d\n", __func__, ClassCode[2]));

  Status           = EFI_UNSUPPORTED;
  NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_OFFSET;
  do {
    ExtCapOffset = NextExtCapOffset;
    DEBUG ((EFI_D_INFO, "[%a]: ExtCapOffset = %d \n", __func__, ExtCapOffset));
    RegStatus = PciIo->Pci.Read (
                             PciIo,
                             EfiPciIoWidthUint32,
                             ExtCapOffset,
                             PcieDvsecHeaderMax,
                             PcieExtCapAndDvsecHeader
                             );

    if (EFI_ERROR (RegStatus)) {
      DEBUG ((EFI_D_ERROR, "[%a]: Failed to read PCI IO for Ext. capability \n", __func__));
      goto EXIT;
    }

    /* Check whether this is a CXL device */
    if (CXL_IS_DVSEC (PcieExtCapAndDvsecHeader[PcieDvsecHeader1])) {
      Status = EFI_SUCCESS;
      break;
    }

    NextExtCapOffset = CXL_PCIE_EXTENDED_CAP_NEXT (
                         PcieExtCapAndDvsecHeader[PcieExtCapHeader]
                         );
  } while (NextExtCapOffset);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "[%a]: ****[CXL-%08X] Error: Non CXL Device****\n", __func__, Controller));
  }

EXIT:

  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );
  return Status;
}

/**
  Stops a device controller or a bus controller.

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
CxlDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  gBS->CloseProtocol (
         Controller,
         &gEfiPciIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  gBS->CloseProtocol (
         Controller,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
CxlDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                Status;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  UINTN                     Seg;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] path OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiPciIoProtocolGuid,
                  (VOID **)&PciIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo OpenProtocol Returning status = %r\n", __func__, Controller, Status));
    goto EXIT;
  }

  if (Status == EFI_ALREADY_STARTED) {
    DEBUG ((EFI_D_ERROR, "[%a]: [CXL-%08X] PciIo EFI_ALREADY_STARTED status = %r\n", __func__, Controller, Status));
  } else {
    Status = PciIo->GetLocation (PciIo, &Seg, &Bus, &Device, &Function);
    if (Status != EFI_SUCCESS) {
      DEBUG ((EFI_D_ERROR, "[%a]: Error PciIo GetLocation\n", __func__));
      return Status;
    }

    DEBUG ((EFI_D_INFO, "[%a]: Bus = %d, DEVICE = %d, FUNCTION = %d\n", __func__, Bus, Device, Function));
  }

  return EFI_SUCCESS;

EXIT:

  gBS->CloseProtocol (
                      Controller,
                      &gEfiPciIoProtocolGuid,
                      This->DriverBindingHandle,
                      Controller
                      );

  gBS->CloseProtocol (
                      Controller,
                      &gEfiDevicePathProtocolGuid,
                      This->DriverBindingHandle,
                      Controller
                      );

  DEBUG ((EFI_D_INFO, "[%a]: [CXL-%08X] Completed status = %r\n", __func__, Controller, Status));
  return Status;
}

/**
  The entry point for CXL driver, used to install CXL driver on the ImageHandle.

  @param  ImageHandle   The firmware allocated handle for this driver image.
  @param  SystemTable   Pointer to the EFI system table.

  @retval EFI_SUCCESS   Driver loaded.
  @retval other         Driver not loaded.

**/
EFI_STATUS
EFIAPI
CxlDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((EFI_D_INFO, "[%a]: Driver entery point get called\n", __func__));

  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gCxlDriverBinding,
             ImageHandle,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL,
             NULL
             );

  DEBUG ((EFI_D_INFO, "[%a]: CXL Installing DriverBinding status = %r\n", __func__, Status));
  ASSERT_EFI_ERROR (Status);
  gRT = SystemTable->RuntimeServices;
  return Status;
}

