/** @file
  The UEFI driver model driver which is responsible for locating the
  Redfish service through Redfish host interface and executing EDKII
  Redfish feature drivers.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishConfigHandlerDriver.h"

EFI_EVENT  gEfiRedfishDiscoverProtocolEvent = NULL;

//
// Variables for using RFI Redfish Discover Protocol
//
VOID                                    *gEfiRedfishDiscoverRegistration;
EFI_HANDLE                              gEfiRedfishDiscoverControllerHandle = NULL;
EFI_REDFISH_DISCOVER_PROTOCOL           *gEfiRedfishDiscoverProtocol        = NULL;
BOOLEAN                                 gRedfishDiscoverActivated           = FALSE;
BOOLEAN                                 gRedfishServiceDiscovered           = FALSE;
EFI_REDFISH_DISCOVER_NETWORK_INTERFACE  *mNetworkInterfaces                 = NULL;
UINTN                                   mNumberOfNetworkInterfaces;
EFI_EVENT                               mEdkIIRedfishHostInterfaceReadyEvent;
VOID                                    *mEdkIIRedfishHostInterfaceRegistration;

///
/// Driver Binding Protocol instance
///
EFI_DRIVER_BINDING_PROTOCOL  gRedfishConfigDriverBinding = {
  RedfishConfigDriverBindingSupported,
  RedfishConfigDriverBindingStart,
  RedfishConfigDriverBindingStop,
  REDFISH_CONFIG_VERSION,
  NULL,
  NULL
};

/**
  Stop acquiring Redfish service.

**/
VOID
RedfishConfigStopRedfishDiscovery (
  VOID
  )
{
  if (gRedfishDiscoverActivated) {
    //
    // No more EFI Discover Protocol.
    //
    if (gEfiRedfishDiscoverProtocolEvent != NULL) {
      gBS->CloseEvent (gEfiRedfishDiscoverProtocolEvent);
    }

    gEfiRedfishDiscoverControllerHandle = NULL;
    gEfiRedfishDiscoverProtocol         = NULL;
    gRedfishDiscoverActivated           = FALSE;
    gRedfishServiceDiscovered           = FALSE;
  }
}

/**
  Callback function executed when a Redfish Config Handler Protocol is installed.

  @param[in]  Event    Event whose notification function is being invoked.
  @param[in]  Context  Pointer to the REDFISH_CONFIG_DRIVER_DATA buffer.

**/
VOID
EFIAPI
RedfishConfigHandlerInstalledCallback (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  if (!gRedfishDiscoverActivated) {
    //
    // No Redfish service is discovered yet.
    //
    return;
  }

  RedfishConfigHandlerInitialization ();
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
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
RedfishConfigDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_REST_EX_PROTOCOL  *RestEx;
  EFI_STATUS            Status;
  EFI_HANDLE            ChildHandle;

  ChildHandle = NULL;

  //
  // Check if REST EX is ready. This just makes sure
  // the network stack is brought up.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->ImageHandle,
             &gEfiRestExServiceBindingProtocolGuid,
             &ChildHandle
             );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Test if REST EX protocol is ready.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiRestExProtocolGuid,
                  (VOID **)&RestEx,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_UNSUPPORTED;
  }

  NetLibDestroyServiceChild (
    ControllerHandle,
    This->ImageHandle,
    &gEfiRestExServiceBindingProtocolGuid,
    ChildHandle
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

  @retval EFI_SUCCESS              The driver is started.
  @retval EFI_ALREADY_STARTED      The driver was already started.

**/
EFI_STATUS
EFIAPI
RedfishConfigDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  VOID  *ConfigHandlerRegistration;

  if (gRedfishConfigData.Event != NULL) {
    return EFI_ALREADY_STARTED;
  }

  gRedfishConfigData.Event = EfiCreateProtocolNotifyEvent (
                               &gEdkIIRedfishConfigHandlerProtocolGuid,
                               TPL_CALLBACK,
                               RedfishConfigHandlerInstalledCallback,
                               (VOID *)&gRedfishConfigData,
                               &ConfigHandlerRegistration
                               );
  return EFI_SUCCESS;
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
RedfishConfigDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS  Status;

  if (ControllerHandle == gEfiRedfishDiscoverControllerHandle) {
    RedfishConfigStopRedfishDiscovery ();
  }

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiRedfishDiscoverProtocolGuid,
         gRedfishConfigData.Image,
         gRedfishConfigData.Image
         );

  Status = RedfishConfigCommonStop ();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  if (gRedfishConfigData.Event != NULL) {
    gBS->CloseEvent (gRedfishConfigData.Event);
    gRedfishConfigData.Event = NULL;
  }

  return EFI_SUCCESS;
}

