/** @file
  Driver Binding functions and Service Binding functions
  implementationfor for Dhcp6 Driver.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Dhcp6Impl.h"


EFI_DRIVER_BINDING_PROTOCOL gDhcp6DriverBinding = {
  Dhcp6DriverBindingSupported,
  Dhcp6DriverBindingStart,
  Dhcp6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL gDhcp6ServiceBindingTemplate = {
  Dhcp6ServiceBindingCreateChild,
  Dhcp6ServiceBindingDestroyChild
};

/**
  Configure the default Udp6Io to receive all the DHCP6 traffic
  on this network interface.

  @param[in]  UdpIo                  The pointer to Udp6Io to be configured.
  @param[in]  Context                The pointer to the context.

  @retval EFI_SUCCESS            The Udp6Io is successfully configured.
  @retval Others                 Failed to configure the Udp6Io.

**/
EFI_STATUS
EFIAPI
Dhcp6ConfigureUdpIo (
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  )
{
  EFI_UDP6_PROTOCOL         *Udp6;
  EFI_UDP6_CONFIG_DATA      *Config;

  Udp6   = UdpIo->Protocol.Udp6;
  Config = &(UdpIo->Config.Udp6);

  ZeroMem (Config, sizeof (EFI_UDP6_CONFIG_DATA));

  //
  // Set Udp6 configure data for the Dhcp6 instance.
  //
  Config->AcceptPromiscuous  = FALSE;
  Config->AcceptAnyPort      = FALSE;
  Config->AllowDuplicatePort = FALSE;
  Config->TrafficClass       = 0;
  Config->HopLimit           = 128;
  Config->ReceiveTimeout     = 0;
  Config->TransmitTimeout    = 0;

  //
  // Configure an endpoint of client(0, 546), server(0, 0), the addresses
  // will be overridden later. Note that we MUST not limit RemotePort.
  // More details, refer to RFC 3315 section 5.2.
  //
  Config->StationPort        = DHCP6_PORT_CLIENT;
  Config->RemotePort         = 0;

  return Udp6->Configure (Udp6, Config);;
}


/**
  Destroy the Dhcp6 service. The Dhcp6 service may be partly initialized,
  or partly destroyed. If a resource is destroyed, it is marked as such in
  case the destroy failed and being called again later.

  @param[in, out]  Service       The pointer to Dhcp6 service to be destroyed.

**/
VOID
Dhcp6DestroyService (
  IN OUT DHCP6_SERVICE          *Service
  )
{
  //
  // All children instances should have been already destroyed here.
  //
  ASSERT (Service->NumOfChild == 0);

  if (Service->ClientId != NULL) {
    FreePool (Service->ClientId);
  }

  if (Service->UdpIo != NULL) {
    UdpIoFreeIo (Service->UdpIo);
  }

  FreePool (Service);
}


