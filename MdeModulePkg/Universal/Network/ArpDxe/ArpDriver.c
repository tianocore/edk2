/** @file

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  ArpDriver.c

Abstract:


**/


#include "ArpDriver.h"
#include "ArpImpl.h"

EFI_DRIVER_BINDING_PROTOCOL gArpDriverBinding = {
  ArpDriverBindingSupported,
  ArpDriverBindingStart,
  ArpDriverBindingStop,
  0xa,
  NULL,
  NULL
};


/**
  Create and initialize the arp service context data.

  @param  ImageHandle            The image handle representing the loaded driver
                                 image.
  @param  ControllerHandle       The controller handle the driver binds to.
  @param  ArpService             Pointer to the buffer containing the arp service
                                 context data.

  @retval EFI_SUCCESS            The arp service context is initialized.
  @retval other                  Failed to initialize the arp service context.

**/
EFI_STATUS
ArpCreateService (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_HANDLE        ControllerHandle,
  IN ARP_SERVICE_DATA  *ArpService
  )
{
  EFI_STATUS  Status;

  ASSERT (ArpService != NULL);

  ArpService->Signature = ARP_SERVICE_DATA_SIGNATURE;

  //
  // Init the servicebinding protocol members.
  //
  ArpService->ServiceBinding.CreateChild  = ArpServiceBindingCreateChild;
  ArpService->ServiceBinding.DestroyChild = ArpServiceBindingDestroyChild;

  //
  // Save the handles.
  //
  ArpService->ImageHandle      = ImageHandle;
  ArpService->ControllerHandle = ControllerHandle;

  //
  // Create a MNP child instance.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             ImageHandle,
             &gEfiManagedNetworkServiceBindingProtocolGuid,
             &ArpService->MnpChildHandle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Open the MNP protocol.
  //
  Status = gBS->OpenProtocol (
                  ArpService->MnpChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **)&ArpService->Mnp,
                  ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  //
  // Get the underlayer Snp mode data.
  //
  Status = ArpService->Mnp->GetModeData (ArpService->Mnp, NULL, &ArpService->SnpMode);
  if ((Status != EFI_NOT_STARTED) && EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  if (ArpService->SnpMode.IfType != NET_IFTYPE_ETHERNET) {
    //
    // Only support the ethernet.
    //
    Status = EFI_UNSUPPORTED;
    goto ERROR_EXIT;
  }

  //
  // Set the Mnp config parameters.
  //
  ArpService->MnpConfigData.ReceivedQueueTimeoutValue = 0;
  ArpService->MnpConfigData.TransmitQueueTimeoutValue = 0;
  ArpService->MnpConfigData.ProtocolTypeFilter        = ARP_ETHER_PROTO_TYPE;
  ArpService->MnpConfigData.EnableUnicastReceive      = TRUE;
  ArpService->MnpConfigData.EnableMulticastReceive    = FALSE;
  ArpService->MnpConfigData.EnableBroadcastReceive    = TRUE;
  ArpService->MnpConfigData.EnablePromiscuousReceive  = FALSE;
  ArpService->MnpConfigData.FlushQueuesOnReset        = TRUE;
  ArpService->MnpConfigData.EnableReceiveTimestamps   = FALSE;
  ArpService->MnpConfigData.DisableBackgroundPolling  = FALSE;

  //
  // Configure the Mnp child.
  //
  Status = ArpService->Mnp->Configure (ArpService->Mnp, &ArpService->MnpConfigData);
  if (EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  //
  // Create the event used in the RxToken.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  ArpOnFrameRcvd,
                  ArpService,
                  &ArpService->RxToken.Event
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  //
  // Create the Arp heartbeat timer.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  ArpTimerHandler,
                  ArpService,
                  &ArpService->PeriodicTimer
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  //
  // Start the heartbeat timer.
  //
  Status = gBS->SetTimer (
                  ArpService->PeriodicTimer,
                  TimerPeriodic,
                  ARP_PERIODIC_TIMER_INTERVAL
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR_EXIT;
  }

  //
  // Init the lists.
  //
  InitializeListHead (&ArpService->ChildrenList);
  InitializeListHead (&ArpService->PendingRequestTable);
  InitializeListHead (&ArpService->DeniedCacheTable);
  InitializeListHead (&ArpService->ResolvedCacheTable);

ERROR_EXIT:

  return Status;
}


/**
  Clean the arp service context data.

  @param  ArpService             Pointer to the buffer containing the arp service
                                 context data.

  @return None.

**/
VOID
ArpCleanService (
  IN ARP_SERVICE_DATA  *ArpService
  )
{
  NET_CHECK_SIGNATURE (ArpService, ARP_SERVICE_DATA_SIGNATURE);

  if (ArpService->PeriodicTimer != NULL) {
    //
    // Cancle and close the PeriodicTimer.
    //
    gBS->SetTimer (ArpService->PeriodicTimer, TimerCancel, 0);
    gBS->CloseEvent (ArpService->PeriodicTimer);
  }

  if (ArpService->RxToken.Event != NULL) {
    //
    // Cancle the RxToken and close the event in the RxToken.
    //
    ArpService->Mnp->Cancel (ArpService->Mnp, NULL);
    gBS->CloseEvent (ArpService->RxToken.Event);
  }

  if (ArpService->Mnp != NULL) {
    //
    // Reset the Mnp child and close the Mnp protocol.
    //
    ArpService->Mnp->Configure (ArpService->Mnp, NULL);
    gBS->CloseProtocol (
           ArpService->MnpChildHandle,
           &gEfiManagedNetworkProtocolGuid,
           ArpService->ImageHandle,
           ArpService->ControllerHandle
           );
  }

  if (ArpService->MnpChildHandle != NULL) {
    //
    // Destroy the mnp child.
    //
    NetLibDestroyServiceChild(
      ArpService->ControllerHandle,
      ArpService->ImageHandle,
      &gEfiManagedNetworkServiceBindingProtocolGuid,
      ArpService->MnpChildHandle
      );
  }
}


/**
  Test to see if this driver supports ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to test.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver supports this device
  @retval EFI_ALREADY_STARTED    This driver is already running on this device.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
ArpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  //
  // Test to see if Arp SB is already installed.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiArpServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (Status == EFI_SUCCESS) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test to see if MNP SB is installed.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
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
ArpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;
  ARP_SERVICE_DATA  *ArpService;

  //
  // Allocate a zero pool for ArpService.
  //
  ArpService = AllocateZeroPool (sizeof(ARP_SERVICE_DATA));
  if (ArpService == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the arp service context data.
  //
  Status = ArpCreateService (This->DriverBindingHandle, ControllerHandle, ArpService);
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  //
  // Install the ARP service binding protocol.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiArpServiceBindingProtocolGuid,
                  &ArpService->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  //
  // OK, start to receive arp packets from Mnp.
  //
  Status = ArpService->Mnp->Receive (ArpService->Mnp, &ArpService->RxToken);
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  return Status;

ERROR:

  //
  // On error, clean the arp service context data, and free the memory allocated.
  //
  ArpCleanService (ArpService);
  gBS->FreePool (ArpService);

  return Status;
}


/**
  Stop this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to stop driver on
  @param  NumberOfChildren       Number of Handles in ChildHandleBuffer. If number
                                 of  children is zero stop the entire bus driver.
  @param  ChildHandleBuffer      List of Child Handles to Stop.

  @retval EFI_SUCCES             This driver is removed ControllerHandle
  @retval other                  This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
ArpDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    NicHandle;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  ARP_SERVICE_DATA              *ArpService;
  ARP_INSTANCE_DATA             *Instance;

  //
  // Get the NicHandle which the arp servicebinding is installed on.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiManagedNetworkProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Try to get the arp servicebinding protocol on the NicHandle.
  //
  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiArpServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArpDriverBindingStop: Open ArpSb failed, %r.\n", Status));
    return EFI_DEVICE_ERROR;
  }

  ArpService = ARP_SERVICE_DATA_FROM_THIS (ServiceBinding);

  if (NumberOfChildren == 0) {
    //
    // Uninstall the ARP ServiceBinding protocol.
    //
    gBS->UninstallMultipleProtocolInterfaces (
           NicHandle,
           &gEfiArpServiceBindingProtocolGuid,
           &ArpService->ServiceBinding,
           NULL
           );

    //
    // Clean the arp servicebinding context data and free the memory allocated.
    //
    ArpCleanService (ArpService);

    gBS->FreePool (ArpService);
  } else {

    while (!IsListEmpty (&ArpService->ChildrenList)) {
      Instance = NET_LIST_HEAD (&ArpService->ChildrenList, ARP_INSTANCE_DATA, List);

      ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
    }

    ASSERT (IsListEmpty (&ArpService->PendingRequestTable));
    ASSERT (IsListEmpty (&ArpService->DeniedCacheTable));
    ASSERT (IsListEmpty (&ArpService->ResolvedCacheTable));
  }

  return EFI_SUCCESS;
}

/**
  Creates a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Pointer to the handle of the child to create. If
                                 it is NULL, then a new handle is created. If it is
                                 not NULL, then the I/O services are  added to the
                                 existing child handle.

  @retval EFI_SUCCES             The child handle was created with the I/O
                                 services.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources availabe to create
                                 the child.
  @retval other                  The child handle was not created.

**/
EFI_STATUS
EFIAPI
ArpServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  EFI_STATUS         Status;
  ARP_SERVICE_DATA   *ArpService;
  ARP_INSTANCE_DATA  *Instance;
  VOID               *Mnp;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ArpService = ARP_SERVICE_DATA_FROM_THIS (This);

  //
  // Allocate memory for the instance context data.
  //
  Instance = AllocateZeroPool (sizeof(ARP_INSTANCE_DATA));
  if (Instance == NULL) {
    DEBUG ((EFI_D_ERROR, "ArpSBCreateChild: Failed to allocate memory for Instance.\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Init the instance context data.
  //
  ArpInitInstance (ArpService, Instance);

  //
  // Install the ARP protocol onto the ChildHandle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiArpProtocolGuid,
                  (VOID *)&Instance->ArpProto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArpSBCreateChild: faild to install ARP protocol, %r.\n", Status));

    gBS->FreePool (Instance);
    return Status;
  }

  //
  // Save the ChildHandle.
  //
  Instance->Handle = *ChildHandle;

  //
  // Open the Managed Network protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  ArpService->MnpChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp,
                  gArpDriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Insert the instance into children list managed by the arp service context data.
  //
  InsertTailList (&ArpService->ChildrenList, &Instance->List);
  ArpService->ChildrenNumber++;

  gBS->RestoreTPL (OldTpl);

ERROR:

  if (EFI_ERROR (Status)) {

    gBS->CloseProtocol (
           ArpService->MnpChildHandle,
           &gEfiManagedNetworkProtocolGuid,
           gArpDriverBinding.DriverBindingHandle,
           Instance->Handle
           );

    gBS->UninstallMultipleProtocolInterfaces (
           Instance->Handle,
           &gEfiArpProtocolGuid,
           &Instance->ArpProto,
           NULL
           );

    //
    // Free the allocated memory.
    //
    gBS->FreePool (Instance);
  }

  return Status;
}


/**
  Destroys a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is not a valid EFI Handle.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
ArpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS         Status;
  ARP_SERVICE_DATA   *ArpService;
  ARP_INSTANCE_DATA  *Instance;
  EFI_ARP_PROTOCOL   *Arp;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ArpService = ARP_SERVICE_DATA_FROM_THIS (This);

  //
  // Get the arp protocol.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiArpProtocolGuid,
                  (VOID **)&Arp,
                  ArpService->ImageHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = ARP_INSTANCE_DATA_FROM_THIS (Arp);

  if (Instance->Destroyed) {
    return EFI_SUCCESS;
  }

  //
  // Use the Destroyed as a flag to avoid re-entrance.
  //
  Instance->Destroyed = TRUE;

  //
  // Close the Managed Network protocol.
  //
  gBS->CloseProtocol (
         ArpService->MnpChildHandle,
         &gEfiManagedNetworkProtocolGuid,
         gArpDriverBinding.DriverBindingHandle,
         ChildHandle
         );

  //
  // Uninstall the ARP protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiArpProtocolGuid,
                  &Instance->ArpProto,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "ArpSBDestroyChild: Failed to uninstall the arp protocol, %r.\n",
      Status));

    Instance->Destroyed = FALSE;
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (Instance->Configured) {
    //
    // Delete the related cache entry.
    //
    ArpDeleteCacheEntry (Instance, FALSE, NULL, TRUE);

    //
    // Reset the instance configuration.
    //
    ArpConfigureInstance (Instance, NULL);
  }

  //
  // Remove this instance from the ChildrenList.
  //
  RemoveEntryList (&Instance->List);
  ArpService->ChildrenNumber--;

  gBS->RestoreTPL (OldTpl);

  gBS->FreePool (Instance);

  return Status;
}

/**
  The entry point for Arp driver which installs the driver binding and component name
  protocol on its ImageHandle.

  @param  ImageHandle            The image handle of the driver.
  @param  SystemTable            The system table.

  @retval EFI_SUCCES             if the driver binding and component name protocols are successfully
  @retval Others                 Failed to install the protocols.

**/
EFI_STATUS
EFIAPI
ArpDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gArpDriverBinding,
           ImageHandle,
           &gArpComponentName,
           &gArpComponentName2
           );
}

