/** @file
  Implementation of driver entry point and driver binding protocol.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MnpDriver.h"
#include "MnpImpl.h"
#include "MnpVlan.h"

EFI_DRIVER_BINDING_PROTOCOL gMnpDriverBinding = {
  MnpDriverBindingSupported,
  MnpDriverBindingStart,
  MnpDriverBindingStop,
  0xa,
  NULL,
  NULL
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
MnpDestroyServiceDataEntry (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  MNP_SERVICE_DATA              *MnpServiceData;

  MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);
  return MnpDestroyServiceData (MnpServiceData);
}

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
MnpDestroyServiceChildEntry (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  )
{
  MNP_SERVICE_DATA              *MnpServiceData;

  MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);
  return MnpDestroyServiceChild (MnpServiceData);
}

/**
  Test to see if this driver supports ControllerHandle. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test.
  @param[in]  RemainingDevicePath  Optional parameter use to pick a specific
                                   child device to start.

  @retval EFI_SUCCESS              This driver supports this device.
  @retval EFI_ALREADY_STARTED      This driver is already running on this device.
  @retval Others                   This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;

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
  // Close the opened SNP protocol.
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
  Start this driver on ControllerHandle. This service is called by the
  EFI boot service ConnectController(). In order to make drivers as small
  as possible, there are a few calling restrictions for this service.
  ConnectController() must follow these calling restrictions. If any other
  agent wishes to call Start() it must also follow these calling restrictions.

  @param[in]       This                 Protocol instance pointer.
  @param[in]       ControllerHandle     Handle of device to bind driver to.
  @param[in]       RemainingDevicePath  Optional parameter use to pick a specific
                                        child device to start.

  @retval EFI_SUCCESS           This driver is added to ControllerHandle.
  @retval EFI_ALREADY_STARTED   This driver is already running on ControllerHandle.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate memory for Mnp Service Data.
  @retval Others                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath OPTIONAL
  )
{
  EFI_STATUS        Status;
  MNP_SERVICE_DATA  *MnpServiceData;
  MNP_DEVICE_DATA   *MnpDeviceData;
  LIST_ENTRY        *Entry;
  VLAN_TCI          *VlanVariable;
  UINTN             NumberOfVlan;
  UINTN             Index;

  VlanVariable = NULL;

  //
  // Initialize the Mnp Device Data
  //
  MnpDeviceData = AllocateZeroPool (sizeof (MNP_DEVICE_DATA));
  if (MnpDeviceData == NULL) {
    DEBUG ((EFI_D_ERROR, "MnpDriverBindingStart(): Failed to allocate the Mnp Device Data.\n"));

    return EFI_OUT_OF_RESOURCES;
  }

  Status = MnpInitializeDeviceData (MnpDeviceData, This->DriverBindingHandle, ControllerHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "MnpDriverBindingStart: MnpInitializeDeviceData failed, %r.\n", Status));

    FreePool (MnpDeviceData);
    return Status;
  }

  //
  // Check whether NIC driver has already produced VlanConfig protocol
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // NIC hardware already implement VLAN,
    // no need to provide software VLAN implementation in MNP driver
    //
    MnpDeviceData->NumberOfVlan = 0;
    ZeroMem (&MnpDeviceData->VlanConfig, sizeof (EFI_VLAN_CONFIG_PROTOCOL));
    MnpServiceData = MnpCreateServiceData (MnpDeviceData, 0, 0);
    Status = (MnpServiceData != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Install VLAN Config Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiVlanConfigProtocolGuid,
                  &MnpDeviceData->VlanConfig,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  //
  // Get current VLAN configuration from EFI Variable
  //
  NumberOfVlan = 0;
  Status = MnpGetVlanVariable (MnpDeviceData, &NumberOfVlan, &VlanVariable);
  if (EFI_ERROR (Status)) {
    //
    // No VLAN is set, create a default MNP service data for untagged frame
    //
    MnpDeviceData->NumberOfVlan = 0;
    MnpServiceData = MnpCreateServiceData (MnpDeviceData, 0, 0);
    Status = (MnpServiceData != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Create MNP service data for each VLAN
  //
  MnpDeviceData->NumberOfVlan = NumberOfVlan;
  for (Index = 0; Index < NumberOfVlan; Index++) {
    MnpServiceData = MnpCreateServiceData (
                       MnpDeviceData,
                       VlanVariable[Index].Bits.Vid,
                       (UINT8) VlanVariable[Index].Bits.Priority
                       );

    if (MnpServiceData == NULL) {
      Status = EFI_OUT_OF_RESOURCES;

      goto Exit;
    }
  }

Exit:
  if (VlanVariable != NULL) {
    FreePool (VlanVariable);
  }

  if (EFI_ERROR (Status)) {
    //
    // Destroy all MNP service data
    //
    while (!IsListEmpty (&MnpDeviceData->ServiceList)) {
      Entry = GetFirstNode (&MnpDeviceData->ServiceList);
      MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);
      MnpDestroyServiceData (MnpServiceData);
    }

    //
    // Uninstall the VLAN Config Protocol if any
    //
    if (MnpDeviceData->VlanConfig.Set != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             MnpDeviceData->ControllerHandle,
             &gEfiVlanConfigProtocolGuid,
             &MnpDeviceData->VlanConfig,
             NULL
             );
    }

    //
    // Destroy Mnp Device Data
    //
    MnpDestroyDeviceData (MnpDeviceData, This->DriverBindingHandle);
    FreePool (MnpDeviceData);
  }

  return Status;
}

/**
  Stop this driver on ControllerHandle. This service is called by the
  EFI boot service DisconnectController(). In order to make drivers as
  small as possible, there are a few calling restrictions for this service.
  DisconnectController() must follow these calling restrictions. If any other
  agent wishes to call Stop() it must also follow these calling restrictions.

  @param[in]  This               Protocol instance pointer.
  @param[in]  ControllerHandle   Handle of device to stop driver on.
  @param[in]  NumberOfChildren   Number of Handles in ChildHandleBuffer. If
                                 number of children is zero stop the entire
                                 bus driver.
  @param[in]  ChildHandleBuffer  List of Child Handles to Stop.

  @retval EFI_SUCCESS            This driver is removed ControllerHandle.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
MnpDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN EFI_HANDLE                      ControllerHandle,
  IN UINTN                           NumberOfChildren,
  IN EFI_HANDLE                      *ChildHandleBuffer OPTIONAL
  )
{
  EFI_STATUS                    Status;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;
  EFI_VLAN_CONFIG_PROTOCOL      *VlanConfig;
  MNP_DEVICE_DATA               *MnpDeviceData;
  MNP_SERVICE_DATA              *MnpServiceData;
  LIST_ENTRY                    *List;
  UINTN                         ListLength;

  //
  // Try to retrieve MNP service binding protocol from the ControllerHandle
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
    //
    // Retrieve VLAN Config Protocol from the ControllerHandle
    //
    Status = gBS->OpenProtocol (
                    ControllerHandle,
                    &gEfiVlanConfigProtocolGuid,
                    (VOID **) &VlanConfig,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "MnpDriverBindingStop: try to stop unknown Controller.\n"));
      return EFI_DEVICE_ERROR;
    }

    MnpDeviceData = MNP_DEVICE_DATA_FROM_THIS (VlanConfig);
  } else {
    MnpServiceData = MNP_SERVICE_DATA_FROM_THIS (ServiceBinding);
    MnpDeviceData = MnpServiceData->MnpDeviceData;
  }

  if (NumberOfChildren == 0) {
    //
    // Destroy all MNP service data
    //
    List = &MnpDeviceData->ServiceList;
    Status = NetDestroyLinkList (
               List,
               MnpDestroyServiceDataEntry,
               NULL,
               &ListLength
               );
    if (EFI_ERROR (Status) || ListLength !=0) {
      return EFI_DEVICE_ERROR;
    }

    //
    // Uninstall the VLAN Config Protocol if any
    //
    if (MnpDeviceData->VlanConfig.Set != NULL) {
      gBS->UninstallMultipleProtocolInterfaces (
             MnpDeviceData->ControllerHandle,
             &gEfiVlanConfigProtocolGuid,
             &MnpDeviceData->VlanConfig,
             NULL
             );
    }

    //
    // Destroy Mnp Device Data
    //
    MnpDestroyDeviceData (MnpDeviceData, This->DriverBindingHandle);
    FreePool (MnpDeviceData);

    if (gMnpControllerNameTable != NULL) {
      FreeUnicodeStringTable (gMnpControllerNameTable);
      gMnpControllerNameTable = NULL;
    }
    return EFI_SUCCESS;
  }

  //
  // Stop all MNP child
  //
  List = &MnpDeviceData->ServiceList;
  Status = NetDestroyLinkList (
             List,
             MnpDestroyServiceChildEntry,
             NULL,
             &ListLength
             );
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}


/**
  Creates a child handle with a set of I/O services.

  @param[in]       This              Protocol instance pointer.
  @param[in, out]  ChildHandle       Pointer to the handle of the child to create. If
                                     it is NULL, then a new handle is created. If
                                     it is not NULL, then the I/O services are added
                                     to the existing child handle.

  @retval EFI_SUCCESS                The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER      ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources available to
                                     create the child.
  @retval Others                     The child handle was not created.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingCreateChild (
  IN     EFI_SERVICE_BINDING_PROTOCOL    *This,
  IN OUT EFI_HANDLE                      *ChildHandle
  )
{
  EFI_STATUS         Status;
  MNP_SERVICE_DATA   *MnpServiceData;
  MNP_INSTANCE_DATA  *Instance;
  VOID               *MnpSb;
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
    DEBUG ((DEBUG_ERROR, "MnpServiceBindingCreateChild: Failed to allocate memory for the new instance.\n"));

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
                  MnpServiceData->ServiceHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  (VOID **) &MnpSb,
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
            Instance->Handle,
            &gEfiManagedNetworkProtocolGuid,
            &Instance->ManagedNetwork,
            NULL
            );
    }

    FreePool (Instance);
  }

  return Status;
}


/**
  Destroys a child handle with a set of I/O services.

  The DestroyChild() function does the opposite of CreateChild(). It removes a
  protocol that was installed by CreateChild() from ChildHandle. If the removed
  protocol is the last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in]  This               Pointer to the EFI_SERVICE_BINDING_PROTOCOL
                                 instance.
  @param[in]  ChildHandle        Handle of the child to destroy.

  @retval EFI_SUCCESS            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED        ChildHandle does not support the protocol that
                                 is being removed.
  @retval EFI_INVALID_PARAMETER  ChildHandle is NULL.
  @retval EFI_ACCESS_DENIED      The protocol could not be removed from the
                                 ChildHandle because its services are being
                                 used.
  @retval Others                 The child handle was not destroyed.

**/
EFI_STATUS
EFIAPI
MnpServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                      ChildHandle
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
         MnpServiceData->ServiceHandle,
         &gEfiManagedNetworkServiceBindingProtocolGuid,
         MnpServiceData->MnpDeviceData->ImageHandle,
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

  FreePool (Instance);

  return Status;
}

/**
  The entry point for Mnp driver which installs the driver binding and component
  name protocol on its ImageHandle.

  @param[in]  ImageHandle  The image handle of the driver.
  @param[in]  SystemTable  The system table.

  @retval EFI_SUCCESS      The driver binding and component name protocols are
                           successfully installed.
  @retval Others           Other errors as indicated.

**/
EFI_STATUS
EFIAPI
MnpDriverEntryPoint (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
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
