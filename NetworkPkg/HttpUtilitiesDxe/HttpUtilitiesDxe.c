/** @file
  The DriverEntryPoint and Unload for HttpUtilities driver.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HttpUtilitiesDxe.h"


/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS 
EFIAPI
HttpUtilitiesDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                      Status;
  UINTN                           HandleNum;
  EFI_HANDLE                      *HandleBuffer;
  UINT32                          Index;
  EFI_HTTP_UTILITIES_PROTOCOL     *HttpUtilitiesProtocol;


  HandleBuffer   = NULL;

  //
  // Locate all the handles with HttpUtilities protocol.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiHttpUtilitiesProtocolGuid,
                  NULL,
                  &HandleNum,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleNum; Index++) {
    //
    // Firstly, find HttpUtilitiesProtocol interface
    //
    Status = gBS->OpenProtocol (
                    HandleBuffer[Index], 
                    &gEfiHttpUtilitiesProtocolGuid, 
                    (VOID **) &HttpUtilitiesProtocol, 
                    ImageHandle, 
                    NULL, 
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Then, uninstall HttpUtilities interface
    // 
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    HandleBuffer[Index],
                    &gEfiHttpUtilitiesProtocolGuid, HttpUtilitiesProtocol,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}


/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
HttpUtilitiesDxeDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;

  EFI_HANDLE             Handle;

  Handle = NULL;

  //
  // Install the HttpUtilities Protocol onto Handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiHttpUtilitiesProtocolGuid,
                  &mHttpUtilitiesProtocol,
                  NULL
                  );
  
  return Status;
}

