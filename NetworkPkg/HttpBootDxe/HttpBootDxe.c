/** @file
  Driver Binding functions implementation for UEFI HTTP boot.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                          
    
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

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
  ASSERT (Private->UsingIpv6 == FALSE);

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

  if (Private->HttpCreated) {
    HttpIoDestroyIo (&Private->HttpIo);
    Private->HttpCreated = FALSE;
  }

  gBS->CloseProtocol (
         Private->Controller,
         &gEfiCallerIdGuid,
         This->DriverBindingHandle,
         Private->ChildHandle
         );

  gBS->UninstallMultipleProtocolInterfaces (
         Private->ChildHandle,
         &gEfiLoadFileProtocolGuid,
         &Private->LoadFile,
         &gEfiDevicePathProtocolGuid,
         Private->DevicePath,
         NULL
         );

  if (Private->DevicePath != NULL) {
    FreePool (Private->DevicePath);
    Private->DevicePath = NULL;
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

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Initialize the private data structure.
  //
  Private = AllocateZeroPool (sizeof (HTTP_BOOT_PRIVATE_DATA));
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Private->Signature = HTTP_BOOT_PRIVATE_DATA_SIGNATURE;
  Private->Controller = ControllerHandle;
  Private->Image = This->ImageHandle;
  Private->UsingIpv6 = FALSE;
  InitializeListHead (&Private->CacheList);

  //
  // Create DHCP child instance.
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
  Private->DevicePath = AppendDevicePathNode (DevicePath, (EFI_DEVICE_PATH_PROTOCOL*) Node);
  FreePool (Node);
  FreePool (DevicePath);
  if (Private->DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Create a child handle for the HTTP boot and install DevPath and Load file protocol on it.
  //
  CopyMem (&Private->LoadFile, &gHttpBootDxeLoadFile, sizeof (Private->LoadFile));
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->ChildHandle,
                  &gEfiLoadFileProtocolGuid,
                  &Private->LoadFile,
                  &gEfiDevicePathProtocolGuid,
                  Private->DevicePath,
                  NULL
                  );
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

  //
  // Open the Caller Id child to setup a parent-child relationship between
  // real NIC handle and the HTTP boot child handle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &Id,
                  This->DriverBindingHandle,
                  Private->ChildHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return EFI_SUCCESS;

ON_ERROR:
  
  HttpBootDestroyIp4Children (This, Private);
  FreePool (Private);

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

  //
  // Release the cached data.
  //
  HttpBootFreeCacheList (Private);

  gBS->UninstallProtocolInterface (
         NicHandle,
         &gEfiCallerIdGuid,
         &Private->Id
         );
  FreePool (Private);

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
  //
  // Install UEFI Driver Model protocol(s).
  //
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gHttpBootIp4DxeDriverBinding,
           ImageHandle,
           &gHttpBootDxeComponentName,
           &gHttpBootDxeComponentName2
           );
}