/**
  Create a new Dhcp6 service for the Nic controller.

  @param[in]  Controller             The controller to be installed DHCP6 service
                                     binding protocol.
  @param[in]  ImageHandle            The image handle of the Dhcp6 driver.
  @param[out] Service                The return pointer of the new Dhcp6 service.

  @retval EFI_SUCCESS            The Dhcp6 service is created successfully.
  @retval EFI_DEVICE_ERROR       An unexpected system or network error occurred.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource.

**/
EFI_STATUS
Dhcp6CreateService (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            ImageHandle,
  OUT DHCP6_SERVICE         **Service
  )
{
  DHCP6_SERVICE             *Dhcp6Srv;
  EFI_STATUS                Status;

  *Service = NULL;
  Dhcp6Srv = AllocateZeroPool (sizeof (DHCP6_SERVICE));

  if (Dhcp6Srv == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Open the SNP protocol to get mode data later.
  //
  Dhcp6Srv->Snp = NULL;
  NetLibGetSnpHandle (Controller, &Dhcp6Srv->Snp);
  if (Dhcp6Srv->Snp == NULL) {
    FreePool (Dhcp6Srv);
    return EFI_DEVICE_ERROR;
  }

  //
  // Initialize the fields of the new Dhcp6 service.
  //
  Dhcp6Srv->Signature       = DHCP6_SERVICE_SIGNATURE;
  Dhcp6Srv->Controller      = Controller;
  Dhcp6Srv->Image           = ImageHandle;
  Dhcp6Srv->Xid             = (0xffffff & NET_RANDOM (NetRandomInitSeed ()));

  CopyMem (
    &Dhcp6Srv->ServiceBinding,
    &gDhcp6ServiceBindingTemplate,
    sizeof (EFI_SERVICE_BINDING_PROTOCOL)
    );

  //
  // Locate Ip6->Ip6Config and store it for get IP6 Duplicate Address Detection transmits.
  //
  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiIp6ConfigProtocolGuid,
                  (VOID **) &Dhcp6Srv->Ip6Cfg
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Dhcp6Srv);
    return Status;
  }

  //
  // Generate client Duid: If SMBIOS system UUID is located, generate DUID in DUID-UUID format.
  // Otherwise, in DUID-LLT format.
  //
  Dhcp6Srv->ClientId        = Dhcp6GenerateClientId (Dhcp6Srv->Snp->Mode);

  if (Dhcp6Srv->ClientId == NULL) {
    FreePool (Dhcp6Srv);
    return EFI_DEVICE_ERROR;
  }

  //
  // Create an Udp6Io for stateful transmit/receive of each Dhcp6 instance.
  //
  Dhcp6Srv->UdpIo = UdpIoCreateIo (
                      Controller,
                      ImageHandle,
                      Dhcp6ConfigureUdpIo,
                      UDP_IO_UDP6_VERSION,
                      NULL
                      );

  if (Dhcp6Srv->UdpIo == NULL) {
    FreePool (Dhcp6Srv->ClientId);
    FreePool (Dhcp6Srv);
    return EFI_DEVICE_ERROR;
  }

  InitializeListHead (&Dhcp6Srv->Child);

  *Service = Dhcp6Srv;

  return EFI_SUCCESS;
}


/**
  Destroy the Dhcp6 instance and recycle the resources.

  @param[in, out]  Instance        The pointer to the Dhcp6 instance.

**/
VOID
Dhcp6DestroyInstance (
  IN OUT DHCP6_INSTANCE         *Instance
  )
{
  //
  // Clean up the retry list first.
  //
  Dhcp6CleanupRetry (Instance, DHCP6_PACKET_ALL);
  gBS->CloseEvent (Instance->Timer);

  //
  // Clean up the current configure data.
  //
  if (Instance->Config != NULL) {
    Dhcp6CleanupConfigData (Instance->Config);
    FreePool (Instance->Config);
  }

  //
  // Clean up the current Ia.
  //
  if (Instance->IaCb.Ia != NULL) {
    if (Instance->IaCb.Ia->ReplyPacket != NULL) {
      FreePool (Instance->IaCb.Ia->ReplyPacket);
    }
    FreePool (Instance->IaCb.Ia);
  }

  if (Instance->Unicast != NULL) {
    FreePool (Instance->Unicast);
  }

  if (Instance->AdSelect != NULL) {
    FreePool (Instance->AdSelect);
  }

  FreePool (Instance);
}


