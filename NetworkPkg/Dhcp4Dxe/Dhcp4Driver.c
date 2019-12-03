/** @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Dhcp4Impl.h"
#include "Dhcp4Driver.h"

EFI_DRIVER_BINDING_PROTOCOL gDhcp4DriverBinding = {
  Dhcp4DriverBindingSupported,
  Dhcp4DriverBindingStart,
  Dhcp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL mDhcp4ServiceBindingTemplate = {
  Dhcp4ServiceBindingCreateChild,
  Dhcp4ServiceBindingDestroyChild
};

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  Entry point of the DHCP driver to install various protocols.

  @param[in]  ImageHandle           The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
Dhcp4DriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDhcp4DriverBinding,
           ImageHandle,
           &gDhcp4ComponentName,
           &gDhcp4ComponentName2
           );
}


/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to test
  @param[in]  RemainingDevicePath Optional parameter use to pick a specific child
                                  device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
Dhcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiUdp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}



/**
  Configure the default UDP child to receive all the DHCP traffics
  on this network interface.

  @param[in]  UdpIo                  The UDP IO to configure
  @param[in]  Context                The context to the function

  @retval EFI_SUCCESS            The UDP IO is successfully configured.
  @retval Others                 Failed to configure the UDP child.

**/
EFI_STATUS
EFIAPI
DhcpConfigUdpIo (
  IN UDP_IO                 *UdpIo,
  IN VOID                   *Context
  )
{
  EFI_UDP4_CONFIG_DATA      UdpConfigData;

  UdpConfigData.AcceptBroadcast           = TRUE;
  UdpConfigData.AcceptPromiscuous         = FALSE;
  UdpConfigData.AcceptAnyPort             = FALSE;
  UdpConfigData.AllowDuplicatePort        = TRUE;
  UdpConfigData.TypeOfService             = 0;
  UdpConfigData.TimeToLive                = 64;
  UdpConfigData.DoNotFragment             = FALSE;
  UdpConfigData.ReceiveTimeout            = 0;
  UdpConfigData.TransmitTimeout           = 0;

  UdpConfigData.UseDefaultAddress         = FALSE;
  UdpConfigData.StationPort               = DHCP_CLIENT_PORT;
  UdpConfigData.RemotePort                = DHCP_SERVER_PORT;

  ZeroMem (&UdpConfigData.StationAddress, sizeof (EFI_IPv4_ADDRESS));
  ZeroMem (&UdpConfigData.SubnetMask, sizeof (EFI_IPv4_ADDRESS));
  ZeroMem (&UdpConfigData.RemoteAddress, sizeof (EFI_IPv4_ADDRESS));

  return UdpIo->Protocol.Udp4->Configure (UdpIo->Protocol.Udp4, &UdpConfigData);;
}



/**
  Destroy the DHCP service. The Dhcp4 service may be partly initialized,
  or partly destroyed. If a resource is destroyed, it is marked as so in
  case the destroy failed and being called again later.

  @param[in]  DhcpSb                 The DHCP service instance to destroy.

  @retval EFI_SUCCESS            Always return success.

**/
EFI_STATUS
Dhcp4CloseService (
  IN DHCP_SERVICE           *DhcpSb
  )
{
  DhcpCleanLease (DhcpSb);

  if (DhcpSb->UdpIo != NULL) {
    UdpIoFreeIo (DhcpSb->UdpIo);
    DhcpSb->UdpIo = NULL;
  }

  if (DhcpSb->Timer != NULL) {
    gBS->SetTimer (DhcpSb->Timer, TimerCancel, 0);
    gBS->CloseEvent (DhcpSb->Timer);

    DhcpSb->Timer = NULL;
  }

  return EFI_SUCCESS;
}



