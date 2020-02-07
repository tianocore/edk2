/** @file
  This file consumes the ISA Host Controller protocol produced by the ISA Host
  Controller and installs the ISA Host Controller Service Binding protocol
  on the ISA Host Controller's handle.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "IsaBusDxe.h"
#include "ComponentName.h"

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
IsaBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  VOID                              *Instance;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaHcProtocolGuid,
                  &Instance,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
      Controller,
      &gEfiIsaHcProtocolGuid,
      This->DriverBindingHandle,
      Controller
      );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  &Instance,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
      Controller,
      &gEfiDevicePathProtocolGuid,
      This->DriverBindingHandle,
      Controller
      );
  }

  return Status;
}

ISA_BUS_CHILD_PRIVATE_DATA mIsaBusChildPrivateTemplate = {
  ISA_BUS_CHILD_PRIVATE_DATA_SIGNATURE,
  FALSE
};

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Pointer to the handle of the child to create. If it is NULL,
                      then a new handle is created. If it is a pointer to an existing UEFI handle,
                      then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
IsaBusCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
{
  EFI_STATUS                           Status;
  ISA_BUS_PRIVATE_DATA                 *Private;
  EFI_ISA_HC_PROTOCOL                  *IsaHc;
  ISA_BUS_CHILD_PRIVATE_DATA           *Child;

  Private = ISA_BUS_PRIVATE_DATA_FROM_THIS (This);

  Child = AllocateCopyPool (sizeof (mIsaBusChildPrivateTemplate), &mIsaBusChildPrivateTemplate);
  if (Child == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiIsaHcProtocolGuid, Private->IsaHc,
                  &gEfiCallerIdGuid,      Child,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Child);
    return Status;
  }

  return gBS->OpenProtocol (
                Private->IsaHcHandle,
                &gEfiIsaHcProtocolGuid,
                (VOID **) &IsaHc,
                gIsaBusDriverBinding.DriverBindingHandle,
                *ChildHandle,
                EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                );
}

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
IsaBusDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                       ChildHandle
  )
{
  EFI_STATUS                           Status;
  ISA_BUS_PRIVATE_DATA                 *Private;
  EFI_ISA_HC_PROTOCOL                  *IsaHc;
  ISA_BUS_CHILD_PRIVATE_DATA           *Child;

  Private = ISA_BUS_PRIVATE_DATA_FROM_THIS (This);

  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Child,
                  gIsaBusDriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Child->Signature == ISA_BUS_CHILD_PRIVATE_DATA_SIGNATURE);

  if (Child->InDestroying) {
    return EFI_SUCCESS;
  }

  Child->InDestroying = TRUE;
  Status = gBS->CloseProtocol (
                  Private->IsaHcHandle,
                  &gEfiIsaHcProtocolGuid,
                  gIsaBusDriverBinding.DriverBindingHandle,
                  ChildHandle
                  );
  ASSERT_EFI_ERROR (Status);
  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    ChildHandle,
                    &gEfiIsaHcProtocolGuid, Private->IsaHc,
                    &gEfiCallerIdGuid,      Child,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      gBS->OpenProtocol (
             Private->IsaHcHandle,
             &gEfiIsaHcProtocolGuid,
             (VOID **) &IsaHc,
             gIsaBusDriverBinding.DriverBindingHandle,
             ChildHandle,
             EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
             );
    }
  }

  if (EFI_ERROR (Status)) {
    Child->InDestroying = FALSE;
  } else {
    FreePool (Child);
  }

  return Status;
}

ISA_BUS_PRIVATE_DATA   mIsaBusPrivateTemplate = {
  ISA_BUS_PRIVATE_DATA_SIGNATURE,
  {
    IsaBusCreateChild,
    IsaBusDestroyChild
  }
};

/**
  Starts a device controller or a bus controller.

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
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
IsaBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  ISA_BUS_PRIVATE_DATA              *Private;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaHcProtocolGuid,
                  (VOID **) &mIsaBusPrivateTemplate.IsaHc,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiIsaHcProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
    return Status;
  }

  Private = AllocateCopyPool (sizeof (mIsaBusPrivateTemplate), &mIsaBusPrivateTemplate);
  ASSERT (Private != NULL);

  Private->IsaHcHandle = Controller;

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiIsaHcServiceBindingProtocolGuid, &Private->ServiceBinding,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Stops a device controller or a bus controller.

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
IsaBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
{
  EFI_STATUS                         Status;
  EFI_SERVICE_BINDING_PROTOCOL       *ServiceBinding;
  ISA_BUS_PRIVATE_DATA               *Private;
  UINTN                              Index;
  BOOLEAN                            AllChildrenStopped;

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiIsaHcServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = ISA_BUS_PRIVATE_DATA_FROM_THIS (ServiceBinding);

  if (NumberOfChildren == 0) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiIsaHcServiceBindingProtocolGuid, &Private->ServiceBinding,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      gBS->CloseProtocol (
             Controller,
             &gEfiDevicePathProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      gBS->CloseProtocol (
             Controller,
             &gEfiIsaHcProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
      FreePool (Private);
    }

    return Status;
  }

  AllChildrenStopped = TRUE;
  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = ServiceBinding->DestroyChild (ServiceBinding, ChildHandleBuffer[Index]);
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  return AllChildrenStopped ? EFI_SUCCESS : EFI_DEVICE_ERROR;
}

//
// ISA Bus Driver Binding Protocol Instance
//
EFI_DRIVER_BINDING_PROTOCOL gIsaBusDriverBinding = {
  IsaBusDriverBindingSupported,
  IsaBusDriverBindingStart,
  IsaBusDriverBindingStop,
  0x10,
  NULL,
  NULL
};

/**
  Entry point of the IsaBusDxe driver.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.
**/
EFI_STATUS
EFIAPI
InitializeIsaBus (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIsaBusDriverBinding,
             ImageHandle,
             &gIsaBusComponentName,
             &gIsaBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