/**
  Create the Dhcp6 instance and initialize it.

  @param[in]  Service              The pointer to the Dhcp6 service.
  @param[out] Instance             The pointer to the Dhcp6 instance.

  @retval EFI_SUCCESS            The Dhcp6 instance is created.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.

**/
EFI_STATUS
Dhcp6CreateInstance (
  IN  DHCP6_SERVICE         *Service,
  OUT DHCP6_INSTANCE        **Instance
  )
{
  EFI_STATUS                Status;
  DHCP6_INSTANCE            *Dhcp6Ins;

  *Instance = NULL;
  Dhcp6Ins  = AllocateZeroPool (sizeof (DHCP6_INSTANCE));

  if (Dhcp6Ins == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the fields of the new Dhcp6 instance.
  //
  Dhcp6Ins->Signature       = DHCP6_INSTANCE_SIGNATURE;
  Dhcp6Ins->UdpSts          = EFI_ALREADY_STARTED;
  Dhcp6Ins->Service         = Service;
  Dhcp6Ins->InDestroy       = FALSE;
  Dhcp6Ins->MediaPresent    = TRUE;

  CopyMem (
    &Dhcp6Ins->Dhcp6,
    &gDhcp6ProtocolTemplate,
    sizeof (EFI_DHCP6_PROTOCOL)
    );

  InitializeListHead (&Dhcp6Ins->TxList);
  InitializeListHead (&Dhcp6Ins->InfList);

  //
  // There is a timer for each Dhcp6 instance, which is used to track the
  // lease time of Ia and the retransmisson time of all sent packets.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  Dhcp6OnTimerTick,
                  Dhcp6Ins,
                  &Dhcp6Ins->Timer
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Dhcp6Ins);
    return Status;
  }

  *Instance = Dhcp6Ins;

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
Dhcp6DestroyChildEntry (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  DHCP6_INSTANCE                   *Instance;
  EFI_SERVICE_BINDING_PROTOCOL     *ServiceBinding;

  if (Entry == NULL || Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = NET_LIST_USER_STRUCT_S (Entry, DHCP6_INSTANCE, Link, DHCP6_INSTANCE_SIGNATURE);
  ServiceBinding = (EFI_SERVICE_BINDING_PROTOCOL *) Context;
  
  return ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
}


/**
  Entry point of the DHCP6 driver to install various protocols.

  @param[in]  ImageHandle           The handle of the UEFI image file.
  @param[in]  SystemTable           The pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                Unexpected error occurs.

**/
EFI_STATUS
EFIAPI
Dhcp6DriverEntryPoint (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDhcp6DriverBinding,
           ImageHandle,
           &gDhcp6ComponentName,
           &gDhcp6ComponentName2
           );
}


/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                The pointer to the driver binding protocol.
  @param[in]  ControllerHandle    The handle of device to be tested.
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to be started.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval Others              This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Dhcp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return gBS->OpenProtocol (
                ControllerHandle,
                &gEfiUdp6ServiceBindingProtocolGuid,
                NULL,
                This->DriverBindingHandle,
                ControllerHandle,
                EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );
}


/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                 The pointer to the driver binding protocol.
  @param[in]  ControllerHandle     The handle of device to be started.
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to be started.

  @retval EFI_SUCCESS          This driver is installed to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Dhcp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                      Status;
  DHCP6_SERVICE                   *Service;

  //
  // Check the Dhcp6 serivce whether already started.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDhcp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Create and initialize the Dhcp6 service.
  //
  Status = Dhcp6CreateService (
             ControllerHandle,
             This->DriverBindingHandle,
             &Service
             );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Service != NULL);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiDhcp6ServiceBindingProtocolGuid,
                  &Service->ServiceBinding,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    Dhcp6DestroyService (Service);
    return Status;
  }

  return EFI_SUCCESS;
}


