/** @file
  The driver binding for IP4 CONFIG protocol.

Copyright (c) 2006 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at<BR>
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Ip4Config.h"
#include "Ip4ConfigNv.h"
#include "NicIp4Variable.h"

EFI_DRIVER_BINDING_PROTOCOL gIp4ConfigDriverBinding = {
  Ip4ConfigDriverBindingSupported,
  Ip4ConfigDriverBindingStart,
  Ip4ConfigDriverBindingStop,
  0xa,
  NULL,
  NULL
};

//
// The intance of template of IP4 Config private data
//
IP4_CONFIG_INSTANCE        mIp4ConfigTemplate = {
  IP4_CONFIG_INSTANCE_SIGNATURE,
  NULL,
  NULL,
  (EFI_DEVICE_PATH_PROTOCOL *) NULL,
  {
    NULL,
    NULL,
    NULL
  },
  {
    NULL,
    NULL,
    NULL
  },
  NULL,
  (EFI_DEVICE_PATH_PROTOCOL *) NULL,
  NULL,
  {
    FALSE,
    FALSE,
    {
      {
        0
      }
    },
    {
      {
        0
      }
    },
    {
      {
        0
      }
    }
  },
  0,
  (EFI_MANAGED_NETWORK_PROTOCOL *) NULL,
  NULL,
  NULL,
  NULL,
  0,
  {
    0,
    0,
    {
      {
        0
      }
    }
  },
  (CHAR16 *) NULL,
  (NIC_IP4_CONFIG_INFO *) NULL,
  (EFI_DHCP4_PROTOCOL *) NULL,
  NULL,
  NULL,
  NULL,
  TRUE,
  FALSE
};

/**
  The entry point for IP4 config driver which install the driver
  binding and component name protocol on its image.

  @param  ImageHandle            The image handle of the driver.
  @param  SystemTable            The system table.

  @retval EFI_SUCCES             All the related protocols are installed on the driver.
  @retval Others                 Failed to install protocols.

**/
EFI_STATUS
EFIAPI
Ip4ConfigDriverEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  mIp4ConfigTemplate.Result = EFI_NOT_READY;

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
  NIC_IP4_CONFIG_INFO           *NicConfig;
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *ParentDevicePath;

  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &ParentDevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
  Instance = AllocateCopyPool (sizeof (IP4_CONFIG_INSTANCE), &mIp4ConfigTemplate);

  if (Instance == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  Instance->Controller        = ControllerHandle;
  Instance->Image             = This->DriverBindingHandle;
  Instance->ParentDevicePath  = ParentDevicePath;

  CopyMem (&Instance->Ip4ConfigProtocol, &mIp4ConfigProtocolTemplate, sizeof (mIp4ConfigProtocolTemplate));

  Instance->State             = IP4_CONFIG_STATE_IDLE;
  Instance->Mnp               = Mnp;
  Instance->MnpHandle         = MnpHandle;

  Status = Mnp->GetModeData (Mnp, NULL, &SnpMode);

  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    goto ON_ERROR;
  }

  Instance->NicAddr.Type    = (UINT16) SnpMode.IfType;
  Instance->NicAddr.Len     = (UINT8) SnpMode.HwAddressSize;
  CopyMem (&Instance->NicAddr.MacAddr, &SnpMode.CurrentAddress, Instance->NicAddr.Len);

  //
  // Add it to the global list, and compose the name
  //
  Status = NetLibGetMacString (Instance->Controller, Instance->Image, &Instance->MacString);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Status = Ip4ConfigDeviceInit (Instance);

  //
  // Install the IP4_CONFIG protocols
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ControllerHandle,
                  &gEfiIp4ConfigProtocolGuid,
                  &Instance->Ip4ConfigProtocol,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // A dedicated timer is used to poll underlying media status.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  MediaChangeDetect,
                  Instance,
                  &Instance->Timer
                  );

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  //
  // Get the previous configure parameters. If an error happend here,
  // just ignore it because the driver should be able to operate.
  //
  NicConfig = Ip4ConfigReadVariable (Instance);
  if (NicConfig != NULL) {
    if (!NicConfig->Permanent) {
      //
      // Delete the non-permanent configuration.
      //
      Ip4ConfigWriteVariable (Instance, NULL);
    }

    FreePool (NicConfig);
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Instance != NULL) {
    FreePool (Instance);
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
    &gEfiManagedNetworkServiceBindingProtocolGuid,
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

  Ip4ConfigDeviceUnload (Instance);

  //
  // Unload the protocols first to inform the top drivers
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  NicHandle,
                  &gEfiIp4ConfigProtocolGuid,
                  &Instance->Ip4ConfigProtocol,
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

  if (Instance->MacString != NULL) {
    FreePool (Instance->MacString);
  }

  if (Instance->Timer != NULL) {
    gBS->SetTimer (Instance->Timer, TimerCancel, 0);
    gBS->CloseEvent (Instance->Timer);
    Instance->Timer = NULL;
  }
  
  Ip4ConfigCleanConfig (Instance);
  FreePool (Instance);

  return EFI_SUCCESS;
}
