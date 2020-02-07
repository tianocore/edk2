/** @file
  Driver Binding functions and Service Binding functions
  implementation for Mtftp6 Driver.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Mtftp6Impl.h"


EFI_DRIVER_BINDING_PROTOCOL   gMtftp6DriverBinding = {
  Mtftp6DriverBindingSupported,
  Mtftp6DriverBindingStart,
  Mtftp6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL  gMtftp6ServiceBindingTemplate = {
  Mtftp6ServiceBindingCreateChild,
  Mtftp6ServiceBindingDestroyChild
};


/**
  Destroy the MTFTP6 service. The MTFTP6 service may be partly initialized,
  or partly destroyed. If a resource is destroyed, it is marked as such in
  case the destroy failed and is called again later.

  @param[in]  Service            The MTFTP6 service to be destroyed.

**/
VOID
Mtftp6DestroyService (
  IN MTFTP6_SERVICE     *Service
  )
{
  //
  // Make sure all children instances have been already destroyed.
  //
  ASSERT (Service->ChildrenNum == 0);

  if (Service->DummyUdpIo != NULL) {
    UdpIoFreeIo (Service->DummyUdpIo);
  }

  if (Service->Timer != NULL) {
    gBS->CloseEvent (Service->Timer);
  }

  FreePool (Service);
}


/**
  Create then initialize a MTFTP6 service binding instance.

  @param[in]  Controller             The controller to install the MTFTP6 service
                                     binding on.
  @param[in]  Image                  The driver binding image of the MTFTP6 driver.
  @param[out] Service                The variable to receive the created service
                                     binding instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources to create the instance
  @retval EFI_DEVICE_ERROR       Failed to create a NULL UDP port to keep connection with UDP.
  @retval EFI_SUCCESS            The service instance is created for the controller.

**/
EFI_STATUS
Mtftp6CreateService (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            Image,
  OUT MTFTP6_SERVICE        **Service
  )
{
  MTFTP6_SERVICE            *Mtftp6Srv;
  EFI_STATUS                Status;

  ASSERT (Service != NULL);

  *Service  = NULL;
  Mtftp6Srv = AllocateZeroPool (sizeof (MTFTP6_SERVICE));

  if (Mtftp6Srv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Mtftp6Srv->Signature      = MTFTP6_SERVICE_SIGNATURE;
  Mtftp6Srv->Controller     = Controller;
  Mtftp6Srv->Image          = Image;
  Mtftp6Srv->ChildrenNum    = 0;

  CopyMem (
    &Mtftp6Srv->ServiceBinding,
    &gMtftp6ServiceBindingTemplate,
    sizeof (EFI_SERVICE_BINDING_PROTOCOL)
    );

  InitializeListHead (&Mtftp6Srv->Children);

  //
  // Create a internal timer for all instances.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  Mtftp6OnTimerTick,
                  Mtftp6Srv,
                  &Mtftp6Srv->Timer
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Mtftp6Srv);
    return Status;
  }

  //
  // Create a dummy Udp6Io to build parent-child relationship between Udp6 driver
  // and Mtftp6 driver.
  //
  Mtftp6Srv->DummyUdpIo = UdpIoCreateIo (
                            Controller,
                            Image,
                            Mtftp6ConfigDummyUdpIo,
                            UDP_IO_UDP6_VERSION,
                            NULL
                            );

  if (Mtftp6Srv->DummyUdpIo == NULL) {
    gBS->CloseEvent (Mtftp6Srv->Timer);
    FreePool (Mtftp6Srv);
    return EFI_DEVICE_ERROR;
  }

  *Service = Mtftp6Srv;
  return EFI_SUCCESS;
}


/**
  Destroy the MTFTP6 instance and recycle the resources.

  @param[in]  Instance        The pointer to the MTFTP6 instance.

**/
VOID
Mtftp6DestroyInstance (
  IN MTFTP6_INSTANCE         *Instance
  )
{
  LIST_ENTRY                 *Entry;
  LIST_ENTRY                 *Next;
  MTFTP6_BLOCK_RANGE         *Block;

  if (Instance->Config != NULL) {
    FreePool (Instance->Config);
  }

  if (Instance->Token != NULL && Instance->Token->Event != NULL) {
    gBS->SignalEvent (Instance->Token->Event);
  }

  if (Instance->LastPacket != NULL) {
    NetbufFree (Instance->LastPacket);
  }

  if (Instance->UdpIo!= NULL) {
    UdpIoFreeIo (Instance->UdpIo);
  }

  if (Instance->McastUdpIo != NULL) {
    UdpIoFreeIo (Instance->McastUdpIo);
  }

  NET_LIST_FOR_EACH_SAFE (Entry, Next, &Instance->BlkList) {
    Block = NET_LIST_USER_STRUCT (Entry, MTFTP6_BLOCK_RANGE, Link);
    RemoveEntryList (Entry);
    FreePool (Block);
  }

  FreePool (Instance);
}


