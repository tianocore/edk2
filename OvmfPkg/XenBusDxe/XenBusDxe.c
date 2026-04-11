/** @file
  This driver produces XenBus Protocol instances for each Xen PV devices.

  This XenBus bus driver will first initialize different services in order to
  enumerate the ParaVirtualized devices available.

  Those services are:
    - HyperCall
    - Event Channel
    - Grant Table
    - XenStore
    - XenBus

  Copyright (C) 2014, Citrix Ltd.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/XenHypercallLib.h>

#include "XenBusDxe.h"

#include "GrantTable.h"
#include "XenStore.h"
#include "XenBus.h"

#include <IndustryStandard/Xen/hvm/params.h>
#include <IndustryStandard/Xen/memory.h>

///
/// Driver Binding Protocol instance
///
EFI_DRIVER_BINDING_PROTOCOL  gXenBusDxeDriverBinding = {
  XenBusDxeDriverBindingSupported,
  XenBusDxeDriverBindingStart,
  XenBusDxeDriverBindingStop,
  XENBUS_DXE_VERSION,
  NULL,
  NULL
};

STATIC EFI_LOCK       mMyDeviceLock = EFI_INITIALIZE_LOCK_VARIABLE (TPL_CALLBACK);
STATIC XENBUS_DEVICE  *mMyDevice    = NULL;

/**
  Map the shared_info_t page into memory.

  @param Dev    A XENBUS_DEVICE instance.

  @retval EFI_SUCCESS     Dev->SharedInfo whill contain a pointer to
                          the shared info page
  @retval EFI_LOAD_ERROR  The shared info page could not be mapped. The
                          hypercall returned an error.
**/
STATIC
EFI_STATUS
XenGetSharedInfoPage (
  IN OUT XENBUS_DEVICE  *Dev
  )
{
  xen_add_to_physmap_t  Parameter;

  ASSERT (Dev->SharedInfo == NULL);

  Parameter.domid = DOMID_SELF;
  Parameter.space = XENMAPSPACE_shared_info;
  Parameter.idx   = 0;

  //
  // using reserved page because the page is not released when Linux is
  // starting because of the add_to_physmap. QEMU might try to access the
  // page, and fail because it have no right to do so (segv).
  //
  Dev->SharedInfo = AllocateReservedPages (1);
  Parameter.gpfn  = (UINTN)Dev->SharedInfo >> EFI_PAGE_SHIFT;
  if (XenHypercallMemoryOp (XENMEM_add_to_physmap, &Parameter) != 0) {
    FreePages (Dev->SharedInfo, 1);
    Dev->SharedInfo = NULL;
    return EFI_LOAD_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
XenBusDxeUnload (
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
                  &gEfiDriverBindingProtocolGuid,
                  &gXenBusDxeDriverBinding,
                  &gEfiComponentNameProtocolGuid,
                  &gXenBusDxeComponentName,
                  &gEfiComponentName2ProtocolGuid,
                  &gXenBusDxeComponentName2,
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
  @retval EFI_ABORTED           Xen hypercalls are not available.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
XenBusDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (!XenHypercallIsAvailable ()) {
    return EFI_ABORTED;
  }

  //
  // Install UEFI Driver Model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gXenBusDxeDriverBinding,
             ImageHandle,
             &gXenBusDxeComponentName,
             &gXenBusDxeComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

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
XenBusDxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS      Status;
  XENIO_PROTOCOL  *XenIo;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gXenIoProtocolGuid,
                  (VOID **)&XenIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  gBS->CloseProtocol (
         ControllerHandle,
         &gXenIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}

VOID
EFIAPI
NotifyExitBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  XENBUS_DEVICE  *Dev = Context;

  gBS->DisconnectController (
         Dev->ControllerHandle,
         Dev->This->DriverBindingHandle,
         NULL
         );
}

/**
  Starts a bus controller.

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
  @retval EFI_UNSUPPORTED          Something is missing on the system that
                                   prevent to start the device.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
XenBusDxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  XENBUS_DEVICE             *Dev;
  XENIO_PROTOCOL            *XenIo;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gXenIoProtocolGuid,
                  (VOID **)&XenIo,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ErrorOpenningProtocol;
  }

  Dev                   = AllocateZeroPool (sizeof (*Dev));
  Dev->Signature        = XENBUS_DEVICE_SIGNATURE;
  Dev->This             = This;
  Dev->ControllerHandle = ControllerHandle;
  Dev->XenIo            = XenIo;
  Dev->DevicePath       = DevicePath;
  InitializeListHead (&Dev->ChildList);

  EfiAcquireLock (&mMyDeviceLock);
  if (mMyDevice != NULL) {
    EfiReleaseLock (&mMyDeviceLock);
    //
    // There is already a XenBus running, only one can be used at a time.
    //
    Status = EFI_ALREADY_STARTED;
    goto ErrorAllocated;
  }

  mMyDevice = Dev;
  EfiReleaseLock (&mMyDeviceLock);

  Status = XenGetSharedInfoPage (Dev);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "XenBus: Unable to get the shared info page.\n"));
    Status = EFI_UNSUPPORTED;
    goto ErrorAllocated;
  }

  XenGrantTableInit (Dev);

  Status = XenStoreInit (Dev);
  ASSERT_EFI_ERROR (Status);

  XenBusEnumerateBus (Dev);

  Status = gBS->CreateEvent (
                  EVT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_CALLBACK,
                  NotifyExitBoot,
                  (VOID *)Dev,
                  &Dev->ExitBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;

ErrorAllocated:
  FreePool (Dev);
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
ErrorOpenningProtocol:
  gBS->CloseProtocol (
         ControllerHandle,
         &gXenIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  return Status;
}

/**
  Stops a bus controller.

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
XenBusDxeDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  UINTN                Index;
  XENBUS_PROTOCOL      *XenBusIo;
  XENBUS_PRIVATE_DATA  *ChildData;
  EFI_STATUS           Status;
  XENBUS_DEVICE        *Dev = mMyDevice;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[Index],
                    &gXenBusProtocolGuid,
                    (VOID **)&XenBusIo,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "XenBusDxe: get children protocol failed: %r\n", Status));
      continue;
    }

    ChildData = XENBUS_PRIVATE_DATA_FROM_THIS (XenBusIo);

    Status = gBS->CloseProtocol (
                    Dev->ControllerHandle,
                    &gXenIoProtocolGuid,
                    Dev->This->DriverBindingHandle,
                    ChildData->Handle
                    );
    ASSERT_EFI_ERROR (Status);

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildData->Handle,
                    &gEfiDevicePathProtocolGuid,
                    ChildData->DevicePath,
                    &gXenBusProtocolGuid,
                    &ChildData->XenBusIo,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    FreePool ((VOID *)ChildData->XenBusIo.Type);
    FreePool ((VOID *)ChildData->XenBusIo.Node);
    FreePool ((VOID *)ChildData->XenBusIo.Backend);
    FreePool (ChildData->DevicePath);
    RemoveEntryList (&ChildData->Link);
    FreePool (ChildData);
  }

  if (NumberOfChildren > 0) {
    return EFI_SUCCESS;
  }

  gBS->CloseEvent (Dev->ExitBootEvent);
  XenStoreDeinit (Dev);
  XenGrantTableDeinit (Dev);

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );
  gBS->CloseProtocol (
         ControllerHandle,
         &gXenIoProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  mMyDevice = NULL;
  FreePool (Dev);
  return EFI_SUCCESS;
}
