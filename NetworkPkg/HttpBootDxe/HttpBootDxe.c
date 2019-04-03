/** @file
  Driver Binding functions implementation for UEFI HTTP boot.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HttpBootDxe.h"

///
/// Driver Binding Protocol instance
///
EFI_DRIVER_BINDING_PROTOCOL gHttpBootIp4DxeDriverBinding = {
  HttpBootIp4DxeDriverBindingSupported,
  HttpBootIp4DxeDriverBindingStart,
  HttpBootIp4DxeDriverBindingStop,
  HTTP_BOOT_DXE_VERSION,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL gHttpBootIp6DxeDriverBinding = {
  HttpBootIp6DxeDriverBindingSupported,
  HttpBootIp6DxeDriverBindingStart,
  HttpBootIp6DxeDriverBindingStop,
  HTTP_BOOT_DXE_VERSION,
  NULL,
  NULL
};



/**
  Check whether UNDI protocol supports IPv6.

  @param[in]   Private           Pointer to HTTP_BOOT_PRIVATE_DATA.
  @param[out]  Ipv6Support       TRUE if UNDI supports IPv6.

  @retval EFI_SUCCESS            Get the result whether UNDI supports IPv6 by NII or AIP protocol successfully.
  @retval EFI_NOT_FOUND          Don't know whether UNDI supports IPv6 since NII or AIP is not available.

**/
EFI_STATUS
HttpBootCheckIpv6Support (
  IN  HTTP_BOOT_PRIVATE_DATA       *Private,
  OUT BOOLEAN                      *Ipv6Support
  )
{
  EFI_HANDLE                       Handle;
  EFI_ADAPTER_INFORMATION_PROTOCOL *Aip;
  EFI_STATUS                       Status;
  EFI_GUID                         *InfoTypesBuffer;
  UINTN                            InfoTypeBufferCount;
  UINTN                            TypeIndex;
  BOOLEAN                          Supported;
  VOID                             *InfoBlock;
  UINTN                            InfoBlockSize;

  ASSERT (Private != NULL && Ipv6Support != NULL);

  //
  // Check whether the UNDI supports IPv6 by NII protocol.
  //
  if (Private->Nii != NULL) {
    *Ipv6Support = Private->Nii->Ipv6Supported;
    return EFI_SUCCESS;
  }

  //
  // Get the NIC handle by SNP protocol.
  //
  Handle = NetLibGetSnpHandle (Private->Controller, NULL);
  if (Handle == NULL) {
    return EFI_NOT_FOUND;
  }

  Aip    = NULL;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiAdapterInformationProtocolGuid,
                  (VOID *) &Aip
                  );
  if (EFI_ERROR (Status) || Aip == NULL) {
    return EFI_NOT_FOUND;
  }

  InfoTypesBuffer     = NULL;
  InfoTypeBufferCount = 0;
  Status = Aip->GetSupportedTypes (Aip, &InfoTypesBuffer, &InfoTypeBufferCount);
  if (EFI_ERROR (Status) || InfoTypesBuffer == NULL) {
    FreePool (InfoTypesBuffer);
    return EFI_NOT_FOUND;
  }

  Supported = FALSE;
  for (TypeIndex = 0; TypeIndex < InfoTypeBufferCount; TypeIndex++) {
    if (CompareGuid (&InfoTypesBuffer[TypeIndex], &gEfiAdapterInfoUndiIpv6SupportGuid)) {
      Supported = TRUE;
      break;
    }
  }

  FreePool (InfoTypesBuffer);
  if (!Supported) {
    return EFI_NOT_FOUND;
  }

  //
  // We now have adapter information block.
  //
  InfoBlock     = NULL;
  InfoBlockSize = 0;
  Status = Aip->GetInformation (Aip, &gEfiAdapterInfoUndiIpv6SupportGuid, &InfoBlock, &InfoBlockSize);
  if (EFI_ERROR (Status) || InfoBlock == NULL) {
    FreePool (InfoBlock);
    return EFI_NOT_FOUND;
  }

  *Ipv6Support = ((EFI_ADAPTER_INFO_UNDI_IPV6_SUPPORT *) InfoBlock)->Ipv6Support;
  FreePool (InfoBlock);

  return EFI_SUCCESS;
}

