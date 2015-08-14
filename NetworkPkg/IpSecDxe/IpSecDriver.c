/** @file
  Driver Binding Protocol for IPsec Driver.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseCryptLib.h>

#include "IpSecConfigImpl.h"
#include "IkeService.h"
#include "IpSecDebug.h"

/**
  Test to see if this driver supports ControllerHandle. This is the worker function
  for IpSec4(6)DriverbindingSupported.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to start.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.
  
  @retval EFI_SUCCES           This driver supports this device.
  @retval EFI_ALREADY_STARTED  This driver is already running on this device.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
IpSecSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL,
  IN UINT8                        IpVersion
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *UdpServiceBindingGuid;
  
  if (IpVersion == IP_VERSION_4) {
    UdpServiceBindingGuid  = &gEfiUdp4ServiceBindingProtocolGuid;
  } else {
    UdpServiceBindingGuid  = &gEfiUdp6ServiceBindingProtocolGuid;
  }

  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  UdpServiceBindingGuid,
                  NULL,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  return EFI_SUCCESS;
}

/**
  Start this driver on ControllerHandle. This is the worker function
  for IpSec4(6)DriverbindingStart.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to start.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCES           This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval EFI_DEVICE_ERROR     The device could not be started due to a device error.
                               Currently not implemented.
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
IpSecStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL,
  IN UINT8                        IpVersion
  )
{
  EFI_IPSEC2_PROTOCOL *IpSec;
  EFI_STATUS          Status;
  IPSEC_PRIVATE_DATA  *Private;

  //
  // Ipsec protocol should be installed when load image.
  //
  Status = gBS->LocateProtocol (&gEfiIpSec2ProtocolGuid, NULL, (VOID **) &IpSec);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = IPSEC_PRIVATE_DATA_FROM_IPSEC (IpSec);

  if (IpVersion == IP_VERSION_4) {
    //
    // Try to open a udp4 io for input.
    //
    Status = gBS->OpenProtocol (
                        ControllerHandle,
                        &gEfiUdp4ServiceBindingProtocolGuid,
                        NULL,
                        This->DriverBindingHandle,
                        ControllerHandle,
                        EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                        );

    if (!EFI_ERROR (Status)) {
      Status = IkeOpenInputUdp4 (Private, ControllerHandle, This->DriverBindingHandle);
    }
  } else {
    //
    // Try to open a udp6 io for input.
    //
    Status = gBS->OpenProtocol (
                        ControllerHandle,
                        &gEfiUdp6ServiceBindingProtocolGuid,
                        NULL,
                        This->DriverBindingHandle,
                        ControllerHandle,
                        EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                        );

    if (!EFI_ERROR (Status)) {
      Status = IkeOpenInputUdp6 (Private, ControllerHandle, This->DriverBindingHandle);
    }
  }

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }
  return EFI_SUCCESS;
}

/**
  Stop this driver on ControllerHandle. This is the worker function
  for IpSec4(6)DriverbindingStop.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of a device to stop the driver on.
  @param[in]  NumberOfChildren     Number of Handles in ChildHandleBuffer. If the number of
                                   children is zero, stop the entire bus driver.
  @param[in]  ChildHandleBuffer    List of Child Handles to Stop.
  @param[in]  IpVersion            IP_VERSION_4 or IP_VERSION_6.

  @retval EFI_SUCCES           This driver removed ControllerHandle.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
IpSecStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer,
  IN UINT8                        IpVersion
  )
{
  EFI_IPSEC2_PROTOCOL *IpSec;
  EFI_STATUS          Status;
  IPSEC_PRIVATE_DATA  *Private;
  IKE_UDP_SERVICE     *UdpSrv;
  LIST_ENTRY          *Entry;
  LIST_ENTRY          *Next;
  IKEV2_SA_SESSION    *Ikev2SaSession;

  //
  // Locate ipsec protocol to get private data.
  //
  Status = gBS->LocateProtocol (&gEfiIpSec2ProtocolGuid, NULL, (VOID **) &IpSec);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Private = IPSEC_PRIVATE_DATA_FROM_IPSEC (IpSec);

  //
  // The SAs are shared by both IP4 and IP6 stack. So we skip the cleanup
  // and leave the SAs unchanged if the other IP stack is still running.
  //
  if ((IpVersion == IP_VERSION_4 && Private->Udp6Num ==0) ||
      (IpVersion == IP_VERSION_6 && Private->Udp4Num ==0)) {
    //
    // If IKEv2 SAs are under establishing, delete it directly.
    //
    if (!IsListEmpty (&Private->Ikev2SessionList)) {
      NET_LIST_FOR_EACH_SAFE (Entry, Next, &Private->Ikev2SessionList) {
        Ikev2SaSession = IKEV2_SA_SESSION_BY_SESSION (Entry);
        RemoveEntryList (&Ikev2SaSession->BySessionTable);
        Ikev2SaSessionFree (Ikev2SaSession);
      }
    }

    //
    // Delete established IKEv2 SAs.
    //
    if (!IsListEmpty (&Private->Ikev2EstablishedList)) {
      NET_LIST_FOR_EACH_SAFE (Entry, Next, &Private->Ikev2EstablishedList) {
        Ikev2SaSession = IKEV2_SA_SESSION_BY_SESSION (Entry); 
        RemoveEntryList (&Ikev2SaSession->BySessionTable);
        Ikev2SaSessionFree (Ikev2SaSession);
      }
    }
  }

  if (IpVersion == IP_VERSION_4) {
    //
    // If has udp4 io opened on the controller, close and free it.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &Private->Udp4List) {

      UdpSrv = IPSEC_UDP_SERVICE_FROM_LIST (Entry);
      //
      // Find the right udp service which installed on the appointed nic handle.
      //
      if (UdpSrv->Input != NULL && ControllerHandle == UdpSrv->Input->UdpHandle) {
        UdpIoFreeIo (UdpSrv->Input);
        UdpSrv->Input = NULL;
      }

      if (UdpSrv->Output != NULL && ControllerHandle == UdpSrv->Output->UdpHandle) {
        UdpIoFreeIo (UdpSrv->Output);
        UdpSrv->Output = NULL;
      }

      if (UdpSrv->Input == NULL && UdpSrv->Output == NULL) {
        RemoveEntryList (&UdpSrv->List);
        FreePool (UdpSrv);
        ASSERT (Private->Udp4Num > 0);
        Private->Udp4Num--;
      }
    }
  } else {
    //
    // If has udp6 io opened on the controller, close and free it.
    //
    NET_LIST_FOR_EACH_SAFE (Entry, Next, &Private->Udp6List) {

      UdpSrv = IPSEC_UDP_SERVICE_FROM_LIST (Entry);
      //
      // Find the right udp service which installed on the appointed nic handle.
      //
      if (UdpSrv->Input != NULL && ControllerHandle == UdpSrv->Input->UdpHandle) {
        UdpIoFreeIo (UdpSrv->Input);
        UdpSrv->Input = NULL;
      }

      if (UdpSrv->Output != NULL && ControllerHandle == UdpSrv->Output->UdpHandle) {
        UdpIoFreeIo (UdpSrv->Output);
        UdpSrv->Output = NULL;
      }

      if (UdpSrv->Input == NULL && UdpSrv->Output == NULL) {
        RemoveEntryList (&UdpSrv->List);
        FreePool (UdpSrv);
        ASSERT (Private->Udp6Num > 0);
        Private->Udp6Num--;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to start.

  @retval EFI_SUCCES           This driver supports this device.
  @retval EFI_ALREADY_STARTED  This driver is already running on this device.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
IpSec4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  return IpSecSupported (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_4
           );
}

/**
  Start this driver on ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to start.

  @retval EFI_SUCCES           This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval EFI_DEVICE_ERROR     The device could not be started due to a device error.
                               Currently not implemented.
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
IpSec4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return IpSecStart (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_4
           );
}

/**
  Stop this driver on ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of a device to stop the driver on.
  @param[in]  NumberOfChildren     Number of Handles in ChildHandleBuffer. If the number of
                                   children is zero, stop the entire bus driver.
  @param[in]  ChildHandleBuffer    List of Child Handles to Stop.

  @retval EFI_SUCCES           This driver removed ControllerHandle.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
IpSec4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  return IpSecStop (
           This,
           ControllerHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_4
           );
}

/**
  Test to see if this driver supports ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to test.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to start.

  @retval EFI_SUCCES           This driver supports this device.
  @retval EFI_ALREADY_STARTED  This driver is already running on this device.
  @retval other                This driver does not support this device.

**/
EFI_STATUS
EFIAPI
IpSec6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  return IpSecSupported (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_6
           );
}

