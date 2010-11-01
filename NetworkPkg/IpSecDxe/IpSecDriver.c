/** @file
  Driver Binding Protocol for IPsec Driver.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/UdpIoLib.h>
#include "IpSecConfigImpl.h"
#include "IpSecDebug.h"

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
IpSecDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath  OPTIONAL
  )
{
  //
  //TODO: Add Udp4Protocol and Udp6Protocol testing.
  //
  return EFI_UNSUPPORTED;
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
IpSecDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  )
{
  //
  //TODO: Add Udp4Io and Udp6Io creation for the IKE.
  //
  return EFI_SUCCESS;
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
IpSecDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  )
{
  //
  //TODO: Add UdpIo4 and UdpIo6 destruction when the Udp driver unload or stop.
  //
  return EFI_UNSUPPORTED;
}

EFI_DRIVER_BINDING_PROTOCOL gIpSecDriverBinding = {
  IpSecDriverBindingSupported,
  IpSecDriverBindingStart,
  IpSecDriverBindingStop,
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
  UINT8               Value;
  EFI_STATUS          Status;

  Private = (IPSEC_PRIVATE_DATA *) Context;

  //
  // Set the Status Variable
  //
  Value  = IPSEC_STATUS_DISABLED;
  Status = gRT->SetVariable (
                  IPSECCONFIG_STATUS_NAME,
                  &gEfiIpSecConfigProtocolGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                  sizeof (Value),
                  &Value
                  );
  if (!EFI_ERROR (Status)) {
    Private->IpSec.DisabledFlag = TRUE;
  }

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
  EFI_IPSEC_PROTOCOL  *IpSec;

  //
  // Check whether ipsec protocol has already been installed.
  //
  Status = gBS->LocateProtocol (&gEfiIpSecProtocolGuid, NULL, (VOID **) &IpSec);

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
  // Create disable event to cleanup all sa when ipsec disabled by user.
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
  CopyMem (&Private->IpSec, &mIpSecInstance, sizeof (EFI_IPSEC_PROTOCOL));

  //
  // Initilize Private's members. Thess members is used for IKE.
  //
  InitializeListHead (&Private->Udp4List);
  InitializeListHead (&Private->Udp6List);
  InitializeListHead (&Private->Ikev1SessionList);
  InitializeListHead (&Private->Ikev1EstablishedList);
  InitializeListHead (&Private->Ikev2SessionList);
  InitializeListHead (&Private->Ikev2EstablishedList);

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
                  &gEfiIpSecProtocolGuid,
                  &Private->IpSec,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ON_UNINSTALL_CONFIG;
  }

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gIpSecDriverBinding,
             ImageHandle,
             &gIpSecComponentName,
             &gIpSecComponentName2
             );
  if (EFI_ERROR (Status)) {
    goto ON_UNINSTALL_CONFIG;
  }

  return Status;

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

