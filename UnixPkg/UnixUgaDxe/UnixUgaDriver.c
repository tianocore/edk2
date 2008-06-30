/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixUgaDriver.c

Abstract:

  This file implements the EFI 1.1 Device Driver model requirements for UGA

  UGA is short hand for Universal Graphics Abstraction protocol.

  This file is a verision of UgaIo the uses UnixThunk system calls as an IO 
  abstraction. For a PCI device UnixIo would be replaced with
  a PCI IO abstraction that abstracted a specific PCI device. 

--*/

#include "UnixUga.h"

EFI_DRIVER_BINDING_PROTOCOL gUnixUgaDriverBinding = {
  UnixUgaDriverBindingSupported,
  UnixUgaDriverBindingStart,
  UnixUgaDriverBindingStop,
  0xa,
  NULL,
  NULL
};


EFI_STATUS
EFIAPI
UnixUgaDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
{
  EFI_STATUS              Status;
  EFI_UNIX_IO_PROTOCOL  *UnixIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID **)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = UnixUgaSupported (UnixIo);

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiUnixIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  return Status;
}

EFI_STATUS
EFIAPI
UnixUgaDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
{
  EFI_UNIX_IO_PROTOCOL  *UnixIo;
  EFI_STATUS              Status;
  UGA_PRIVATE_DATA        *Private;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (VOID **)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Allocate Private context data for SGO inteface.
  //
  Private = NULL;
  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (UGA_PRIVATE_DATA),
                  (VOID **)&Private
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Set up context record
  //
  Private->Signature            = UGA_PRIVATE_DATA_SIGNATURE;
  Private->Handle               = Handle;
  Private->UnixThunk           = UnixIo->UnixThunk;

  Private->ControllerNameTable  = NULL;

  AddUnicodeString (
    "eng",
    gUnixUgaComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    UnixIo->EnvString
    );

  Private->WindowName = UnixIo->EnvString;

  Status              = UnixUgaConstructor (Private);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Publish the Uga interface to the world
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiUgaDrawProtocolGuid,
                  &Private->UgaDraw,
                  &gEfiSimpleTextInProtocolGuid,
                  &Private->SimpleTextIn,
                  NULL
                  );

Done:
  if (EFI_ERROR (Status)) {

    gBS->CloseProtocol (
          Handle,
          &gEfiUnixIoProtocolGuid,
          This->DriverBindingHandle,
          Handle
          );

    if (Private != NULL) {
      //
      // On Error Free back private data
      //
      if (Private->ControllerNameTable != NULL) {
        FreeUnicodeStringTable (Private->ControllerNameTable);
      }

      gBS->FreePool (Private);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixUgaDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

Arguments:

Returns:

  None

--*/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    NumberOfChildren - add argument and description to function comment
// TODO:    ChildHandleBuffer - add argument and description to function comment
// TODO:    EFI_NOT_STARTED - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
{
  EFI_UGA_DRAW_PROTOCOL *UgaDraw;
  EFI_STATUS            Status;
  UGA_PRIVATE_DATA      *Private;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUgaDrawProtocolGuid,
                  (VOID **)&UgaDraw,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // If the UGA interface does not exist the driver is not started
    //
    return EFI_NOT_STARTED;
  }

  //
  // Get our private context information
  //
  Private = UGA_DRAW_PRIVATE_DATA_FROM_THIS (UgaDraw);

  //
  // Remove the SGO interface from the system
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->Handle,
                  &gEfiUgaDrawProtocolGuid,
                  &Private->UgaDraw,
                  &gEfiSimpleTextInProtocolGuid,
                  &Private->SimpleTextIn,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Shutdown the hardware
    //
    Status = UnixUgaDestructor (Private);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    gBS->CloseProtocol (
          Handle,
          &gEfiUnixIoProtocolGuid,
          This->DriverBindingHandle,
          Handle
          );

    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    gBS->FreePool (Private);

  }

  return Status;
}