/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.

  @param[in]  This              Protocol instance pointer.
  @param[in]  ControllerHandle  Handle of device to stop driver on
  @param[in]  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS           This driver is removed ControllerHandle
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval other                 This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
Dhcp6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer   OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_HANDLE                       NicHandle;
  EFI_SERVICE_BINDING_PROTOCOL     *ServiceBinding;
  DHCP6_SERVICE                    *Service;
  LIST_ENTRY                       *List;
  UINTN                            ListLength;

  //
  // Find and check the Nic handle by the controller handle.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiUdp6ProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiDhcp6ServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Service = DHCP6_SERVICE_FROM_THIS (ServiceBinding);
  if (!IsListEmpty (&Service->Child)) {
    //
    // Destroy all the children instances before destory the service.
    //  
    List = &Service->Child;
    Status = NetDestroyLinkList (
               List,
               Dhcp6DestroyChildEntry,
               ServiceBinding,
               &ListLength
               );
    if (EFI_ERROR (Status) || ListLength != 0) {
      Status = EFI_DEVICE_ERROR;
    }
  }

  if (NumberOfChildren == 0 && !IsListEmpty (&Service->Child)) {
    Status = EFI_DEVICE_ERROR;
  }

  if (NumberOfChildren == 0 && IsListEmpty (&Service->Child)) {
    //
    // Destroy the service itself if no child instance left.
    //
    Status = gBS->UninstallProtocolInterface (
                    NicHandle,
                    &gEfiDhcp6ServiceBindingProtocolGuid,
                    ServiceBinding
                    );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }

    Dhcp6DestroyService (Service);
    Status = EFI_SUCCESS;
  }
  
ON_EXIT:
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

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
Dhcp6ServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                    *ChildHandle
  )
{
  EFI_STATUS                       Status;
  EFI_TPL                          OldTpl;
  DHCP6_SERVICE                    *Service;
  DHCP6_INSTANCE                   *Instance;
  VOID                             *Udp6;

  if (This == NULL || ChildHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Service = DHCP6_SERVICE_FROM_THIS (This);

  Status  = Dhcp6CreateInstance (Service, &Instance);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Instance != NULL);

  //
  // Start the timer when the instance is ready to use.
  //
  Status = gBS->SetTimer (
                  Instance->Timer,
                  TimerPeriodic,
                  TICKS_PER_SECOND
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the DHCP6 protocol onto ChildHandle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiDhcp6ProtocolGuid,
                  &Instance->Dhcp6,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->Handle = *ChildHandle;

  //
  // Open the UDP6 protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  Service->UdpIo->UdpHandle,
                  &gEfiUdp6ProtocolGuid,
                  (VOID **) &Udp6,
                  gDhcp6DriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );

  if (EFI_ERROR (Status)) {

    gBS->UninstallMultipleProtocolInterfaces (
           Instance->Handle,
           &gEfiDhcp6ProtocolGuid,
           &Instance->Dhcp6,
           NULL
           );
    goto ON_ERROR;
  }

  //
  // Add into the children list of its parent service.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&Service->Child, &Instance->Link);
  Service->NumOfChild++;

  gBS->RestoreTPL (OldTpl);
  return EFI_SUCCESS;

ON_ERROR:

  Dhcp6DestroyInstance (Instance);
  return Status;
}


/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in]  ChildHandle Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Dhcp6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS                       Status;
  EFI_TPL                          OldTpl;
  EFI_DHCP6_PROTOCOL               *Dhcp6;
  DHCP6_SERVICE                    *Service;
  DHCP6_INSTANCE                   *Instance;

  if (This == NULL || ChildHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiDhcp6ProtocolGuid,
                  (VOID **) &Dhcp6,
                  gDhcp6DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = DHCP6_INSTANCE_FROM_THIS (Dhcp6);
  Service  = DHCP6_SERVICE_FROM_THIS (This);

  if (Instance->Service != Service) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->InDestroy = TRUE;

  Status = gBS->CloseProtocol (
                  Service->UdpIo->UdpHandle,
                  &gEfiUdp6ProtocolGuid,
                  gDhcp6DriverBinding.DriverBindingHandle,
                  ChildHandle
                  );

  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  // Uninstall the MTFTP6 protocol first to enable a top down destruction.
  //
  gBS->RestoreTPL (OldTpl);
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiDhcp6ProtocolGuid,
                  Dhcp6
                  );
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  //
  // Remove it from the children list of its parent service.
  //
  RemoveEntryList (&Instance->Link);
  Service->NumOfChild--;

  gBS->RestoreTPL (OldTpl);

  Dhcp6DestroyInstance (Instance);
  return EFI_SUCCESS;
}
