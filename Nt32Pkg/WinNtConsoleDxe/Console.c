/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  Console.c

Abstract:

  Console based on Win32 APIs.

--*/

//
// The package level header files this module uses
//
#include <Uefi.h>
#include <WinNtDxe.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Protocol/SimpleTextIn.h>
#include <Protocol/WinNtIo.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include "Console.h"

EFI_STATUS
EFIAPI
WinNtConsoleDriverBindingSupported (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
WinNtConsoleDriverBindingStart (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

EFI_STATUS
EFIAPI
WinNtConsoleDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Handle,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  );

EFI_DRIVER_BINDING_PROTOCOL gWinNtConsoleDriverBinding = {
  WinNtConsoleDriverBindingSupported,
  WinNtConsoleDriverBindingStart,
  WinNtConsoleDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**
  The user Entry Point for module WinNtConsole. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeWinNtConsole(
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
             &gWinNtConsoleDriverBinding,
             ImageHandle,
             &gWinNtConsoleComponentName,
             &gWinNtConsoleComponentName2
             );
  ASSERT_EFI_ERROR (Status);


  return Status;
}


EFI_STATUS
EFIAPI
WinNtConsoleDriverBindingSupported (
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

  //
  // Make sure that the WinNt Thunk Protocol is valid
  //
  Status = EFI_UNSUPPORTED;
  if (WinNtIo->WinNtThunk->Signature == EFI_WIN_NT_THUNK_PROTOCOL_SIGNATURE) {

    //
    // Check the GUID to see if this is a handle type the driver supports
    //
    if (CompareGuid (WinNtIo->TypeGuid, &gEfiWinNtConsoleGuid)) {
      Status = EFI_SUCCESS;
    }
  }

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
WinNtConsoleDriverBindingStart (
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
  EFI_WIN_NT_IO_PROTOCOL          *WinNtIo;
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  //
  // Grab the IO abstraction we need to get any work done
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

  Private = AllocatePool (sizeof (WIN_NT_SIMPLE_TEXT_PRIVATE_DATA));
  if (Private == NULL) {
    goto Done;
  }

  ZeroMem (Private, sizeof (WIN_NT_SIMPLE_TEXT_PRIVATE_DATA));

  Private->Signature  = WIN_NT_SIMPLE_TEXT_PRIVATE_DATA_SIGNATURE;
  Private->Handle     = Handle;
  Private->WinNtIo    = WinNtIo;
  Private->WinNtThunk = WinNtIo->WinNtThunk;

  WinNtSimpleTextOutOpenWindow (Private);
  WinNtSimpleTextInAttachToWindow (Private);

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
        &gEfiWinNtIoProtocolGuid,
        This->DriverBindingHandle,
        Handle
        );
  if (Private != NULL) {

    FreeUnicodeStringTable (Private->ControllerNameTable);

    if (Private->NtOutHandle != NULL) {
      Private->WinNtThunk->CloseHandle (Private->NtOutHandle);
    }

    if (Private->SimpleTextIn.WaitForKey != NULL) {
      gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
    }

    FreePool (Private);
  }

  return Status;
}

EFI_STATUS
EFIAPI
WinNtConsoleDriverBindingStop (
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
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL    *SimpleTextOut;
  EFI_STATUS                      Status;
  WIN_NT_SIMPLE_TEXT_PRIVATE_DATA *Private;

  //
  // Kick people off our interface???
  //
  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  &SimpleTextOut,
                  This->DriverBindingHandle,
                  Handle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Private = WIN_NT_SIMPLE_TEXT_OUT_PRIVATE_DATA_FROM_THIS (SimpleTextOut);

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
                    &gEfiWinNtIoProtocolGuid,
                    This->DriverBindingHandle,
                    Handle
                    );

    Status = gBS->CloseEvent (Private->SimpleTextIn.WaitForKey);
    ASSERT_EFI_ERROR (Status);

    Private->WinNtThunk->CloseHandle (Private->NtOutHandle);
    //
    // DO NOT close Private->NtInHandle. It points to StdIn and not
    //  the Private->NtOutHandle is StdIn and should not be closed!
    //
    FreeUnicodeStringTable (Private->ControllerNameTable);

    FreePool (Private);
  }

  return Status;
}