/**
  Create the MTFTP6 instance and initialize it.

  @param[in]  Service              The pointer to the MTFTP6 service.
  @param[out] Instance             The pointer to the MTFTP6 instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The MTFTP6 instance is created.

**/
EFI_STATUS
Mtftp6CreateInstance (
  IN  MTFTP6_SERVICE         *Service,
  OUT MTFTP6_INSTANCE        **Instance
  )
{
  MTFTP6_INSTANCE            *Mtftp6Ins;

  *Instance = NULL;
  Mtftp6Ins = AllocateZeroPool (sizeof (MTFTP6_INSTANCE));

  if (Mtftp6Ins == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Mtftp6Ins->Signature = MTFTP6_INSTANCE_SIGNATURE;
  Mtftp6Ins->InDestroy = FALSE;
  Mtftp6Ins->Service   = Service;

  CopyMem (
    &Mtftp6Ins->Mtftp6,
    &gMtftp6ProtocolTemplate,
    sizeof (EFI_MTFTP6_PROTOCOL)
    );

  InitializeListHead (&Mtftp6Ins->Link);
  InitializeListHead (&Mtftp6Ins->BlkList);

  *Instance = Mtftp6Ins;

  return EFI_SUCCESS;
}


/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
Mtftp6DestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  MTFTP6_INSTANCE               *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if (Entry == NULL || Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = NET_LIST_USER_STRUCT_S (Entry, MTFTP6_INSTANCE, Link, MTFTP6_INSTANCE_SIGNATURE);
  ServiceBinding    = ((MTFTP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ServiceBinding;
  NumberOfChildren  = ((MTFTP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->NumberOfChildren;
  ChildHandleBuffer = ((MTFTP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *) Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Instance->Handle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
}


/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers, including
  both device drivers and bus drivers.

  Entry point of the MTFTP6 driver to install various protocols.

  @param[in]  ImageHandle           The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable           The pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gMtftp6DriverBinding,
           ImageHandle,
           &gMtftp6ComponentName,
           &gMtftp6ComponentName2
           );
}


/**
  Test to see if this driver supports Controller. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported(), it must also follow these calling restrictions.

  @param[in]  This                Protocol instance pointer.
  @param[in]  Controller          Handle of device to test
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child.
                                  device to start.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval Others              This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  )
{
  return gBS->OpenProtocol (
                Controller,
                &gEfiUdp6ServiceBindingProtocolGuid,
                NULL,
                This->DriverBindingHandle,
                Controller,
                EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );
}


/**
  Start this driver on Controller. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  Controller           Handle of device to bind driver to.
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver is added to Controller.
  @retval EFI_ALREADY_STARTED  This driver is already running on Controller.
  @retval Others               This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  MTFTP6_SERVICE            *Service;
  EFI_STATUS                Status;

  //
  // Directly return if driver is already running on this Nic handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiMtftp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Create Mtftp6 service for this Nic handle
  //
  Status = Mtftp6CreateService (
             Controller,
             This->DriverBindingHandle,
             &Service
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Service != NULL);

  //
  // Start the internal timer to track the packet retransmission.
  //
  Status = gBS->SetTimer (
                  Service->Timer,
                  TimerPeriodic,
                  TICKS_PER_SECOND
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the Mtftp6 service on the Nic handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiMtftp6ServiceBindingProtocolGuid,
                  &Service->ServiceBinding,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:

  Mtftp6DestroyService (Service);
  return Status;
}


/**
  Stop this driver on Controller. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop(), it must also follow these calling restrictions.

  @param[in]  This              Protocol instance pointer.
  @param[in]  Controller        Handle of device to stop driver on
  @param[in]  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                                children is zero, stop the entire bus driver.
  @param[in]  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed Controller.
  @retval EFI_DEVICE_ERROR  An unexpected error.
  @retval Others            This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
Mtftp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL *This,
  IN  EFI_HANDLE                  Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_SERVICE_BINDING_PROTOCOL               *ServiceBinding;
  MTFTP6_SERVICE                             *Service;
  EFI_HANDLE                                 NicHandle;
  EFI_STATUS                                 Status;
  LIST_ENTRY                                 *List;
  MTFTP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT Context;

  //
  // Locate the Nic handle to retrieve the Mtftp6 private data.
  //
  NicHandle = NetLibGetNicHandle (Controller, &gEfiUdp6ProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiMtftp6ServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Service = MTFTP6_SERVICE_FROM_THIS (ServiceBinding);

  if (!IsListEmpty (&Service->Children)) {
    //
    // Destroy the Mtftp6 child instance in ChildHandleBuffer.
    //
    List = &Service->Children;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status = NetDestroyLinkList (
               List,
               Mtftp6DestroyChildEntryInHandleBuffer,
               &Context,
               NULL
               );
  }

  if (NumberOfChildren == 0 && IsListEmpty (&Service->Children)) {
    //
    // Destroy the Mtftp6 service if there is no Mtftp6 child instance left.
    //
    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiMtftp6ServiceBindingProtocolGuid,
           ServiceBinding
           );

    Mtftp6DestroyService (Service);
    Status = EFI_SUCCESS;
  }

  return Status;
}


/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in]      This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in, out] ChildHandle Pointer to the handle of the child to create. If it is NULL,
                              then a new handle is created. If it is a pointer to an existing
                              UEFI handle, then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval Others                The child handle was not created.

**/
EFI_STATUS
EFIAPI
Mtftp6ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
{
  MTFTP6_SERVICE            *Service;
  MTFTP6_INSTANCE           *Instance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  VOID                      *Udp6;

  if (This == NULL || ChildHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Service = MTFTP6_SERVICE_FROM_THIS (This);

  Status = Mtftp6CreateInstance (Service, &Instance);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Instance != NULL);

  //
  // Install the Mtftp6 protocol on the new child handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiMtftp6ProtocolGuid,
                  &Instance->Mtftp6,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->Handle = *ChildHandle;

  //
  // Open the Udp6 protocol by child.
  //
  Status = gBS->OpenProtocol (
                  Service->DummyUdpIo->UdpHandle,
                  &gEfiUdp6ProtocolGuid,
                  (VOID **) &Udp6,
                  gMtftp6DriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->Handle,
           &gEfiMtftp6ProtocolGuid,
           &Instance->Mtftp6,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Add the new Mtftp6 instance into the children list of Mtftp6 service.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&Service->Children, &Instance->Link);
  Service->ChildrenNum++;

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

ON_ERROR:

  Mtftp6DestroyInstance (Instance);
  return Status;
}


/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in]  ChildHandle Handle of the child to destroy.

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval Others                The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Mtftp6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                   ChildHandle
  )
{
  MTFTP6_SERVICE            *Service;
  MTFTP6_INSTANCE           *Instance;
  EFI_MTFTP6_PROTOCOL       *Mtftp6;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;

  if (This == NULL || ChildHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Locate the Nic handle to retrieve the Mtftp6 private data.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiMtftp6ProtocolGuid,
                  (VOID **) &Mtftp6,
                  gMtftp6DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = MTFTP6_INSTANCE_FROM_THIS (Mtftp6);
  Service  = MTFTP6_SERVICE_FROM_THIS (This);

  if (Instance->Service != Service) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check whether the instance already in Destroy state.
  //
  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->InDestroy = TRUE;

  gBS->CloseProtocol (
         Service->DummyUdpIo->UdpHandle,
         &gEfiUdp6ProtocolGuid,
         gMtftp6DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  if (Instance->UdpIo != NULL) {
    gBS->CloseProtocol (
         Instance->UdpIo->UdpHandle,
         &gEfiUdp6ProtocolGuid,
         gMtftp6DriverBinding.DriverBindingHandle,
         Instance->Handle
         );
  }

  if (Instance->McastUdpIo != NULL) {
    gBS->CloseProtocol (
           Instance->McastUdpIo->UdpHandle,
           &gEfiUdp6ProtocolGuid,
           gMtftp6DriverBinding.DriverBindingHandle,
           Instance->Handle
           );
  }

  //
  // Uninstall the MTFTP6 protocol first to enable a top down destruction.
  //
  gBS->RestoreTPL (OldTpl);
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiMtftp6ProtocolGuid,
                  Mtftp6
                  );
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  // Remove the Mtftp6 instance from the children list of Mtftp6 service.
  //
  RemoveEntryList (&Instance->Link);
  Service->ChildrenNum --;

  gBS->RestoreTPL (OldTpl);

  Mtftp6DestroyInstance (Instance);

  return EFI_SUCCESS;
}
