/** @file

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Ip4ConfigDriver.c

Abstract:

  The driver binding for IP4 CONFIG protocol.


**/


#include "Ip4Config.h"


/**
  Stop all the auto configuration when the IP4 configure driver is
  being unloaded.

  @param  ImageHandle          The driver that is being unloaded

  @retval EFI_SUCCESS          The driver has been ready for unload.

**/
EFI_STATUS
EFIAPI
EfiIp4ConfigUnload (
  IN EFI_HANDLE             ImageHandle
  )
{
  UINT32      Index;

  //
  //  Stop all the IP4_CONFIG instances
  //
  for (Index = 0; Index < MAX_IP4_CONFIG_IN_VARIABLE; Index++) {
    if (mIp4ConfigNicList[Index] == NULL) {
      continue;
    }

    gIp4ConfigDriverBinding.Stop (
                              &gIp4ConfigDriverBinding,
                              mIp4ConfigNicList[Index]->MnpHandle,
                              0,
                              NULL
                              );
  }

  return NetLibDefaultUnload (ImageHandle);
}

/**
  The entry point for IP4 config driver which install the driver
  binding and component name protocol on its image.

  @param  ImageHandle            The image handle of the driver.
  @param  SystemTable            The system table.

  @retval EFI_SUCCES             All the related protocols are installed on the driver.
  @retval Others                 Failed to install protocols.

**/
EFI_STATUS
Ip4ConfigDriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gIp4ConfigDriverBinding,
           ImageHandle,
           &gIp4ConfigComponentName,
           &gIp4ConfigComponentName2
           );
}


