/** @file
  Top level C file for debug support driver.  Contains initialization function.

Copyright (c) 2006 - 2008, Intel Corporation. <BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

//
// private header files
//
#include "PlDebugSupport.h"

//
// This is a global that is the actual interface
//
EFI_DEBUG_SUPPORT_PROTOCOL  gDebugSupportProtocolInterface = {
  EFI_ISA,
  GetMaximumProcessorIndex,
  RegisterPeriodicCallback,
  RegisterExceptionCallback,
  InvalidateInstructionCache
};


/**
  Debug Port Driver entry point. 

  Checks to see there's not already a DebugSupport protocol installed for 
  the selected processor before installing protocol.

  @param[in] ImageHandle       The firmware allocated handle for the EFI image.  
  @param[in] SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS          The entry point is executed successfully.
  @retval EFI_ALREADY_STARTED  DebugSupport protocol is installed already.
  @retval other                Some error occurs when executing this entry point.

**/
EFI_STATUS
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
  //  Install Protocol Interface...
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
      if (Status == EFI_SUCCESS && DebugSupportProtocolPtr->Isa == EFI_ISA) {
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
  // Call hook for platform specific initialization
  //
  Status = PlInitializeDebugSupportDriver ();
  ASSERT (!EFI_ERROR (Status));
  if (Status != EFI_SUCCESS) {
    goto ErrExit;
  }

  //
  // Install DebugSupport protocol to new handle
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDebugSupportProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gDebugSupportProtocolInterface
                  );
  ASSERT (!EFI_ERROR (Status));
  if (Status != EFI_SUCCESS) {
    goto ErrExit;
  }

ErrExit:
  return Status;
}
