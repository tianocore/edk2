/** @file
  The driver binding protocol for the WiFi Connection Manager.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "WifiConnectionMgrDxe.h"

///
/// Driver Binding Protocol instance
///
EFI_DRIVER_BINDING_PROTOCOL  gWifiMgrDxeDriverBinding = {
  WifiMgrDxeDriverBindingSupported,
  WifiMgrDxeDriverBindingStart,
  WifiMgrDxeDriverBindingStop,
  WIFI_MGR_DXE_VERSION,
  NULL,
  NULL
};

//
// The private global data for WiFi Connection Manager
//
WIFI_MGR_PRIVATE_DATA  *mPrivate = NULL;

//
// The private guid to identify WiFi Connection Manager
//
EFI_GUID  mEfiWifiMgrPrivateGuid = EFI_WIFIMGR_PRIVATE_GUID;

//
// The Hii config guids
//
EFI_GUID  gWifiConfigFormSetGuid            = WIFI_CONNECTION_MANAGER_CONFIG_GUID;
EFI_GUID  mWifiConfigNetworkListRefreshGuid = WIFI_CONFIG_NETWORK_LIST_REFRESH_GUID;
EFI_GUID  mWifiConfigConnectFormRefreshGuid = WIFI_CONFIG_CONNECT_FORM_REFRESH_GUID;
EFI_GUID  mWifiConfigMainFormRefreshGuid    = WIFI_CONFIG_MAIN_FORM_REFRESH_GUID;

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
WifiMgrDxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &mEfiWifiMgrPrivateGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test for the wireless MAC connection 2 protocol
  //
  return gBS->OpenProtocol (
                ControllerHandle,
                &gEfiWiFi2ProtocolGuid,
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
WifiMgrDxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                               Status;
  EFI_TPL                                  OldTpl;
  UINTN                                    AddressSize;
  WIFI_MGR_DEVICE_DATA                     *Nic;
  EFI_WIRELESS_MAC_CONNECTION_II_PROTOCOL  *Wmp;
  EFI_SUPPLICANT_PROTOCOL                  *Supplicant;
  EFI_EAP_CONFIGURATION_PROTOCOL           *EapConfig;

  Nic = NULL;

  //
  // Open Protocols
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiWiFi2ProtocolGuid,
                  (VOID **)&Wmp,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSupplicantProtocolGuid,
                  (VOID **)&Supplicant,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    Supplicant = NULL;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiEapConfigurationProtocolGuid,
                  (VOID **)&EapConfig,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    EapConfig = NULL;
  }

  //
  // Initialize Nic device data
  //
  Nic = AllocateZeroPool (sizeof (WIFI_MGR_DEVICE_DATA));
  if (Nic == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR1;
  }

  Nic->Signature           = WIFI_MGR_DEVICE_DATA_SIGNATURE;
  Nic->DriverHandle        = This->DriverBindingHandle;
  Nic->ControllerHandle    = ControllerHandle;
  Nic->Private             = mPrivate;
  Nic->Wmp                 = Wmp;
  Nic->Supplicant          = Supplicant;
  Nic->EapConfig           = EapConfig;
  Nic->UserSelectedProfile = NULL;
  Nic->OneTimeScanRequest  = FALSE;
  Nic->ScanTickTime        = WIFI_SCAN_FREQUENCY;   // Initialize the first scan

  if (Nic->Supplicant != NULL) {
    WifiMgrGetSupportedSuites (Nic);
  }

  InitializeListHead (&Nic->ProfileList);

  //
  // Record the MAC address of the incoming NIC.
  //
  Status = NetLibGetMacAddress (
             ControllerHandle,
             (EFI_MAC_ADDRESS *)&Nic->MacAddress,
             &AddressSize
             );
  if (EFI_ERROR (Status)) {
    goto ERROR2;
  }

  //
  // Create and start the timer for the status check
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  WifiMgrOnTimerTick,
                  Nic,
                  &Nic->TickTimer
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR2;
  }

  Status = gBS->SetTimer (Nic->TickTimer, TimerPeriodic, EFI_TIMER_PERIOD_MILLISECONDS (500));
  if (EFI_ERROR (Status)) {
    goto ERROR3;
  }

  Nic->ConnectState = WifiMgrDisconnected;
  Nic->ScanState    = WifiMgrScanFinished;

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  InsertTailList (&mPrivate->NicList, &Nic->Link);
  Nic->NicIndex = mPrivate->NicCount++;
  if (mPrivate->CurrentNic == NULL) {
    mPrivate->CurrentNic = Nic;
  }

  gBS->RestoreTPL (OldTpl);

  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &mEfiWifiMgrPrivateGuid,
                  EFI_NATIVE_INTERFACE,
                  &Nic->WifiMgrIdentifier
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR4;
  }

  return EFI_SUCCESS;

ERROR4:

  gBS->SetTimer (Nic->TickTimer, TimerCancel, 0);
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  RemoveEntryList (&Nic->Link);
  mPrivate->NicCount--;
  gBS->RestoreTPL (OldTpl);

ERROR3:

  gBS->CloseEvent (Nic->TickTimer);

ERROR2:

  if (Nic->Supplicant != NULL) {
    if (Nic->SupportedSuites.SupportedAKMSuites != NULL) {
      FreePool (Nic->SupportedSuites.SupportedAKMSuites);
    }

    if (Nic->SupportedSuites.SupportedSwCipherSuites != NULL) {
      FreePool (Nic->SupportedSuites.SupportedSwCipherSuites);
    }

    if (Nic->SupportedSuites.SupportedHwCipherSuites != NULL) {
      FreePool (Nic->SupportedSuites.SupportedHwCipherSuites);
    }
  }

  FreePool (Nic);

ERROR1:

  if (Supplicant != NULL) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiSupplicantProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

  if (EapConfig != NULL) {
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiEapConfigurationProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );
  }

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiWiFi2ProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

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
WifiMgrDxeDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                 Status;
  EFI_TPL                    OldTpl;
  WIFI_MGR_PRIVATE_PROTOCOL  *WifiMgrIdentifier;
  WIFI_MGR_DEVICE_DATA       *Nic;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &mEfiWifiMgrPrivateGuid,
                  (VOID **)&WifiMgrIdentifier,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Nic = WIFI_MGR_DEVICE_DATA_FROM_IDENTIFIER (WifiMgrIdentifier);
  if (Nic == NULL) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Close Event
  //
  gBS->SetTimer (Nic->TickTimer, TimerCancel, 0);
  gBS->CloseEvent (Nic->TickTimer);

  //
  // Clean Supported Suites
  //
  if (Nic->Supplicant != NULL) {
    if (Nic->SupportedSuites.SupportedAKMSuites != NULL) {
      FreePool (Nic->SupportedSuites.SupportedAKMSuites);
    }

    if (Nic->SupportedSuites.SupportedSwCipherSuites != NULL) {
      FreePool (Nic->SupportedSuites.SupportedSwCipherSuites);
    }

    if (Nic->SupportedSuites.SupportedHwCipherSuites != NULL) {
      FreePool (Nic->SupportedSuites.SupportedHwCipherSuites);
    }
  }

  //
  // Close Protocols
  //
  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &mEfiWifiMgrPrivateGuid,
                  &Nic->WifiMgrIdentifier
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiWiFi2ProtocolGuid,
                  Nic->DriverHandle,
                  Nic->ControllerHandle
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Nic->Supplicant != NULL) {
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiSupplicantProtocolGuid,
                    Nic->DriverHandle,
                    Nic->ControllerHandle
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if (Nic->EapConfig != NULL) {
    Status = gBS->CloseProtocol (
                    ControllerHandle,
                    &gEfiEapConfigurationProtocolGuid,
                    Nic->DriverHandle,
                    Nic->ControllerHandle
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Remove this Nic from Nic list
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  RemoveEntryList (&Nic->Link);
  mPrivate->NicCount--;
  if (mPrivate->CurrentNic == Nic) {
    mPrivate->CurrentNic = NULL;
  }

  gBS->RestoreTPL (OldTpl);

  WifiMgrFreeProfileList (&Nic->ProfileList);
  FreePool (Nic);

  DEBUG ((DEBUG_INFO, "[WiFi Connection Manager] Device Controller has been Disconnected!\n"));
  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval Others                An unexpected error occurred.

**/
EFI_STATUS
EFIAPI
WifiMgrDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gWifiMgrDxeDriverBinding,
             ImageHandle,
             &gWifiMgrDxeComponentName,
             &gWifiMgrDxeComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize the global private data structure.
  //
  mPrivate = AllocateZeroPool (sizeof (WIFI_MGR_PRIVATE_DATA));
  if (mPrivate == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ERROR1;
  }

  mPrivate->Signature    = WIFI_MGR_PRIVATE_DATA_SIGNATURE;
  mPrivate->DriverHandle = ImageHandle;
  InitializeListHead (&mPrivate->NicList);
  mPrivate->NicCount   = 0;
  mPrivate->CurrentNic = NULL;
  InitializeListHead (&mPrivate->HiddenNetworkList);
  mPrivate->HiddenNetworkCount = 0;

  //
  // Create events for page refresh
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WifiMgrInternalEmptyFunction,
                  NULL,
                  &mWifiConfigNetworkListRefreshGuid,
                  &mPrivate->NetworkListRefreshEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR2;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WifiMgrInternalEmptyFunction,
                  NULL,
                  &mWifiConfigConnectFormRefreshGuid,
                  &mPrivate->ConnectFormRefreshEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR3;
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  WifiMgrInternalEmptyFunction,
                  NULL,
                  &mWifiConfigMainFormRefreshGuid,
                  &mPrivate->MainPageRefreshEvent
                  );
  if (EFI_ERROR (Status)) {
    goto ERROR4;
  }

  Status = WifiMgrDxeConfigFormInit (mPrivate);
  if (EFI_ERROR (Status)) {
    goto ERROR5;
  }

  return Status;

ERROR5:
  gBS->CloseEvent (mPrivate->MainPageRefreshEvent);

ERROR4:
  gBS->CloseEvent (mPrivate->ConnectFormRefreshEvent);

ERROR3:
  gBS->CloseEvent (mPrivate->NetworkListRefreshEvent);

ERROR2:
  if (mPrivate != NULL) {
    FreePool (mPrivate);
    mPrivate = NULL;
  }

ERROR1:
  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiDriverBindingProtocolGuid,
         &gWifiMgrDxeDriverBinding,
         &gEfiComponentNameProtocolGuid,
         &gWifiMgrDxeComponentName,
         &gEfiComponentName2ProtocolGuid,
         &gWifiMgrDxeComponentName2,
         NULL
         );

  return Status;
}