/**
  Callback function when Redfish service is discovered.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[out]  Context  Pointer to the Context buffer

**/
VOID
EFIAPI
RedfishServiceDiscoveredCallback (
  IN  EFI_EVENT  Event,
  OUT VOID       *Context
  )
{
  EFI_REDFISH_DISCOVERED_TOKEN     *RedfishDiscoveredToken;
  EFI_REDFISH_DISCOVERED_INSTANCE  *RedfishInstance;

  RedfishDiscoveredToken = (EFI_REDFISH_DISCOVERED_TOKEN *)Context;
  gBS->CloseEvent (RedfishDiscoveredToken->Event);

  //
  // Only support one Redfish service on platform.
  //
  if (!gRedfishServiceDiscovered) {
    RedfishInstance = RedfishDiscoveredToken->DiscoverList.RedfishInstances;
    //
    // Only pick up the first found Redfish service.
    //
    if (RedfishInstance->Status == EFI_SUCCESS) {
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceRestExHandle = RedfishInstance->Information.RedfishRestExHandle;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceVersion      = RedfishInstance->Information.RedfishVersion;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceLocation     = RedfishInstance->Information.Location;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceUuid         = RedfishInstance->Information.Uuid;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceOs           = RedfishInstance->Information.Os;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceOsVersion    = RedfishInstance->Information.OsVersion;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceProduct      = RedfishInstance->Information.Product;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceProductVer   = RedfishInstance->Information.ProductVer;
      gRedfishConfigData.RedfishServiceInfo.RedfishServiceUseHttps     = RedfishInstance->Information.UseHttps;
      gRedfishServiceDiscovered                                        = TRUE;
    }

    //
    // Invoke RedfishConfigHandlerInstalledCallback to execute
    // the initialization of Redfish Configure Handler instance.
    //
    RedfishConfigHandlerInstalledCallback (gRedfishConfigData.Event, &gRedfishConfigData);
  }

  FreePool (RedfishDiscoveredToken);
}

