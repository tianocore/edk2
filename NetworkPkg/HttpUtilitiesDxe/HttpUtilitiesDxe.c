/** @file
  The DriverEntryPoint and Unload for HttpUtilities driver.

  Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

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
  EFI_STATUS                   Status;
  UINTN                        HandleNum;
  EFI_HANDLE                   *HandleBuffer;
  UINTN                        Index;
  EFI_HTTP_UTILITIES_PROTOCOL  *HttpUtilitiesProtocol;

  HandleBuffer = NULL;

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
                    (VOID **)&HttpUtilitiesProtocol,
                    ImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Then, uninstall HttpUtilities interface
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    HandleBuffer[Index],
                    &gEfiHttpUtilitiesProtocolGuid,
                    HttpUtilitiesProtocol,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }
  }

  Status = EFI_SUCCESS;

Exit:
  gBS->FreePool (HandleBuffer);

  return Status;
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
  EFI_STATUS  Status;

  EFI_HANDLE  Handle;

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
