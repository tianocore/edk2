/** @file
  The driver binding and service binding protocol for Redfish RestExDxe driver.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include "RedfishRestExDriver.h"

EFI_DRIVER_BINDING_PROTOCOL  gRedfishRestExDriverBinding = {
  RedfishRestExDriverBindingSupported,
  RedfishRestExDriverBindingStart,
  RedfishRestExDriverBindingStop,
  REDFISH_RESTEX_DRIVER_VERSION,
  NULL,
  NULL
};

EFI_SERVICE_BINDING_PROTOCOL  mRedfishRestExServiceBinding = {
  RedfishRestExServiceBindingCreateChild,
  RedfishRestExServiceBindingDestroyChild
};

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
RestExDestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY  *Entry,
  IN VOID        *Context
  )
{
  RESTEX_INSTANCE               *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if ((Entry == NULL) || (Context == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Instance          = NET_LIST_USER_STRUCT_S (Entry, RESTEX_INSTANCE, Link, RESTEX_INSTANCE_SIGNATURE);
  ServiceBinding    = ((RESTEX_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ServiceBinding;
  NumberOfChildren  = ((RESTEX_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->NumberOfChildren;
  ChildHandleBuffer = ((RESTEX_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (Instance->ChildHandle, NumberOfChildren, ChildHandleBuffer)) {
    RemoveEntryList (&Instance->Link);
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, Instance->ChildHandle);
}

/**
  Destroy the RestEx instance and recycle the resources.

  @param[in]  Instance        The pointer to the RestEx instance.

**/
VOID
RestExDestroyInstance (
  IN RESTEX_INSTANCE  *Instance
  )
{
  HttpIoDestroyIo (&(Instance->HttpIo));

  FreePool (Instance);
}

