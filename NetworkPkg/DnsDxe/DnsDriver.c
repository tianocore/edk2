/** @file
The driver binding and service binding protocol for DnsDxe driver.

Copyright (c) 2015 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DnsImpl.h"

EFI_DRIVER_BINDING_PROTOCOL  gDns4DriverBinding = {
  Dns4DriverBindingSupported,
  Dns4DriverBindingStart,
  Dns4DriverBindingStop,
  DNS_VERSION,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL  gDns6DriverBinding = {
  Dns6DriverBindingSupported,
  Dns6DriverBindingStart,
  Dns6DriverBindingStop,
  DNS_VERSION,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL  mDns4ServiceBinding = {
  Dns4ServiceBindingCreateChild,
  Dns4ServiceBindingDestroyChild
};

EFI_SERVICE_BINDING_PROTOCOL  mDns6ServiceBinding = {
  Dns6ServiceBindingCreateChild,
  Dns6ServiceBindingDestroyChild
};

DNS_DRIVER_DATA  *mDriverData = NULL;

/**
  Destroy the DNS instance and recycle the resources.

  @param[in]  Instance        The pointer to the DNS instance.

**/
VOID
DnsDestroyInstance (
  IN DNS_INSTANCE  *Instance
  )
{
  ZeroMem (&Instance->Dns4CfgData, sizeof (EFI_DNS4_CONFIG_DATA));

  ZeroMem (&Instance->Dns6CfgData, sizeof (EFI_DNS6_CONFIG_DATA));

  if (!NetMapIsEmpty (&Instance->Dns4TxTokens)) {
    Dns4InstanceCancelToken (Instance, NULL);
  }

  if (!NetMapIsEmpty (&Instance->Dns6TxTokens)) {
    Dns6InstanceCancelToken (Instance, NULL);
  }

  if (Instance->UdpIo != NULL) {
    UdpIoFreeIo (Instance->UdpIo);
  }

  FreePool (Instance);
}

