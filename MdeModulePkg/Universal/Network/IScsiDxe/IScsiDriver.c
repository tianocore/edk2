/** @file
  The entry point of IScsi driver.

Copyright (c) 2004 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

EFI_DRIVER_BINDING_PROTOCOL gIScsiDriverBinding = {
  IScsiDriverBindingSupported,
  IScsiDriverBindingStart,
  IScsiDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  Tests to see if this driver supports the RemainingDevicePath. 

  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This 
                                   parameter is ignored by device drivers, and is optional for bus 
                                   drivers. For bus drivers, if this parameter is not NULL, then 
                                   the bus driver must determine if the bus controller specified 
                                   by ControllerHandle and the child controller specified 
                                   by RemainingDevicePath are both supported by this 
                                   bus driver.

  @retval EFI_SUCCESS              The RemainingDevicePath is supported or NULL.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
IScsiIsDevicePathSupported (
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;

  CurrentDevicePath = RemainingDevicePath;
  if (CurrentDevicePath != NULL) {
    while (!IsDevicePathEnd (CurrentDevicePath)) {
      if ((CurrentDevicePath->Type == MESSAGING_DEVICE_PATH) && (CurrentDevicePath->SubType == MSG_ISCSI_DP)) {
        return EFI_SUCCESS;
      }

      CurrentDevicePath = NextDevicePathNode (CurrentDevicePath);
    }

    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Tests to see if this driver supports a given controller. If a child device is provided, 
  it further tests to see if this driver supports creating a handle for the specified child device.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, and is optional for bus drivers.


  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive acces.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiTcp4ServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = IScsiIsDevicePathSupported (RemainingDevicePath);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  if (IScsiDhcpIsConfigured (ControllerHandle)) {
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiDhcp4ServiceBindingProtocolGuid,
                    NULL,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Start this driver on ControllerHandle. 
  
  The Start() function is designed to be invoked from the EFI boot service ConnectController(). 
  As a result, much of the error checking on the parameters to Start() has been moved into this 
  common boot service. It is legal to call Start() from other locations, but the following calling 
  restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.  

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle 
                                   must support a protocol interface that supplies 
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                                   This parameter is ignored by device drivers, and is optional for bus drivers.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.
                                   Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.
**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;
  ISCSI_DRIVER_DATA *Private;
  VOID              *Interface;

  Private = IScsiCreateDriverData (This->DriverBindingHandle, ControllerHandle);
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Create a underlayer child instance, but not need to configure it. Just open ChildHandle
  // via BY_DRIVER. That is, establishing the relationship between ControllerHandle and ChildHandle.
  // Therefore, when DisconnectController(), especially VLAN virtual controller handle,
  // IScsiDriverBindingStop() will be called.
  //
  Status = NetLibCreateServiceChild (
             ControllerHandle,
             This->DriverBindingHandle,
             &gEfiTcp4ServiceBindingProtocolGuid,
             &Private->ChildHandle
             );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Private->ChildHandle,
                  &gEfiTcp4ProtocolGuid,
                  &Interface,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Always install private protocol no matter what happens later. We need to 
  // keep the relationship between ControllerHandle and ChildHandle.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiCallerIdGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiIdentifier
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Try to add a port configuration page for this controller.
  //
  IScsiConfigUpdateForm (This->DriverBindingHandle, ControllerHandle, TRUE);

  //
  // Get the iSCSI configuration data of this controller.
  //
  Status = IScsiGetConfigData (Private);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  //
  // Try to login and create an iSCSI session according to the configuration.
  //
  Status = IScsiSessionLogin (Private);
  if (Status == EFI_MEDIA_CHANGED) {
    //
    // The specified target is not available and the redirection information is
    // got, login the session again with the updated target address.
    //
    Status = IScsiSessionLogin (Private);
  }

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  //
  // Duplicate the Session's tcp connection device path. The source port field
  // will be set to zero as one iSCSI session is comprised of several iSCSI
  // connections.
  //
  Private->DevicePath = IScsiGetTcpConnDevicePath (Private);
  if (Private->DevicePath == NULL) {
    goto ON_ERROR;
  }
  //
  // Install the updated device path onto the ExtScsiPassThruHandle.
  //
  Status = gBS->InstallProtocolInterface (
                  &Private->ExtScsiPassThruHandle,
                  &gEfiDevicePathProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  Private->DevicePath
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // ISCSI children should share the default Tcp child, just open the default Tcp child via BY_CHILD_CONTROLLER.
  //
  Status = gBS->OpenProtocol (
                  Private->ChildHandle, /// Default Tcp child
                  &gEfiTcp4ProtocolGuid,
                  &Interface,
                  This->DriverBindingHandle,
                  Private->ExtScsiPassThruHandle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );              
  if (EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           Private->ExtScsiPassThruHandle,
           &gEfiExtScsiPassThruProtocolGuid,
           &Private->IScsiExtScsiPassThru,
           &gEfiDevicePathProtocolGuid,
           Private->DevicePath,
           NULL
           );
    
    goto ON_ERROR;
  }

  //
  // Update/Publish the iSCSI Boot Firmware Table.
  //
  IScsiPublishIbft ();

  return EFI_SUCCESS;

ON_ERROR:

  IScsiSessionAbort (&Private->Session);

  return Status;
}

/**
  Stop this driver on ControllerHandle. 
  
  Release the control of this controller and remove the IScsi functions. The Stop()
  function is designed to be invoked from the EFI boot service DisconnectController(). 
  As a result, much of the error checking on the parameters to Stop() has been moved 
  into this common boot service. It is legal to call Stop() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
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
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.Not used.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL 
                                if NumberOfChildren is 0.Not used.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the Handle
                                because its interfaces are being used.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  )
{
  EFI_HANDLE                      IScsiController;
  EFI_STATUS                      Status;
  ISCSI_PRIVATE_PROTOCOL          *IScsiIdentifier;
  ISCSI_DRIVER_DATA               *Private;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL *PassThru;
  ISCSI_CONNECTION                *Conn;

  if (NumberOfChildren != 0) {
    //
    // We should have only one child.
    //
    Status = gBS->OpenProtocol (
                    ChildHandleBuffer[0],
                    &gEfiExtScsiPassThruProtocolGuid,
                    (VOID **) &PassThru,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    Private = ISCSI_DRIVER_DATA_FROM_EXT_SCSI_PASS_THRU (PassThru);
    Conn    = NET_LIST_HEAD (&Private->Session.Conns, ISCSI_CONNECTION, Link);

    //
    // Previously the TCP4 protocol is opened BY_CHILD_CONTROLLER. Just close
    // the protocol here but not uninstall the device path protocol and
    // EXT SCSI PASS THRU protocol installed on ExtScsiPassThruHandle.
    //
    gBS->CloseProtocol (
           Private->ChildHandle,
           &gEfiTcp4ProtocolGuid,
           Private->Image,
           Private->ExtScsiPassThruHandle
           );
    
    gBS->CloseProtocol (
          Conn->Tcp4Io.Handle,
          &gEfiTcp4ProtocolGuid,
          Private->Image,
          Private->ExtScsiPassThruHandle
          );

    return EFI_SUCCESS;
  }
  //
  // Get the handle of the controller we are controling.
  //
  IScsiController = NetLibGetNicHandle (ControllerHandle, &gEfiTcp4ProtocolGuid);

  Status = gBS->OpenProtocol (
                  IScsiController,
                  &gEfiCallerIdGuid,
                  (VOID **)&IScsiIdentifier,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = ISCSI_DRIVER_DATA_FROM_IDENTIFIER (IScsiIdentifier);

  if (Private->ChildHandle != NULL) {
    Status = gBS->CloseProtocol (
                    Private->ChildHandle,
                    &gEfiTcp4ProtocolGuid,
                    This->DriverBindingHandle,
                    IScsiController
                    );

    ASSERT (!EFI_ERROR (Status));

    Status = NetLibDestroyServiceChild (
               IScsiController,
               This->DriverBindingHandle,
               &gEfiTcp4ServiceBindingProtocolGuid,
               Private->ChildHandle
               );
    ASSERT (!EFI_ERROR (Status));
  }

  IScsiConfigUpdateForm (This->DriverBindingHandle, IScsiController, FALSE);

  //
  // Uninstall the private protocol.
  //
  gBS->UninstallProtocolInterface (
        IScsiController,
        &gEfiCallerIdGuid,
        &Private->IScsiIdentifier
        );

  //
  // Update the iSCSI Boot Firware Table.
  //
  IScsiPublishIbft ();

  IScsiSessionAbort (&Private->Session);
  Status = IScsiCleanDriverData (Private);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Unloads an image(the iSCSI driver).

  @param[in]  ImageHandle       Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval Others                Other errors as indicated.
**/
EFI_STATUS
EFIAPI
EfiIScsiUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                        Status;
  UINTN                             DeviceHandleCount;
  EFI_HANDLE                        *DeviceHandleBuffer;
  UINTN                             Index;
  EFI_COMPONENT_NAME_PROTOCOL       *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL      *ComponentName2;

  //
  // Try to disonnect the driver from the devices it's controlling.
  //
  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < DeviceHandleCount; Index++) {
    Status = IScsiTestManagedDevice (
               DeviceHandleBuffer[Index],
               gIScsiDriverBinding.DriverBindingHandle,
               &gEfiTcp4ProtocolGuid
               );
    if (EFI_ERROR (Status)) {
      continue;
    }
    Status = gBS->DisconnectController (
                    DeviceHandleBuffer[Index],
                    gIScsiDriverBinding.DriverBindingHandle,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }  

  //
  // Unload the iSCSI configuration form.
  //
  Status = IScsiConfigFormUnload (gIScsiDriverBinding.DriverBindingHandle);
  if (EFI_ERROR (Status)) {
    goto ON_EXIT;
  }

  //
  // Uninstall the ComponentName and ComponentName2 protocol from iSCSI4 driver binding handle
  // if it has been installed.
  //
  Status = gBS->HandleProtocol (
                  gIScsiDriverBinding.DriverBindingHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **) &ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->UninstallMultipleProtocolInterfaces (
           gIScsiDriverBinding.DriverBindingHandle,
           &gEfiComponentNameProtocolGuid,
           ComponentName,
           NULL
           );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }
  
  Status = gBS->HandleProtocol (
                  gIScsiDriverBinding.DriverBindingHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallMultipleProtocolInterfaces (
           gIScsiDriverBinding.DriverBindingHandle,
           &gEfiComponentName2ProtocolGuid,
           ComponentName2,
           NULL
           );
    if (EFI_ERROR (Status)) {
      goto ON_EXIT;
    }
  }

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ImageHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gIScsiDriverBinding,
                  &gEfiIScsiInitiatorNameProtocolGuid,
                  &gIScsiInitiatorName,
                  NULL
                  );