/**
  Create the RestEx instance and initialize it.

  @param[in]  Service              The pointer to the RestEx service.
  @param[out] Instance             The pointer to the RestEx instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The RestEx instance is created.

**/
EFI_STATUS
RestExCreateInstance (
  IN  RESTEX_SERVICE   *Service,
  OUT RESTEX_INSTANCE  **Instance
  )
{
  RESTEX_INSTANCE  *RestExIns;
  EFI_STATUS       Status;

  *Instance = NULL;
  Status    = EFI_SUCCESS;

  RestExIns = AllocateZeroPool (sizeof (RESTEX_INSTANCE));
  if (RestExIns == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RestExIns->Signature = RESTEX_INSTANCE_SIGNATURE;
  InitializeListHead (&RestExIns->Link);
  RestExIns->InDestroy = FALSE;
  RestExIns->Service   = Service;

  CopyMem (&RestExIns->RestEx, &mRedfishRestExProtocol, sizeof (RestExIns->RestEx));

  //
  // Create a HTTP_IO to access the HTTP service.
  //
  Status = HttpIoCreateIo (
             RestExIns->Service->ImageHandle,
             RestExIns->Service->ControllerHandle,
             IP_VERSION_4,
             NULL,
             NULL,
             NULL,
             &(RestExIns->HttpIo)
             );
  if (EFI_ERROR (Status)) {
    FreePool (RestExIns);
    return Status;
  }

  *Instance = RestExIns;

  return EFI_SUCCESS;
}

/**
  Release all the resource used the RestEx service binding instance.

  @param[in]  RestExSb                The RestEx service binding instance.

**/
VOID
RestExDestroyService (
  IN RESTEX_SERVICE  *RestExSb
  )
{
  if (RestExSb->HttpChildHandle != NULL) {
    gBS->CloseProtocol (
           RestExSb->HttpChildHandle,
           &gEfiHttpProtocolGuid,
           RestExSb->ImageHandle,
           RestExSb->ControllerHandle
           );

    NetLibDestroyServiceChild (
      RestExSb->ControllerHandle,
      RestExSb->ImageHandle,
      &gEfiHttpServiceBindingProtocolGuid,
      RestExSb->HttpChildHandle
      );

    RestExSb->HttpChildHandle = NULL;
  }

  gBS->UninstallProtocolInterface (
         RestExSb->ControllerHandle,
         &gEfiCallerIdGuid,
         &RestExSb->Id
         );

  FreePool (RestExSb);
}

/**
  Check the NIC controller handle represents an in-band or out-of-band Redfish host
  interface device. If not in-band, treat it as out-of-band interface device.

  @param[in]   Controller       The NIC controller handle needs to be checked.

  @return     EFI_REST_EX_SERVICE_ACCESS_MODE of the device.

**/
EFI_REST_EX_SERVICE_ACCESS_MODE
RestExServiceAccessMode (
  IN     EFI_HANDLE  Controller
  )
{
  //
  // This is EFI REST EX driver instance to connect
  // to Redfish service using HTTP in out of band.
  //
  if (FixedPcdGetBool (PcdRedfishRestExServiceAccessModeInBand)) {
    return EfiRestExServiceInBandAccess;
  } else {
    return EfiRestExServiceOutOfBandAccess;
  }
}

/**
  Create then initialize a RestEx service binding instance.

  @param[in]   Controller       The controller to install the RestEx service
                                binding on.
  @param[in]   Image            The driver binding image of the RestEx driver.
  @param[out]  Service          The variable to receive the created service
                                binding instance.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to create the instance.
  @retval EFI_SUCCESS           The service instance is created for the controller.

**/
EFI_STATUS
RestExCreateService (
  IN     EFI_HANDLE      Controller,
  IN     EFI_HANDLE      Image,
  OUT    RESTEX_SERVICE  **Service
  )
{
  EFI_STATUS      Status;
  RESTEX_SERVICE  *RestExSb;

  Status   = EFI_SUCCESS;
  RestExSb = NULL;

  *Service = NULL;

  RestExSb = AllocateZeroPool (sizeof (RESTEX_SERVICE));
  if (RestExSb == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  RestExSb->Signature = RESTEX_SERVICE_SIGNATURE;

  RestExSb->ServiceBinding = mRedfishRestExServiceBinding;

  RestExSb->RestExChildrenNum = 0;
  InitializeListHead (&RestExSb->RestExChildrenList);

  RestExSb->ControllerHandle = Controller;
  RestExSb->ImageHandle      = Image;

  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.EfiRestExServiceInfoHeader.Length                   = sizeof (EFI_REST_EX_SERVICE_INFO);
  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.EfiRestExServiceInfoHeader.RestServiceInfoVer.Major = 1;
  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.EfiRestExServiceInfoHeader.RestServiceInfoVer.Minor = 0;
  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.RestServiceType                                     = EfiRestExServiceRedfish;
  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.RestServiceAccessMode                               = RestExServiceAccessMode (Controller);
  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.RestExConfigType                                    = EfiRestExConfigHttp;
  RestExSb->RestExServiceInfo.EfiRestExServiceInfoV10.RestExConfigDataLength                              = sizeof (EFI_REST_EX_HTTP_CONFIG_DATA);

  Status = gBS->InstallProtocolInterface (
                  &Controller,
                  &gEfiCallerIdGuid,
                  EFI_NATIVE_INTERFACE,
                  &RestExSb->Id
                  );
  if (EFI_ERROR (Status)) {
    FreePool (RestExSb);
    RestExSb = NULL;
  }

  *Service = RestExSb;
  return Status;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param[in]   ImageHandle     The firmware allocated handle for the UEFI image.
  @param[in]   SystemTable     A pointer to the EFI System Table.

  @retval EFI_SUCCESS          The operation completed successfully.
  @retval Others               An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RedfishRestExDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  //
  // Install the RestEx Driver Binding Protocol.
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gRedfishRestExDriverBinding,
             ImageHandle,
             &gRedfishRestExComponentName,
             &gRedfishRestExComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
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
RedfishRestExDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      *Id;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **)&Id,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test for the HttpServiceBinding Protocol.
  //
  return gBS->OpenProtocol (
                ControllerHandle,
                &gEfiHttpServiceBindingProtocolGuid,
                NULL,
                This->DriverBindingHandle,
                ControllerHandle,
                EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                );
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
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
RedfishRestExDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  RESTEX_SERVICE  *RestExSb;
  EFI_STATUS      Status;
  UINT32          *Id;
  VOID            *Interface;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **)&Id,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = RestExCreateService (ControllerHandle, This->DriverBindingHandle, &RestExSb);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (RestExSb != NULL);

  //
  // Create a Http child instance, but do not configure it.
  // This will establish the parent-child relationship.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiHttpServiceBindingProtocolGuid,
             &RestExSb->HttpChildHandle
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  RestExSb->HttpChildHandle,
                  &gEfiHttpProtocolGuid,
                  &Interface,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Install the RestEx ServiceBinding Protocol onto ControllerHandle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiRestExServiceBindingProtocolGuid,
                  &RestExSb->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  RestExDestroyService (RestExSb);

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
RedfishRestExDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_SERVICE_BINDING_PROTOCOL                *ServiceBinding;
  RESTEX_SERVICE                              *RestExSb;
  EFI_HANDLE                                  NicHandle;
  EFI_STATUS                                  Status;
  LIST_ENTRY                                  *List;
  RESTEX_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;

  //
  // RestEx driver opens HTTP child, So, Controller is a HTTP
  // child handle. Locate the Nic handle first. Then get the
  // RestEx private data back.
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiHttpProtocolGuid);
  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiRestExServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  RestExSb = RESTEX_SERVICE_FROM_THIS (ServiceBinding);

  if (!IsListEmpty (&RestExSb->RestExChildrenList)) {
    //
    // Destroy the RestEx child instance in ChildHandleBuffer.
    //
    List                      = &RestExSb->RestExChildrenList;
    Context.ServiceBinding    = ServiceBinding;
    Context.NumberOfChildren  = NumberOfChildren;
    Context.ChildHandleBuffer = ChildHandleBuffer;
    Status                    = NetDestroyLinkList (
                                  List,
                                  RestExDestroyChildEntryInHandleBuffer,
                                  &Context,
                                  NULL
                                  );
  }

  if (IsListEmpty (&RestExSb->RestExChildrenList)) {
    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiRestExServiceBindingProtocolGuid,
           ServiceBinding
           );

    RestExDestroyService (RestExSb);

    if (gRedfishRestExControllerNameTable != NULL) {
      FreeUnicodeStringTable (gRedfishRestExControllerNameTable);
      gRedfishRestExControllerNameTable = NULL;
    }

    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Callback function that is invoked when HTTP event occurs.

  @param[in]  This                Pointer to the EDKII_HTTP_CALLBACK_PROTOCOL instance.
  @param[in]  Event               The event that occurs in the current state.
  @param[in]  EventStatus         The Status of Event, EFI_SUCCESS or other errors.
