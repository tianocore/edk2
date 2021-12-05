/** @file
  The driver binding and service binding protocol for IP6 driver.

  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ip6Impl.h"

EFI_DRIVER_BINDING_PROTOCOL  gIp6DriverBinding = {
  Ip6DriverBindingSupported,
  Ip6DriverBindingStart,
  Ip6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

BOOLEAN  mIpSec2Installed = FALSE;

/**
   Callback function for IpSec2 Protocol install.

   @param[in] Event           Event whose notification function is being invoked
   @param[in] Context         Pointer to the notification function's context

**/
VOID
EFIAPI
IpSec2InstalledCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Test if protocol was even found.
  // Notification function will be called at least once.
  //
  Status = gBS->LocateProtocol (&gEfiIpSec2ProtocolGuid, NULL, (VOID **)&mIpSec);
  if ((Status == EFI_SUCCESS) && (mIpSec != NULL)) {
    //
    // Close the event so it does not get called again.
    //
    gBS->CloseEvent (Event);

    mIpSec2Installed = TRUE;
  }
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  The entry point for IP6 driver which installs the driver
  binding and component name protocol on its image.

  @param[in]  ImageHandle           The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
Ip6DriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID  *Registration;

  EfiCreateProtocolNotifyEvent (
    &gEfiIpSec2ProtocolGuid,
    TPL_CALLBACK,
    IpSec2InstalledCallback,
    NULL,
    &Registration
    );

  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gIp6DriverBinding,
           ImageHandle,
           &gIp6ComponentName,
           &gIp6ComponentName2
           );
}

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                   Protocol instance pointer.
  @param[in]  ControllerHandle       Handle of device to test.
  @param[in]  RemainingDevicePath    Optional parameter use to pick a specific child
                                     device to start.

  @retval EFI_SUCCESS                This driver supports this device.
  @retval EFI_ALREADY_STARTED        This driver is already running on this device.
  @retval other                      This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Ip6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  //
  // Test for the MNP service binding Protocol
  //
  return gBS->OpenProtocol (
                ControllerHandle,
                &gEfiManagedNetworkServiceBindingProtocolGuid,
                NULL,
                This->DriverBindingHandle,
                ControllerHandle,
                EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );
}