ON_EXIT:

  if (DeviceHandleBuffer != NULL) {
    FreePool (DeviceHandleBuffer);
  }
  
  return Status;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers. It initialize the global variables and 
  publish the driver binding protocol.

  @param[in]   ImageHandle      The firmware allocated handle for the UEFI image.
  @param[in]   SystemTable      A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_ACCESS_DENIED     EFI_ISCSI_INITIATOR_NAME_PROTOCOL was installed unexpectedly.
  @retval Others                Other errors as indicated.
**/
EFI_STATUS
EFIAPI
IScsiDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                         Status;
  EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *IScsiInitiatorName;

  //
  // There should be only one EFI_ISCSI_INITIATOR_NAME_PROTOCOL.
  //
  Status = gBS->LocateProtocol (
                   &gEfiIScsiInitiatorNameProtocolGuid,
                   NULL,
                   (VOID**) &IScsiInitiatorName
                   );

  if (!EFI_ERROR (Status)) {
    return EFI_ACCESS_DENIED;
  }

  //
  // Initialize the EFI Driver Library
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIScsiDriverBinding,
             ImageHandle,
             &gIScsiComponentName,
             &gIScsiComponentName2
           );

  if (!EFI_ERROR (Status)) {
    //
    // Install the iSCSI Initiator Name Protocol.
    //
    Status = gBS->InstallProtocolInterface (
                    &ImageHandle,
                    &gEfiIScsiInitiatorNameProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &gIScsiInitiatorName
                    );
    if (EFI_ERROR (Status)) {
      gBS->UninstallMultipleProtocolInterfaces (
            ImageHandle,
            &gEfiDriverBindingProtocolGuid,
            &gIScsiDriverBinding,
            &gEfiComponentName2ProtocolGuid,
            &gIScsiComponentName2,
            &gEfiComponentNameProtocolGuid,
            &gIScsiComponentName,
            NULL
            );
      return Status;
    }
  
    //
    // Initialize the configuration form of iSCSI.
    //
    Status = IScsiConfigFormInit ();
    if (EFI_ERROR (Status)) {
      gBS->UninstallMultipleProtocolInterfaces (
            ImageHandle,
            &gEfiDriverBindingProtocolGuid,
            &gIScsiDriverBinding,
            &gEfiComponentName2ProtocolGuid,
            &gIScsiComponentName2,
            &gEfiComponentNameProtocolGuid,
            &gIScsiComponentName,
            &gEfiIScsiInitiatorNameProtocolGuid,
            &gIScsiInitiatorName,
            NULL
            );
    }
  }
  return Status;
}