**/
VOID
EFIAPI
RestExHttpCallback (
  IN EDKII_HTTP_CALLBACK_PROTOCOL  *This,
  IN EDKII_HTTP_CALLBACK_EVENT     Event,
  IN EFI_STATUS                    EventStatus
  )
{
  EFI_STATUS        Status;
  EFI_TLS_PROTOCOL  *TlsProtocol;
  RESTEX_INSTANCE   *Instance;
  EFI_TLS_VERIFY    TlsVerifyMethod;

  if ((Event == HttpEventTlsConfigured) && (EventStatus == EFI_SUCCESS)) {
    // Reconfigure TLS configuration data.
    Instance = RESTEX_INSTANCE_FROM_HTTP_CALLBACK (This);
    Status   = gBS->HandleProtocol (
                      Instance->HttpIo.Handle,
                      &gEfiTlsProtocolGuid,
                      (VOID **)&TlsProtocol
                      );
    if (EFI_ERROR (Status)) {
      return;
    }

    TlsVerifyMethod = EFI_TLS_VERIFY_NONE;
    Status          = TlsProtocol->SetSessionData (
                                     TlsProtocol,
                                     EfiTlsVerifyMethod,
                                     &TlsVerifyMethod,
                                     sizeof (EFI_TLS_VERIFY)
                                     );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_MANAGEABILITY, "%a: REST EX reconfigures TLS verify method.\n", __func__));
    }
  }

  return;
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
RedfishRestExServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  RESTEX_SERVICE   *RestExSb;
  RESTEX_INSTANCE  *Instance;
  EFI_STATUS       Status;
  EFI_TPL          OldTpl;
  VOID             *Http;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RestExSb = RESTEX_SERVICE_FROM_THIS (This);

  Status = RestExCreateInstance (RestExSb, &Instance);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Instance != NULL);

  //
  // Install the RestEx protocol onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiRestExProtocolGuid,
                  &Instance->RestEx,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Instance->ChildHandle = *ChildHandle;

  //
  // Open the Http protocol BY_CHILD.
  //
  Status = gBS->OpenProtocol (
                  RestExSb->HttpChildHandle,
                  &gEfiHttpProtocolGuid,
                  (VOID **)&Http,
                  gRedfishRestExDriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiRestExProtocolGuid,
           &Instance->RestEx,
           NULL
           );

    goto ON_ERROR;
  }

  //
  // Open the Http protocol by child.
  //
  Status = gBS->OpenProtocol (
                  Instance->HttpIo.Handle,
                  &gEfiHttpProtocolGuid,
                  (VOID **)&Http,
                  gRedfishRestExDriverBinding.DriverBindingHandle,
                  Instance->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    //
    // Close the Http protocol.
    //
    gBS->CloseProtocol (
           RestExSb->HttpChildHandle,
           &gEfiHttpProtocolGuid,
           gRedfishRestExDriverBinding.DriverBindingHandle,
           ChildHandle
           );

    gBS->UninstallMultipleProtocolInterfaces (
           Instance->ChildHandle,
           &gEfiRestExProtocolGuid,
           &Instance->RestEx,
           NULL
           );

    goto ON_ERROR;
  }

  // Initial HTTP callback function on this REST EX instance
  Instance->HttpCallbakFunction.Callback = RestExHttpCallback;
  Status                                 = gBS->InstallProtocolInterface (
                                                  &Instance->HttpIo.Handle,
                                                  &gEdkiiHttpCallbackProtocolGuid,
                                                  EFI_NATIVE_INTERFACE,
                                                  &Instance->HttpCallbakFunction
                                                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to install HttpCallbackFunction.\n", __func__));
    goto ON_ERROR;
  }

  //
  // Add it to the parent's child list.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&RestExSb->RestExChildrenList, &Instance->Link);
  RestExSb->RestExChildrenNum++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:

  RestExDestroyInstance (Instance);
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
RedfishRestExServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  RESTEX_SERVICE   *RestExSb;
  RESTEX_INSTANCE  *Instance;

  EFI_REST_EX_PROTOCOL  *RestEx;
  EFI_STATUS            Status;
  EFI_TPL               OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Retrieve the private context data structures
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiRestExProtocolGuid,
                  (VOID **)&RestEx,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Instance = RESTEX_INSTANCE_FROM_THIS (RestEx);
  RestExSb = RESTEX_SERVICE_FROM_THIS (This);

  if (Instance->Service != RestExSb) {
    return EFI_INVALID_PARAMETER;
  }

  if (Instance->InDestroy) {
    return EFI_SUCCESS;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  Instance->InDestroy = TRUE;

  //
  // Close the Http protocol.
  //
  gBS->CloseProtocol (
         RestExSb->HttpChildHandle,
         &gEfiHttpProtocolGuid,
         gRedfishRestExDriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->CloseProtocol (
         Instance->HttpIo.Handle,
         &gEfiHttpProtocolGuid,
         gRedfishRestExDriverBinding.DriverBindingHandle,
         ChildHandle
         );

  gBS->RestoreTPL (OldTpl);

  //
  // Uninstall the RestEx protocol first to enable a top down destruction.
  //
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiRestExProtocolGuid,
                  RestEx
                  );

  //
  // Uninstall the HTTP callback protocol.
  //
  Status = gBS->UninstallProtocolInterface (
                  Instance->HttpIo.Handle,
                  &gEdkiiHttpCallbackProtocolGuid,
                  &Instance->HttpCallbakFunction
                  );

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (EFI_ERROR (Status)) {
    Instance->InDestroy = FALSE;
    gBS->RestoreTPL (OldTpl);
    return Status;
  }

  RemoveEntryList (&Instance->Link);
  RestExSb->RestExChildrenNum--;

  gBS->RestoreTPL (OldTpl);

  RestExDestroyInstance (Instance);
  return EFI_SUCCESS;
}
