/** @file

Copyright (c) 2005 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MnpDriver.c

Abstract:


**/

#include "MnpDriver.h"
#include "MnpImpl.h"


EFI_DRIVER_BINDING_PROTOCOL gMnpDriverBinding = {
  MnpDriverBindingSupported,
  MnpDriverBindingStart,
  MnpDriverBindingStop,
  0xa,
  NULL,
  NULL
};


/**
  Test to see if this driver supports ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to test.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver supports this device.
  @retval EFI_ALREADY_STARTED    This driver is already running on this device.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;

  //
  // Test to see if MNP is already installed.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Test to open the Simple Network protocol BY_DRIVER.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Snp,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the openned SNP protocol.
  //
  gBS->CloseProtocol (
         ControllerHandle,
         &gEfiSimpleNetworkProtocolGuid,
         This->DriverBindingHandle,
         ControllerHandle
         );

  return EFI_SUCCESS;
}


/**
  Start this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCES             This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED    This driver is already running on
                                 ControllerHandle.
  @retval other                  This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;
  MNP_SERVICE_DATA  *MnpServiceData;
  BOOLEAN           MnpInitialized;

  MnpInitialized  = FALSE;

  MnpServiceData  = AllocateZeroPool (sizeof (MNP_SERVICE_DATA));
  if (MnpServiceData == NULL) {
    DEBUG ((EFI_D_ERROR, "MnpDriverBindingStart(): Failed to allocate the Mnp Service Data.\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the Mnp Service Data.
  //
  Status = MnpInitializeServiceData (MnpServiceData, This->DriverBindingHandle, ControllerHandle);
  if (EFI_ERROR (Status)) {

    DEBUG ((EFI_D_ERROR, "MnpDriverBindingStart: MnpInitializeServiceData failed, %r.\n",Status));
    goto ErrorExit;
  }

  MnpInitialized = TRUE;

  //
  // Install the MNP Service Binding Protocol.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  &MnpServiceData->ServiceBinding,
                  NULL
                  );

ErrorExit:

  if (EFI_ERROR (Status)) {

    if (MnpInitialized) {
      //
      // Flush the Mnp Service Data.
      //
      MnpFlushServiceData (MnpServiceData);
    }

    //
    // Close the Simple Network Protocol.
    //
    gBS->CloseProtocol (
          ControllerHandle,
          &gEfiSimpleNetworkProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );

    gBS->FreePool (MnpServiceData);
  }

  return Status;
}


/**
  Stop this driver on ControllerHandle.

  @param  This                   Protocol instance pointer.
  @param  ControllerHandle       Handle of device to stop driver on.
  @param  NumberOfChildren       Number of Handles in ChildHandleBuffer. If number
                                 of children is zero stop the entire bus driver.
  @param  ChildHandleBuffer      List of Child Handles to Stop.

  @retval EFI_SUCCES             This driver is removed ControllerHandle.
  @retval other                  This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  MNP_SERVICE_DATA              *MnpServiceData;
  MNP_INSTANCE_DATA             *Instance;

  //
  // Retrieve the MNP service binding protocol from the ControllerHandle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  (VOID **) &ServiceBinding,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "MnpDriverBindingStop: Locate MNP Service Binding Protocol failed, %r.\n",
      Status)
      );
    return EFI_DEVICE_ERROR;
  }

  MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (ServiceBinding);

  if (NumberOfChildren == 0) {
    //
    // Uninstall the MNP Service Binding Protocol.
    //
    gBS->UninstallMultipleProtocolInterfaces (
           ControllerHandle,
           &gEfiManagedNetworkServiceBindingProtocolGuid,
           ServiceBinding,
           NULL
           );

    //
    // Close the openned Snp protocol.
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiSimpleNetworkProtocolGuid,
           This->DriverBindingHandle,
           ControllerHandle
           );

    //
    // Flush the Mnp service data.
    //
    MnpFlushServiceData (MnpServiceData);

    gBS->FreePool (MnpServiceData);
  } else {
    while (!IsListEmpty (&MnpServiceData->ChildrenList)) {
      //
      // Don't use NetListRemoveHead here, the remove opreration will be done
      // in ServiceBindingDestroyChild.
      //
      Instance = NET_LIST_HEAD (
                   &MnpServiceData->ChildrenList,
                   MNP_INSTANCE_DATA,
                   InstEntry
                   );

      ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
    }
  }

  return Status;
}


/**
  Creates a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Pointer to the handle of the child to create. If
                                 it is NULL, then a new handle is created. If it is
                                 not NULL, then the I/O services are  added to the
                                 existing child handle.

  @retval EFI_SUCCES             The child handle was created with the I/O
                                 services.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources availabe to create
                                 the child.
  @retval other                  The child handle was not created.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  )
{
  EFI_STATUS         Status;
  MNP_SERVICE_DATA   *MnpServiceData;
  MNP_INSTANCE_DATA  *Instance;
  VOID               *Snp;
  EFI_TPL            OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (This);

  //
  // Allocate buffer for the new instance.
  //
  Instance = AllocateZeroPool (sizeof (MNP_INSTANCE_DATA));
  if (Instance == NULL) {

    DEBUG ((EFI_D_ERROR, "MnpServiceBindingCreateChild: Faild to allocate memory for the new instance.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Init the instance data.
  //
  MnpInitializeInstanceData (MnpServiceData, Instance);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  &Instance->ManagedNetwork,
                  NULL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "MnpServiceBindingCreateChild: Failed to install the MNP protocol, %r.\n",
      Status)
      );
    goto ErrorExit;
  }

  //
  // Save the instance's childhandle.
  //
  Instance->Handle = *ChildHandle;

  Status = gBS->OpenProtocol (
                  MnpServiceData->ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Snp,
                  gMnpDriverBinding.DriverBindingHandle,
                  Instance->Handle,
                  EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Add the child instance into ChildrenList.
  //
  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  InsertTailList (&MnpServiceData->ChildrenList, &Instance->InstEntry);
  MnpServiceData->ChildrenNumber++;

  gBS->RestoreTPL (OldTpl);

ErrorExit:

  if (EFI_ERROR (Status)) {

    if (Instance->Handle != NULL) {

      gBS->UninstallMultipleProtocolInterfaces (
            &gEfiManagedNetworkProtocolGuid,
            &Instance->ManagedNetwork,
            NULL
            );
    }

    gBS->FreePool (Instance);
  }

  return Status;
}


/**
  Destroys a child handle with a set of I/O services.

  @param  This                   Protocol instance pointer.
  @param  ChildHandle            Handle of the child to destroy.

  @retval EFI_SUCCES             The I/O services were removed from the child
                                 handle.
  @retval EFI_UNSUPPORTED        The child handle does not support the I/O services
                                  that are being removed.
  @retval EFI_INVALID_PARAMETER  Child handle is not a valid EFI Handle.
  @retval EFI_ACCESS_DENIED      The child handle could not be destroyed because
                                 its  I/O services are being used.
  @retval other                  The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  )
{
  EFI_STATUS                    Status;
  MNP_SERVICE_DATA              *MnpServiceData;
  EFI_MANAGED_NETWORK_PROTOCOL  *ManagedNetwork;
  MNP_INSTANCE_DATA             *Instance;
  EFI_TPL                       OldTpl;

  if ((This == NULL) || (ChildHandle == NULL)) {

    return EFI_INVALID_PARAMETER;
  }

  MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (This);

  //
  // Try to retrieve ManagedNetwork Protocol from ChildHandle.
  //
  Status = gBS->OpenProtocol (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &ManagedNetwork,
                  gMnpDriverBinding.DriverBindingHandle,
                  ChildHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {

    return EFI_UNSUPPORTED;
  }

  Instance = MNP_INSTANCE_DATA_FROM_THIS (ManagedNetwork);

  //
  // MnpServiceBindingDestroyChild may be called twice: first called by
  // MnpServiceBindingStop, second called by uninstalling the MNP protocol
  // in this ChildHandle. Use destroyed to make sure the resource clean code
  // will only excecute once.
  //
  if (Instance->Destroyed) {

    return EFI_SUCCESS;
  }

  Instance->Destroyed = TRUE;

  //
  // Close the Simple Network protocol.
  //
  gBS->CloseProtocol (
         MnpServiceData->ControllerHandle,
         &gEfiSimpleNetworkProtocolGuid,
         gMnpDriverBinding.DriverBindingHandle,
         ChildHandle
         );

  //
  // Uninstall the ManagedNetwork protocol.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  ChildHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  &Instance->ManagedNetwork,
                  NULL
                  );
  if (EFI_ERROR (Status)) {

    DEBUG (
      (EFI_D_ERROR,
      "MnpServiceBindingDestroyChild: Failed to uninstall the ManagedNetwork protocol, %r.\n",
      Status)
      );

    Instance->Destroyed = FALSE;
    return Status;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  //
  // Reset the configuration.
  //
  ManagedNetwork->Configure (ManagedNetwork, NULL);

  //
  // Try to flush the RcvdPacketQueue.
  //
  MnpFlushRcvdDataQueue (Instance);

  //
  // Clean the RxTokenMap.
  //
  NetMapClean (&Instance->RxTokenMap);

  //
  // Remove this instance from the ChildrenList.
  //
  RemoveEntryList (&Instance->InstEntry);
  MnpServiceData->ChildrenNumber--;

  gBS->RestoreTPL (OldTpl);

  gBS->FreePool (Instance);

  return Status;
}


EFI_STATUS
EFIAPI
MnpDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
/*++

Routine Description:

  The entry point for Mnp driver which installs the driver binding and component name
  protocol on its ImageHandle.

Arguments:

  ImageHandle - The image handle of the driver.
  SystemTable - The system table.

Returns:

  EFI_SUCCESS - If the driver binding and component name protocols are successfully
                installed, otherwise if failed.

--*/
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gMnpDriverBinding,
           ImageHandle,
           &gMnpComponentName,
           &gMnpComponentName2
           );
}