/**
  Test to see if this driver supports ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to test
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCES           This driver supports this device
  @retval EFI_ALREADY_STARTED  This driver is already running on this device
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  EFI_STATUS  Status;

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );

  return Status;
}


/**
  Start this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCES           This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  EFI_IP4_CONFIG_PROTOCOL       *Ip4Config;
  EFI_MANAGED_NETWORK_PROTOCOL  *Mnp;
  EFI_HANDLE                    MnpHandle;
  IP4_CONFIG_INSTANCE           *Instance;
  EFI_SIMPLE_NETWORK_MODE       SnpMode;
  IP4_CONFIG_VARIABLE           *Variable;
  NIC_IP4_CONFIG_INFO           *NicConfig;
  IP4_CONFIG_VARIABLE           *NewVariable;
  EFI_STATUS                    Status;
  UINT32                        Index;

  //
  // Check for multiple start.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiIp4ConfigProtocolGuid,
                  (VOID **) &Ip4Config,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (!EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Create a MNP child
  //
  Mnp       = NULL;
  MnpHandle = NULL;
  Instance  = NULL;

  Status    = NetLibCreateServiceChild (
                ControllerHandle,
                This->DriverBindingHandle,
                &gEfiManagedNetworkServiceBindingProtocolGuid,
                &MnpHandle
                );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  MnpHandle,
                  &gEfiManagedNetworkProtocolGuid,
                  (VOID **) &Mnp,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Allocate an instance then initialize it
  //
  Instance = AllocatePool (sizeof (IP4_CONFIG_INSTANCE));

  if (Instance == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Instance->Signature         = IP4_CONFIG_INSTANCE_SIGNATURE;
  Instance->Controller        = ControllerHandle;
  Instance->Image             = This->DriverBindingHandle;

  CopyMem (&Instance->Ip4ConfigProtocol, &mIp4ConfigProtocolTemplate, sizeof (mIp4ConfigProtocolTemplate));
  CopyMem (&Instance->NicIp4Protocol, &mNicIp4ConfigProtocolTemplate, sizeof (mNicIp4ConfigProtocolTemplate));

  Instance->State             = IP4_CONFIG_STATE_IDLE;
  Instance->Mnp               = Mnp;
  Instance->MnpHandle         = MnpHandle;

  Instance->DoneEvent         = NULL;
  Instance->ReconfigEvent     = NULL;
  Instance->Result            = EFI_NOT_READY;
  Instance->NicConfig         = NULL;

  Instance->Dhcp4             = NULL;
  Instance->Dhcp4Handle       = NULL;
  Instance->Dhcp4Event        = NULL;

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);

  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto ON_ERROR;
  }

  Instance->NicAddr.Type    = (UINT16) SnpMode.IfType;
  Instance->NicAddr.Len     = (UINT8) SnpMode.HwAddressSize;
  CopyMem (&Instance->NicAddr.MacAddr, &SnpMode.CurrentAddress, sizeof (Instance->NicAddr.MacAddr));

  //
  // Add it to the global list, and compose the name
  //
  for (Index = 0; Index < MAX_IP4_CONFIG_IN_VARIABLE; Index++) {
    if (mIp4ConfigNicList[Index] == NULL) {
      mIp4ConfigNicList[Index]  = Instance;
      Instance->NicIndex        = Index;

      if (Instance->NicAddr.Type == NET_IFTYPE_ETHERNET) {
        Instance->NicName[0]  = 'e';
        Instance->NicName[1]  = 't';
        Instance->NicName[2]  = 'h';
        Instance->NicName[3]  = (UINT16) ('0' + Index);
        Instance->NicName[4]  = 0;
      } else {
        Instance->NicName[0]  = 'u';
        Instance->NicName[1]  = 'n';
        Instance->NicName[2]  = 'k';
        Instance->NicName[3]  = (UINT16) ('0' + Index);
        Instance->NicName[4]  = 0;
      }

      break;
    }
  }

  if (Index == MAX_IP4_CONFIG_IN_VARIABLE) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  // Install the IP4_CONFIG and NIC_IP4CONFIG protocols
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiIp4ConfigProtocolGuid,
                  &Instance->Ip4ConfigProtocol,
                  &gEfiNicIp4ConfigProtocolGuid,
                  &Instance->NicIp4Protocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    mIp4ConfigNicList[Index] = NULL;
    goto ON_ERROR;
  }

  //
  // Get the previous configure parameters. If an error happend here,
  // just ignore it because the driver should be able to operate.
  //
  Variable = Ip4ConfigReadVariable ();

  if (Variable == NULL) {
    return EFI_SUCCESS;
  }

  NicConfig = Ip4ConfigFindNicVariable (Variable, &Instance->NicAddr);

  if (NicConfig == NULL) {
    goto ON_EXIT;
  }

  //
  // Don't modify the permant static configuration
  //
  if (NicConfig->Perment && (NicConfig->Source == IP4_CONFIG_SOURCE_STATIC)) {
    goto ON_EXIT;
  }

  //
  // Delete the non-permant configuration and remove the previous
  // acquired DHCP parameters. Only doing DHCP itself is permant
  //
  NewVariable = NULL;

  if (!NicConfig->Perment) {
    NewVariable = Ip4ConfigModifyVariable (Variable, &Instance->NicAddr, NULL);

  } else if (NicConfig->Source == IP4_CONFIG_SOURCE_DHCP) {
    ZeroMem (&NicConfig->Ip4Info, sizeof (EFI_IP4_IPCONFIG_DATA));
    NewVariable = Ip4ConfigModifyVariable (Variable, &Instance->NicAddr, NicConfig);

  }

  Ip4ConfigWriteVariable (NewVariable);

  if (NewVariable != NULL) {
    gBS->FreePool (NewVariable);
  }

ON_EXIT:
  gBS->FreePool (Variable);

  if (NicConfig != NULL) {
    gBS->FreePool (NicConfig);
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Instance != NULL) {
    gBS->FreePool (Instance);
  }

  if (Mnp != NULL) {
    gBS->CloseProtocol (
          MnpHandle,
          &gEfiManagedNetworkProtocolGuid,
          This->DriverBindingHandle,
          ControllerHandle
          );
  }

  NetLibDestroyServiceChild (
    ControllerHandle,
    This->DriverBindingHandle,
    &gEfiManagedNetworkProtocolGuid,
    MnpHandle
    );

  return Status;
}


/**
  Stop this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to stop driver on
  @param  NumberOfChildren     Number of Handles in ChildHandleBuffer. If number of
                                children is zero stop the entire bus driver.
  @param  ChildHandleBuffer    List of Child Handles to Stop.

  @retval EFI_SUCCES           This driver is removed ControllerHandle
  @retval other                This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  IP4_CONFIG_INSTANCE       *Instance;
  EFI_IP4_CONFIG_PROTOCOL   *Ip4Config;
  EFI_HANDLE                NicHandle;
  EFI_STATUS                Status;

  //
  // IP4_CONFIG instance opens an MNP child. It may also create and open
  // a DHCP child. If this is the DHCP handle, stop the DHCP process. If
  // it is the MNP child, stop the whole driver.
  //
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiDhcp4ProtocolGuid);

  if (NicHandle != NULL) {
    //
    // Get our context back then clean the DHCP up. Notify the user if necessary.
    //
    Status = gBS->OpenProtocol (
                    NicHandle,
                    &gEfiIp4ConfigProtocolGuid,
                    (VOID **) &Ip4Config,
                    This->DriverBindingHandle,
                    ControllerHandle,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    Instance = IP4_CONFIG_INSTANCE_FROM_IP4CONFIG (Ip4Config);
    ASSERT (ControllerHandle == Instance->Dhcp4Handle);

    Ip4ConfigCleanDhcp4 (Instance);

    Instance->State   = IP4_CONFIG_STATE_CONFIGURED;
    Instance->Result  = EFI_DEVICE_ERROR;

    if (Instance->DoneEvent != NULL) {
      gBS->SignalEvent (Instance->DoneEvent);
    }

    return EFI_SUCCESS;
  }

  //
  // This is a MNP handle, stop the whole driver
  //
  NicHandle = NetLibGetNicHandle (ControllerHandle, &gEfiManagedNetworkProtocolGuid);

  if (NicHandle == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  NicHandle,
                  &gEfiIp4ConfigProtocolGuid,
                  (VOID **) &Ip4Config,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Instance = IP4_CONFIG_INSTANCE_FROM_IP4CONFIG (Ip4Config);

  //
  // Unload the protocols first to inform the top drivers
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  NicHandle,
                  &gEfiIp4ConfigProtocolGuid,
                  &Instance->Ip4ConfigProtocol,
                  &gEfiNicIp4ConfigProtocolGuid,
                  &Instance->NicIp4Protocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Release all the resources
  //
  if (Instance->MnpHandle != NULL) {
    gBS->CloseProtocol (
          Instance->MnpHandle,
          &gEfiManagedNetworkProtocolGuid,
          This->DriverBindingHandle,
          NicHandle
          );

    NetLibDestroyServiceChild (
      NicHandle,
      Instance->Image,
      &gEfiManagedNetworkServiceBindingProtocolGuid,
      Instance->MnpHandle
      );

    Instance->Mnp       = NULL;
    Instance->MnpHandle = NULL;
  }

  Ip4ConfigCleanConfig (Instance);
  mIp4ConfigNicList[Instance->NicIndex] = NULL;
  gBS->FreePool (Instance);

  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gIp4ConfigDriverBinding = {
  Ip4ConfigDriverBindingSupported,
  Ip4ConfigDriverBindingStart,
  Ip4ConfigDriverBindingStop,
  0xa,
  NULL,
  NULL
};
