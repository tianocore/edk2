/** @file
  ARP driver functions.
  
Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

  @param[in]       ImageHandle       The image handle representing the loaded driver
                                     image.
  @param[in]       ControllerHandle  The controller handle the driver binds to.
  @param[in, out]  ArpService        Pointer to the buffer containing the arp service
                                     context data.

  @retval EFI_SUCCESS                The arp service context is initialized.
  
  @retval EFI_UNSUPPORTED            The underlayer Snp mode type is not ethernet.
                                     Failed to initialize the service context.
  @retval other                      Failed to initialize the arp service context.

**/
EFI_STATUS
ArpCreateService (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_HANDLE        ControllerHandle,
  IN OUT ARP_SERVICE_DATA  *ArpService
  )
{
  EFI_STATUS  Status;

  ASSERT (ArpService != NULL);

  ArpService->Signature = ARP_SERVICE_DATA_SIGNATURE;

  //
  // Init the lists.
  //
  InitializeListHead (&ArpService->ChildrenList);
  InitializeListHead (&ArpService->PendingRequestTable);
  InitializeListHead (&ArpService->DeniedCacheTable);
  InitializeListHead (&ArpService->ResolvedCacheTable);

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

ERROR_EXIT:

  return Status;
}


/**
  Clean the arp service context data.

  @param[in, out]  ArpService        Pointer to the buffer containing the arp service
                                     context data.

  @return None.

**/
VOID
ArpCleanService (
  IN OUT ARP_SERVICE_DATA  *ArpService
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
  Tests to see if this driver supports a given controller. 
  
  If a child device is provided, it further tests to see if this driver supports 
  creating a handle for the specified child device.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, 
                                   and is optional for bus drivers.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver 
                                   specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed 
                                   by the driver specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by 
                                   a different driver or an application that 
                                   requires exclusive acces. Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the 
                                   driver specified by This.

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
  
  The Start() function is designed to be invoked from the EFI boot service ConnectController(). 
  As a result, much of the error checking on the parameters to Start() has been 
  moved into this common boot service. It is legal to call Start() from other locations, 
  but the following calling restrictions must be followed or the system behavior 
  will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally 
     aligned EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified 
     by This must have been called with the same calling parameters, and Supported() 
     must have returned EFI_SUCCESS.  

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, 
                                   and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of 
                                   resources.
  @retval Others                   The driver failded to start the device.

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
  FreePool (ArpService);

  return Status;
}


/**
  Stop this driver on ControllerHandle. 
  
  Release the control of this controller and remove the IScsi functions. The Stop()
  function is designed to be invoked from the EFI boot service DisconnectController(). 
  As a result, much of the error checking on the parameters to Stop() has been moved 
  into this common boot service. It is legal to call Stop() from other locations, 
  but the following calling restrictions must be followed or the system behavior 
  will not be deterministic.
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
                                Not used.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.Not used.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

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

    FreePool (ArpService);
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
  Creates a child handle and installs a protocol.
  
  The CreateChild() function installs a protocol on ChildHandle. 
  If ChildHandle is a pointer to NULL, then a new handle is created and returned 
  in ChildHandle. If ChildHandle is not a pointer to NULL, then the protocol 
  installs on the existing ChildHandle.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Pointer to the handle of the child to create. If it is NULL,
                      then a new handle is created. If it is a pointer to an existing 
                      UEFI handle, then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources availabe to create
                                the child
  @retval other                 The child handle was not created

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

    FreePool (Instance);
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
    FreePool (Instance);
  }

  return Status;
}


/**
  Destroys a child handle with a protocol installed on it.
  
  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol 
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the 
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is 
                                being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

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

  FreePool (Instance);

  return Status;
}

/**
  The entry point for Arp driver which installs the driver binding and component name
  protocol on its ImageHandle.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            if the driver binding and component name protocols 
                                 are successfully
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

