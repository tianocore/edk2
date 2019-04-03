/** @file
  Driver Binding functions and Service Binding functions for the Network driver module.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Udp6Impl.h"

EFI_DRIVER_BINDING_PROTOCOL gUdp6DriverBinding = {
  Udp6DriverBindingSupported,
  Udp6DriverBindingStart,
  Udp6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL mUdp6ServiceBinding = {
  Udp6ServiceBindingCreateChild,
  Udp6ServiceBindingDestroyChild
};

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
Udp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  EFI_STATUS  Status;
  //
  // Test for the Udp6ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUdp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }
  //
  // Test for the Ip6ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}

/**
  Start this driver on ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to bind the driver to.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCES             This driver is added to ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES   The required system resource can't be allocated.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Udp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  EFI_STATUS         Status;
  UDP6_SERVICE_DATA  *Udp6Service;

  //
  // Allocate Private Context Data Structure.
  //
  Udp6Service = AllocateZeroPool (sizeof (UDP6_SERVICE_DATA));
  if (Udp6Service == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  Status = Udp6CreateService (Udp6Service, This->DriverBindingHandle, ControllerHandle);
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Install the Udp6ServiceBindingProtocol on the ControllerHandle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiUdp6ServiceBindingProtocolGuid,
                  &Udp6Service->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    Udp6CleanService (Udp6Service);
  }

EXIT:
  if (EFI_ERROR (Status)) {
    if (Udp6Service != NULL) {
      FreePool (Udp6Service);
    }
  }
  return Status;
}

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_INVALID_PARAMETER  Entry is NULL or Context is NULL.
  @retval EFI_SUCCESS            The entry has been removed successfully.
  @retval Others                 Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
Udp6DestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  UDP6_INSTANCE_DATA            *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if (Entry == NULL || Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = NET_LIST_USER_STRUCT_S (Entry, UDP6_INSTANCE_DATA, Link, UDP6_INSTANCE_DATA_SIGNATURE);
  ServiceBinding    = ((UDP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ServiceBinding;
  NumberOfChildren  = ((UDP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->NumberOfChildren;
  ChildHandleBuffer = ((UDP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Instance->ChildHandle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Instance->ChildHandle);
}

/**
  Stop this driver on ControllerHandle.

  This service is called by the  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop(), it must also follow these calling restrictions.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to stop the driver on.
  @param[in]  NumberOfChildren       Number of Handles in ChildHandleBuffer. If the number
                                     of children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer      List of Child Handles to Stop. It is optional.

  @retval EFI_SUCCES             This driver is removed ControllerHandle.
  @retval EFI_DEVICE_ERROR       Can't find the NicHandle from the ControllerHandle and specified GUID.
  @retval other                  This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
Udp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                                Status;
  EFI_HANDLE                                NicHandle;
  EFI_SERVICE_BINDING_PROTOCOL              *ServiceBinding;
  UDP6_SERVICE_DATA                         *Udp6Service;
  LIST_ENTRY                                *List;
  UDP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  //
  // Find the NicHandle where UDP6 ServiceBinding Protocol is installed.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiIp6ProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Retrieve the UDP6 ServiceBinding Protocol.
  //
  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiUdp6ServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Udp6Service = UDP6_SERVICE_DATA_FROM_THIS (ServiceBinding);

  if (NumberOfChildren != 0) {
    //
    // NumberOfChildren is not zero, destroy the children instances in ChildHandleBuffer.
    //
    List = &Udp6Service->ChildrenList;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status = NetDestroyLinkList (
               List,
               Udp6DestroyChildEntryInHandleBuffer,
               &Context,
               NULL
               );
  } else if (IsListEmpty (&Udp6Service->ChildrenList)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
               NicHandle,
               &gEfiUdp6ServiceBindingProtocolGuid,
               &Udp6Service->ServiceBinding,
               NULL
               );

    Udp6CleanService (Udp6Service);
    FreePool (Udp6Service);
  }

  return Status;
}

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in]       This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in, out]  ChildHandle Pointer to the handle of the child to create. If it is NULL,
                               then a new handle is created. If it is a pointer to an existing UEFI handle,
                               then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER This is NULL or ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
Udp6ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                *ChildHandle
  )
{
  EFI_STATUS          Status;
  UDP6_SERVICE_DATA   *Udp6Service;
  UDP6_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;
  VOID                *Ip6;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Udp6Service = UDP6_SERVICE_DATA_FROM_THIS (This);

  //
  // Allocate the instance private data structure.
  //
  Instance = AllocateZeroPool (sizeof (UDP6_INSTANCE_DATA));
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Udp6InitInstance (Udp6Service, Instance);

  //
  // Add an IpInfo for this instance.
  //
  Instance->IpInfo = IpIoAddIp (Udp6Service->IpIo);
  if (Instance->IpInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Install the Udp6Protocol for this instance.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiUdp6ProtocolGuid,
                  &Instance->Udp6Proto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->ChildHandle = *ChildHandle;

  //
  // Open the default Ip6 protocol in the IP_IO BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  Udp6Service->IpIo->ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  (VOID **) &Ip6,
                  gUdp6DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Open this instance's Ip6 protocol in the IpInfo BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  Instance->IpInfo->ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  (VOID **) &Ip6,
                  gUdp6DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Link this instance into the service context data and increase the ChildrenNumber.
  //
  InsertTailList (&Udp6Service->ChildrenList, &Instance->Link);
  Udp6Service->ChildrenNumber++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:

  if (Instance->ChildHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiUdp6ProtocolGuid,
           &Instance->Udp6Proto,
           NULL
           );
  }

  if (Instance->IpInfo != NULL) {
    IpIoRemoveIp (Udp6Service->IpIo, Instance->IpInfo);
  }

  Udp6CleanInstance (Instance);

  FreePool (Instance);

  return Status;
}

/**
  Destroys a child handle with a set of I/O services.
  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                 that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is NULL.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Udp6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS          Status;
  UDP6_SERVICE_DATA   *Udp6Service;
  EFI_UDP6_PROTOCOL   *Udp6Proto;
  UDP6_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Udp6Service = UDP6_SERVICE_DATA_FROM_THIS (This);

  //
  // Try to get the Udp6 protocol from the ChildHandle.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiUdp6ProtocolGuid,
                  (VOID **) &Udp6Proto,
                  gUdp6DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = UDP6_INSTANCE_DATA_FROM_THIS (Udp6Proto);

  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  //
  // Use the Destroyed flag to avoid the re-entering of the following code.
  //
  Instance->InDestroy = TRUE;

  //
  // Close the Ip6 protocol on the default IpIo.
  //
  Status = gBS->CloseProtocol (
             Udp6Service->IpIo->ChildHandle,
             &gEfiIp6ProtocolGuid,
             gUdp6DriverBinding.DriverBindingHandle,
             Instance->ChildHandle
             );
  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    return Status;
  }

  //
  // Close the Ip6 protocol on this instance's IpInfo.
  //
  Status = gBS->CloseProtocol (
             Instance->IpInfo->ChildHandle,
             &gEfiIp6ProtocolGuid,
             gUdp6DriverBinding.DriverBindingHandle,
             Instance->ChildHandle
             );
  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    return Status;
  }

  //
  // Uninstall the Udp6Protocol previously installed on the ChildHandle.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiUdp6ProtocolGuid,
                  (VOID *) &Instance->Udp6Proto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    return Status;
  }

  //
  // Reset the configuration in case the instance's consumer forgets to do this.
  //
  Udp6Proto->Configure (Udp6Proto, NULL);

  //
  // Remove the IpInfo this instance consumes.
  //
  IpIoRemoveIp (Udp6Service->IpIo, Instance->IpInfo);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Remove this instance from the service context data's ChildrenList.
  //
  RemoveEntryList (&Instance->Link);
  Udp6Service->ChildrenNumber--;

  //
  // Clean the instance.
  //
  Udp6CleanInstance (Instance);

  gBS->RestoreTPL (OldTpl);

  FreePool (Instance);

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers, including
  both device drivers and bus drivers.

  The entry point for Udp6 driver that installs the driver binding
  and component name protocol on its ImageHandle.

  @param[in] ImageHandle        The firmware allocated handle for the UEFI image.
  @param[in] SystemTable        A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
Udp6DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install the Udp6DriverBinding and Udp6ComponentName protocols.
  //

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUdp6DriverBinding,
             ImageHandle,
             &gUdp6ComponentName,
             &gUdp6ComponentName2
             );
  if (!EFI_ERROR (Status)) {
    //
    // Initialize the UDP random port.
    //
    mUdp6RandomPort = (UINT16)(
                        ((UINT16) NetRandomInitSeed ()) %
                         UDP6_PORT_KNOWN +
                         UDP6_PORT_KNOWN
                         );
  }

  return Status;
}


