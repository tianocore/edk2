/** @file

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Udp4Driver.c

Abstract:


**/


#include "Udp4Impl.h"

EFI_DRIVER_BINDING_PROTOCOL gUdp4DriverBinding = {
  Udp4DriverBindingSupported,
  Udp4DriverBindingStart,
  Udp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL mUdp4ServiceBinding = {
  Udp4ServiceBindingCreateChild,
  Udp4ServiceBindingDestroyChild
};


/**
  Test to see if this driver supports ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to test.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver supports this device.
  @retval EFI_ALREADY_STARTED    This driver is already running on this device.

**/
EFI_STATUS
EFIAPI
Udp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Test for the Udp4ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test for the Ip4 Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}


/**
  Start this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to bind driver to
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED    This driver is already running on ControllerHandle
  @retval other                  This driver does not support this device

**/
EFI_STATUS
EFIAPI
Udp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  EFI_STATUS         Status;
  UDP4_SERVICE_DATA  *Udp4Service;

  //
  // Allocate Private Context Data Structure.
  //
  Udp4Service = NetAllocatePool (sizeof (UDP4_SERVICE_DATA));
  if (Udp4Service == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Udp4CreateService (Udp4Service, This->DriverBindingHandle, ControllerHandle);
  if (EFI_ERROR (Status)) {
    goto FREE_SERVICE;
  }

  //
  // Install the Udp4ServiceBindingProtocol on the ControllerHandle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  &Udp4Service->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto CLEAN_SERVICE;
  }

  Udp4SetVariableData (Udp4Service);

  return Status;

CLEAN_SERVICE:

  Udp4CleanService (Udp4Service);

FREE_SERVICE:

  NetFreePool (Udp4Service);

  return Status;
}


/**
  Stop this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to stop driver on
  @param  NumberOfChildren       Number of Handles in ChildHandleBuffer. If number
                                 of  children is zero stop the entire bus driver.
  @param  ChildHandleBuffer      List of Child Handles to Stop.

  @retval EFI_SUCCES             This driver is removed ControllerHandle.
  @retval other                  This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
Udp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    NicHandle;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UDP4_SERVICE_DATA             *Udp4Service;
  UDP4_INSTANCE_DATA            *Instance;

  //
  // Find the NicHandle where UDP4 ServiceBinding Protocol is installed.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiIp4ProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Retrieve the UDP4 ServiceBinding Protocol.
  //
  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Udp4Service = UDP4_SERVICE_DATA_FROM_THIS (ServiceBinding);

  //
  // Uninstall the UDP4 ServiceBinding Protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  NicHandle,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  &Udp4Service->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  while (!NetListIsEmpty (&Udp4Service->ChildrenList)) {
    //
    // Destroy all instances.
    //
    Instance = NET_LIST_HEAD (&Udp4Service->ChildrenList, UDP4_INSTANCE_DATA, Link);

    ServiceBinding->DestroyChild (ServiceBinding, Instance->ChildHandle);
  }

  Udp4ClearVariableData (Udp4Service);

  Udp4CleanService (Udp4Service);

  NetFreePool (Udp4Service);

  return EFI_SUCCESS;
}


/**
  Creates a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Pointer to the handle of the child to create.   If
                                 it is NULL, then a new handle is created.   If it
                                 is not NULL, then the I/O services are  added to
                                 the existing child handle.

  @retval EFI_SUCCES             The child handle was created with the I/O services
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources availabe to create
                                 the child
  @retval other                  The child handle was not created

**/
EFI_STATUS
EFIAPI
Udp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  EFI_STATUS          Status;
  UDP4_SERVICE_DATA   *Udp4Service;
  UDP4_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;
  VOID                *Ip4;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Udp4Service = UDP4_SERVICE_DATA_FROM_THIS (This);

  //
  // Allocate the instance private data structure.
  //
  Instance = NetAllocatePool (sizeof (UDP4_INSTANCE_DATA));
  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Udp4InitInstance (Udp4Service, Instance);

  //
  // Add an IpInfo for this instance.
  //
  Instance->IpInfo = IpIoAddIp (Udp4Service->IpIo);
  if (Instance->IpInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FREE_INSTANCE;
  }

  //
  // Install the Udp4Protocol for this instance.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiUdp4ProtocolGuid,
                  &Instance->Udp4Proto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto REMOVE_IPINFO;
  }

  Instance->ChildHandle = *ChildHandle;

  //
  // Open the default Ip4 protocol in the IP_IO BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  Udp4Service->IpIo->ChildHandle,
                  &gEfiIp4ProtocolGuid,
                  (VOID **) &Ip4,
                  gUdp4DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_PROTOCOL;
  }

  OldTpl = NET_RAISE_TPL (NET_TPL_LOCK);

  //
  // Link this instance into the service context data and increase the ChildrenNumber.
  //
  NetListInsertTail (&Udp4Service->ChildrenList, &Instance->Link);
  Udp4Service->ChildrenNumber++;

  NET_RESTORE_TPL (OldTpl);

  return Status;