/**
  Create a new DHCP service binding instance for the controller.

  @param[in]  Controller             The controller to install DHCP service binding
                                     protocol onto
  @param[in]  ImageHandle            The driver's image handle
  @param[out] Service                The variable to receive the created DHCP service
                                     instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource .
  @retval EFI_SUCCESS            The DHCP service instance is created.
  @retval other                  Other error occurs.

**/
EFI_STATUS
Dhcp4CreateService (
  IN  EFI_HANDLE            Controller,
  IN  EFI_HANDLE            ImageHandle,
  OUT DHCP_SERVICE          **Service
  )
{
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;

  *Service  = NULL;
  DhcpSb    = AllocateZeroPool (sizeof (DHCP_SERVICE));

  if (DhcpSb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DhcpSb->Signature       = DHCP_SERVICE_SIGNATURE;
  DhcpSb->ServiceState    = DHCP_UNCONFIGED;
  DhcpSb->Controller      = Controller;
  DhcpSb->Image           = ImageHandle;
  InitializeListHead (&DhcpSb->Children);
  DhcpSb->DhcpState       = Dhcp4Stopped;
  DhcpSb->Xid             = NET_RANDOM (NetRandomInitSeed ());
  CopyMem (
    &DhcpSb->ServiceBinding,
    &mDhcp4ServiceBindingTemplate,
    sizeof (EFI_SERVICE_BINDING_PROTOCOL)
    );
  //
  // Create various resources, UdpIo, Timer, and get Mac address
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  DhcpOnTimerTick,
                  DhcpSb,
                  &DhcpSb->Timer
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  DhcpSb->UdpIo = UdpIoCreateIo (
                    Controller,
                    ImageHandle,
                    DhcpConfigUdpIo,
                    UDP_IO_UDP4_VERSION,
                    NULL
                    );

  if (DhcpSb->UdpIo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  DhcpSb->HwLen  = (UINT8) DhcpSb->UdpIo->SnpMode.HwAddressSize;
  DhcpSb->HwType = DhcpSb->UdpIo->SnpMode.IfType;
  CopyMem (&DhcpSb->Mac, &DhcpSb->UdpIo->SnpMode.CurrentAddress, sizeof (DhcpSb->Mac));

  *Service       = DhcpSb;
  return EFI_SUCCESS;

ON_ERROR:
  Dhcp4CloseService (DhcpSb);
  FreePool (DhcpSb);

  return Status;
}


/**
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
Dhcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  DHCP_SERVICE              *DhcpSb;
  EFI_STATUS                Status;

  //
  // First: test for the DHCP4 Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  Status = Dhcp4CreateService (ControllerHandle, This->DriverBindingHandle, &DhcpSb);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (DhcpSb != NULL);

  //
  // Start the receiving
  //
  Status = UdpIoRecvDatagram (DhcpSb->UdpIo, DhcpInput, DhcpSb, 0);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  Status = gBS->SetTimer (DhcpSb->Timer, TimerPeriodic, TICKS_PER_SECOND);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the Dhcp4ServiceBinding Protocol onto ControlerHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  &DhcpSb->ServiceBinding,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return Status;

ON_ERROR:
  Dhcp4CloseService (DhcpSb);
  FreePool (DhcpSb);
  return Status;
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
Dhcp4DestroyChildEntry (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  DHCP_PROTOCOL                    *Instance;
  EFI_SERVICE_BINDING_PROTOCOL     *ServiceBinding;

  if (Entry == NULL || Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = NET_LIST_USER_STRUCT_S (Entry, DHCP_PROTOCOL, Link, DHCP_PROTOCOL_SIGNATURE);
  ServiceBinding = (EFI_SERVICE_BINDING_PROTOCOL *) Context;

  return ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
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

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
Dhcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  DHCP_SERVICE                  *DhcpSb;
  EFI_HANDLE                    NicHandle;
  EFI_STATUS                    Status;
  LIST_ENTRY                    *List;
  UINTN                         ListLength;

  //
  // DHCP driver opens UDP child, So, the ControllerHandle is the
  // UDP child handle. locate the Nic handle first.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiUdp4ProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

   Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DhcpSb = DHCP_SERVICE_FROM_THIS (ServiceBinding);
  if (!IsListEmpty (&DhcpSb->Children)) {
    //
    // Destroy all the children instances before destory the service.
    //
    List = &DhcpSb->Children;
    Status = NetDestroyLinkList (
               List,
               Dhcp4DestroyChildEntry,
               ServiceBinding,
               &ListLength
               );
    if (EFI_ERROR (Status) || ListLength != 0) {
      Status = EFI_DEVICE_ERROR;
    }
  }

  if (NumberOfChildren == 0 && !IsListEmpty (&DhcpSb->Children)) {
    Status = EFI_DEVICE_ERROR;
  }

  if (NumberOfChildren == 0 && IsListEmpty (&DhcpSb->Children)) {
    //
    // Destroy the service itself if no child instance left.
    //
    DhcpSb->ServiceState = DHCP_DESTROY;

    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiDhcp4ServiceBindingProtocolGuid,
           ServiceBinding
           );

    Dhcp4CloseService (DhcpSb);

    if (gDhcpControllerNameTable != NULL) {
      FreeUnicodeStringTable (gDhcpControllerNameTable);
      gDhcpControllerNameTable = NULL;
    }
    FreePool (DhcpSb);

    Status = EFI_SUCCESS;
  }

  return Status;
}


/**
  Initialize a new DHCP instance.

  @param  DhcpSb                 The dhcp service instance
  @param  Instance               The dhcp instance to initialize

**/
VOID
DhcpInitProtocol (
  IN     DHCP_SERVICE           *DhcpSb,
  IN OUT DHCP_PROTOCOL          *Instance
  )
{
  Instance->Signature         = DHCP_PROTOCOL_SIGNATURE;
  CopyMem (&Instance->Dhcp4Protocol, &mDhcp4ProtocolTemplate, sizeof (Instance->Dhcp4Protocol));
  InitializeListHead (&Instance->Link);
  Instance->Handle            = NULL;
  Instance->Service           = DhcpSb;
  Instance->InDestroy         = FALSE;
  Instance->CompletionEvent   = NULL;
  Instance->RenewRebindEvent  = NULL;
  Instance->Token             = NULL;
  Instance->UdpIo             = NULL;
  Instance->ElaspedTime       = 0;
  NetbufQueInit (&Instance->ResponseQueue);
}


/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Pointer to the handle of the child to create. If it is NULL,
                      then a new handle is created. If it is a pointer to an existing UEFI handle,
                      then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
Dhcp4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  DHCP_SERVICE              *DhcpSb;
  DHCP_PROTOCOL             *Instance;
  EFI_STATUS                Status;
  EFI_TPL                   OldTpl;
  VOID                      *Udp4;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance = AllocatePool (sizeof (*Instance));

  if (Instance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DhcpSb = DHCP_SERVICE_FROM_THIS (This);
  DhcpInitProtocol (DhcpSb, Instance);

  //
  // Install DHCP4 onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiDhcp4ProtocolGuid,
                  &Instance->Dhcp4Protocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    FreePool (Instance);
    return Status;
  }

  Instance->Handle  = *ChildHandle;

  //
  // Open the Udp4 protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  DhcpSb->UdpIo->UdpHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **) &Udp4,
                  gDhcp4DriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->Handle,
           &gEfiDhcp4ProtocolGuid,
           &Instance->Dhcp4Protocol,
           NULL
           );

    FreePool (Instance);
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&DhcpSb->Children, &Instance->Link);
  DhcpSb->NumChildren++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;
}


