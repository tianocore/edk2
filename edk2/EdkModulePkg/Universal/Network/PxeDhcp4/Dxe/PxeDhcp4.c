/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PxeDhcp4.c
  
Abstract:

--*/


#include "PxeDhcp4.h"

//
// PXE DHCP Protocol Interface
//
EFI_DRIVER_BINDING_PROTOCOL gPxeDhcp4DriverBinding = {
  PxeDhcp4DriverBindingSupported,
  PxeDhcp4DriverBindingStart,
  PxeDhcp4DriverBindingStop,
  0xa,
  NULL,
  NULL
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
/*++

  Routine Description:
    Test to see if this driver supports ControllerHandle.  Any
    ControllerHandle that contains a PxeBaseCode protocol can be
    supported.

  Arguments:
    This                - Protocol instance pointer.
    ControllerHandle    - Handle of device to test.
    RemainingDevicePath - Not used.

  Returns:
    EFI_SUCCESS         - This driver supports this device.
    EFI_ALREADY_STARTED - This driver is already running on this
          device.
    other               - This driver does not support this device.

--*/
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;

  //
  // Open the IO Abstraction(s) needed to perform the supported test.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  (VOID **) &PxeBc,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close the I/O Abstraction(s) used to perform the supported test.
  //
  return gBS->CloseProtocol (
                ControllerHandle,
                &gEfiPxeBaseCodeProtocolGuid,
                This->DriverBindingHandle,
                ControllerHandle
                );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  * This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * RemainingDevicePath OPTIONAL
  )
/*++

  Routine Description:
    Start this driver on ControllerHandle by opening a PxeBaseCode
    protocol and installing a PxeDhcp4 protocol on ControllerHandle.

  Arguments:
    This                - Protocol instance pointer.
    ControllerHandle    - Handle of device to bind driver to.
    RemainingDevicePath - Not used, always produce all possible
          children.

  Returns:
    EFI_SUCCESS         - This driver is added to ControllerHandle.
    EFI_ALREADY_STARTED - This driver is already running on
          ControllerHandle.
    other               - This driver does not support this device.

--*/
{
  EFI_STATUS                  Status;
  EFI_PXE_BASE_CODE_PROTOCOL  *PxeBc;
  EFI_SIMPLE_NETWORK_PROTOCOL *Snp;
  PXE_DHCP4_PRIVATE_DATA      *Private;

  //
  // Connect to the PxeBaseCode interface on ControllerHandle.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  (VOID **) &PxeBc,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // BaseCode has already grabbed the SimpleNetwork interface
  // so just do a HandleProtocol() to get it.
  //
  Status = gBS->HandleProtocol (
                  ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **) &Snp
                  );

  if (EFI_ERROR (Status)) {
    goto error_exit;
  }

  ASSERT (Snp);

  //
  // Initialize the PXE DHCP device instance.
  //
  Private = AllocateZeroPool (sizeof (PXE_DHCP4_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto error_exit;
  }

  Private->Signature          = PXE_DHCP4_PRIVATE_DATA_SIGNATURE;
  Private->PxeBc              = PxeBc;
  Private->Snp                = Snp;
  Private->Handle             = ControllerHandle;
  Private->PxeDhcp4.Revision  = EFI_PXE_DHCP4_PROTOCOL_REVISION;
  Private->PxeDhcp4.Run       = PxeDhcp4Run;
  Private->PxeDhcp4.Setup     = PxeDhcp4Setup;
  Private->PxeDhcp4.Init      = PxeDhcp4Init;
  Private->PxeDhcp4.Select    = PxeDhcp4Select;
  Private->PxeDhcp4.Renew     = PxeDhcp4Renew;
  Private->PxeDhcp4.Rebind    = PxeDhcp4Rebind;
  Private->PxeDhcp4.Release   = PxeDhcp4Release;
  Private->PxeDhcp4.Data      = NULL;

  //
  // Install protocol interfaces for the PXE DHCP device.
  //
  Status = gBS->InstallProtocolInterface (
                  &ControllerHandle,
                  &gEfiPxeDhcp4ProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &Private->PxeDhcp4
                  );

  if (!EFI_ERROR (Status)) {
    return Status;
  }

error_exit: ;
  gBS->CloseProtocol (
        ControllerHandle,
        &gEfiPxeBaseCodeProtocolGuid,
        This->DriverBindingHandle,
        ControllerHandle
        );

  return Status;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
EFI_STATUS
EFIAPI
PxeDhcp4DriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  )
/*++

  Routine Description:
    Stop this driver on ControllerHandle by removing PXE DHCP
    protocol and closing the PXE Base Code protocol on
    ControllerHandle.

  Arguments:
    This              - Protocol instance pointer.
    ControllerHandle  - Handle of device to stop driver on.
    NumberOfChildren  - Not used.
    ChildHandleBuffer - Not used.

  Returns:
    EFI_SUCCESS         - This driver is removed ControllerHandle.
    other               - This driver was not removed from this
          device.

--*/
{
  EFI_STATUS              Status;
  EFI_PXE_DHCP4_PROTOCOL  *PxeDhcp4;
  PXE_DHCP4_PRIVATE_DATA  *Private;

  //
  // Get our context back.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiPxeDhcp4ProtocolGuid,
                  (VOID **) &PxeDhcp4,
                  This->DriverBindingHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = PXE_DHCP4_PRIVATE_DATA_FROM_THIS (PxeDhcp4);

  //
  // Release allocated resources
  //
  if (Private->PxeDhcp4.Data) {
    FreePool (Private->PxeDhcp4.Data);
    Private->PxeDhcp4.Data = NULL;
  }
  //
  // Uninstall our protocol
  //
  Status = gBS->UninstallProtocolInterface (
                  ControllerHandle,
                  &gEfiPxeDhcp4ProtocolGuid,
                  &Private->PxeDhcp4
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Close any consumed protocols
  //
  Status = gBS->CloseProtocol (
                  ControllerHandle,
                  &gEfiPxeBaseCodeProtocolGuid,
                  This->DriverBindingHandle,
                  ControllerHandle
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  //
  // Release our private data
  //
  FreePool (Private);

  return Status;
}

/* EOF - PxeDhcp4.c */