/**
  Callback function executed when the gEdkIIRedfishHostInterfaceReadyProtocolGuid
  protocol interface is installed.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[in]   Context  Pointer to the Context buffer

**/
VOID
EFIAPI
AcquireRedfishServiceOnNetworkInterfaceCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                              Status;
  EFI_REDFISH_DISCOVER_NETWORK_INTERFACE  *ThisNetworkInterface;
  UINTN                                   NetworkInterfaceIndex;
  EFI_REDFISH_DISCOVERED_TOKEN            *ThisRedfishDiscoveredToken;

  ThisNetworkInterface = mNetworkInterfaces;
  //
  // Loop to discover Redfish service on each network interface.
  //
  for (NetworkInterfaceIndex = 0; NetworkInterfaceIndex < mNumberOfNetworkInterfaces; NetworkInterfaceIndex++) {
    ThisRedfishDiscoveredToken = (EFI_REDFISH_DISCOVERED_TOKEN *)AllocateZeroPool (sizeof (EFI_REDFISH_DISCOVERED_TOKEN));
    if (ThisRedfishDiscoveredToken == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Not enough memory for EFI_REDFISH_DISCOVERED_TOKEN.\n", __func__));
      return;
    }

    ThisRedfishDiscoveredToken->Signature = REDFISH_DISCOVER_TOKEN_SIGNATURE;

    //
    // Initial this Redfish Discovered Token
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    RedfishServiceDiscoveredCallback,
                    (VOID *)ThisRedfishDiscoveredToken,
                    &ThisRedfishDiscoveredToken->Event
                    );
    if (EFI_ERROR (Status)) {
      FreePool (ThisRedfishDiscoveredToken);
      DEBUG ((DEBUG_ERROR, "%a: Failed to create event for Redfish discovered token.\n", __func__));
      return;
    }

    //
    // Acquire for Redfish service which is reported by
    // Redfish Host Interface.
    //
    Status = gEfiRedfishDiscoverProtocol->AcquireRedfishService (
                                            gEfiRedfishDiscoverProtocol,
                                            gRedfishConfigData.Image,
                                            ThisNetworkInterface,
                                            EFI_REDFISH_DISCOVER_HOST_INTERFACE,
                                            ThisRedfishDiscoveredToken
                                            );

    //
    // Free Redfish Discovered Token if Discover Instance was not created and
    // Redfish Service Discovered Callback event was not triggered.
    //
    if ((ThisRedfishDiscoveredToken->DiscoverList.NumberOfServiceFound == 0) ||
        EFI_ERROR (ThisRedfishDiscoveredToken->DiscoverList.RedfishInstances->Status))
    {
      gBS->CloseEvent (ThisRedfishDiscoveredToken->Event);
      DEBUG ((DEBUG_MANAGEABILITY, "%a: Free Redfish discovered token - %x.\n", __func__, ThisRedfishDiscoveredToken));
      FreePool (ThisRedfishDiscoveredToken);
    }

    ThisNetworkInterface++;
  }
}

/**
  Callback function executed when the EFI_REDFISH_DISCOVER_PROTOCOL
  protocol interface is installed.

  @param[in]   Event    Event whose notification function is being invoked.
  @param[in]   Context  Pointer to the Context buffer

**/
VOID
EFIAPI
RedfishDiscoverProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  EFI_HANDLE  HandleBuffer;
  VOID        *RedfishHostInterfaceReadyProtocol;

  DEBUG ((DEBUG_MANAGEABILITY, "%a: New network interface is installed on system by EFI Redfish discover driver.\n", __func__));

  BufferSize = sizeof (EFI_HANDLE);
  Status     = gBS->LocateHandle (
                      ByRegisterNotify,
                      NULL,
                      gEfiRedfishDiscoverRegistration,
                      &BufferSize,
                      &HandleBuffer
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Can't locate handle with EFI_REDFISH_DISCOVER_PROTOCOL installed.\n", __func__));
  }

  gRedfishDiscoverActivated = TRUE;
  if (gEfiRedfishDiscoverProtocol == NULL) {
    gEfiRedfishDiscoverControllerHandle = HandleBuffer;
    //
    // First time to open EFI_REDFISH_DISCOVER_PROTOCOL.
    //
    Status = gBS->OpenProtocol (
                    gEfiRedfishDiscoverControllerHandle,
                    &gEfiRedfishDiscoverProtocolGuid,
                    (VOID **)&gEfiRedfishDiscoverProtocol,
                    gRedfishConfigData.Image,
                    gRedfishConfigData.Image,
                    EFI_OPEN_PROTOCOL_BY_DRIVER
                    );
    if (EFI_ERROR (Status)) {
      gEfiRedfishDiscoverProtocol = NULL;
      gRedfishDiscoverActivated   = FALSE;
      DEBUG ((DEBUG_ERROR, "%a: Can't locate EFI_REDFISH_DISCOVER_PROTOCOL.\n", __func__));
      return;
    }
  }

  Status = gEfiRedfishDiscoverProtocol->GetNetworkInterfaceList (
                                          gEfiRedfishDiscoverProtocol,
                                          gRedfishConfigData.Image,
                                          &mNumberOfNetworkInterfaces,
                                          &mNetworkInterfaces
                                          );
  if (EFI_ERROR (Status) || (mNumberOfNetworkInterfaces == 0)) {
    DEBUG ((DEBUG_ERROR, "%a: No network interfaces found on the handle.\n", __func__));
    return;
  }

  //
  // Check if Redfish Host Interface is ready or not.
  //
  Status = gBS->LocateProtocol (&gEdkIIRedfishHostInterfaceReadyProtocolGuid, NULL, &RedfishHostInterfaceReadyProtocol);
  if (!EFI_ERROR (Status)) {
    // Acquire Redfish service;
    AcquireRedfishServiceOnNetworkInterfaceCallback ((EFI_EVENT)NULL, (VOID *)NULL);
  } else {
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    AcquireRedfishServiceOnNetworkInterfaceCallback,
                    NULL,
                    &mEdkIIRedfishHostInterfaceReadyEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to create event for gEdkIIRedfishHostInterfaceReadyProtocolGuid installation.", __func__));
      return;
    }

    Status = gBS->RegisterProtocolNotify (
                    &gEdkIIRedfishHostInterfaceReadyProtocolGuid,
                    mEdkIIRedfishHostInterfaceReadyEvent,
                    &mEdkIIRedfishHostInterfaceRegistration
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Fail to register event for the installation of gEdkIIRedfishHostInterfaceReadyProtocolGuid.", __func__));
      return;
    }
  }

  return;
}

