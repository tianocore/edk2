/** @file
  The driver binding and service binding protocol for HttpDxe driver.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpDriver.h"

EFI_HTTP_UTILITIES_PROTOCOL  *mHttpUtilities = NULL;

///
/// Driver Binding Protocol instance
///
EFI_DRIVER_BINDING_PROTOCOL  gHttpDxeIp4DriverBinding = {
  HttpDxeIp4DriverBindingSupported,
  HttpDxeIp4DriverBindingStart,
  HttpDxeIp4DriverBindingStop,
  HTTP_DRIVER_VERSION,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL  gHttpDxeIp6DriverBinding = {
  HttpDxeIp6DriverBindingSupported,
  HttpDxeIp6DriverBindingStart,
  HttpDxeIp6DriverBindingStop,
  HTTP_DRIVER_VERSION,
  NULL,
  NULL
};

/**
  Create a HTTP driver service binding private instance.

  @param[in]  Controller         The controller that has TCP4 service binding
                                 installed.
  @param[out] ServiceData        Point to HTTP driver private instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate some resources.
  @retval EFI_SUCCESS            A new HTTP driver private instance is created.

**/
EFI_STATUS
HttpCreateService (
  IN  EFI_HANDLE    Controller,
  OUT HTTP_SERVICE  **ServiceData
  )
{
  HTTP_SERVICE  *HttpService;

  ASSERT (ServiceData != NULL);
  *ServiceData = NULL;

  HttpService = AllocateZeroPool (sizeof (HTTP_SERVICE));
  if (HttpService == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HttpService->Signature                   = HTTP_SERVICE_SIGNATURE;
  HttpService->ServiceBinding.CreateChild  = HttpServiceBindingCreateChild;
  HttpService->ServiceBinding.DestroyChild = HttpServiceBindingDestroyChild;
  HttpService->ControllerHandle            = Controller;
  HttpService->ChildrenNumber              = 0;
  InitializeListHead (&HttpService->ChildrenList);

  *ServiceData = HttpService;
  return EFI_SUCCESS;
}

/**
  Release all the resource used the HTTP service binding instance.

  @param[in]  HttpService        The HTTP private instance.
  @param[in]  UsingIpv6          Indicate use TCP4 protocol or TCP6 protocol.
                                 if TRUE, use Tcp6 protocol.
                                 if FALSE, use Tcp4 protocol.
**/
VOID
HttpCleanService (
  IN HTTP_SERVICE  *HttpService,
  IN BOOLEAN       UsingIpv6
  )
{
  if (HttpService == NULL) {
    return;
  }

  if (!UsingIpv6) {
    if (HttpService->Tcp4ChildHandle != NULL) {
      gBS->CloseProtocol (
             HttpService->Tcp4ChildHandle,
             &gEfiTcp4ProtocolGuid,
             HttpService->Ip4DriverBindingHandle,
             HttpService->ControllerHandle
             );

      NetLibDestroyServiceChild (
        HttpService->ControllerHandle,
        HttpService->Ip4DriverBindingHandle,
        &gEfiTcp4ServiceBindingProtocolGuid,
        HttpService->Tcp4ChildHandle
        );

      HttpService->Tcp4ChildHandle = NULL;
    }
  } else {
    if (HttpService->Tcp6ChildHandle != NULL) {
      gBS->CloseProtocol (
             HttpService->Tcp6ChildHandle,
             &gEfiTcp6ProtocolGuid,
             HttpService->Ip6DriverBindingHandle,
             HttpService->ControllerHandle
             );

      NetLibDestroyServiceChild (
        HttpService->ControllerHandle,
        HttpService->Ip6DriverBindingHandle,
        &gEfiTcp6ServiceBindingProtocolGuid,
        HttpService->Tcp6ChildHandle
        );

      HttpService->Tcp6ChildHandle = NULL;
    }
  }
}

/**
  The event process routine when the http utilities protocol is installed
  in the system.

  @param[in]     Event         Not used.
  @param[in]     Context       The pointer to the IP4 config2 instance data or IP6 Config instance data.

**/
VOID
EFIAPI
HttpUtilitiesInstalledCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  gBS->LocateProtocol (
         &gEfiHttpUtilitiesProtocolGuid,
         NULL,
         (VOID **)&mHttpUtilities
         );

  //
  // Close the event if Http utilities protocol is located.
  //
  if ((mHttpUtilities != NULL) && (Event != NULL)) {
    gBS->CloseEvent (Event);
  }
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
HttpDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  gBS->LocateProtocol (
         &gEfiHttpUtilitiesProtocolGuid,
         NULL,
         (VOID **)&mHttpUtilities
         );

  if (mHttpUtilities == NULL) {
    //
    // No Http utilities protocol, register a notify.
    //
    EfiCreateProtocolNotifyEvent (
      &gEfiHttpUtilitiesProtocolGuid,
      TPL_CALLBACK,
      HttpUtilitiesInstalledCallback,
      NULL,
      &Registration
      );
  }

  //
  // Install UEFI Driver Model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gHttpDxeIp4DriverBinding,
             ImageHandle,
             &gHttpDxeComponentName,
             &gHttpDxeComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gHttpDxeIp6DriverBinding,
             NULL,
             &gHttpDxeComponentName,
             &gHttpDxeComponentName2
             );
  if (EFI_ERROR (Status)) {
    EfiLibUninstallDriverBindingComponentName2 (
      &gHttpDxeIp4DriverBinding,
      &gHttpDxeComponentName,
      &gHttpDxeComponentName2
      );
  }

  return Status;
}

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_INVALID_PARAMETER Any input parameter is NULL.
  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
HttpDestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY  *Entry,
  IN VOID        *Context
  )
{
  HTTP_PROTOCOL                 *HttpInstance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  UINTN                         NumberOfChildren;
  EFI_HANDLE                    *ChildHandleBuffer;

  if ((Entry == NULL) || (Context == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpInstance      = NET_LIST_USER_STRUCT_S (Entry, HTTP_PROTOCOL, Link, HTTP_PROTOCOL_SIGNATURE);
  ServiceBinding    = ((HTTP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ServiceBinding;
  NumberOfChildren  = ((HTTP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->NumberOfChildren;
  ChildHandleBuffer = ((HTTP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT *)Context)->ChildHandleBuffer;

  if (!NetIsInHandleBuffer (HttpInstance->Handle, NumberOfChildren, ChildHandleBuffer)) {
    return EFI_SUCCESS;
  }

  return ServiceBinding->DestroyChild (ServiceBinding, HttpInstance->Handle);
}

/**
  Test to see if this driver supports ControllerHandle. This is the worker function for
  HttpDxeIp4(6)DriverBindingSupported.

  @param[in]  This                The pointer to the driver binding protocol.
  @param[in]  ControllerHandle    The handle of device to be tested.
  @param[in]  RemainingDevicePath Optional parameter used to pick a specific child
                                  device to be started.
  @param[in]  IpVersion           IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCESS         This driver supports this device.
  @retval EFI_UNSUPPORTED     This driver does not support this device.

**/
EFI_STATUS
EFIAPI
HttpDxeSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL,
  IN UINT8                        IpVersion
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *TcpServiceBindingProtocolGuid;

  if (IpVersion == IP_VERSION_4) {
    TcpServiceBindingProtocolGuid = &gEfiTcp4ServiceBindingProtocolGuid;
  } else {
    TcpServiceBindingProtocolGuid = &gEfiTcp6ServiceBindingProtocolGuid;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  TcpServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Start this driver on ControllerHandle. This is the worker function for
  HttpDxeIp4(6)DriverBindingStart.

  @param[in]  This                 The pointer to the driver binding protocol.
  @param[in]  ControllerHandle     The handle of device to be started.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to be started.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.


  @retval EFI_SUCCESS          This driver is installed to ControllerHandle.
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
HttpDxeStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL,
  IN UINT8                        IpVersion
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  HTTP_SERVICE                  *HttpService;
  VOID                          *Interface;
  BOOLEAN                       UsingIpv6;

  UsingIpv6 = FALSE;

  //
  // Test for the Http service binding protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiHttpServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    HttpService = HTTP_SERVICE_FROM_PROTOCOL (ServiceBinding);
  } else {
    Status = HttpCreateService (ControllerHandle, &HttpService);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    ASSERT (HttpService != NULL);

    //
    // Install the HttpServiceBinding Protocol onto Controller
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gEfiHttpServiceBindingProtocolGuid,
                    &HttpService->ServiceBinding,
                    NULL
                    );

    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  if (IpVersion == IP_VERSION_4) {
    HttpService->Ip4DriverBindingHandle = This->DriverBindingHandle;

    if (HttpService->Tcp4ChildHandle == NULL) {
      //
      // Create a TCP4 child instance, but do not configure it. This will establish the parent-child relationship.
      //
      Status = NetLibCreateServiceChild (
                 ControllerHandle,
                 This->DriverBindingHandle,
                 &gEfiTcp4ServiceBindingProtocolGuid,
                 &HttpService->Tcp4ChildHandle
                 );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }

      Status = gBS->OpenProtocol (
                      HttpService->Tcp4ChildHandle,
                      &gEfiTcp4ProtocolGuid,
                      &Interface,
                      This->DriverBindingHandle,
                      ControllerHandle,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    } else {
      return EFI_ALREADY_STARTED;
    }
  } else {
    UsingIpv6                           = TRUE;
    HttpService->Ip6DriverBindingHandle = This->DriverBindingHandle;

    if (HttpService->Tcp6ChildHandle == NULL) {
      //
      // Create a TCP6 child instance, but do not configure it. This will establish the parent-child relationship.
      //
      Status = NetLibCreateServiceChild (
                 ControllerHandle,
                 This->DriverBindingHandle,
                 &gEfiTcp6ServiceBindingProtocolGuid,
                 &HttpService->Tcp6ChildHandle
                 );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }

      Status = gBS->OpenProtocol (
                      HttpService->Tcp6ChildHandle,
                      &gEfiTcp6ProtocolGuid,
                      &Interface,
                      This->DriverBindingHandle,
                      ControllerHandle,
                      EFI_OPEN_PROTOCOL_BY_DRIVER
                      );

      if (EFI_ERROR (Status)) {
        goto ON_ERROR;
      }
    } else {
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;

ON_ERROR:

  if (HttpService != NULL) {
    HttpCleanService (HttpService, UsingIpv6);
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    &ControllerHandle,
                    &gEfiHttpServiceBindingProtocolGuid,
                    &HttpService->ServiceBinding,
                    NULL
                    );
    if (!EFI_ERROR (Status)) {
      if ((HttpService->Tcp4ChildHandle == NULL) && (HttpService->Tcp6ChildHandle == NULL)) {
        FreePool (HttpService);
      }
    }
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle. This is the worker function for
  HttpDxeIp4(6)DriverBindingStop.

  @param[in]  This              Protocol instance pointer.
  @param[in]  ControllerHandle  Handle of device to stop driver on.
  @param[in]  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer List of Child Handles to Stop.
  @param[in]  IpVersion         IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCESS           This driver was removed ControllerHandle.
  @retval EFI_DEVICE_ERROR      An unexpected system or network error occurred.
  @retval Others                This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
HttpDxeStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer,
  IN UINT8                        IpVersion
  )
{
  EFI_HANDLE                                NicHandle;
  EFI_STATUS                                Status;
  EFI_SERVICE_BINDING_PROTOCOL              *ServiceBinding;
  HTTP_SERVICE                              *HttpService;
  LIST_ENTRY                                *List;
  HTTP_DESTROY_CHILD_IN_HANDLE_BUF_CONTEXT  Context;
  BOOLEAN                                   UsingIpv6;

  //
  // HTTP driver opens TCP4(6) child, So, Controller is a TCP4(6)
  // child handle. Locate the Nic handle first. Then get the
  // HTTP private data back.
  //
  if (IpVersion == IP_VERSION_4) {
    UsingIpv6 = FALSE;
    NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiTcp4ProtocolGuid);
  } else {
    UsingIpv6 = TRUE;
    NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiTcp6ProtocolGuid);
  }

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiHttpServiceBindingProtocolGuid,
                  (VOID **)&ServiceBinding,
                  This->DriverBindingHandle,
                  NicHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    HttpService = HTTP_SERVICE_FROM_PROTOCOL (ServiceBinding);

    if (NumberOfChildren != 0) {
      //
      // Destroy the HTTP child instance in ChildHandleBuffer.
      //
      List                      = &HttpService->ChildrenList;
      Context.ServiceBinding    = ServiceBinding;
      Context.NumberOfChildren  = NumberOfChildren;
      Context.ChildHandleBuffer = ChildHandleBuffer;
      Status                    = NetDestroyLinkList (
                                    List,
                                    HttpDestroyChildEntryInHandleBuffer,
                                    &Context,
                                    NULL
                                    );
    } else {
      HttpCleanService (HttpService, UsingIpv6);

      if ((HttpService->Tcp4ChildHandle == NULL) && (HttpService->Tcp6ChildHandle == NULL)) {
        gBS->UninstallProtocolInterface (
               NicHandle,
               &gEfiHttpServiceBindingProtocolGuid,
               ServiceBinding
               );
        FreePool (HttpService);
      }

      Status = EFI_SUCCESS;
    }
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
HttpDxeIp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return HttpDxeSupported (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_4
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
  @retval EFI_ALREADY_STARTED      This device is already running on ControllerHandle.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
HttpDxeIp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return HttpDxeStart (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_4
           );
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
HttpDxeIp4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return HttpDxeStop (
           This,
           ControllerHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_4
           );
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
HttpDxeIp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return HttpDxeSupported (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_6
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
  @retval EFI_ALREADY_STARTED      This device is already running on ControllerHandle.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
HttpDxeIp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return HttpDxeStart (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_6
           );
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
HttpDxeIp6DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  return HttpDxeStop (
           This,
           ControllerHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_6
           );
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

  @retval EFI_SUCCESS           The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER This is NULL, or ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child.
  @retval other                 The child handle was not created.

**/
EFI_STATUS
EFIAPI
HttpServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN OUT EFI_HANDLE                *ChildHandle
  )
{
  HTTP_SERVICE   *HttpService;
  HTTP_PROTOCOL  *HttpInstance;
  EFI_STATUS     Status;
  EFI_TPL        OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpService  = HTTP_SERVICE_FROM_PROTOCOL (This);
  HttpInstance = AllocateZeroPool (sizeof (HTTP_PROTOCOL));
  if (HttpInstance == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  HttpInstance->Signature = HTTP_PROTOCOL_SIGNATURE;
  HttpInstance->Service   = HttpService;
  HttpInstance->Method    = HttpMethodMax;

  CopyMem (&HttpInstance->Http, &mEfiHttpTemplate, sizeof (HttpInstance->Http));
  NetMapInit (&HttpInstance->TxTokens);
  NetMapInit (&HttpInstance->RxTokens);

  //
  // Install HTTP protocol onto ChildHandle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiHttpProtocolGuid,
                  &HttpInstance->Http,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  HttpInstance->Handle = *ChildHandle;

  //
  // Add it to the HTTP service's child list.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&HttpService->ChildrenList, &HttpInstance->Link);
  HttpService->ChildrenNumber++;

  gBS->RestoreTPL (OldTpl);

  return EFI_SUCCESS;

ON_ERROR:

  NetMapClean (&HttpInstance->TxTokens);
  NetMapClean (&HttpInstance->RxTokens);
  FreePool (HttpInstance);

  return Status;
}

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param  This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param  ChildHandle Handle of the child to destroy

  @retval EFI_SUCCESS           The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
HttpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  HTTP_SERVICE       *HttpService;
  HTTP_PROTOCOL      *HttpInstance;
  EFI_HTTP_PROTOCOL  *Http;
  EFI_STATUS         Status;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HttpService = HTTP_SERVICE_FROM_PROTOCOL (This);
  Status      = gBS->OpenProtocol (
                       ChildHandle,
                       &gEfiHttpProtocolGuid,
                       (VOID **)&Http,
                       NULL,
                       NULL,
                       EFI_OPEN_PROTOCOL_GET_PROTOCOL
                       );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  HttpInstance = HTTP_INSTANCE_FROM_PROTOCOL (Http);
  if (HttpInstance->Service != HttpService) {
    return EFI_INVALID_PARAMETER;
  }

  if (HttpInstance->InDestroy) {
    return EFI_SUCCESS;
  }

  HttpInstance->InDestroy = TRUE;

  //
  // Uninstall the HTTP protocol.
  //
  Status = gBS->UninstallProtocolInterface (
                  ChildHandle,
                  &gEfiHttpProtocolGuid,
                  Http
                  );

  if (EFI_ERROR (Status)) {
    HttpInstance->InDestroy = FALSE;
    return Status;
  }

  HttpCleanProtocol (HttpInstance);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  RemoveEntryList (&HttpInstance->Link);
  HttpService->ChildrenNumber--;

  gBS->RestoreTPL (OldTpl);

  FreePool (HttpInstance);
  return EFI_SUCCESS;
}
