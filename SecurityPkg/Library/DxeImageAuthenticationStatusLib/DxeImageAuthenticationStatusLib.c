/** @file
  Implement image authentication status check in UEFI2.3.1.

Copyright (c) 2012 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/SecurityManagementLib.h>


/**
  Check image authentication status returned from Section Extraction Protocol

  @param[in]    AuthenticationStatus  This is the authentication status returned from
                             the Section Extraction Protocol when reading the input file.
  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]    FileBuffer File buffer matches the input file device path.
  @param[in]    FileSize   Size of File buffer matches the input file device path.
  @param[in]    BootPolicy A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS            The input file specified by File did authenticate, and the
                                 platform policy dictates that the DXE Core may use File.
  @retval EFI_ACCESS_DENIED      The file specified by File and FileBuffer did not
                                 authenticate, and the platform policy dictates that the DXE
                                 Foundation many not use File.

**/
EFI_STATUS
EFIAPI
DxeImageAuthenticationStatusHandler (
  IN  UINT32                           AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize,
  IN  BOOLEAN                          BootPolicy
  )
{
  if ((AuthenticationStatus & EFI_AUTH_STATUS_IMAGE_SIGNED) != 0) {
    if ((AuthenticationStatus & (EFI_AUTH_STATUS_TEST_FAILED | EFI_AUTH_STATUS_NOT_TESTED)) != 0) {
      return EFI_ACCESS_DENIED;
    }
  }

  return EFI_SUCCESS;
}


/**
  Register image authenticaion status check handler.

  @param  ImageHandle   ImageHandle of the loaded driver.
  @param  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
DxeImageAuthenticationStatusLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return RegisterSecurity2Handler (
           DxeImageAuthenticationStatusHandler,
           EFI_AUTH_OPERATION_AUTHENTICATION_STATE
           );
}
