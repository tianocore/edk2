/** @file
  The driver binding for VLAN configuration module.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "VlanConfigImpl.h"

EFI_DRIVER_BINDING_PROTOCOL gVlanConfigDriverBinding = {
  VlanConfigDriverBindingSupported,
  VlanConfigDriverBindingStart,
  VlanConfigDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The entry point for IP4 config driver which install the driver
  binding and component name protocol on its image.

  @param[in]  ImageHandle        The image handle of the driver.
  @param[in]  SystemTable        The system table.

  @retval EFI_SUCCESS            All the related protocols are installed on the driver.
  @retval Others                 Failed to install protocols.

**/
EFI_STATUS
EFIAPI
VlanConfigDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gVlanConfigDriverBinding,
           ImageHandle,
           &gVlanConfigComponentName,
           &gVlanConfigComponentName2
           );
}


/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver supports this device
  @retval EFI_ALREADY_STARTED  This driver is already running on this device
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
VlanConfigDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  (VOID **) &VlanConfig,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the VlanConfig protocol opened for supported test
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiVlanConfigProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return Status;
}


/**
  Start this driver on ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific child
                                   device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
VlanConfigDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                Status;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  VLAN_CONFIG_PRIVATE_DATA  *PrivateData;

  //
  // Check for multiple start
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Open VlanConfig protocol by driver
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  (VOID **) &VlanConfig,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get parent device path
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Create a private data for this network device
  //
  PrivateData = AllocateCopyPool (sizeof (VLAN_CONFIG_PRIVATE_DATA), &mVlanConfigPrivateDateTemplate);
  if (PrivateData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  PrivateData->ImageHandle = This->DriverBindingHandle;
  PrivateData->ControllerHandle = ControllerHandle;
  PrivateData->VlanConfig = VlanConfig;
  PrivateData->ParentDevicePath = DevicePath;

  //
  // Install VLAN configuration form
  //
  Status = InstallVlanConfigForm (PrivateData);
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Install private GUID
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }
  return Status;

ErrorExit:
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiVlanConfigProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiDevicePathProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  if (PrivateData != NULL) {
    UninstallVlanConfigForm (PrivateData);
    FreePool (PrivateData);
  }

  return Status;
}


/**
  Stop this driver on ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to stop driver on
  @param[in]  NumberOfChildren     Number of Handles in ChildHandleBuffer. If number
                                   of children is zero stop the entire bus driver.
  @param[in]  ChildHandleBuffer    List of Child Handles to Stop.

  @retval EFI_SUCCESS          This driver is removed ControllerHandle
  @retval other                This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
VlanConfigDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      ControllerHandle,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer
  )
{
  EFI_STATUS                Status;
  VLAN_CONFIG_PRIVATE_DATA  *PrivateData;

  //
  // Retrieve the PrivateData from ControllerHandle
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  (VOID **) &PrivateData,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (PrivateData->Signature == VLAN_CONFIG_PRIVATE_DATA_SIGNATURE);

  if (NumberOfChildren != 0) {
    if (NumberOfChildren != 1 || ChildHandleBuffer[0] != PrivateData->DriverHandle) {
      return EFI_DEVICE_ERROR;
    }

    return UninstallVlanConfigForm (PrivateData);
  }

  //
  // Uninstall the private GUID
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ControllerHandle,
                  &gEfiCallerIdGuid,
                  PrivateData,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );
  return Status;
}