/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Dhcp4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  DHCP_SERVICE              *DhcpSb;
  DHCP_PROTOCOL             *Instance;
  EFI_DHCP4_PROTOCOL        *Dhcp;
  EFI_TPL                   OldTpl;
  EFI_STATUS                Status;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **) &Dhcp,
                  gDhcp4DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance  = DHCP_INSTANCE_FROM_THIS (Dhcp);
  DhcpSb    = DHCP_SERVICE_FROM_THIS (This);

  if (Instance->Service != DhcpSb) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // A child can be destroyed more than once. For example,
  // Dhcp4DriverBindingStop will destroy all of its children.
  // when caller driver is being stopped, it will destroy the
  // dhcp child it opens.
  //
  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  Instance->InDestroy = TRUE;

  //
  // Close the Udp4 protocol.
  //
  gBS->CloseProtocol (
         DhcpSb->UdpIo->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         gDhcp4DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  //
  // Uninstall the DHCP4 protocol first to enable a top down destruction.
  //
  gBS->RestoreTPL (OldTpl);
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiDhcp4ProtocolGuid,
                  Dhcp
                  );
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;

    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  if (DhcpSb->ActiveChild == Instance) {
    DhcpYieldControl (DhcpSb);
  }

  RemoveEntryList (&Instance->Link);
  DhcpSb->NumChildren--;

  if (Instance->UdpIo != NULL) {
    UdpIoCleanIo (Instance->UdpIo);
    gBS->CloseProtocol (
           Instance->UdpIo->UdpHandle,
           &gEfiUdp4ProtocolGuid,
           Instance->Service->Image,
           Instance->Handle
           );
    UdpIoFreeIo (Instance->UdpIo);
    Instance->UdpIo = NULL;
    Instance->Token = NULL;
  }

  gBS->RestoreTPL (OldTpl);

  FreePool (Instance);
  return EFI_SUCCESS;
}