/**
  Destroy the HTTP child based on IPv4 stack.

  @param[in]  This              Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in]  Private           Pointer to HTTP_BOOT_PRIVATE_DATA.

**/
VOID
HttpBootDestroyIp4Children (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN HTTP_BOOT_PRIVATE_DATA       *Private
  )
{
  ASSERT (This != NULL);
  ASSERT (Private != NULL);

  if (Private->Dhcp4Child != NULL) {
    gBS->CloseProtocol (
           Private->Dhcp4Child,
           &gEfiDhcp4ProtocolGuid,
           This->DriverBindingHandle,
           Private->Controller
           );

    NetLibDestroyServiceChild (
      Private->Controller,
      This->DriverBindingHandle,
      &gEfiDhcp4ServiceBindingProtocolGuid,
      Private->Dhcp4Child
      );
  }

  if (Private->Ip6Nic == NULL && Private->HttpCreated) {
    HttpIoDestroyIo (&Private->HttpIo);
    Private->HttpCreated = FALSE;
  }

  if (Private->Ip4Nic != NULL) {

    gBS->CloseProtocol (
           Private->Controller,
           &gEfiCallerIdGuid,
           This->DriverBindingHandle,
           Private->Ip4Nic->Controller
           );

    gBS->UninstallMultipleProtocolInterfaces (
           Private->Ip4Nic->Controller,
           &gEfiLoadFileProtocolGuid,
           &Private->Ip4Nic->LoadFile,
           &gEfiDevicePathProtocolGuid,
           Private->Ip4Nic->DevicePath,
           NULL
           );
    FreePool (Private->Ip4Nic);
    Private->Ip4Nic = NULL;
  }

}