/**
  Start this driver on ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of device to bind driver to.
  @param[in]  RemainingDevicePath  Optional parameter used to pick a specific child
                                   device to start.

  @retval EFI_SUCCES           This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval EFI_DEVICE_ERROR     The device could not be started due to a device error.
                               Currently not implemented.
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
IpSec6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  return IpSecStart (
           This,
           ControllerHandle,
           RemainingDevicePath,
           IP_VERSION_6
           );
}

/**
  Stop this driver on ControllerHandle.

  @param[in]  This                 Protocol instance pointer.
  @param[in]  ControllerHandle     Handle of a device to stop the driver on.
  @param[in]  NumberOfChildren     Number of Handles in ChildHandleBuffer. If the number of
                                   children is zero, stop the entire bus driver.
  @param[in]  ChildHandleBuffer    List of Child Handles to Stop.

  @retval EFI_SUCCES           This driver removed ControllerHandle.
  @retval other                This driver was not removed from this device.

**/
EFI_STATUS
EFIAPI
IpSec6DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  return IpSecStop (
           This,
           ControllerHandle,
           NumberOfChildren,
           ChildHandleBuffer,
           IP_VERSION_6
           );
}

EFI_DRIVER_BINDING_PROTOCOL gIpSec4DriverBinding = {
  IpSec4DriverBindingSupported,
  IpSec4DriverBindingStart,
  IpSec4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_DRIVER_BINDING_PROTOCOL gIpSec6DriverBinding = {
  IpSec6DriverBindingSupported,
  IpSec6DriverBindingStart,
  IpSec6DriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  This is a callback function when the mIpSecInstance.DisabledEvent is signaled.

  @param[in]  Event        Event whose notification function is being invoked.
  @param[in]  Context      Pointer to the notification function's context.

**/
VOID
EFIAPI
IpSecCleanupAllSa (
  IN  EFI_EVENT     Event,
  IN  VOID          *Context
  )
{
  IPSEC_PRIVATE_DATA  *Private;
  Private                   = (IPSEC_PRIVATE_DATA *) Context;
  Private->IsIPsecDisabling = TRUE;
  IkeDeleteAllSas (Private, TRUE);
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers, including
  both device drivers and bus drivers.

  The entry point for IPsec driver which installs the driver binding,
  component name protocol, IPsec Config protcolon, and IPsec protocol in
  its ImageHandle.

  @param[in] ImageHandle        The firmware allocated handle for the UEFI image.
  @param[in] SystemTable        A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_ALREADY_STARTED   The IPsec driver has been already loaded.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval Others                The operation is failed.

**/
EFI_STATUS
EFIAPI
IpSecDriverEntryPoint (
  IN EFI_HANDLE              ImageHandle,
  IN EFI_SYSTEM_TABLE        *SystemTable
  )
{
  EFI_STATUS          Status;
  IPSEC_PRIVATE_DATA  *Private;
  EFI_IPSEC2_PROTOCOL *IpSec;

  //
  // Check whether ipsec protocol has already been installed.
  //
  Status = gBS->LocateProtocol (&gEfiIpSec2ProtocolGuid, NULL, (VOID **) &IpSec);

  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "_ModuleEntryPoint: IpSec has been already loaded\n"));
    Status = EFI_ALREADY_STARTED;
    goto ON_EXIT;
  }

  Status = gBS->LocateProtocol (&gEfiDpcProtocolGuid, NULL, (VOID **) &mDpc);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "_ModuleEntryPoint: Failed to locate EfiDpcProtocol\n"));
    goto ON_EXIT;
  }

  Private = AllocateZeroPool (sizeof (IPSEC_PRIVATE_DATA));

  if (Private == NULL) {
    DEBUG ((DEBUG_ERROR, "_ModuleEntryPoint: Failed to allocate private data\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }
  //
  // Create disable event to cleanup all SA when ipsec disabled by user.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  IpSecCleanupAllSa,
                  Private,
                  &mIpSecInstance.DisabledEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "_ModuleEntryPoint: Failed to create disable event\n"));
    goto ON_FREE_PRIVATE;
  }

  Private->Signature    = IPSEC_PRIVATE_DATA_SIGNATURE;
  Private->ImageHandle  = ImageHandle;
  CopyMem (&Private->IpSec, &mIpSecInstance, sizeof (EFI_IPSEC2_PROTOCOL));

  //
  // Initilize Private's members. Thess members is used for IKE.
  //
  InitializeListHead (&Private->Udp4List);
  InitializeListHead (&Private->Udp6List);
  InitializeListHead (&Private->Ikev1SessionList);
  InitializeListHead (&Private->Ikev1EstablishedList);
  InitializeListHead (&Private->Ikev2SessionList);
  InitializeListHead (&Private->Ikev2EstablishedList);

  RandomSeed (NULL, 0);
  //
  // Initialize the ipsec config data and restore it from variable.
  //
  Status = IpSecConfigInitialize (Private);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "_ModuleEntryPoint: Failed to initialize IpSecConfig\n"));
    goto ON_CLOSE_EVENT;
  }
  //
  // Install ipsec protocol which is used by ip driver to process ipsec header.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiIpSec2ProtocolGuid,
                  &Private->IpSec,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_UNINSTALL_CONFIG;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIpSec4DriverBinding,
             ImageHandle,
             &gIpSecComponentName,
             &gIpSecComponentName2
             );
  if (EFI_ERROR (Status)) {
    goto ON_UNINSTALL_IPSEC;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIpSec6DriverBinding,
             NULL,
             &gIpSecComponentName,
             &gIpSecComponentName2
             );
  if (EFI_ERROR (Status)) {
    goto ON_UNINSTALL_IPSEC4_DB;
  }

  return Status;

ON_UNINSTALL_IPSEC4_DB:
  gBS->UninstallMultipleProtocolInterfaces (
         ImageHandle,
         &gEfiDriverBindingProtocolGuid,
         &gIpSec4DriverBinding,
         &gEfiComponentName2ProtocolGuid,
         &gIpSecComponentName2,
         &gEfiComponentNameProtocolGuid,
         &gIpSecComponentName,
         NULL
         );

ON_UNINSTALL_IPSEC:
  gBS->UninstallProtocolInterface (
         Private->Handle,
         &gEfiIpSec2ProtocolGuid,
         &Private->IpSec
         );
ON_UNINSTALL_CONFIG:
  gBS->UninstallProtocolInterface (
        Private->Handle,
        &gEfiIpSecConfigProtocolGuid,
        &Private->IpSecConfig
        );
ON_CLOSE_EVENT:
  gBS->CloseEvent (mIpSecInstance.DisabledEvent);
  mIpSecInstance.DisabledEvent = NULL;
ON_FREE_PRIVATE:
  FreePool (Private);
ON_EXIT:
  return Status;
}

