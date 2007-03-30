/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtUgaDriver.c

Abstract:

  This file implements the EFI 1.1 Device Driver model requirements for UGA

  UGA is short hand for Universal Graphics Abstraction protocol.

  This file is a verision of UgaIo the uses WinNtThunk system calls as an IO
  abstraction. For a PCI device WinNtIo would be replaced with
  a PCI IO abstraction that abstracted a specific PCI device.

--*/

#include "WinNtUga.h"

EFI_DRIVER_BINDING_PROTOCOL gWinNtUgaDriverBinding = {
  WinNtUgaDriverBindingSupported,
  WinNtUgaDriverBindingStart,
  WinNtUgaDriverBindingStop,
  0xa,
  NULL,
  NULL
};


EFI_STATUS
EFIAPI
WinNtUgaDriverBindingSupported (
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
  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  &WinNtIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WinNtUgaSupported (WinNtIo);

  //
  // Close the I/O Abstraction(s) used to perform the supported test
  //
  gBS->CloseProtocol (
        Handle,
        &gEfiWinNtIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );

  return Status;
}

EFI_STATUS
EFIAPI
WinNtUgaDriverBindingStart (
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
  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo;
  EFI_STATUS              Status;
  UGA_PRIVATE_DATA        *Private;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  &WinNtIo,
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
  Private = AllocatePool (sizeof (UGA_PRIVATE_DATA));
  if (Private == NULL) {
    goto Done;
  }
  //
  // Set up context record
  //
  Private->Signature            = UGA_PRIVATE_DATA_SIGNATURE;
  Private->Handle               = Handle;
  Private->WinNtThunk           = WinNtIo->WinNtThunk;

  Private->ControllerNameTable  = NULL;

  AddUnicodeString (
    "eng",
    gWinNtUgaComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString
    );

  Private->WindowName = WinNtIo->EnvString;

  Status              = WinNtUgaConstructor (Private);
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
          &gEfiWinNtIoProtocolGuid,
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

      FreePool (Private);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
WinNtUgaDriverBindingStop (
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
                  &UgaDraw,
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
    Status = WinNtUgaDestructor (Private);
    if (EFI_ERROR (Status)) {
      return EFI_DEVICE_ERROR;
    }

    gBS->CloseProtocol (
          Handle,
          &gEfiWinNtIoProtocolGuid,
          This->DriverBindingHandle,
          Handle
          );

    //
    // Free our instance data
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    FreePool (Private);

  }

  return Status;
}

UINTN
Atoi (
  CHAR16  *String
  )
/*++

Routine Description:

  Convert a unicode string to a UINTN

Arguments:

  String - Unicode string.

Returns:

  UINTN of the number represented by String.

--*/
{
  UINTN   Number;
  CHAR16  *Str;

  //
  // skip preceeding white space
  //
  Str = String;
  while ((*Str) && (*Str == ' ' || *Str == '"')) {
    Str++;
  }

  //
  // Convert ot a Number
  //
  Number = 0;
  while (*Str != '\0') {
    if ((*Str >= '0') && (*Str <= '9')) {
      Number = (Number * 10) +*Str - '0';
    } else {
      break;
    }

    Str++;
  }

  return Number;
}
