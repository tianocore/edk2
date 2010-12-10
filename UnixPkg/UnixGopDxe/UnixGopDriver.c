/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2010, Apple, Inc. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  UnixGopDriver.c

Abstract:

  This file implements the EFI 1.1 Device Driver model requirements for UGA

  UGA is short hand for Universal Graphics Abstraction protocol.

  This file is a verision of UgaIo the uses UnixThunk system calls as an IO 
  abstraction. For a PCI device UnixIo would be replaced with
  a PCI IO abstraction that abstracted a specific PCI device. 

--*/

#include "UnixGop.h"


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
  UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY *NotifyNode;

  if (ListHead == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  while (!IsListEmpty (ListHead)) {
    NotifyNode = CR (
                   ListHead->ForwardLink, 
                   UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY, 
                   NotifyEntry, 
                   UNIX_GOP_SIMPLE_TEXTIN_EX_NOTIFY_SIGNATURE
                   );
    RemoveEntryList (ListHead->ForwardLink);
    gBS->FreePool (NotifyNode);
  }
  
  return EFI_SUCCESS;
}


EFI_STATUS
EFIAPI
UnixGopDriverBindingSupported (
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

  Status = UnixGopSupported (UnixIo);

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
UnixGopDriverBindingStart (
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
  GOP_PRIVATE_DATA        *Private;

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
                  sizeof (GOP_PRIVATE_DATA),
                  (VOID **)&Private
                  );
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Set up context record
  //
  Private->Signature           = GOP_PRIVATE_DATA_SIGNATURE;
  Private->Handle              = Handle;
  Private->UnixThunk           = UnixIo->UnixThunk;

  Private->ControllerNameTable  = NULL;

  AddUnicodeString (
    "eng",
    gUnixGopComponentName.SupportedLanguages,
    &Private->ControllerNameTable,
    UnixIo->EnvString
    );
  AddUnicodeString2 (
    "en",
    gUnixGopComponentName2.SupportedLanguages,
    &Private->ControllerNameTable,
    UnixIo->EnvString,
    FALSE
    );

  Private->WindowName = UnixIo->EnvString;

  Status              = UnixGopConstructor (Private);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Publish the Gop interface to the world
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Private->Handle,
                  &gEfiGraphicsOutputProtocolGuid,    &Private->GraphicsOutput,
                  &gEfiSimpleTextInProtocolGuid,      &Private->SimpleTextIn,
                  &gEfiSimplePointerProtocolGuid,     &Private->SimplePointer,
//                  &gEfiSimpleTextInputExProtocolGuid, &Private->SimpleTextInEx,
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
      if (Private->SimpleTextIn.WaitForKey != NULL) {
        gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
      }
      if (Private->SimpleTextInEx.WaitForKeyEx != NULL) {
        gBS->CloseEvent (Private->SimpleTextInEx.WaitForKeyEx);
      }
      FreeNotifyList (&Private->NotifyList);

      gBS->FreePool (Private);
    }
  }

  return Status;
}

EFI_STATUS
EFIAPI
UnixGopDriverBindingStop (
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
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GraphicsOutput;
  EFI_STATUS                   Status;
  GOP_PRIVATE_DATA             *Private;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **)&GraphicsOutput,
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
                  &gEfiGraphicsOutputProtocolGuid,    &Private->GraphicsOutput,
                  &gEfiSimpleTextInProtocolGuid,      &Private->SimpleTextIn,
                  &gEfiSimplePointerProtocolGuid,     &Private->SimplePointer,
//                  &gEfiSimpleTextInputExProtocolGuid, &Private->SimpleTextInEx,
                  NULL
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Shutdown the hardware
    //
    Status = UnixGopDestructor (Private);
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
    
    Status = gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
    ASSERT_EFI_ERROR (Status);
    
    Status = gBS->CloseEvent (Private->SimpleTextInEx.WaitForKeyEx);
    ASSERT_EFI_ERROR (Status);
    
    FreeNotifyList (&Private->NotifyList);

    gBS->FreePool (Private);

  }

  return Status;
}




EFI_DRIVER_BINDING_PROTOCOL gUnixGopDriverBinding = {
  UnixGopDriverBindingSupported,
  UnixGopDriverBindingStart,
  UnixGopDriverBindingStop,
  0xa,
  NULL,
  NULL
};



/**
  The user Entry Point for module UnixGop. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUnixGop (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUnixGopDriverBinding,
             ImageHandle,
             &gUnixGopComponentName,
             &gUnixGopComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}