UNINSTALL_PROTOCOL:

  gBS->UninstallMultipleProtocolInterfaces (
         Instance->ChildHandle,
         &gEfiUdp4ProtocolGuid,
         &Instance->Udp4Proto,
         NULL
         );

REMOVE_IPINFO:

  IpIoRemoveIp (Udp4Service->IpIo, Instance->IpInfo);

FREE_INSTANCE:

  Udp4CleanInstance (Instance);

  NetFreePool (Instance);

  return Status;
}


/**
  Destroys a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Handle of the child to destroy

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed
  @retval EFI_INVALID_PARAMETER  Child handle is not a valid EFI Handle.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Udp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS          Status;
  UDP4_SERVICE_DATA   *Udp4Service;
  EFI_UDP4_PROTOCOL   *Udp4Proto;
  UDP4_INSTANCE_DATA  *Instance;
  EFI_TPL             OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Udp4Service = UDP4_SERVICE_DATA_FROM_THIS (This);

  //
  // Try to get the Udp4 protocol from the ChildHandle.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **) &Udp4Proto,
                  gUdp4DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = UDP4_INSTANCE_DATA_FROM_THIS (Udp4Proto);

  if (Instance->Destroyed) {
    return EFI_SUCCESS;
  }

  //
  // Use the Destroyed flag to avoid the re-entering of the following code.
  //
  Instance->Destroyed = TRUE;

  //
  // Close the Ip4 protocol.
  //
  gBS->CloseProtocol (
         Udp4Service->IpIo->ChildHandle,
         &gEfiIp4ProtocolGuid,
         gUdp4DriverBinding.DriverBindingHandle,
         Instance->ChildHandle
         );

  //
  // Uninstall the Udp4Protocol previously installed on the ChildHandle.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID *) &Instance->Udp4Proto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    Instance->Destroyed = FALSE;
    return Status;
  }

  //
  // Reset the configuration in case the instance's consumer forgets to do this.
  //
  Udp4Proto->Configure (Udp4Proto, NULL);

  //
  // Remove the IpInfo this instance consumes.
  //
  IpIoRemoveIp (Udp4Service->IpIo, Instance->IpInfo);

  OldTpl = NET_RAISE_TPL (NET_TPL_LOCK);

  //
  // Remove this instance from the service context data's ChildrenList.
  //
  NetListRemoveEntry (&Instance->Link);
  Udp4Service->ChildrenNumber--;

  //
  // Clean the instance.
  //
  Udp4CleanInstance (Instance);

  NET_RESTORE_TPL (OldTpl);

  NetFreePool (Instance);

  return EFI_SUCCESS;
}

//@MT: EFI_DRIVER_ENTRY_POINT (Udp4DriverEntryPoint)

EFI_STATUS
EFIAPI
Udp4DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  The entry point for Udp4 driver which installs the driver binding
  and component name protocol on its ImageHandle.

Arguments:

  ImageHandle - The image handle of the driver.
  SystemTable - The system table.

Returns:

  EFI_SUCCESS - if the driver binding and component name protocols are
                successfully installed, otherwise if failed.

--*/
{
  EFI_STATUS  Status;

  //
  // Install the Udp4DriverBinding and Udp4ComponentName protocols.
  //
  Status = NetLibInstallAllDriverProtocols (
             ImageHandle,
             SystemTable,
             &gUdp4DriverBinding,
             ImageHandle,
             &gUdp4ComponentName,
             NULL,
             NULL
             );
  if (!EFI_ERROR (Status)) {
    //
    // Initialize the UDP random port.
    //
    mUdp4RandomPort = ((UINT16) NetRandomInitSeed ()) % UDP4_PORT_KNOWN + UDP4_PORT_KNOWN;
  }

  return Status;
}