/**
  Unloads an image.

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.

**/
EFI_STATUS
EFIAPI
RedfishConfigHandlerDriverUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  RedfishConfigDriverCommonUnload (ImageHandle);

  RedfishConfigStopRedfishDiscovery ();

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param[in]  ImageHandle       The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RedfishConfigHandlerDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  ZeroMem ((VOID *)&gRedfishConfigData, sizeof (REDFISH_CONFIG_DRIVER_DATA));
  gRedfishConfigData.Image = ImageHandle;
  //
  // Register event for EFI_REDFISH_DISCOVER_PROTOCOL protocol install
  // notification.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  RedfishDiscoverProtocolInstalled,
                  NULL,
                  &gEfiRedfishDiscoverProtocolGuid,
                  &gEfiRedfishDiscoverProtocolEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to create event for the installation of EFI_REDFISH_DISCOVER_PROTOCOL.", __func__));
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEfiRedfishDiscoverProtocolGuid,
                  gEfiRedfishDiscoverProtocolEvent,
                  &gEfiRedfishDiscoverRegistration
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Fail to register event for the installation of EFI_REDFISH_DISCOVER_PROTOCOL.", __func__));
    return Status;
  }

  Status = RedfishConfigCommonInit (ImageHandle, SystemTable);
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (gEfiRedfishDiscoverProtocolEvent);
    gEfiRedfishDiscoverProtocolEvent = NULL;
    return Status;
  }

  //
  // Install UEFI Driver Model protocol(s).
  //
  Status = EfiLibInstallAllDriverProtocols2 (
             ImageHandle,
             SystemTable,
             &gRedfishConfigDriverBinding,
             ImageHandle,
             &gRedfishConfigHandlerComponentName,
             &gRedfishConfigHandlerComponentName2,
             NULL,
             NULL,
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    gBS->CloseEvent (gEndOfDxeEvent);
    gEndOfDxeEvent = NULL;
    gBS->CloseEvent (gExitBootServiceEvent);
    gExitBootServiceEvent = NULL;
    gBS->CloseEvent (gEfiRedfishDiscoverProtocolEvent);
    gEfiRedfishDiscoverProtocolEvent = NULL;
    DEBUG ((DEBUG_ERROR, "%a: Fail to install EFI Binding Protocol of EFI Redfish Config driver.", __func__));
    return Status;
  }

  return Status;
}