/**
  Destroy the HTTP child based on IPv6 stack.

  @param[in]  This              Pointer to the EFI_DRIVER_BINDING_PROTOCOL.
  @param[in]  Private           Pointer to HTTP_BOOT_PRIVATE_DATA.

**/
VOID
HttpBootDestroyIp6Children (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN HTTP_BOOT_PRIVATE_DATA       *Private
  )
{
  ASSERT (This != NULL);
  ASSERT (Private != NULL);

  if (Private->Ip6Child != NULL) {
    gBS->CloseProtocol (
           Private->Ip6Child,
           &gEfiIp6ProtocolGuid,
           This->DriverBindingHandle,
           Private->Controller
           );

    NetLibDestroyServiceChild (
      Private->Controller,
      This->DriverBindingHandle,
      &gEfiIp6ServiceBindingProtocolGuid,
      Private->Ip6Child
      );
  }

  if (Private->Dhcp6Child != NULL) {
    gBS->CloseProtocol (
           Private->Dhcp6Child,
           &gEfiDhcp6ProtocolGuid,
           This->DriverBindingHandle,
           Private->Controller
           );

    NetLibDestroyServiceChild (
      Private->Controller,
      This->DriverBindingHandle,
      &gEfiDhcp6ServiceBindingProtocolGuid,
      Private->Dhcp6Child
      );
  }

  if (Private->Ip4Nic == NULL && Private->HttpCreated) {
    HttpIoDestroyIo(&Private->HttpIo);
    Private->HttpCreated = FALSE;
  }

  if (Private->Ip6Nic != NULL) {

    gBS->CloseProtocol (
           Private->Controller,
           &gEfiCallerIdGuid,
           This->DriverBindingHandle,
           Private->Ip6Nic->Controller
           );

    gBS->UninstallMultipleProtocolInterfaces (
           Private->Ip6Nic->Controller,
           &gEfiLoadFileProtocolGuid,
           &Private->Ip6Nic->LoadFile,
           &gEfiDevicePathProtocolGuid,
           Private->Ip6Nic->DevicePath,
           NULL
           );
    FreePool (Private->Ip6Nic);
    Private->Ip6Nic = NULL;
  }
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
HttpBootIp4DxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                    Status;

  //
  // Try to open the DHCP4, HTTP4 and Device Path protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDhcp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiHttpServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
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
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
HttpBootIp4DxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                 Status;
  HTTP_BOOT_PRIVATE_DATA     *Private;
  EFI_DEV_PATH               *Node;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  UINT32                     *Id;
  BOOLEAN                    FirstStart;

  FirstStart = FALSE;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Private = HTTP_BOOT_PRIVATE_DATA_FROM_ID(Id);
  } else {
    FirstStart = TRUE;

    //
    // Initialize the private data structure.
    //
    Private = AllocateZeroPool (sizeof (HTTP_BOOT_PRIVATE_DATA));
    if (Private == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Private->Signature = HTTP_BOOT_PRIVATE_DATA_SIGNATURE;
    Private->Controller = ControllerHandle;
    InitializeListHead (&Private->CacheList);
    //
    // Get the NII interface if it exists, it's not required.
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    (VOID **) &Private->Nii,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      Private->Nii = NULL;
    }

    //
    // Open Device Path Protocol to prepare for appending IP and URI node.
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &Private->ParentDevicePath,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Initialize the HII configuration form.
    //
    Status = HttpBootConfigFormInit (Private);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Install a protocol with Caller Id Guid to the NIC, this is just to build the relationship between
    // NIC handle and the private data.
    //
    Status = gBS->InstallProtocolInterface (
                    &ControllerHandle,
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    &Private->Id
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

  }

  if (Private->Ip4Nic != NULL) {
    //
    // Already created before
    //
    return EFI_SUCCESS;
  }

  Private->Ip4Nic = AllocateZeroPool (sizeof (HTTP_BOOT_VIRTUAL_NIC));
  if (Private->Ip4Nic == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }
  Private->Ip4Nic->Private     = Private;
  Private->Ip4Nic->ImageHandle = This->DriverBindingHandle;
  Private->Ip4Nic->Signature   = HTTP_BOOT_VIRTUAL_NIC_SIGNATURE;

  //
  // Create DHCP4 child instance.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiDhcp4ServiceBindingProtocolGuid,
             &Private->Dhcp4Child
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Dhcp4Child,
                  &gEfiDhcp4ProtocolGuid,
                  (VOID **) &Private->Dhcp4,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Get the Ip4Config2 protocol, it's required to configure the default gateway address.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp4Config2ProtocolGuid,
                  (VOID **) &Private->Ip4Config2,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Append IPv4 device path node.
  //
  Node = AllocateZeroPool (sizeof (IPv4_DEVICE_PATH));
  if (Node == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }
  Node->Ipv4.Header.Type = MESSAGING_DEVICE_PATH;
  Node->Ipv4.Header.SubType = MSG_IPv4_DP;
  SetDevicePathNodeLength (Node, sizeof (IPv4_DEVICE_PATH));
  Node->Ipv4.StaticIpAddress = FALSE;
  DevicePath = AppendDevicePathNode (Private->ParentDevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Append URI device path node.
  //
  Node = AllocateZeroPool (sizeof (EFI_DEVICE_PATH_PROTOCOL));
  if (Node == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }
  Node->DevPath.Type = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, sizeof (EFI_DEVICE_PATH_PROTOCOL));
  Private->Ip4Nic->DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  FreePool (DevicePath);
  if (Private->Ip4Nic->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Create a child handle for the HTTP boot and install DevPath and Load file protocol on it.
  //
  CopyMem (&Private->Ip4Nic->LoadFile, &gHttpBootDxeLoadFile, sizeof (EFI_LOAD_FILE_PROTOCOL));
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Ip4Nic->Controller,
                  &gEfiLoadFileProtocolGuid,
                  &Private->Ip4Nic->LoadFile,
                  &gEfiDevicePathProtocolGuid,
                  Private->Ip4Nic->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Open the Caller Id child to setup a parent-child relationship between
  // real NIC handle and the HTTP boot Ipv4 NIC handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  This->DriverBindingHandle,
                  Private->Ip4Nic->Controller,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Private != NULL) {
    if (FirstStart) {
      gBS->UninstallProtocolInterface (
             ControllerHandle,
             &gEfiCallerIdGuid,
             &Private->Id
             );
    }

    HttpBootDestroyIp4Children (This, Private);
    HttpBootConfigFormUnload (Private);

    if (FirstStart) {
      FreePool (Private);
    }
  }

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
HttpBootIp4DxeDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_LOAD_FILE_PROTOCOL          *LoadFile;
  HTTP_BOOT_PRIVATE_DATA          *Private;
  EFI_HANDLE                      NicHandle;
  UINT32                          *Id;

  //
  // Try to get the Load File Protocol from the controller handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiLoadFileProtocolGuid,
                  (VOID **) &LoadFile,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // If failed, try to find the NIC handle for this controller.
    //
    NicHandle = HttpBootGetNicByIp4Children (ControllerHandle);
    if (NicHandle == NULL) {
      return EFI_SUCCESS;
    }

    //
    // Try to retrieve the private data by the Caller Id Guid.
    //
    Status = gBS->OpenProtocol (
                    NicHandle,
                    &gEfiCallerIdGuid,
                    (VOID **) &Id,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Private = HTTP_BOOT_PRIVATE_DATA_FROM_ID (Id);
  } else {
    Private = HTTP_BOOT_PRIVATE_DATA_FROM_LOADFILE (LoadFile);
    NicHandle = Private->Controller;
  }

  //
  // Disable the HTTP boot function.
  //
  Status = HttpBootStop (Private);
  if (Status != EFI_SUCCESS && Status != EFI_NOT_STARTED) {
    return Status;
  }

  //
  // Destory all child instance and uninstall protocol interface.
  //
  HttpBootDestroyIp4Children (This, Private);

  if (Private->Ip4Nic == NULL && Private->Ip6Nic == NULL) {
    //
    // Release the cached data.
    //
    HttpBootFreeCacheList (Private);

    //
    // Unload the config form.
    //
    HttpBootConfigFormUnload (Private);

    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiCallerIdGuid,
           &Private->Id
           );
    FreePool (Private);

  }

  return EFI_SUCCESS;
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
HttpBootIp6DxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                    Status;

  //
  // Try to open the DHCP6, HTTP and Device Path protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDhcp6ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiHttpServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
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
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
HttpBootIp6DxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                 Status;
  HTTP_BOOT_PRIVATE_DATA     *Private;
  EFI_DEV_PATH               *Node;
  EFI_DEVICE_PATH_PROTOCOL   *DevicePath;
  UINT32                     *Id;
  BOOLEAN                    Ipv6Available;
  BOOLEAN                    FirstStart;

  FirstStart = FALSE;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    Private = HTTP_BOOT_PRIVATE_DATA_FROM_ID(Id);
  } else {
    FirstStart = TRUE;

    //
    // Initialize the private data structure.
    //
    Private = AllocateZeroPool (sizeof (HTTP_BOOT_PRIVATE_DATA));
    if (Private == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Private->Signature = HTTP_BOOT_PRIVATE_DATA_SIGNATURE;
    Private->Controller = ControllerHandle;
    InitializeListHead (&Private->CacheList);
    //
    // Get the NII interface if it exists, it's not required.
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiNetworkInterfaceIdentifierProtocolGuid_31,
                    (VOID **) &Private->Nii,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      Private->Nii = NULL;
    }

    //
    // Open Device Path Protocol to prepare for appending IP and URI node.
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiDevicePathProtocolGuid,
                    (VOID **) &Private->ParentDevicePath,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Initialize the HII configuration form.
    //
    Status = HttpBootConfigFormInit (Private);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

    //
    // Install a protocol with Caller Id Guid to the NIC, this is just to build the relationship between
    // NIC handle and the private data.
    //
    Status = gBS->InstallProtocolInterface (
                    &ControllerHandle,
                    &gEfiCallerIdGuid,
                    EFI_NATIVE_INTERFACE,
                    &Private->Id
                    );
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }

  }

  //
  // Set IPv6 available flag.
  //
  Status = HttpBootCheckIpv6Support (Private, &Ipv6Available);
  if (EFI_ERROR (Status)) {
    //
    // Fail to get the data whether UNDI supports IPv6.
    // Set default value to TRUE.
    //
    Ipv6Available = TRUE;
  }

  if (!Ipv6Available) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  if (Private->Ip6Nic != NULL) {
    //
    // Already created before
    //
    return EFI_SUCCESS;
  }

  Private->Ip6Nic = AllocateZeroPool (sizeof (HTTP_BOOT_VIRTUAL_NIC));
  if (Private->Ip6Nic == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }
  Private->Ip6Nic->Private     = Private;
  Private->Ip6Nic->ImageHandle = This->DriverBindingHandle;
  Private->Ip6Nic->Signature   = HTTP_BOOT_VIRTUAL_NIC_SIGNATURE;

  //
  // Create Dhcp6 child and open Dhcp6 protocol
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiDhcp6ServiceBindingProtocolGuid,
             &Private->Dhcp6Child
             );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Dhcp6Child,
                  &gEfiDhcp6ProtocolGuid,
                  (VOID **) &Private->Dhcp6,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Create Ip6 child and open Ip6 protocol for background ICMP packets.
  //
  Status = NetLibCreateServiceChild (
              ControllerHandle,
              This->DriverBindingHandle,
              &gEfiIp6ServiceBindingProtocolGuid,
              &Private->Ip6Child
              );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->Ip6Child,
                  &gEfiIp6ProtocolGuid,
                  (VOID **) &Private->Ip6,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Locate Ip6Config protocol, it's required to configure the default gateway address.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp6ConfigProtocolGuid,
                  (VOID **) &Private->Ip6Config,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Append IPv6 device path node.
  //
  Node = AllocateZeroPool (sizeof (IPv6_DEVICE_PATH));
  if (Node == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }
  Node->Ipv6.Header.Type = MESSAGING_DEVICE_PATH;
  Node->Ipv6.Header.SubType = MSG_IPv6_DP;
  Node->Ipv6.PrefixLength = IP6_PREFIX_LENGTH;
  SetDevicePathNodeLength (Node, sizeof (IPv6_DEVICE_PATH));
  DevicePath = AppendDevicePathNode(Private->ParentDevicePath, (EFI_DEVICE_PATH*) Node);
  FreePool(Node);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Append URI device path node.
  //
  Node = AllocateZeroPool (sizeof (EFI_DEVICE_PATH_PROTOCOL));
  if (Node == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }
  Node->DevPath.Type = MESSAGING_DEVICE_PATH;
  Node->DevPath.SubType = MSG_URI_DP;
  SetDevicePathNodeLength (Node, sizeof (EFI_DEVICE_PATH_PROTOCOL));
  Private->Ip6Nic->DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  FreePool (DevicePath);
  if (Private->Ip6Nic->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Create a child handle for the HTTP boot and install DevPath and Load file protocol on it.
  //
  CopyMem (&Private->Ip6Nic->LoadFile, &gHttpBootDxeLoadFile, sizeof (Private->LoadFile));
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Ip6Nic->Controller,
                  &gEfiLoadFileProtocolGuid,
                  &Private->Ip6Nic->LoadFile,
                  &gEfiDevicePathProtocolGuid,
                  Private->Ip6Nic->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Open the Caller Id child to setup a parent-child relationship between
  // real NIC handle and the HTTP boot child handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  This->DriverBindingHandle,
                  Private->Ip6Nic->Controller,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Private != NULL) {
    if (FirstStart) {
      gBS->UninstallProtocolInterface (
             ControllerHandle,
             &gEfiCallerIdGuid,
             &Private->Id
             );
    }

    HttpBootDestroyIp6Children(This, Private);
    HttpBootConfigFormUnload (Private);

    if (FirstStart) {
      FreePool (Private);
    }
  }

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
HttpBootIp6DxeDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                      Status;
  EFI_LOAD_FILE_PROTOCOL          *LoadFile;
  HTTP_BOOT_PRIVATE_DATA          *Private;
  EFI_HANDLE                      NicHandle;
  UINT32                          *Id;

  //
  // Try to get the Load File Protocol from the controller handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiLoadFileProtocolGuid,
                  (VOID **) &LoadFile,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // If failed, try to find the NIC handle for this controller.
    //
    NicHandle = HttpBootGetNicByIp6Children (ControllerHandle);
    if (NicHandle == NULL) {
      return EFI_SUCCESS;
    }

    //
    // Try to retrieve the private data by the Caller Id Guid.
    //
    Status = gBS->OpenProtocol (
                    NicHandle,
                    &gEfiCallerIdGuid,
                    (VOID **) &Id,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Private = HTTP_BOOT_PRIVATE_DATA_FROM_ID (Id);
  } else {
    Private = HTTP_BOOT_PRIVATE_DATA_FROM_LOADFILE (LoadFile);
    NicHandle = Private->Controller;
  }

  //
  // Disable the HTTP boot function.
  //
  Status = HttpBootStop (Private);
  if (Status != EFI_SUCCESS && Status != EFI_NOT_STARTED) {
    return Status;
  }

  //
  // Destory all child instance and uninstall protocol interface.
  //
  HttpBootDestroyIp6Children (This, Private);

  if (Private->Ip4Nic == NULL && Private->Ip6Nic == NULL) {
    //
    // Release the cached data.
    //
    HttpBootFreeCacheList (Private);

    //
    // Unload the config form.
    //
    HttpBootConfigFormUnload (Private);

    gBS->UninstallProtocolInterface (
           NicHandle,
           &gEfiCallerIdGuid,
           &Private->Id
           );
    FreePool (Private);

  }

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
HttpBootDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS   Status;

  //
  // Install UEFI Driver Model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gHttpBootIp4DxeDriverBinding,
             ImageHandle,
             &gHttpBootDxeComponentName,
             &gHttpBootDxeComponentName2
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gHttpBootIp6DxeDriverBinding,
             NULL,
             &gHttpBootDxeComponentName,
             &gHttpBootDxeComponentName2
             );
  if (EFI_ERROR (Status)) {
    EfiLibUninstallDriverBindingComponentName2(
      &gHttpBootIp4DxeDriverBinding,
      &gHttpBootDxeComponentName,
      &gHttpBootDxeComponentName2
      );
  }
  return Status;
}

