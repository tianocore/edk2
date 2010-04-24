/** @file
  Top level C file for debug support driver.  Contains initialization function.

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PlDebugSupport.h"

EFI_DEBUG_SUPPORT_PROTOCOL  mDebugSupportProtocolInterface = {
  EFI_ISA,
  GetMaximumProcessorIndex,
  RegisterPeriodicCallback,
  RegisterExceptionCallback,
  InvalidateInstructionCache
};


/**
  Debug Support Driver entry point. 

  Checks to see if there's not already a Debug Support protocol installed for 
  the selected processor before installing it.

  @param[in] ImageHandle       The firmware allocated handle for the EFI image.  
  @param[in] SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  Debug Support protocol is installed already.
  @retval other                Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeDebugSupportDriver (
  IN EFI_HANDLE               ImageHandle,
  IN EFI_SYSTEM_TABLE         *SystemTable
  )
{
  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImageProtocolPtr;
  EFI_STATUS                  Status;
  EFI_HANDLE                  Handle;
  EFI_HANDLE                  *HandlePtr;
  UINTN                       NumHandles;
  EFI_DEBUG_SUPPORT_PROTOCOL  *DebugSupportProtocolPtr;

  //
  // First check to see that the debug support protocol for this processor
  // type is not already installed
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiDebugSupportProtocolGuid,
                  NULL,
                  &NumHandles,
                  &HandlePtr
                  );

  if (Status != EFI_NOT_FOUND) {
    do {
      NumHandles--;
      Status = gBS->OpenProtocol (
                      HandlePtr[NumHandles],
                      &gEfiDebugSupportProtocolGuid,
                      (VOID **) &DebugSupportProtocolPtr,
                      ImageHandle,
                      NULL,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if ((Status == EFI_SUCCESS) && (DebugSupportProtocolPtr->Isa == EFI_ISA)) {
        //
        // a Debug Support protocol has been installed for this processor
        //
        FreePool (HandlePtr);
        Status = EFI_ALREADY_STARTED;
        goto ErrExit;
      }
    } while (NumHandles > 0);
    FreePool (HandlePtr);
  }

  //
  // Get our image information and install platform specific unload handler
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImageProtocolPtr,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT (!EFI_ERROR (Status));
  if (Status != EFI_SUCCESS) {
    goto ErrExit;
  }

  LoadedImageProtocolPtr->Unload = PlUnloadDebugSupportDriver;

  //
  // Call hook for processor specific initialization 
  //
  Status = PlInitializeDebugSupportDriver ();
  ASSERT (!EFI_ERROR (Status));
  if (Status != EFI_SUCCESS) {
    goto ErrExit;
  }

  //
  // Install Debug Support protocol to new handle
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDebugSupportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mDebugSupportProtocolInterface
                  );
  ASSERT (!EFI_ERROR (Status));
  if (Status != EFI_SUCCESS) {
    goto ErrExit;
  }

ErrExit:
  return Status;
}
