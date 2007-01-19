/*++

Copyright (c) 2004 - 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Console.c

Abstract:

  Console based on Posix APIs. 

--*/

#include "Console.h"

EFI_STATUS
EFIAPI
UnixConsoleDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UnixConsoleDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
UnixConsoleDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

EFI_DRIVER_BINDING_PROTOCOL gUnixConsoleDriverBinding = {
  UnixConsoleDriverBindingSupported,
  UnixConsoleDriverBindingStart,
  UnixConsoleDriverBindingStop,
  0xa,
  NULL,
  NULL
};

EFI_STATUS
EFIAPI
UnixConsoleDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN  EFI_HANDLE                    Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
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
                  (void *)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Make sure that the Unix Thunk Protocol is valid
  //
  Status = EFI_UNSUPPORTED;
  if (UnixIo->UnixThunk->Signature == EFI_UNIX_THUNK_PROTOCOL_SIGNATURE) {

    //
    // Check the GUID to see if this is a handle type the driver supports
    //
    if (CompareGuid (UnixIo->TypeGuid, &gEfiUnixConsoleGuid)) {
      Status = EFI_SUCCESS;
    }
  }

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
UnixConsoleDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
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
  EFI_STATUS                      Status;
  EFI_UNIX_IO_PROTOCOL          *UnixIo;
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  //
  // Grab the IO abstraction we need to get any work done
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiUnixIoProtocolGuid,
                  (void *)&UnixIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  sizeof (UNIX_SIMPLE_TEXT_PRIVATE_DATA),
                  (void *)&Private
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  ZeroMem (Private, sizeof (UNIX_SIMPLE_TEXT_PRIVATE_DATA));

  Private->Signature  = UNIX_SIMPLE_TEXT_PRIVATE_DATA_SIGNATURE;
  Private->Handle     = Handle;
  Private->UnixIo    = UnixIo;
  Private->UnixThunk = UnixIo->UnixThunk;

  UnixSimpleTextOutOpenWindow (Private);
  UnixSimpleTextInAttachToWindow (Private);

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  &Private->SimpleTextOut,
                  &gEfiSimpleTextInProtocolGuid,
                  &Private->SimpleTextIn,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    return Status;
  }

Done:
  gBS->CloseProtocol (
        Handle,
        &gEfiUnixIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );
  if (Private != NULL) {

    FreeUnicodeStringTable (Private->ControllerNameTable);

#if 0
    if (Private->NtOutHandle != NULL) {
      Private->UnixThunk->CloseHandle (Private->NtOutHandle);
    }
#endif

    if (Private->SimpleTextIn.WaitForKey != NULL) {
      gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
    }

    gBS->FreePool (Private);
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixConsoleDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  This              - TODO: add argument description
  Handle            - TODO: add argument description
  NumberOfChildren  - TODO: add argument description
  ChildHandleBuffer - TODO: add argument description

Returns:

  EFI_UNSUPPORTED - TODO: Add description for return value

--*/
{
  EFI_SIMPLE_TEXT_OUT_PROTOCOL    *SimpleTextOut;
  EFI_STATUS                      Status;
  UNIX_SIMPLE_TEXT_PRIVATE_DATA *Private;

  //
  // Kick people off our interface???
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (void *)&SimpleTextOut,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = UNIX_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (SimpleTextOut);

  ASSERT (Private->Handle == Handle);

  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  &Private->SimpleTextOut,
                  &gEfiSimpleTextInProtocolGuid,
                  &Private->SimpleTextIn,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {

    //
    // Shut down our device
    //
    Status = gBS->CloseProtocol (
                    Handle,
                    &gEfiUnixIoProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );

    Status = gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
    ASSERT_EFI_ERROR (Status);

#if 0
    Private->UnixThunk->CloseHandle (Private->NtOutHandle);
#endif
    //
    // DO NOT close Private->NtInHandle. It points to StdIn and not
    //  the Private->NtOutHandle is StdIn and should not be closed!
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    gBS->FreePool (Private);
  }

  return Status;
}
