/** @file
  SetImage instance to report system firmware and act as agent to system update.

  Caution: This module requires additional review when modified.
  This module will have external input - capsule image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  FmpSetImage() will receive untrusted input and do basic validation.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SystemFirmwareDxe.h"

//
// SystemFmp driver private data
//
SYSTEM_FMP_PRIVATE_DATA *mSystemFmpPrivate = NULL;

/**
  Dispatch system FMP images.

  Caution: This function may receive untrusted input.

  @param[in]  Image              The EDKII system FMP capsule image.
  @param[in]  ImageSize          The size of the EDKII system FMP capsule image in bytes.
  @param[out] LastAttemptVersion The last attempt version, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out] LastAttemptStatus  The last attempt status, which will be recorded in ESRT and FMP EFI_FIRMWARE_IMAGE_DESCRIPTOR.

  @retval EFI_SUCESS            Process Capsule Image successfully.
  @retval EFI_UNSUPPORTED       Capsule image is not supported by the firmware.
  @retval EFI_VOLUME_CORRUPTED  FV volume in the capsule is corrupted.
  @retval EFI_OUT_OF_RESOURCES  Not enough memory.
**/
EFI_STATUS
DispatchSystemFmpImages (
  IN VOID                         *Image,
  IN UINTN                        ImageSize,
  OUT UINT32                      *LastAttemptVersion,
  OUT UINT32                      *LastAttemptStatus
  )
{
  EFI_STATUS                                    Status;
  VOID                                          *AuthenticatedImage;
  UINTN                                         AuthenticatedImageSize;
  VOID                                          *DispatchFvImage;
  UINTN                                         DispatchFvImageSize;
  EFI_HANDLE                                    FvProtocolHandle;
  EFI_FIRMWARE_VOLUME_HEADER                    *FvImage;
  BOOLEAN                                       Result;

  AuthenticatedImage     = NULL;
  AuthenticatedImageSize = 0;

  DEBUG((DEBUG_INFO, "DispatchSystemFmpImages\n"));

  //
  // Verify
  //
  Status = CapsuleAuthenticateSystemFirmware(Image, ImageSize, FALSE, LastAttemptVersion, LastAttemptStatus, &AuthenticatedImage, &AuthenticatedImageSize);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_INFO, "SystemFirmwareAuthenticateImage - %r\n", Status));
    return Status;
  }

  //
  // Get FV
  //
  Result = ExtractDriverFvImage(AuthenticatedImage, AuthenticatedImageSize, &DispatchFvImage, &DispatchFvImageSize);
  if (Result) {
    DEBUG((DEBUG_INFO, "ExtractDriverFvImage\n"));
    //
    // Dispatch
    //
    if (((EFI_FIRMWARE_VOLUME_HEADER *)DispatchFvImage)->FvLength == DispatchFvImageSize) {
      FvImage = AllocatePages(EFI_SIZE_TO_PAGES(DispatchFvImageSize));
      if (FvImage != NULL) {
        CopyMem(FvImage, DispatchFvImage, DispatchFvImageSize);
        Status = gDS->ProcessFirmwareVolume(
                        (VOID *)FvImage,
                        (UINTN)FvImage->FvLength,
                        &FvProtocolHandle
                        );
        DEBUG((DEBUG_INFO, "ProcessFirmwareVolume - %r\n", Status));
        if (!EFI_ERROR(Status)) {
          gDS->Dispatch();
          DEBUG((DEBUG_INFO, "Dispatch Done\n"));
        }
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Updates the firmware image of the device.

  This function updates the hardware with the new firmware image.
  This function returns EFI_UNSUPPORTED if the firmware image is not updatable.
  If the firmware image is updatable, the function should perform the following minimal validations
  before proceeding to do the firmware image update.
  - Validate the image authentication if image has attribute
    IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED. The function returns
    EFI_SECURITY_VIOLATION if the validation fails.
  - Validate the image is a supported image for this device. The function returns EFI_ABORTED if
    the image is unsupported. The function can optionally provide more detailed information on
    why the image is not a supported image.
  - Validate the data from VendorCode if not null. Image validation must be performed before
    VendorCode data validation. VendorCode data is ignored or considered invalid if image
    validation failed. The function returns EFI_ABORTED if the data is invalid.

  VendorCode enables vendor to implement vendor-specific firmware image update policy. Null if
  the caller did not specify the policy or use the default policy. As an example, vendor can implement
  a policy to allow an option to force a firmware image update when the abort reason is due to the new
  firmware image version is older than the current firmware image version or bad image checksum.
  Sensitive operations such as those wiping the entire firmware image and render the device to be
  non-functional should be encoded in the image itself rather than passed with the VendorCode.
  AbortReason enables vendor to have the option to provide a more detailed description of the abort
  reason to the caller.

  @param[in]  This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]  ImageIndex         A unique number identifying the firmware image(s) within the device.
                                 The number is between 1 and DescriptorCount.
  @param[in]  Image              Points to the new image.
  @param[in]  ImageSize          Size of the new image in bytes.
  @param[in]  VendorCode         This enables vendor to implement vendor-specific firmware image update policy.
                                 Null indicates the caller did not specify the policy or use the default policy.
  @param[in]  Progress           A function used by the driver to report the progress of the firmware update.
  @param[out] AbortReason        A pointer to a pointer to a null-terminated string providing more
                                 details for the aborted operation. The buffer is allocated by this function
                                 with AllocatePool(), and it is the caller's responsibility to free it with a
                                 call to FreePool().

  @retval EFI_SUCCESS            The device was successfully updated with the new image.
  @retval EFI_ABORTED            The operation is aborted.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATION The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
FmpSetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL                 *This,
  IN  UINT8                                            ImageIndex,
  IN  CONST VOID                                       *Image,
  IN  UINTN                                            ImageSize,
  IN  CONST VOID                                       *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    Progress,
  OUT CHAR16                                           **AbortReason
  )
{
  SYSTEM_FMP_PRIVATE_DATA             *SystemFmpPrivate;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL    *SystemFmp;
  EFI_STATUS                          Status;
  EFI_STATUS                          VarStatus;

  if (Image == NULL || ImageSize == 0 || AbortReason == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SystemFmpPrivate = SYSTEM_FMP_PRIVATE_DATA_FROM_FMP(This);
  *AbortReason     = NULL;

  if (ImageIndex == 0 || ImageIndex > SystemFmpPrivate->DescriptorCount) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Process FV
  //
  Status = DispatchSystemFmpImages((VOID *)Image, ImageSize, &SystemFmpPrivate->LastAttempt.LastAttemptVersion, &SystemFmpPrivate->LastAttempt.LastAttemptStatus);
  DEBUG((DEBUG_INFO, "(Agent)SetImage - LastAttemp Version - 0x%x, State - 0x%x\n", SystemFmpPrivate->LastAttempt.LastAttemptVersion, SystemFmpPrivate->LastAttempt.LastAttemptStatus));
  if (EFI_ERROR(Status)) {
    VarStatus = gRT->SetVariable(
                       SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                       &gSystemFmpLastAttemptVariableGuid,
                       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                       sizeof(SystemFmpPrivate->LastAttempt),
                       &SystemFmpPrivate->LastAttempt
                       );
    DEBUG((DEBUG_INFO, "(Agent)SetLastAttemp - %r\n", VarStatus));
    return Status;
  }

  //
  // Pass Thru to System FMP Protocol on same handle as FMP Protocol
  //
  Status = gBS->HandleProtocol(
                  SystemFmpPrivate->Handle,
                  &gSystemFmpProtocolGuid,
                  (VOID **)&SystemFmp
                  );
  if (EFI_ERROR(Status)) {
    Status = gBS->LocateProtocol (
                    &gSystemFmpProtocolGuid,
                    NULL,
                    (VOID **)&SystemFmp
                    );
    if (EFI_ERROR(Status)) {
      DEBUG((DEBUG_INFO, "(Agent)SetImage - SystemFmpProtocol - %r\n", Status));
      SystemFmpPrivate->LastAttempt.LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_INVALID_FORMAT;
      VarStatus = gRT->SetVariable(
                         SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                         &gSystemFmpLastAttemptVariableGuid,
                         EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                         sizeof(SystemFmpPrivate->LastAttempt),
                         &SystemFmpPrivate->LastAttempt
                         );
      DEBUG((DEBUG_INFO, "(Agent)SetLastAttemp - %r\n", VarStatus));
      return Status;
    }
  }

  return SystemFmp->SetImage(SystemFmp, ImageIndex, Image, ImageSize, VendorCode, Progress, AbortReason);
}

/**
  System FMP module entrypoint

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @return EFI_SUCCESS System FMP module is initialized.
**/
EFI_STATUS
EFIAPI
SystemFirmwareReportMainDxe (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                                      Status;

  //
  // Initialize SystemFmpPrivateData
  //
  mSystemFmpPrivate = AllocateZeroPool (sizeof(SYSTEM_FMP_PRIVATE_DATA));
  if (mSystemFmpPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitializePrivateData(mSystemFmpPrivate);
  if (EFI_ERROR(Status)) {
    FreePool(mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
    return Status;
  }

  //
  // Install FMP protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mSystemFmpPrivate->Handle,
                  &gEfiFirmwareManagementProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mSystemFmpPrivate->Fmp
                  );
  if (EFI_ERROR (Status)) {
    FreePool(mSystemFmpPrivate);
    mSystemFmpPrivate = NULL;
    return Status;
  }

  return Status;
}
