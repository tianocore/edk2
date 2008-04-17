/** @file
  The entry point of IScsi driver

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IScsiDriver.c

Abstract:
  The entry point of IScsi driver

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

EFI_GUID                    mIScsiPrivateGuid   = ISCSI_PRIVATE_GUID;

/**
  Test to see if IScsi driver supports the given controller. 

  @param  This[in]                Protocol instance pointer.

  @param  ControllerHandle[in]    Handle of controller to test.

  @param  RemainingDevicePath[in] Optional parameter use to pick a specific child device to start.

  @retval EFI_SUCCES              This driver supports the controller.

  @retval EFI_ALREADY_STARTED     This driver is already running on this device.

  @retval EFI_UNSUPPORTED         This driver doesn't support the controller.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *CurrentDevicePath;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &mIScsiPrivateGuid,
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
  Start to manage the controller. 

  @param  This[in]                Protocol instance pointer.

  @param  ControllerHandle[in]    Handle of the controller.

  @param  RemainingDevicePath[in] Optional parameter use to pick a specific child device to start.

  @retval EFI_SUCCES              This driver supports this device.

  @retval EFI_ALREADY_STARTED     This driver is already running on this device.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;
  ISCSI_DRIVER_DATA *Private;

  //
  // Try to add a port configuration page for this controller.
  //
  IScsiConfigUpdateForm (This->DriverBindingHandle, ControllerHandle, TRUE);

  Private = IScsiCreateDriverData (This->DriverBindingHandle, ControllerHandle);
  if (Private == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
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
  // Install the iSCSI private stuff as a flag to indicate this controller
  // is already controlled by iSCSI driver.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &mIScsiPrivateGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->IScsiIdentifier
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  //
  // Update/Publish the iSCSI Boot Firmware Table.
  //
  IScsiPublishIbft ();

  return EFI_SUCCESS;

ON_ERROR:

  IScsiSessionAbort (&Private->Session);
  IScsiCleanDriverData (Private);

  return Status;
}

/**
  Release the control of this controller and remove the IScsi functions.

  @param  This[in]              Protocol instance pointer.

  @param  ControllerHandle[in]  Handle of controller to stop.

  @param  NumberOfChildren[in]  Not used.

  @param  ChildHandleBuffer[in] Not used.

  @retval EFI_SUCCES            This driver supports this device.

**/
EFI_STATUS
EFIAPI
IScsiDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
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
                  &mIScsiPrivateGuid,
                  (VOID **)&IScsiIdentifier,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  Private = ISCSI_DRIVER_DATA_FROM_IDENTIFIER (IScsiIdentifier);

  //
  // Uninstall the private protocol.
  //
  gBS->UninstallProtocolInterface (
        IScsiController,
        &mIScsiPrivateGuid,
        &Private->IScsiIdentifier
        );

  //
  // Update the iSCSI Boot Firware Table.
  //
  IScsiPublishIbft ();

  IScsiSessionAbort (&Private->Session);
  IScsiCleanDriverData (Private);

  return EFI_SUCCESS;
}

/**
  Unload the iSCSI driver.

  @param  ImageHandle[in]  The handle of the driver image.

  @retval EFI_SUCCESS      The driver is unloaded.

  @retval EFI_DEVICE_ERROR Some unexpected error happened.

**/
EFI_STATUS
EFIAPI
EfiIScsiUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;
  UINTN       DeviceHandleCount;
  EFI_HANDLE  *DeviceHandleBuffer;
  UINTN       Index;

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
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < DeviceHandleCount; Index++) {
      Status = gBS->DisconnectController (
                      DeviceHandleBuffer[Index],
                      ImageHandle,
                      NULL
                      );
    }

    if (DeviceHandleBuffer != NULL) {
      gBS->FreePool (DeviceHandleBuffer);
    }
  }
  //
  // Unload the iSCSI configuration form.
  //
  IScsiConfigFormUnload (gIScsiDriverBinding.DriverBindingHandle);

  //
  // Uninstall the protocols installed by iSCSI driver.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
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

  return Status;
}

/**
  Initialize the global variables publish the driver binding protocol.

  @param  ImageHandle[in]  The handle of the driver image.

  @param  SystemTable[in]  The EFI system table.

  @retval EFI_SUCCESS      The protocols are installed.

  @retval EFI_DEVICE_ERROR Some unexpected error happened.

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
    Status = IScsiConfigFormInit (gIScsiDriverBinding.DriverBindingHandle);
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