/**
  Clean up an IP6 service binding instance. It releases all
  the resource allocated by the instance. The instance may be
  partly initialized, or partly destroyed. If a resource is
  destroyed, it is marked as that in case the destroy failed and
  being called again later.

  @param[in]  IpSb               The IP6 service binding instance to clean up.

  @retval EFI_SUCCESS            The resource used by the instance are cleaned up.
  @retval Others                 Failed to clean up some of the resources.

**/
EFI_STATUS
Ip6CleanService (
  IN IP6_SERVICE  *IpSb
  )
{
  EFI_STATUS          Status;
  EFI_IPv6_ADDRESS    AllNodes;
  IP6_NEIGHBOR_ENTRY  *NeighborCache;

  IpSb->State = IP6_SERVICE_DESTROY;

  if (IpSb->Timer != NULL) {
    gBS->SetTimer (IpSb->Timer, TimerCancel, 0);
    gBS->CloseEvent (IpSb->Timer);

    IpSb->Timer = NULL;
  }

  if (IpSb->FasterTimer != NULL) {
    gBS->SetTimer (IpSb->FasterTimer, TimerCancel, 0);
    gBS->CloseEvent (IpSb->FasterTimer);

    IpSb->FasterTimer = NULL;
  }

  Ip6ConfigCleanInstance (&IpSb->Ip6ConfigInstance);

  if (!IpSb->LinkLocalDadFail) {
    //
    // Leave link-scope all-nodes multicast address (FF02::1)
    //
    Ip6SetToAllNodeMulticast (FALSE, IP6_LINK_LOCAL_SCOPE, &AllNodes);

    Status = Ip6LeaveGroup (IpSb, &AllNodes);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (IpSb->DefaultInterface != NULL) {
    Ip6CleanInterface (IpSb->DefaultInterface, NULL);
    IpSb->DefaultInterface = NULL;
  }

  Ip6CleanDefaultRouterList (IpSb);

  Ip6CleanPrefixListTable (IpSb, &IpSb->OnlinkPrefix);
  Ip6CleanPrefixListTable (IpSb, &IpSb->AutonomousPrefix);

  if (IpSb->RouteTable != NULL) {
    Ip6CleanRouteTable (IpSb->RouteTable);
    IpSb->RouteTable = NULL;
  }

  if (IpSb->InterfaceId != NULL) {
    FreePool (IpSb->InterfaceId);
  }

  IpSb->InterfaceId = NULL;

  Ip6CleanAssembleTable (&IpSb->Assemble);

  if (IpSb->MnpChildHandle != NULL) {
    if (IpSb->Mnp != NULL) {
      IpSb->Mnp->Cancel (IpSb->Mnp, NULL);
      IpSb->Mnp->Configure (IpSb->Mnp, NULL);
      gBS->CloseProtocol (
             IpSb->MnpChildHandle,
             &gEfiManagedNetworkProtocolGuid,
             IpSb->Image,
             IpSb->Controller
             );

      IpSb->Mnp = NULL;
    }

    NetLibDestroyServiceChild (
      IpSb->Controller,
      IpSb->Image,
      &gEfiManagedNetworkServiceBindingProtocolGuid,
      IpSb->MnpChildHandle
      );

    IpSb->MnpChildHandle = NULL;
  }

  if (IpSb->RecvRequest.MnpToken.Event != NULL) {
    gBS->CloseEvent (IpSb->RecvRequest.MnpToken.Event);
  }

  //
  // Free the Neighbor Discovery resources
  //
  while (!IsListEmpty (&IpSb->NeighborTable)) {
    NeighborCache = NET_LIST_HEAD (&IpSb->NeighborTable, IP6_NEIGHBOR_ENTRY, Link);
    Ip6FreeNeighborEntry (IpSb, NeighborCache, FALSE, TRUE, EFI_SUCCESS, NULL, NULL);
  }

  return EFI_SUCCESS;
}

/**
  Create a new IP6 driver service binding protocol.

  @param[in]  Controller         The controller that has MNP service binding
                                 installed.
  @param[in]  ImageHandle        The IP6 driver's image handle.
  @param[out]  Service           The variable to receive the newly created IP6
                                 service.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_SUCCESS            A new IP6 service binding private is created.

**/
EFI_STATUS
Ip6CreateService (
  IN  EFI_HANDLE   Controller,
  IN  EFI_HANDLE   ImageHandle,
  OUT IP6_SERVICE  **Service
  )
{
  IP6_SERVICE                           *IpSb;
  EFI_STATUS                            Status;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *MnpToken;
  EFI_MANAGED_NETWORK_CONFIG_DATA       *Config;

  ASSERT (Service != NULL);

  *Service = NULL;

  //
  // allocate a service private data then initialize all the filed to
  // empty resources, so if any thing goes wrong when allocating
  // resources, Ip6CleanService can be called to clean it up.
  //
  IpSb = AllocateZeroPool (sizeof (IP6_SERVICE));

  if (IpSb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  IpSb->Signature                   = IP6_SERVICE_SIGNATURE;
  IpSb->ServiceBinding.CreateChild  = Ip6ServiceBindingCreateChild;
  IpSb->ServiceBinding.DestroyChild = Ip6ServiceBindingDestroyChild;
  IpSb->State                       = IP6_SERVICE_UNSTARTED;

  IpSb->NumChildren = 0;
  InitializeListHead (&IpSb->Children);

  InitializeListHead (&IpSb->Interfaces);
  IpSb->DefaultInterface = NULL;
  IpSb->RouteTable       = NULL;

  IpSb->RecvRequest.Signature = IP6_LINK_RX_SIGNATURE;
  IpSb->RecvRequest.CallBack  = NULL;
  IpSb->RecvRequest.Context   = NULL;
  MnpToken                    = &IpSb->RecvRequest.MnpToken;
  MnpToken->Event             = NULL;
  MnpToken->Status            = EFI_NOT_READY;
  MnpToken->Packet.RxData     = NULL;

  Ip6CreateAssembleTable (&IpSb->Assemble);

  IpSb->MldCtrl.Mldv1QuerySeen = 0;
  InitializeListHead (&IpSb->MldCtrl.Groups);

  ZeroMem (&IpSb->LinkLocalAddr, sizeof (EFI_IPv6_ADDRESS));
  IpSb->LinkLocalOk          = FALSE;
  IpSb->LinkLocalDadFail     = FALSE;
  IpSb->Dhcp6NeedStart       = FALSE;
  IpSb->Dhcp6NeedInfoRequest = FALSE;

  IpSb->CurHopLimit       = IP6_HOP_LIMIT;
  IpSb->LinkMTU           = IP6_MIN_LINK_MTU;
  IpSb->BaseReachableTime = IP6_REACHABLE_TIME;
  Ip6UpdateReachableTime (IpSb);
  //
  // RFC4861 RETRANS_TIMER: 1,000 milliseconds
  //
  IpSb->RetransTimer = IP6_RETRANS_TIMER;

  IpSb->RoundRobin = 0;

  InitializeListHead (&IpSb->NeighborTable);
  InitializeListHead (&IpSb->DefaultRouterList);
  InitializeListHead (&IpSb->OnlinkPrefix);
  InitializeListHead (&IpSb->AutonomousPrefix);

  IpSb->InterfaceIdLen = IP6_IF_ID_LEN;
  IpSb->InterfaceId    = NULL;

  IpSb->RouterAdvertiseReceived = FALSE;
  IpSb->SolicitTimer            = IP6_MAX_RTR_SOLICITATIONS;
  IpSb->Ticks                   = 0;

  IpSb->Image      = ImageHandle;
  IpSb->Controller = Controller;

  IpSb->MnpChildHandle = NULL;
  IpSb->Mnp            = NULL;

  Config                            = &IpSb->MnpConfigData;
  Config->ReceivedQueueTimeoutValue = 0;
  Config->TransmitQueueTimeoutValue = 0;
  Config->ProtocolTypeFilter        = IP6_ETHER_PROTO;
  Config->EnableUnicastReceive      = TRUE;
  Config->EnableMulticastReceive    = TRUE;
  Config->EnableBroadcastReceive    = TRUE;
  Config->EnablePromiscuousReceive  = FALSE;
  Config->FlushQueuesOnReset        = TRUE;
  Config->EnableReceiveTimestamps   = FALSE;
  Config->DisableBackgroundPolling  = FALSE;

  ZeroMem (&IpSb->SnpMode, sizeof (EFI_SIMPLE_NETWORK_MODE));

  IpSb->Timer       = NULL;
  IpSb->FasterTimer = NULL;

  ZeroMem (&IpSb->Ip6ConfigInstance, sizeof (IP6_CONFIG_INSTANCE));

  IpSb->MacString = NULL;

  //
  // Create various resources. First create the route table, timer
  // event, MNP token event and MNP child.
  //

  IpSb->RouteTable = Ip6CreateRouteTable ();
  if (IpSb->RouteTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  Ip6TimerTicking,
                  IpSb,
                  &IpSb->Timer
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  Ip6NdFasterTimerTicking,
                  IpSb,
                  &IpSb->FasterTimer
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = NetLibCreateServiceChild (
             Controller,
             ImageHandle,
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &IpSb->MnpChildHandle
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  IpSb->MnpChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **)(&IpSb->Mnp),
                  ImageHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Ip6ServiceConfigMnp (IpSb, TRUE);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = IpSb->Mnp->GetModeData (IpSb->Mnp, NULL, &IpSb->SnpMode);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  IpSb->MaxPacketSize = IP6_MIN_LINK_MTU - sizeof (EFI_IP6_HEADER);
  if (NetLibGetVlanId (IpSb->Controller) != 0) {
    //
    // This is a VLAN device, reduce MTU by VLAN tag length
    //
    IpSb->MaxPacketSize -= NET_VLAN_TAG_LEN;
  }

  IpSb->OldMaxPacketSize = IpSb->MaxPacketSize;

  //
  // Currently only ETHERNET is supported in IPv6 stack, since
  // link local address requires an IEEE 802 48-bit MACs for
  // EUI-64 format interface identifier mapping.
  //
  if (IpSb->SnpMode.IfType != NET_IFTYPE_ETHERNET) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  Status = Ip6InitMld (IpSb);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = NetLibGetMacString (IpSb->Controller, IpSb->Image, &IpSb->MacString);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Ip6ConfigInitInstance (&IpSb->Ip6ConfigInstance);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  IpSb->DefaultInterface = Ip6CreateInterface (IpSb, TRUE);
  if (IpSb->DefaultInterface == NULL) {
    Status = EFI_DEVICE_ERROR;
    goto ON_ERROR;
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  Ip6OnFrameReceived,
                  &IpSb->RecvRequest,
                  &MnpToken->Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  InsertHeadList (&IpSb->Interfaces, &IpSb->DefaultInterface->Link);

  *Service = IpSb;
  return EFI_SUCCESS;

ON_ERROR:
  Ip6CleanService (IpSb);
  FreePool (IpSb);
  return Status;
}

/**
  Start this driver on ControllerHandle.

  @param[in]  This                Protocol instance pointer.
  @param[in]  ControllerHandle    Handle of device to bind driver to.
  @param[in]  RemainingDevicePath Optional parameter used to pick a specific child
                                  device to start.

  @retval EFI_SUCCESS             This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED     This driver is already running on ControllerHandle.
  @retval other                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
Ip6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  IP6_SERVICE              *IpSb;
  EFI_STATUS               Status;
  EFI_IP6_CONFIG_PROTOCOL  *Ip6Cfg;
  IP6_CONFIG_DATA_ITEM     *DataItem;

  IpSb     = NULL;
  Ip6Cfg   = NULL;
  DataItem = NULL;

  //
  // Test for the Ip6 service binding protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  Status = Ip6CreateService (ControllerHandle, This->DriverBindingHandle, &IpSb);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (IpSb != NULL);

  Ip6Cfg = &IpSb->Ip6ConfigInstance.Ip6Config;

  //
  // Install the Ip6ServiceBinding Protocol onto ControllerHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  &IpSb->ServiceBinding,
                  &gEfiIp6ConfigProtocolGuid,
                  Ip6Cfg,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto FREE_SERVICE;
  }

  //
  // Read the config data from NV variable again.
  // The default data can be changed by other drivers.
  //
  Status = Ip6ConfigReadConfigData (IpSb->MacString, &IpSb->Ip6ConfigInstance);
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_PROTOCOL;
  }

  //
  // If there is any default manual address, set it.
  //
  DataItem = &IpSb->Ip6ConfigInstance.DataItem[Ip6ConfigDataTypeManualAddress];
  if (DataItem->Data.Ptr != NULL) {
    Status = Ip6Cfg->SetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeManualAddress,
                       DataItem->DataSize,
                       DataItem->Data.Ptr
                       );
    if ((Status == EFI_INVALID_PARAMETER) || (Status == EFI_BAD_BUFFER_SIZE)) {
      //
      // Clean the invalid ManualAddress configuration.
      //
      Status = Ip6Cfg->SetData (
                         Ip6Cfg,
                         Ip6ConfigDataTypeManualAddress,
                         0,
                         NULL
                         );
      DEBUG ((DEBUG_WARN, "Ip6DriverBindingStart: Clean the invalid ManualAddress configuration.\n"));
    }
  }

  //
  // If there is any default gateway address, set it.
  //
  DataItem = &IpSb->Ip6ConfigInstance.DataItem[Ip6ConfigDataTypeGateway];
  if (DataItem->Data.Ptr != NULL) {
    Status = Ip6Cfg->SetData (
                       Ip6Cfg,
                       Ip6ConfigDataTypeGateway,
                       DataItem->DataSize,
                       DataItem->Data.Ptr
                       );
    if ((Status == EFI_INVALID_PARAMETER) || (Status == EFI_BAD_BUFFER_SIZE)) {
      //
      // Clean the invalid Gateway configuration.
      //
      Status = Ip6Cfg->SetData (
                         Ip6Cfg,
                         Ip6ConfigDataTypeGateway,
                         0,
                         NULL
                         );
      DEBUG ((DEBUG_WARN, "Ip6DriverBindingStart: Clean the invalid Gateway configuration.\n"));
    }
  }

  //
  // ready to go: start the receiving and timer
  //
  Status = Ip6ReceiveFrame (Ip6AcceptFrame, IpSb);
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_PROTOCOL;
  }

  //
  // The timer expires every 100 (IP6_TIMER_INTERVAL_IN_MS) milliseconds.
  //
  Status = gBS->SetTimer (
                  IpSb->FasterTimer,
                  TimerPeriodic,
                  TICKS_PER_MS * IP6_TIMER_INTERVAL_IN_MS
                  );
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_PROTOCOL;
  }

  //
  // The timer expires every 1000 (IP6_ONE_SECOND_IN_MS) milliseconds.
  //
  Status = gBS->SetTimer (
                  IpSb->Timer,
                  TimerPeriodic,
                  TICKS_PER_MS * IP6_ONE_SECOND_IN_MS
                  );
  if (EFI_ERROR (Status)) {
    goto UNINSTALL_PROTOCOL;
  }

  //
  // Initialize the IP6 ID
  //
  mIp6Id = NET_RANDOM (NetRandomInitSeed ());

  return EFI_SUCCESS;

UNINSTALL_PROTOCOL:
  gBS->UninstallMultipleProtocolInterfaces (
         ControllerHandle,
         &gEfiIp6ServiceBindingProtocolGuid,
         &IpSb->ServiceBinding,
         &gEfiIp6ConfigProtocolGuid,
         Ip6Cfg,
         NULL
         );

FREE_SERVICE:
  Ip6CleanService (IpSb);
  FreePool (IpSb);
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
Ip6DestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY  *Entry,
  IN VOID        *Context
  )
{
  IP6_PROTOCOL                  *IpInstance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if ((Entry == NULL) || (Context == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IpInstance        = NET_LIST_USER_STRUCT_S (Entry, IP6_PROTOCOL, Link, IP6_PROTOCOL_SIGNATURE);
  ServiceBinding    = ((IP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ServiceBinding;
  NumberOfChildren  = ((IP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->NumberOfChildren;
  ChildHandleBuffer = ((IP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (IpInstance->Handle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, IpInstance->Handle);
}

/**
  Stop this driver on ControllerHandle.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ControllerHandle   Handle of device to stop driver on.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer. If number
                                 of children is zero, stop the entire  bus driver.
  @param[in]  ChildHandleBuffer  An array of child handles to be freed. May be NULL
                                 if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
Ip6DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_SERVICE_BINDING_PROTOCOL             *ServiceBinding;
  IP6_SERVICE                              *IpSb;
  EFI_HANDLE                               NicHandle;
  EFI_STATUS                               Status;
  LIST_ENTRY                               *List;
  INTN                                     State;
  BOOLEAN                                  IsDhcp6;
  IP6_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  IsDhcp6   = FALSE;
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiManagedNetworkProtocolGuid);
  if (NicHandle == NULL) {
    NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiDhcp6ProtocolGuid);
    if (NicHandle != NULL) {
      IsDhcp6 = TRUE;
    } else {
      return EFI_SUCCESS;
    }
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiIp6ServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  IpSb = IP6_SERVICE_FROM_PROTOCOL (ServiceBinding);

  if (IsDhcp6) {
    Status = Ip6ConfigDestroyDhcp6 (&IpSb->Ip6ConfigInstance);
    gBS->CloseEvent (IpSb->Ip6ConfigInstance.Dhcp6Event);
    IpSb->Ip6ConfigInstance.Dhcp6Event = NULL;
  } else if (NumberOfChildren != 0) {
    //
    // NumberOfChildren is not zero, destroy the IP6 children instances in ChildHandleBuffer.
    //
    List                      = &IpSb->Children;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status                    = NetDestroyLinkList (
                                  List,
                                  Ip6DestroyChildEntryInHandleBuffer,
                                  &Context,
                                  NULL
                                  );
  } else if (IsListEmpty (&IpSb->Children)) {
    State  = IpSb->State;
    Status = Ip6CleanService (IpSb);
    if (EFI_ERROR (Status)) {
      IpSb->State = State;
      goto Exit;
    }

    Status = gBS->UninstallMultipleProtocolInterfaces (
                    NicHandle,
                    &gEfiIp6ServiceBindingProtocolGuid,
                    ServiceBinding,
                    &gEfiIp6ConfigProtocolGuid,
                    &IpSb->Ip6ConfigInstance.Ip6Config,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
    FreePool (IpSb);
    Status = EFI_SUCCESS;
  }

Exit:
  return Status;
}

/**
  Creates a child handle with a set of I/O services.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ChildHandle        Pointer to the handle of the child to create.   If
                                 it is NULL, then a new handle is created.   If it
                                 is not NULL, then the I/O services are added to
                                 the existing child handle.

  @retval EFI_SUCCESS            The child handle was created with the I/O services.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to create
                                 the child.
  @retval other                  The child handle was not created.

**/
EFI_STATUS
EFIAPI
Ip6ServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  IP6_SERVICE   *IpSb;
  IP6_PROTOCOL  *IpInstance;
  EFI_TPL       OldTpl;
  EFI_STATUS    Status;
  VOID          *Mnp;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  IpSb = IP6_SERVICE_FROM_PROTOCOL (This);

  if (IpSb->LinkLocalDadFail) {
    return EFI_DEVICE_ERROR;
  }

  IpInstance = AllocatePool (sizeof (IP6_PROTOCOL));

  if (IpInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ip6InitProtocol (IpSb, IpInstance);

  //
  // Install Ip6 onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  &IpInstance->Ip6Proto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  IpInstance->Handle = *ChildHandle;

  //
  // Open the Managed Network protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  IpSb->MnpChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **)&Mnp,
                  gIp6DriverBinding.DriverBindingHandle,
                  IpInstance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           *ChildHandle,
           &gEfiIp6ProtocolGuid,
           &IpInstance->Ip6Proto,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Insert it into the service binding instance.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&IpSb->Children, &IpInstance->Link);
  IpSb->NumChildren++;

  gBS->RestoreTPL (OldTpl);

ON_ERROR:

  if (EFI_ERROR (Status)) {
    Ip6CleanProtocol (IpInstance);

    FreePool (IpInstance);
  }

  return Status;
}

/**
  Destroys a child handle with a set of I/O services.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCESS            The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is NULL.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
Ip6ServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS        Status;
  IP6_SERVICE       *IpSb;
  IP6_PROTOCOL      *IpInstance;
  EFI_IP6_PROTOCOL  *Ip6;
  EFI_TPL           OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  IpSb = IP6_SERVICE_FROM_PROTOCOL (This);

  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  (VOID **)&Ip6,
                  gIp6DriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  IpInstance = IP6_INSTANCE_FROM_PROTOCOL (Ip6);

  if (IpInstance->Service != IpSb) {
    return EFI_INVALID_PARAMETER;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // A child can be destroyed more than once. For example,
  // Ip6DriverBindingStop will destroy all of its children.
  // when UDP driver is being stopped, it will destroy all
  // the IP child it opens.
  //
  if (IpInstance->InDestroy) {
    gBS->RestoreTPL (OldTpl);
    return EFI_SUCCESS;
  }

  IpInstance->InDestroy = TRUE;

  //
  // Close the Managed Network protocol.
  //
  gBS->CloseProtocol (
         IpSb->MnpChildHandle,
         &gEfiManagedNetworkProtocolGuid,
         gIp6DriverBinding.DriverBindingHandle,
         ChildHandle
         );

  //
  // Uninstall the IP6 protocol first. Many thing happens during
  // this:
  // 1. The consumer of the IP6 protocol will be stopped if it
  // opens the protocol BY_DRIVER. For example, if MNP driver is
  // stopped, IP driver's stop function will be called, and uninstall
  // EFI_IP6_PROTOCOL will trigger the UDP's stop function. This
  // makes it possible to create the network stack bottom up, and
  // stop it top down.
  // 2. the upper layer will recycle the received packet. The recycle
  // event's TPL is higher than this function. The recycle events
  // will be called back before preceding. If any packets not recycled,
  // that means there is a resource leak.
  //
  gBS->RestoreTPL (OldTpl);
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiIp6ProtocolGuid,
                  &IpInstance->Ip6Proto
                  );
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Ip6CleanProtocol (IpInstance);
  if (EFI_ERROR (Status)) {
    gBS->InstallMultipleProtocolInterfaces (
           &ChildHandle,
           &gEfiIp6ProtocolGuid,
           Ip6,
           NULL
           );

    goto ON_ERROR;
  }

  RemoveEntryList (&IpInstance->Link);
  ASSERT (IpSb->NumChildren > 0);
  IpSb->NumChildren--;

  gBS->RestoreTPL (OldTpl);

  FreePool (IpInstance);
  return EFI_SUCCESS;

ON_ERROR:
  gBS->RestoreTPL (OldTpl);

  return Status;
}
