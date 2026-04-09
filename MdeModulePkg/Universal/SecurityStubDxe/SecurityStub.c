/** @file
  This driver produces Security2 and Security architectural protocol based on SecurityManagementLib.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/Security.h>
#include <Protocol/Security2.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/SecurityManagementLib.h>
#include "Defer3rdPartyImageLoad.h"

//
// Handle for the Security Architectural Protocol instance produced by this driver
//
EFI_HANDLE  mSecurityArchProtocolHandle = NULL;

/**
  The EFI_SECURITY_ARCH_PROTOCOL (SAP) is used to abstract platform-specific
  policy from the DXE core response to an attempt to use a file that returns a
  given status for the authentication check from the section extraction protocol.

  The possible responses in a given SAP implementation may include locking
  flash upon failure to authenticate, attestation logging for all signed drivers,
  and other exception operations.  The File parameter allows for possible logging
  within the SAP of the driver.

  If File is NULL, then EFI_INVALID_PARAMETER is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is safe for the DXE Core to use, then EFI_SUCCESS is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use under any circumstances,
  then EFI_ACCESS_DENIED is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use right now, but it
  might be possible to use it at a future time, then EFI_SECURITY_VIOLATION is
  returned.

  @param  This             The EFI_SECURITY_ARCH_PROTOCOL instance.
  @param  AuthenticationStatus
                           This is the authentication type returned from the Section
                           Extraction protocol. See the Section Extraction Protocol
                           Specification for details on this type.
  @param  File             This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.

  @retval EFI_SUCCESS            Do nothing and return success.
  @retval EFI_INVALID_PARAMETER  File is NULL.
**/
EFI_STATUS
EFIAPI
SecurityStubAuthenticateState (
  IN CONST EFI_SECURITY_ARCH_PROTOCOL  *This,
  IN UINT32                            AuthenticationStatus,
  IN CONST EFI_DEVICE_PATH_PROTOCOL    *File
  )
{
  EFI_STATUS  Status;

  Status = ExecuteSecurity2Handlers (
             EFI_AUTH_OPERATION_AUTHENTICATION_STATE,
             AuthenticationStatus,
             File,
             NULL,
             0,
             FALSE
             );
  if (Status == EFI_SUCCESS) {
    Status = ExecuteSecurityHandlers (AuthenticationStatus, File);
  }

  return Status;
}

/**
  The DXE Foundation uses this service to measure and/or verify a UEFI image.

  This service abstracts the invocation of Trusted Computing Group (TCG) measured boot, UEFI
  Secure boot, and UEFI User Identity infrastructure. For the former two, the DXE Foundation
  invokes the FileAuthentication() with a DevicePath and corresponding image in
  FileBuffer memory. The TCG measurement code will record the FileBuffer contents into the
  appropriate PCR. The image verification logic will confirm the integrity and provenance of the
  image in FileBuffer of length FileSize . The origin of the image will be DevicePath in
  these cases.
  If the FileBuffer is NULL, the interface will determine if the DevicePath can be connected
  in order to support the User Identification policy.

  @param  This             The EFI_SECURITY2_ARCH_PROTOCOL instance.
  @param  File             A pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param  FileBuffer       A pointer to the buffer with the UEFI file image.
  @param  FileSize         The size of the file.
  @param  BootPolicy       A boot policy that was used to call LoadImage() UEFI service. If
                           FileAuthentication() is invoked not from the LoadImage(),
                           BootPolicy must be set to FALSE.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval EFI_SUCCESS             The device path specified by NULL device path DevicePath
                                  and non-NULL FileBuffer did authenticate, and the platform
                                  policy dictates that the DXE Foundation may execute the image in
                                  FileBuffer.
  @retval EFI_SUCCESS             FileBuffer is NULL and current user has permission to start
                                  UEFI device drivers on the device path specified by DevicePath.
  @retval EFI_SECURITY_VIOLATION  The file specified by DevicePath and FileBuffer did not
                                  authenticate, and the platform policy dictates that the file should be
                                  placed in the untrusted state. The image has been added to the file
                                  execution table.
  @retval EFI_ACCESS_DENIED       The file specified by File and FileBuffer did not
                                  authenticate, and the platform policy dictates that the DXE
                                  Foundation many not use File.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is NULL and the user has no
                                  permission to start UEFI device drivers on the device path specified
                                  by DevicePath.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is not NULL and the user has no permission to load
                                  drivers from the device path specified by DevicePath. The
                                  image has been added into the list of the deferred images.
**/
EFI_STATUS
EFIAPI
Security2StubAuthenticate (
  IN CONST EFI_SECURITY2_ARCH_PROTOCOL  *This,
  IN CONST EFI_DEVICE_PATH_PROTOCOL     *File  OPTIONAL,
  IN VOID                               *FileBuffer,
  IN UINTN                              FileSize,
  IN BOOLEAN                            BootPolicy
  )
{
  EFI_STATUS  Status;

  if (FileBuffer != NULL) {
    Status = Defer3rdPartyImageLoad (File, BootPolicy);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return ExecuteSecurity2Handlers (
           EFI_AUTH_OPERATION_VERIFY_IMAGE |
           EFI_AUTH_OPERATION_DEFER_IMAGE_LOAD |
           EFI_AUTH_OPERATION_MEASURE_IMAGE |
           EFI_AUTH_OPERATION_CONNECT_POLICY,
           0,
           File,
           FileBuffer,
           FileSize,
           BootPolicy
           );
}

//
// Security2 and Security Architectural Protocol instance produced by this driver
//
EFI_SECURITY_ARCH_PROTOCOL  mSecurityStub = {
  SecurityStubAuthenticateState
};

EFI_SECURITY2_ARCH_PROTOCOL  mSecurity2Stub = {
  Security2StubAuthenticate
};

/**
  Installs Security2 and Security Architectural Protocol.

  @param  ImageHandle  The image handle of this driver.
  @param  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS   Install the sample Security Architectural Protocol successfully.

**/
EFI_STATUS
EFIAPI
SecurityStubInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Make sure the Security Architectural Protocol is not already installed in the system
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiSecurity2ArchProtocolGuid);
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiSecurityArchProtocolGuid);

  //
  // Install the Security Architectural Protocol onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSecurityArchProtocolHandle,
                  &gEfiSecurity2ArchProtocolGuid,
                  &mSecurity2Stub,
                  &gEfiSecurityArchProtocolGuid,
                  &mSecurityStub,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Defer3rdPartyImageLoadInitialize ();

  return EFI_SUCCESS;
}
