/** @file
  EDKII System Capsule library.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#ifndef __EDKII_SYSTEM_CAPSULE_LIB_H__
#define __EDKII_SYSTEM_CAPSULE_LIB_H__

#include <Guid/EdkiiSystemFmpCapsule.h>

/**
  Extract ImageFmpInfo from system firmware.

  @param[in]  SystemFirmwareImage     The System Firmware image.
  @param[in]  SystemFirmwareImageSize The size of the System Firmware image in bytes.
  @param[out] ImageFmpInfo            The ImageFmpInfo.
  @param[out] ImageFmpInfoSize        The size of the ImageFmpInfo in bytes.

  @retval TRUE  The ImageFmpInfo is extracted.
  @retval FALSE The ImageFmpInfo is not extracted.
**/
BOOLEAN
EFIAPI
ExtractSystemFirmwareImageFmpInfo (
  IN VOID                                      *SystemFirmwareImage,
  IN UINTN                                     SystemFirmwareImageSize,
  OUT EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR   **ImageFmpInfo,
  OUT UINTN                                    *ImageFmpInfoSize
  );

/**
  Extract the driver FV from an authenticated image.

  @param[in]  AuthenticatedImage      The authenticated capsule image.
  @param[in]  AuthenticatedImageSize  The size of the authenticated capsule image in bytes.
  @param[out] DriverFvImage           The driver FV image.
  @param[out] DriverFvImageSize       The size of the driver FV image in bytes.

  @retval TRUE  The driver Fv is extracted.
  @retval FALSE The driver Fv is not extracted.
**/
BOOLEAN
EFIAPI
ExtractDriverFvImage (
  IN VOID                         *AuthenticatedImage,
  IN UINTN                        AuthenticatedImageSize,
  OUT VOID                        **DriverFvImage,
  OUT UINTN                       *DriverFvImageSize
  );

/**
  Extract the config image from an authenticated image.

  @param[in]  AuthenticatedImage      The authenticated capsule image.
  @param[in]  AuthenticatedImageSize  The size of the authenticated capsule image in bytes.
  @param[out] ConfigImage             The config image.
  @param[out] ConfigImageSize         The size of the config image in bytes.

  @retval TRUE  The config image is extracted.
  @retval FALSE The config image is not extracted.
**/
BOOLEAN
EFIAPI
ExtractConfigImage (
  IN VOID                         *AuthenticatedImage,
  IN UINTN                        AuthenticatedImageSize,
  OUT VOID                        **ConfigImage,
  OUT UINTN                       *ConfigImageSize
  );

/**
  Extract the System Firmware image from an authenticated image.

  @param[in]  AuthenticatedImage      The authenticated capsule image.
  @param[in]  AuthenticatedImageSize  The size of the authenticated capsule image in bytes.
  @param[out] SystemFirmwareImage     The System Firmware image.
  @param[out] SystemFirmwareImageSize The size of the System Firmware image in bytes.

  @retval TRUE  The System Firmware image is extracted.
  @retval FALSE The System Firmware image is not extracted.
**/
BOOLEAN
EFIAPI
ExtractSystemFirmwareImage (
  IN VOID                         *AuthenticatedImage,
  IN UINTN                        AuthenticatedImageSize,
  OUT VOID                        **SystemFirmwareImage,
  OUT UINTN                       *SystemFirmwareImageSize
  );

/**
  Extract the authenticated image from an FMP capsule image.

  @param[in]  Image                   The FMP capsule image, including EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               The size of FMP capsule image in bytes.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AuthenticatedImage      The authenticated capsule image, excluding EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[out] AuthenticatedImageSize  The size of the authenticated capsule image in bytes.

  @retval TRUE  The authenticated image is extracted.
  @retval FALSE The authenticated image is not extracted.
**/
BOOLEAN
EFIAPI
ExtractAuthenticatedImage (
  IN VOID                         *Image,
  IN UINTN                        ImageSize,
  OUT UINT32                      *LastAttemptStatus,
  OUT VOID                        **AuthenticatedImage,
  OUT UINTN                       *AuthenticatedImageSize
  );

/**
  Authenticated system firmware FMP capsule image.

  Caution: This function may receive untrusted input.

  @param[in]  Image                   The FMP capsule image, including EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[in]  ImageSize               The size of FMP capsule image in bytes.
  @param[in]  ForceVersionMatch       TRUE: The version of capsule must be as same as the version of current image.
                                      FALSE: The version of capsule must be as same as greater than the lowest
                                             supported version of current image.
  @param[out] LastAttemptVersion      The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus       The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] AuthenticatedImage      The authenticated capsule image, excluding EFI_FIRMWARE_IMAGE_AUTHENTICATION.
  @param[out] AuthenticatedImageSize  The size of the authenticated capsule image in bytes.

  @retval TRUE  Authentication passes and the authenticated image is extracted.
  @retval FALSE Authentication fails and the authenticated image is not extracted.
**/
EFI_STATUS
EFIAPI
CapsuleAuthenticateSystemFirmware (
  IN VOID                         *Image,
  IN UINTN                        ImageSize,
  IN BOOLEAN                      ForceVersionMatch,
  OUT UINT32                      *LastAttemptVersion,
  OUT UINT32                      *LastAttemptStatus,
  OUT VOID                        **AuthenticatedImage,
  OUT UINTN                       *AuthenticatedImageSize
  );

#endif