/**
  Create the DNS instance and initialize it.

  @param[in]  Service              The pointer to the DNS service.
  @param[out] Instance             The pointer to the DNS instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The DNS instance is created.

**/
EFI_STATUS
DnsCreateInstance (
  IN  DNS_SERVICE   *Service,
  OUT DNS_INSTANCE  **Instance
  )
{
  DNS_INSTANCE  *DnsIns;

  *Instance = NULL;

  DnsIns = AllocateZeroPool (sizeof (DNS_INSTANCE));
  if (DnsIns == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DnsIns->Signature = DNS_INSTANCE_SIGNATURE;
  InitializeListHead (&DnsIns->Link);
  DnsIns->State     = DNS_STATE_UNCONFIGED;
  DnsIns->InDestroy = FALSE;
  DnsIns->Service   = Service;

  if (Service->IpVersion == IP_VERSION_4) {
    CopyMem (&DnsIns->Dns4, &mDns4Protocol, sizeof (DnsIns->Dns4));
    NetMapInit (&DnsIns->Dns4TxTokens);
  } else {
    CopyMem (&DnsIns->Dns6, &mDns6Protocol, sizeof (DnsIns->Dns6));
    NetMapInit (&DnsIns->Dns6TxTokens);
  }

  DnsIns->UdpIo = UdpIoCreateIo (
                    Service->ControllerHandle, /// NicHandle
                    Service->ImageHandle,
                    DnsConfigNullUdp,
                    Service->IpVersion,
                    DnsIns
                    );
  if (DnsIns->UdpIo == NULL) {
    FreePool (DnsIns);
    return EFI_OUT_OF_RESOURCES;
  }

  *Instance = DnsIns;

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
DnsDestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY  *Entry,
  IN VOID        *Context
  )
{
  DNS_INSTANCE                  *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if ((Entry == NULL) || (Context == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance          = NET_LIST_USER_STRUCT_S (Entry, DNS_INSTANCE, Link, DNS_INSTANCE_SIGNATURE);
  ServiceBinding    = ((DNS_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ServiceBinding;
  NumberOfChildren  = ((DNS_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->NumberOfChildren;
  ChildHandleBuffer = ((DNS_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Instance->ChildHandle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Instance->ChildHandle);
}

/**
  Config a NULL UDP that is used to keep the connection between UDP and DNS.

  Just leave the Udp child unconfigured. When UDP is unloaded,
    DNS will be informed with DriverBinding Stop.

  @param  UdpIo                  The UDP_IO to configure
  @param  Context                The opaque parameter to the callback

  @retval EFI_SUCCESS            It always return EFI_SUCCESS directly.

**/
EFI_STATUS
EFIAPI
DnsConfigNullUdp (
  IN UDP_IO  *UdpIo,
  IN VOID    *Context
  )
{
  return EFI_SUCCESS;
}

/**
  Release all the resource used the DNS service binding instance.

  @param  DnsSb                The Dns service binding instance.

**/
VOID
DnsDestroyService (
  IN DNS_SERVICE  *DnsSb
  )
{
  UdpIoFreeIo (DnsSb->ConnectUdp);

  if (DnsSb->TimerToGetMap != NULL) {
    gBS->CloseEvent (DnsSb->TimerToGetMap);
  }

  if (DnsSb->Timer != NULL) {
    gBS->CloseEvent (DnsSb->Timer);
  }

  FreePool (DnsSb);
}

/**
  Create then initialize a Dns service binding instance.

  @param  Controller             The controller to install the DNS service
                                 binding on
  @param  Image                  The driver binding image of the DNS driver
  @param  IpVersion              IpVersion for this service
  @param  Service                The variable to receive the created service
                                 binding instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource to create the instance.
  @retval EFI_DEVICE_ERROR       Failed to create a NULL UDP port to keep
                                 connection  with UDP.
  @retval EFI_SUCCESS            The service instance is created for the
                                 controller.

**/
EFI_STATUS
DnsCreateService (
  IN     EFI_HANDLE  Controller,
  IN     EFI_HANDLE  Image,
  IN     UINT8       IpVersion,
  OUT DNS_SERVICE    **Service
  )
{
  EFI_STATUS   Status;
  DNS_SERVICE  *DnsSb;

  Status = EFI_SUCCESS;
  DnsSb  = NULL;

  *Service = NULL;

  DnsSb = AllocateZeroPool (sizeof (DNS_SERVICE));
  if (DnsSb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  DnsSb->Signature = DNS_SERVICE_SIGNATURE;

  if (IpVersion == IP_VERSION_4) {
    DnsSb->ServiceBinding = mDns4ServiceBinding;
  } else {
    DnsSb->ServiceBinding = mDns6ServiceBinding;
  }

  DnsSb->Dns4ChildrenNum = 0;
  InitializeListHead (&DnsSb->Dns4ChildrenList);

  DnsSb->Dns6ChildrenNum = 0;
  InitializeListHead (&DnsSb->Dns6ChildrenList);

  DnsSb->ControllerHandle = Controller;
  DnsSb->ImageHandle      = Image;

  DnsSb->TimerToGetMap = NULL;

  DnsSb->Timer = NULL;

  DnsSb->IpVersion = IpVersion;

  //
  // Create the timer used to time out the procedure which is used to
  // get the default IP address.
  //
  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &DnsSb->TimerToGetMap
                  );
  if (EFI_ERROR (Status)) {
    FreePool (DnsSb);
    return Status;
  }

  //
  // Create the timer to retransmit packets.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  DnsOnTimerRetransmit,
                  DnsSb,
                  &DnsSb->Timer
                  );
  if (EFI_ERROR (Status)) {
    if (DnsSb->TimerToGetMap != NULL) {
      gBS->CloseEvent (DnsSb->TimerToGetMap);
    }

    FreePool (DnsSb);
    return Status;
  }

  DnsSb->ConnectUdp = NULL;
  DnsSb->ConnectUdp = UdpIoCreateIo (
                        Controller,
                        Image,
                        DnsConfigNullUdp,
                        DnsSb->IpVersion,
                        NULL
                        );
  if (DnsSb->ConnectUdp == NULL) {
    if (DnsSb->TimerToGetMap != NULL) {
      gBS->CloseEvent (DnsSb->TimerToGetMap);
    }

    gBS->CloseEvent (DnsSb->Timer);
    FreePool (DnsSb);
    return EFI_DEVICE_ERROR;
  }

  *Service = DnsSb;
  return Status;
}

/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
DnsUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  LIST_ENTRY      *Entry;
  DNS4_CACHE      *ItemCache4;
  DNS4_SERVER_IP  *ItemServerIp4;
  DNS6_CACHE      *ItemCache6;
  DNS6_SERVER_IP  *ItemServerIp6;

  ItemCache4    = NULL;
  ItemServerIp4 = NULL;
  ItemCache6    = NULL;
  ItemServerIp6 = NULL;

  //
  // Disconnect the driver specified by ImageHandle
  //
  Status = NetLibDefaultUnload (ImageHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Free mDriverData.
  //
  if (mDriverData != NULL) {
    if (mDriverData->Timer != NULL) {
      gBS->CloseEvent (mDriverData->Timer);
    }

    while (!IsListEmpty (&mDriverData->Dns4CacheList)) {
      Entry = NetListRemoveHead (&mDriverData->Dns4CacheList);
      ASSERT (Entry != NULL);
      ItemCache4 = NET_LIST_USER_STRUCT (Entry, DNS4_CACHE, AllCacheLink);
      FreePool (ItemCache4->DnsCache.HostName);
      FreePool (ItemCache4->DnsCache.IpAddress);
      FreePool (ItemCache4);
    }

    while (!IsListEmpty (&mDriverData->Dns4ServerList)) {
      Entry = NetListRemoveHead (&mDriverData->Dns4ServerList);
      ASSERT (Entry != NULL);
      ItemServerIp4 = NET_LIST_USER_STRUCT (Entry, DNS4_SERVER_IP, AllServerLink);
      FreePool (ItemServerIp4);
    }

    while (!IsListEmpty (&mDriverData->Dns6CacheList)) {
      Entry = NetListRemoveHead (&mDriverData->Dns6CacheList);
      ASSERT (Entry != NULL);
      ItemCache6 = NET_LIST_USER_STRUCT (Entry, DNS6_CACHE, AllCacheLink);
      FreePool (ItemCache6->DnsCache.HostName);
      FreePool (ItemCache6->DnsCache.IpAddress);
      FreePool (ItemCache6);
    }

    while (!IsListEmpty (&mDriverData->Dns6ServerList)) {
      Entry = NetListRemoveHead (&mDriverData->Dns6ServerList);
      ASSERT (Entry != NULL);
      ItemServerIp6 = NET_LIST_USER_STRUCT (Entry, DNS6_SERVER_IP, AllServerLink);
      FreePool (ItemServerIp6);
    }

    FreePool (mDriverData);
  }

  return Status;
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
DnsDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Install the Dns4 Driver Binding Protocol.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gDns4DriverBinding,
             ImageHandle,
             &gDnsComponentName,
             &gDnsComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install the Dns6 Driver Binding Protocol.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gDns6DriverBinding,
             NULL,
             &gDnsComponentName,
             &gDnsComponentName2
             );
  if (EFI_ERROR (Status)) {
    goto Error1;
  }

  //
  // Create the driver data structures.
  //
  mDriverData = AllocateZeroPool (sizeof (DNS_DRIVER_DATA));
  if (mDriverData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error2;
  }

  //
  // Create the timer event to update DNS cache list.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  DnsOnTimerUpdate,
                  NULL,
                  &mDriverData->Timer
                  );
  if (EFI_ERROR (Status)) {
    goto Error3;
  }

  Status = gBS->SetTimer (mDriverData->Timer, TimerPeriodic, TICKS_PER_SECOND);
  if (EFI_ERROR (Status)) {
    goto Error4;
  }

  InitializeListHead (&mDriverData->Dns4CacheList);
  InitializeListHead (&mDriverData->Dns4ServerList);
  InitializeListHead (&mDriverData->Dns6CacheList);
  InitializeListHead (&mDriverData->Dns6ServerList);

  return Status;

Error4:
  gBS->CloseEvent (mDriverData->Timer);

Error3:
  FreePool (mDriverData);

Error2:
  EfiLibUninstallDriverBindingComponentName2 (
    &gDns6DriverBinding,
    &gDnsComponentName,
    &gDnsComponentName2
    );

Error1:
  EfiLibUninstallDriverBindingComponentName2 (
    &gDns4DriverBinding,
    &gDnsComponentName,
    &gDnsComponentName2
    );

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
Dns4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Test for the Dns4ServiceBinding Protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDns4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test for the Udp4ServiceBinding Protocol.
  //
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
  Starts a device controller or a bus controller.

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
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
Dns4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  DNS_SERVICE  *DnsSb;
  EFI_STATUS   Status;

  Status = DnsCreateService (ControllerHandle, This->DriverBindingHandle, IP_VERSION_4, &DnsSb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (DnsSb != NULL);

  Status = gBS->SetTimer (DnsSb->Timer, TimerPeriodic, TICKS_PER_SECOND);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the Dns4ServiceBinding Protocol onto ControllerHandle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiDns4ServiceBindingProtocolGuid,
                  &DnsSb->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  DnsDestroyService (DnsSb);

  return Status;
}

/**
  Stops a device controller or a bus controller.

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
Dns4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_SERVICE_BINDING_PROTOCOL             *ServiceBinding;
  DNS_SERVICE                              *DnsSb;
  EFI_HANDLE                               NicHandle;
  EFI_STATUS                               Status;
  LIST_ENTRY                               *List;
  DNS_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  //
  // DNS driver opens UDP child, So, Controller is a UDP
  // child handle. Locate the Nic handle first. Then get the
  // DNS private data back.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiUdp4ProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiDns4ServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DnsSb = DNS_SERVICE_FROM_THIS (ServiceBinding);

  if (!IsListEmpty (&DnsSb->Dns4ChildrenList)) {
    //
    // Destroy the Dns child instance in ChildHandleBuffer.
    //
    List                      = &DnsSb->Dns4ChildrenList;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status                    = NetDestroyLinkList (
                                  List,
                                  DnsDestroyChildEntryInHandleBuffer,
                                  &Context,
                                  NULL
                                  );
  }

  if ((NumberOfChildren == 0) && IsListEmpty (&DnsSb->Dns4ChildrenList)) {
    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiDns4ServiceBindingProtocolGuid,
           ServiceBinding
           );

    DnsDestroyService (DnsSb);

    if (gDnsControllerNameTable != NULL) {
      FreeUnicodeStringTable (gDnsControllerNameTable);
      gDnsControllerNameTable = NULL;
    }

    Status = EFI_SUCCESS;
  }

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
Dns6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Test for the Dns6ServiceBinding Protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDns6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

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

  return Status;
}

/**
  Starts a device controller or a bus controller.

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
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
Dns6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  DNS_SERVICE  *DnsSb;
  EFI_STATUS   Status;

  Status = DnsCreateService (ControllerHandle, This->DriverBindingHandle, IP_VERSION_6, &DnsSb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (DnsSb != NULL);

  Status = gBS->SetTimer (DnsSb->Timer, TimerPeriodic, TICKS_PER_SECOND);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the Dns6ServiceBinding Protocol onto ControllerHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiDns6ServiceBindingProtocolGuid,
                  &DnsSb->ServiceBinding,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  DnsDestroyService (DnsSb);

  return Status;
}

/**
  Stops a device controller or a bus controller.

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
Dns6DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_SERVICE_BINDING_PROTOCOL             *ServiceBinding;
  DNS_SERVICE                              *DnsSb;
  EFI_HANDLE                               NicHandle;
  EFI_STATUS                               Status;
  LIST_ENTRY                               *List;
  DNS_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  //
  // DNS driver opens UDP child, So, Controller is a UDP
  // child handle. Locate the Nic handle first. Then get the
  // DNS private data back.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiUdp6ProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiDns6ServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  DnsSb = DNS_SERVICE_FROM_THIS (ServiceBinding);

  if (!IsListEmpty (&DnsSb->Dns6ChildrenList)) {
    //
    // Destroy the Dns child instance in ChildHandleBuffer.
    //
    List                      = &DnsSb->Dns6ChildrenList;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status                    = NetDestroyLinkList (
                                  List,
                                  DnsDestroyChildEntryInHandleBuffer,
                                  &Context,
                                  NULL
                                  );
  }

  if ((NumberOfChildren == 0) && IsListEmpty (&DnsSb->Dns6ChildrenList)) {
    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiDns6ServiceBindingProtocolGuid,
           ServiceBinding
           );

    DnsDestroyService (DnsSb);

    if (gDnsControllerNameTable != NULL) {
      FreeUnicodeStringTable (gDnsControllerNameTable);
      gDnsControllerNameTable = NULL;
    }

    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Pointer to the handle of the child to create. If it is NULL,
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
Dns4ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  DNS_SERVICE   *DnsSb;
  DNS_INSTANCE  *Instance;
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;
  VOID          *Udp4;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DnsSb = DNS_SERVICE_FROM_THIS (This);

  Status = DnsCreateInstance (DnsSb, &Instance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Instance != NULL);

  //
  // Install the DNS protocol onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiDns4ProtocolGuid,
                  &Instance->Dns4,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->ChildHandle = *ChildHandle;

  //
  // Open the Udp4 protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  DnsSb->ConnectUdp->UdpHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **)&Udp4,
                  gDns4DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiDns4ProtocolGuid,
           &Instance->Dns4,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Open the Udp4 protocol by child.
  //
  Status = gBS->OpenProtocol (
                  Instance->UdpIo->UdpHandle,
                  &gEfiUdp4ProtocolGuid,
                  (VOID **)&Udp4,
                  gDns4DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    //
    // Close the Udp4 protocol.
    //
    gBS->CloseProtocol (
           DnsSb->ConnectUdp->UdpHandle,
           &gEfiUdp4ProtocolGuid,
           gDns4DriverBinding.DriverBindingHandle,
           *ChildHandle
           );

    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiDns4ProtocolGuid,
           &Instance->Dns4,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Add it to the parent's child list.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&DnsSb->Dns4ChildrenList, &Instance->Link);
  DnsSb->Dns4ChildrenNum++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:

  DnsDestroyInstance (Instance);
  return Status;
}

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Dns4ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  DNS_SERVICE   *DnsSb;
  DNS_INSTANCE  *Instance;

  EFI_DNS4_PROTOCOL  *Dns4;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiDns4ProtocolGuid,
                  (VOID **)&Dns4,
                  gDns4DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL4 (Dns4);
  DnsSb    = DNS_SERVICE_FROM_THIS (This);

  if (Instance->Service != DnsSb) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->InDestroy = TRUE;

  //
  // Close the Udp4 protocol.
  //
  gBS->CloseProtocol (
         DnsSb->ConnectUdp->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         gDns4DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->CloseProtocol (
         Instance->UdpIo->UdpHandle,
         &gEfiUdp4ProtocolGuid,
         gDns4DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->RestoreTPL (OldTpl);

  //
  // Uninstall the DNS protocol first to enable a top down destruction.
  //
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiDns4ProtocolGuid,
                  Dns4
                  );

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  RemoveEntryList (&Instance->Link);
  DnsSb->Dns4ChildrenNum--;

  gBS->RestoreTPL (OldTpl);

  DnsDestroyInstance (Instance);
  return EFI_SUCCESS;
}

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Pointer to the handle of the child to create. If it is NULL,
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
Dns6ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  DNS_SERVICE   *DnsSb;
  DNS_INSTANCE  *Instance;
  EFI_STATUS    Status;
  EFI_TPL       OldTpl;
  VOID          *Udp6;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DnsSb = DNS_SERVICE_FROM_THIS (This);

  Status = DnsCreateInstance (DnsSb, &Instance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Instance != NULL);

  //
  // Install the DNS protocol onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiDns6ProtocolGuid,
                  &Instance->Dns6,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->ChildHandle = *ChildHandle;

  //
  // Open the Udp6 protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  DnsSb->ConnectUdp->UdpHandle,
                  &gEfiUdp6ProtocolGuid,
                  (VOID **)&Udp6,
                  gDns6DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiDns6ProtocolGuid,
           &Instance->Dns6,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Open the Udp6 protocol by child.
  //
  Status = gBS->OpenProtocol (
                  Instance->UdpIo->UdpHandle,
                  &gEfiUdp6ProtocolGuid,
                  (VOID **)&Udp6,
                  gDns6DriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    //
    // Close the Udp6 protocol.
    //
    gBS->CloseProtocol (
           DnsSb->ConnectUdp->UdpHandle,
           &gEfiUdp6ProtocolGuid,
           gDns6DriverBinding.DriverBindingHandle,
           *ChildHandle
           );

    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiDns6ProtocolGuid,
           &Instance->Dns6,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Add it to the parent's child list.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&DnsSb->Dns6ChildrenList, &Instance->Link);
  DnsSb->Dns6ChildrenNum++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:

  DnsDestroyInstance (Instance);
  return Status;
}

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
Dns6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  DNS_SERVICE   *DnsSb;
  DNS_INSTANCE  *Instance;

  EFI_DNS6_PROTOCOL  *Dns6;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiDns6ProtocolGuid,
                  (VOID **)&Dns6,
                  gDns6DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = DNS_INSTANCE_FROM_THIS_PROTOCOL6 (Dns6);
  DnsSb    = DNS_SERVICE_FROM_THIS (This);

  if (Instance->Service != DnsSb) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->InDestroy = TRUE;

  //
  // Close the Udp6 protocol.
  //
  gBS->CloseProtocol (
         DnsSb->ConnectUdp->UdpHandle,
         &gEfiUdp6ProtocolGuid,
         gDns6DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->CloseProtocol (
         Instance->UdpIo->UdpHandle,
         &gEfiUdp6ProtocolGuid,
         gDns6DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->RestoreTPL (OldTpl);

  //
  // Uninstall the DNS protocol first to enable a top down destruction.
  //
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiDns6ProtocolGuid,
                  Dns6
                  );

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  RemoveEntryList (&Instance->Link);
  DnsSb->Dns6ChildrenNum--;

  gBS->RestoreTPL (OldTpl);

  DnsDestroyInstance (Instance);
  return EFI_SUCCESS;
}
