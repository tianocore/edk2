/** @file

Copyright (c) 2006 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  WinNtGopDriver.c

Abstract:

  This file implements the UEFI Device Driver model requirements for GOP

  GOP is short hand for Graphics Output Protocol.


**/
#include "WinNtGop.h"

EFI_STATUS
FreeNotifyList (
  IN OUT LIST_ENTRY           *ListHead
  )
/*++

Routine Description:

Arguments:

  ListHead   - The list head

Returns:

  EFI_SUCCESS           - Free the notify list successfully
  EFI_INVALID_PARAMETER - ListHead is invalid.

--*/
{
  WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY *NotifyNode;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  while (!IsListEmpty (ListHead)) {
    NotifyNode = CR (
                   ListHead->ForwardLink, 
                   WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                   NotifyEntry, 
                   WIN_NT_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                   );
    RemoveEntryList (ListHead->ForwardLink);
    gBS->FreePool (NotifyNode);
  }
  
  return EFI_SUCCESS;
}

EFI_DRIVER_BINDING_PROTOCOL gWinNtGopDriverBinding = {
  WinNtGopDriverBindingSupported,
  WinNtGopDriverBindingStart,
  WinNtGopDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The user Entry Point for module WinNtGop. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeWinNtGop(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gWinNtGopDriverBinding,
             ImageHandle,
             &gWinNtGopComponentName,
             &gWinNtGopComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

/**


  @return None

**/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
EFI_STATUS
EFIAPI
WinNtGopDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_STATUS              Status;
  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo;

  //
  // Open the IO Abstraction(s) needed to perform the supported test
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = WinNtGopSupported (WinNtIo);

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


/**


  @return None

**/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    RemainingDevicePath - add argument and description to function comment
// TODO:    EFI_UNSUPPORTED - add return value to function comment
EFI_STATUS
EFIAPI
WinNtGopDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL        *RemainingDevicePath
  )
{
  EFI_WIN_NT_IO_PROTOCOL  *WinNtIo;
  EFI_STATUS              Status;
  GOP_PRIVATE_DATA        *Private;

  //
  // Grab the protocols we need
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiWinNtIoProtocolGuid,
                  (VOID **) &WinNtIo,
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
  Private = AllocatePool (sizeof (GOP_PRIVATE_DATA));
  if (Private == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  //
  // Set up context record
  //
  Private->Signature            = GOP_PRIVATE_DATA_SIGNATURE;
  Private->Handle               = Handle;
  Private->WinNtThunk           = WinNtIo->WinNtThunk;

  Private->ControllerNameTable  = NULL;

  AddUnicodeString2 (
    "eng",
    gWinNtGopComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString,
    TRUE
    );
  AddUnicodeString2 (
    "en",
    gWinNtGopComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    WinNtIo->EnvString,
    FALSE
    );


  Private->WindowName = WinNtIo->EnvString;

  Status              = WinNtGopConstructor (Private);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Publish the Gop interface to the world
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Private->GraphicsOutput,
                  &gEfiSimpleTextInProtocolGuid,
                  &Private->SimpleTextIn,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Private->SimpleTextInEx,
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

      if (Private->SimpleTextIn.WaitForKey != NULL) {
        gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
      }
      if (Private->SimpleTextInEx.WaitForKeyEx != NULL) {
        gBS->CloseEvent (Private->SimpleTextInEx.WaitForKeyEx);
      }
      FreeNotifyList (&Private->NotifyList);
      FreePool (Private);
    }
  }

  return Status;
}


/**


  @return None

**/
// TODO:    This - add argument and description to function comment
// TODO:    Handle - add argument and description to function comment
// TODO:    NumberOfChildren - add argument and description to function comment
// TODO:    ChildHandleBuffer - add argument and description to function comment
// TODO:    EFI_NOT_STARTED - add return value to function comment
// TODO:    EFI_DEVICE_ERROR - add return value to function comment
EFI_STATUS
EFIAPI
WinNtGopDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EFI_STATUS                   Status;
  GOP_PRIVATE_DATA             *Private;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    //
    // If the GOP interface does not exist the driver is not started
    //
    return EFI_NOT_STARTED;
  }

  //
  // Get our private context information
  //
  Private = GOP_PRIVATE_DATA_FROM_THIS (GraphicsOutput);

  //
  // Remove the SGO interface from the system
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  &Private->GraphicsOutput,
                  &gEfiSimpleTextInProtocolGuid,
                  &Private->SimpleTextIn,
                  &gEfiSimpleTextInputExProtocolGuid,
                  &Private->SimpleTextInEx,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Shutdown the hardware
    //
    Status = WinNtGopDestructor (Private);
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
    Status = gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
    ASSERT_EFI_ERROR (Status);
    Status = gBS->CloseEvent (Private->SimpleTextInEx.WaitForKeyEx);
    ASSERT_EFI_ERROR (Status);
    FreeNotifyList (&Private->NotifyList);

    gBS->FreePool (Private);

  }

  return Status;
}

