/** @file
  FMP capsule authenitcation Library.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __FMP_AUTHENTICATION_LIB_H__
#define __FMP_AUTHENTICATION_LIB_H__

#include <Protocol/FirmwareManagement.h>

/**
  The function is used to do the authentication for FMP capsule based upon
  EFI_FIRMWARE_IMAGE_AUTHENTICATION.

  The FMP capsule image should start with EFI_FIRMWARE_IMAGE_AUTHENTICATION,
  followed by the payload.

  If the return status is RETURN_SUCCESS, the caller may continue the rest
  FMP update process.
  If the return status is NOT RETURN_SUCCESS, the caller should stop the FMP
  update process and convert the return status to LastAttemptStatus
  to indicate that FMP update fails.
  The LastAttemptStatus can be got from ESRT table or via
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL.GetImageInfo().

  Caution: This function may receive untrusted input.

  @param[in]  Image                   Points to an FMP authentication image, started from EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               Size of the authentication image in bytes.
  @param[in]  PublicKeyData           The public key data used to validate the signature.
  @param[in]  PublicKeyDataLength     The length of the public key data.

  @retval RETURN_SUCCESS            Authentication pass.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_SUCCESS.
  @retval RETURN_SECURITY_VIOLATION Authentication fail.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR.
  @retval RETURN_INVALID_PARAMETER  The image is in an invalid format.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_UNSUPPORTED        No Authentication handler associated with CertType.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_UNSUPPORTED        Image or ImageSize is invalid.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT.
  @retval RETURN_OUT_OF_RESOURCES   No Authentication handler associated with CertType.
                                    The LastAttemptStatus should be LAST_ATTEMPT_STATUS_ERROR_INSUFFICIENT_RESOURCES.
**/
RETURN_STATUS
EFIAPI
AuthenticateFmpImage (
  IN EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN UINTN                              ImageSize,
  IN CONST UINT8                        *PublicKeyData,
  IN UINTN                              PublicKeyDataLength
  );

#endif

