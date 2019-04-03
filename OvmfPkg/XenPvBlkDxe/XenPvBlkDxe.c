/** @file
  This driver produce a BlockIo protocol instance for a Xen PV block device.

  This driver support XenBus protocol of type 'vbd'. Every function that
  comsume XenBus protocol are in BlockFront, which the implementation to access
  a Xen PV device. The BlockIo implementation is in it's one file and will call
  BlockFront functions.

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "XenPvBlkDxe.h"

#include "BlockFront.h"


///
/// Driver Binding Protocol instance
///
EFI_DRIVER_BINDING_PROTOCOL gXenPvBlkDxeDriverBinding = {
  XenPvBlkDxeDriverBindingSupported,
  XenPvBlkDxeDriverBindingStart,
  XenPvBlkDxeDriverBindingStop,
  XEN_PV_BLK_DXE_VERSION,
  NULL,
  NULL
};


/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
XenPvBlkDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  EFI_HANDLE  *HandleBuffer;
  UINTN       HandleCount;
  UINTN       Index;


  //
  // Retrieve array of all handles in the handle database
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Disconnect the current driver from handles in the handle database
  //
  for (Index = 0; Index < HandleCount; Index++) {
    gBS->DisconnectController (HandleBuffer[Index], gImageHandle, NULL);
  }

  //
  // Free the array of handles
  //
  FreePool (HandleBuffer);


  //
  // Uninstall protocols installed in the driver entry point
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid, &gXenPvBlkDxeDriverBinding,
                  &gEfiComponentNameProtocolGuid,  &gXenPvBlkDxeComponentName,
                  &gEfiComponentName2ProtocolGuid, &gXenPvBlkDxeComponentName2,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
XenPvBlkDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install UEFI Driver Model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gXenPvBlkDxeDriverBinding,
             ImageHandle,
             &gXenPvBlkDxeComponentName,
             &gXenPvBlkDxeComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}


/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

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
XenPvBlkDxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS Status;
  XENBUS_PROTOCOL *XenBusIo;

  Status = gBS->OpenProtocol (
                ControllerHandle,
                &gXenBusProtocolGuid,
                (VOID **)&XenBusIo,
                This->DriverBindingHandle,
                ControllerHandle,
                EFI_OPEN_PROTOCOL_BY_DRIVER
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (AsciiStrCmp (XenBusIo->Type, "vbd") == 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  gBS->CloseProtocol (ControllerHandle, &gXenBusProtocolGuid,
                      This->DriverBindingHandle, ControllerHandle);

  return Status;
}

/**
  Starts a device controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
XenPvBlkDxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS Status;
  XENBUS_PROTOCOL *XenBusIo;
  XEN_BLOCK_FRONT_DEVICE *Dev;
  EFI_BLOCK_IO_MEDIA *Media;

  Status = gBS->OpenProtocol (
                ControllerHandle,
                &gXenBusProtocolGuid,
                (VOID **)&XenBusIo,
                This->DriverBindingHandle,
                ControllerHandle,
                EFI_OPEN_PROTOCOL_BY_DRIVER
                );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = XenPvBlockFrontInitialization (XenBusIo, XenBusIo->Node, &Dev);
  if (EFI_ERROR (Status)) {
    goto CloseProtocol;
  }

  CopyMem (&Dev->BlockIo, &gXenPvBlkDxeBlockIo, sizeof (EFI_BLOCK_IO_PROTOCOL));
  Media = AllocateCopyPool (sizeof (EFI_BLOCK_IO_MEDIA),
                            &gXenPvBlkDxeBlockIoMedia);
  if (Dev->MediaInfo.VDiskInfo & VDISK_REMOVABLE) {
    Media->RemovableMedia = TRUE;
  }
  Media->MediaPresent = TRUE;
  Media->ReadOnly = !Dev->MediaInfo.ReadWrite;
  if (Dev->MediaInfo.CdRom) {
    //
    // If it's a cdrom, the blocksize value need to be 2048 for OVMF to
    // recognize it as a cdrom:
    //    MdeModulePkg/Universal/Disk/PartitionDxe/ElTorito.c
    //
    Media->BlockSize = 2048;
    Media->LastBlock = DivU64x32 (Dev->MediaInfo.Sectors,
                                  Media->BlockSize / Dev->MediaInfo.SectorSize) - 1;
  } else {
    Media->BlockSize = Dev->MediaInfo.SectorSize;
    Media->LastBlock = Dev->MediaInfo.Sectors - 1;
  }
  ASSERT (Media->BlockSize % 512 == 0);
  Dev->BlockIo.Media = Media;

  Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gEfiBlockIoProtocolGuid, &Dev->BlockIo,
                    NULL
                    );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "XenPvBlk: install protocol fail: %r\n", Status));
    goto UninitBlockFront;
  }

  return EFI_SUCCESS;

UninitBlockFront:
  FreePool (Media);
  XenPvBlockFrontShutdown (Dev);
CloseProtocol:
  gBS->CloseProtocol (ControllerHandle, &gXenBusProtocolGuid,
                      This->DriverBindingHandle, ControllerHandle);
  return Status;
}

/**
  Stops a device controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
XenPvBlkDxeDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  XEN_BLOCK_FRONT_DEVICE *Dev;
  EFI_BLOCK_IO_MEDIA *Media;
  EFI_STATUS Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle, &gEfiBlockIoProtocolGuid,
                  (VOID **)&BlockIo,
                  This->DriverBindingHandle, ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->UninstallProtocolInterface (ControllerHandle,
                  &gEfiBlockIoProtocolGuid, BlockIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Media = BlockIo->Media;
  Dev = XEN_BLOCK_FRONT_FROM_BLOCK_IO (BlockIo);
  XenPvBlockFrontShutdown (Dev);

  FreePool (Media);

  gBS->CloseProtocol (ControllerHandle, &gXenBusProtocolGuid,
         This->DriverBindingHandle, ControllerHandle);

  return EFI_SUCCESS;
}
