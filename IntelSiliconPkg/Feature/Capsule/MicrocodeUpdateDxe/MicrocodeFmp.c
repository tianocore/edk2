/** @file
  Produce FMP instance for Microcode.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MicrocodeUpdate.h"

//
// MicrocodeFmp driver private data
//
MICROCODE_FMP_PRIVATE_DATA *mMicrocodeFmpPrivate = NULL;

EFI_FIRMWARE_MANAGEMENT_PROTOCOL mFirmwareManagementProtocol = {
  FmpGetImageInfo,
  FmpGetImage,
  FmpSetImage,
  FmpCheckImage,
  FmpGetPackageInfo,
  FmpSetPackageInfo
};

/**
  Initialize Microcode Descriptor.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.

  @return EFI_SUCCESS Microcode Descriptor is initialized.
**/
EFI_STATUS
InitializeMicrocodeDescriptor (
  IN MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate
  );

/**
  Returns information about the current firmware image(s) of the device.

  This function allows a copy of the current firmware image to be created and saved.
  The saved copy could later been used, for example, in firmware image recovery or rollback.

  @param[in]      This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in, out] ImageInfoSize      A pointer to the size, in bytes, of the ImageInfo buffer.
                                     On input, this is the size of the buffer allocated by the caller.
                                     On output, it is the size of the buffer returned by the firmware
                                     if the buffer was large enough, or the size of the buffer needed
                                     to contain the image(s) information if the buffer was too small.
  @param[in, out] ImageInfo          A pointer to the buffer in which firmware places the current image(s)
                                     information. The information is an array of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  @param[out]     DescriptorVersion  A pointer to the location in which firmware returns the version number
                                     associated with the EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]     DescriptorCount    A pointer to the location in which firmware returns the number of
                                     descriptors or firmware images within this device.
  @param[out]     DescriptorSize     A pointer to the location in which firmware returns the size, in bytes,
                                     of an individual EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]     PackageVersion     A version number that represents all the firmware images in the device.
                                     The format is vendor specific and new version must have a greater value
                                     than the old version. If PackageVersion is not supported, the value is
                                     0xFFFFFFFF. A value of 0xFFFFFFFE indicates that package version comparison
                                     is to be performed using PackageVersionName. A value of 0xFFFFFFFD indicates
                                     that package version update is in progress.
  @param[out]     PackageVersionName A pointer to a pointer to a null-terminated string representing the
                                     package version name. The buffer is allocated by this function with
                                     AllocatePool(), and it is the caller's responsibility to free it with a call
                                     to FreePool().

  @retval EFI_SUCCESS                The device was successfully updated with the new image.
  @retval EFI_BUFFER_TOO_SMALL       The ImageInfo buffer was too small. The current buffer size
                                     needed to hold the image(s) information is returned in ImageInfoSize.
  @retval EFI_INVALID_PARAMETER      ImageInfoSize is NULL.
  @retval EFI_DEVICE_ERROR           Valid information could not be returned. Possible corrupted image.

**/
EFI_STATUS
EFIAPI
FmpGetImageInfo (
  IN        EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN OUT    UINTN                             *ImageInfoSize,
  IN OUT    EFI_FIRMWARE_IMAGE_DESCRIPTOR     *ImageInfo,
  OUT       UINT32                            *DescriptorVersion,
  OUT       UINT8                             *DescriptorCount,
  OUT       UINTN                             *DescriptorSize,
  OUT       UINT32                            *PackageVersion,
  OUT       CHAR16                            **PackageVersionName
  )
{
  MICROCODE_FMP_PRIVATE_DATA *MicrocodeFmpPrivate;
  UINTN                      Index;

  MicrocodeFmpPrivate = MICROCODE_FMP_PRIVATE_DATA_FROM_FMP(This);

  if(ImageInfoSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*ImageInfoSize < sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * MicrocodeFmpPrivate->DescriptorCount) {
    *ImageInfoSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * MicrocodeFmpPrivate->DescriptorCount;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (ImageInfo == NULL ||
      DescriptorVersion == NULL ||
      DescriptorCount == NULL ||
      DescriptorSize == NULL ||
      PackageVersion == NULL ||
      PackageVersionName == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ImageInfoSize      = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * MicrocodeFmpPrivate->DescriptorCount;
  *DescriptorSize     = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);
  *DescriptorCount    = MicrocodeFmpPrivate->DescriptorCount;
  *DescriptorVersion  = EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION;

  //
  // supports 1 ImageInfo descriptor
  //
  CopyMem(&ImageInfo[0], MicrocodeFmpPrivate->ImageDescriptor, sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * MicrocodeFmpPrivate->DescriptorCount);
  for (Index = 0; Index < MicrocodeFmpPrivate->DescriptorCount; Index++) {
    if ((ImageInfo[Index].AttributesSetting & IMAGE_ATTRIBUTE_IN_USE) != 0) {
      ImageInfo[Index].LastAttemptVersion = MicrocodeFmpPrivate->LastAttempt.LastAttemptVersion;
      ImageInfo[Index].LastAttemptStatus = MicrocodeFmpPrivate->LastAttempt.LastAttemptStatus;
    }
  }

  //
  // package version
  //
  *PackageVersion = MicrocodeFmpPrivate->PackageVersion;
  if (MicrocodeFmpPrivate->PackageVersionName != NULL) {
    *PackageVersionName = AllocateCopyPool(StrSize(MicrocodeFmpPrivate->PackageVersionName), MicrocodeFmpPrivate->PackageVersionName);
  }

  return EFI_SUCCESS;
}

/**
  Retrieves a copy of the current firmware image of the device.

  This function allows a copy of the current firmware image to be created and saved.
  The saved copy could later been used, for example, in firmware image recovery or rollback.

  @param[in]     This            A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]     ImageIndex      A unique number identifying the firmware image(s) within the device.
                                 The number is between 1 and DescriptorCount.
  @param[in,out] Image           Points to the buffer where the current image is copied to.
  @param[in,out] ImageSize       On entry, points to the size of the buffer pointed to by Image, in bytes.
                                 On return, points to the length of the image, in bytes.

  @retval EFI_SUCCESS            The device was successfully updated with the new image.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by ImageSize is too small to hold the
                                 image. The current buffer size needed to hold the image is returned
                                 in ImageSize.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_NOT_FOUND          The current image is not copied to the buffer.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATION The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
FmpGetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  UINT8                             ImageIndex,
  IN  OUT  VOID                         *Image,
  IN  OUT  UINTN                        *ImageSize
  )
{
  MICROCODE_FMP_PRIVATE_DATA *MicrocodeFmpPrivate;
  MICROCODE_INFO             *MicrocodeInfo;

  if (Image == NULL || ImageSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MicrocodeFmpPrivate = MICROCODE_FMP_PRIVATE_DATA_FROM_FMP(This);

  if (ImageIndex == 0 || ImageIndex > MicrocodeFmpPrivate->DescriptorCount || ImageSize == NULL || Image == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MicrocodeInfo = &MicrocodeFmpPrivate->MicrocodeInfo[ImageIndex - 1];

  if (*ImageSize < MicrocodeInfo->TotalSize) {
    *ImageSize = MicrocodeInfo->TotalSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *ImageSize = MicrocodeInfo->TotalSize;
  CopyMem (Image, MicrocodeInfo->MicrocodeEntryPoint, MicrocodeInfo->TotalSize);
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
  EFI_STATUS                 Status;
  EFI_STATUS                 VarStatus;
  MICROCODE_FMP_PRIVATE_DATA *MicrocodeFmpPrivate;

  if (Image == NULL || AbortReason == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  MicrocodeFmpPrivate = MICROCODE_FMP_PRIVATE_DATA_FROM_FMP(This);
  *AbortReason     = NULL;

  if (ImageIndex == 0 || ImageIndex > MicrocodeFmpPrivate->DescriptorCount || Image == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = MicrocodeWrite(MicrocodeFmpPrivate, (VOID *)Image, ImageSize, &MicrocodeFmpPrivate->LastAttempt.LastAttemptVersion, &MicrocodeFmpPrivate->LastAttempt.LastAttemptStatus, AbortReason);
  DEBUG((DEBUG_INFO, "SetImage - LastAttempt Version - 0x%x, Status - 0x%x\n", MicrocodeFmpPrivate->LastAttempt.LastAttemptVersion, MicrocodeFmpPrivate->LastAttempt.LastAttemptStatus));
  VarStatus = gRT->SetVariable(
                     MICROCODE_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                     &gEfiCallerIdGuid,
                     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                     sizeof(MicrocodeFmpPrivate->LastAttempt),
                     &MicrocodeFmpPrivate->LastAttempt
                     );
  DEBUG((DEBUG_INFO, "SetLastAttempt - %r\n", VarStatus));

  if (!EFI_ERROR(Status)) {
    InitializeMicrocodeDescriptor(MicrocodeFmpPrivate);
    DumpPrivateInfo (MicrocodeFmpPrivate);
  }

  return Status;
}

/**
  Checks if the firmware image is valid for the device.

  This function allows firmware update application to validate the firmware image without
  invoking the SetImage() first.

  @param[in]  This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]  ImageIndex         A unique number identifying the firmware image(s) within the device.
                                 The number is between 1 and DescriptorCount.
  @param[in]  Image              Points to the new image.
  @param[in]  ImageSize          Size of the new image in bytes.
  @param[out] ImageUpdatable     Indicates if the new image is valid for update. It also provides,
                                 if available, additional information if the image is invalid.

  @retval EFI_SUCCESS            The image was successfully checked.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATION The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
FmpCheckImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  UINT8                             ImageIndex,
  IN  CONST VOID                        *Image,
  IN  UINTN                             ImageSize,
  OUT UINT32                            *ImageUpdatable
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Returns information about the firmware package.

  This function returns package information.

  @param[in]  This                     A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[out] PackageVersion           A version number that represents all the firmware images in the device.
                                       The format is vendor specific and new version must have a greater value
                                       than the old version. If PackageVersion is not supported, the value is
                                       0xFFFFFFFF. A value of 0xFFFFFFFE indicates that package version
                                       comparison is to be performed using PackageVersionName. A value of
                                       0xFFFFFFFD indicates that package version update is in progress.
  @param[out] PackageVersionName       A pointer to a pointer to a null-terminated string representing
                                       the package version name. The buffer is allocated by this function with
                                       AllocatePool(), and it is the caller's responsibility to free it with a
                                       call to FreePool().
  @param[out] PackageVersionNameMaxLen The maximum length of package version name if device supports update of
                                       package version name. A value of 0 indicates the device does not support
                                       update of package version name. Length is the number of Unicode characters,
                                       including the terminating null character.
  @param[out] AttributesSupported      Package attributes that are supported by this device. See 'Package Attribute
                                       Definitions' for possible returned values of this parameter. A value of 1
                                       indicates the attribute is supported and the current setting value is
                                       indicated in AttributesSetting. A value of 0 indicates the attribute is not
                                       supported and the current setting value in AttributesSetting is meaningless.
  @param[out] AttributesSetting        Package attributes. See 'Package Attribute Definitions' for possible returned
                                       values of this parameter

  @retval EFI_SUCCESS                  The package information was successfully returned.
  @retval EFI_UNSUPPORTED              The operation is not supported.

**/
EFI_STATUS
EFIAPI
FmpGetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL *This,
  OUT UINT32                           *PackageVersion,
  OUT CHAR16                           **PackageVersionName,
  OUT UINT32                           *PackageVersionNameMaxLen,
  OUT UINT64                           *AttributesSupported,
  OUT UINT64                           *AttributesSetting
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Updates information about the firmware package.

  This function updates package information.
  This function returns EFI_UNSUPPORTED if the package information is not updatable.
  VendorCode enables vendor to implement vendor-specific package information update policy.
  Null if the caller did not specify this policy or use the default policy.

  @param[in]  This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]  Image              Points to the authentication image.
                                 Null if authentication is not required.
  @param[in]  ImageSize          Size of the authentication image in bytes.
                                 0 if authentication is not required.
  @param[in]  VendorCode         This enables vendor to implement vendor-specific firmware
                                 image update policy.
                                 Null indicates the caller did not specify this policy or use
                                 the default policy.
  @param[in]  PackageVersion     The new package version.
  @param[in]  PackageVersionName A pointer to the new null-terminated Unicode string representing
                                 the package version name.
                                 The string length is equal to or less than the value returned in
                                 PackageVersionNameMaxLen.

  @retval EFI_SUCCESS            The device was successfully updated with the new package
                                 information.
  @retval EFI_INVALID_PARAMETER  The PackageVersionName length is longer than the value
                                 returned in PackageVersionNameMaxLen.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATION The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
FmpSetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL   *This,
  IN  CONST VOID                         *Image,
  IN  UINTN                              ImageSize,
  IN  CONST VOID                         *VendorCode,
  IN  UINT32                             PackageVersion,
  IN  CONST CHAR16                       *PackageVersionName
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Sort FIT microcode entries based upon MicrocodeEntryPoint, from low to high.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.

**/
VOID
SortFitMicrocodeInfo (
  IN MICROCODE_FMP_PRIVATE_DATA     *MicrocodeFmpPrivate
  )
{
  FIT_MICROCODE_INFO        *FitMicrocodeEntry;
  FIT_MICROCODE_INFO        *NextFitMicrocodeEntry;
  FIT_MICROCODE_INFO        TempFitMicrocodeEntry;
  FIT_MICROCODE_INFO        *FitMicrocodeEntryEnd;

  FitMicrocodeEntry = MicrocodeFmpPrivate->FitMicrocodeInfo;
  NextFitMicrocodeEntry = FitMicrocodeEntry + 1;
  FitMicrocodeEntryEnd = MicrocodeFmpPrivate->FitMicrocodeInfo + MicrocodeFmpPrivate->FitMicrocodeEntryCount;
  while (FitMicrocodeEntry < FitMicrocodeEntryEnd) {
    while (NextFitMicrocodeEntry < FitMicrocodeEntryEnd) {
      if (FitMicrocodeEntry->MicrocodeEntryPoint > NextFitMicrocodeEntry->MicrocodeEntryPoint) {
        CopyMem (&TempFitMicrocodeEntry, FitMicrocodeEntry, sizeof (FIT_MICROCODE_INFO));
        CopyMem (FitMicrocodeEntry, NextFitMicrocodeEntry, sizeof (FIT_MICROCODE_INFO));
        CopyMem (NextFitMicrocodeEntry, &TempFitMicrocodeEntry, sizeof (FIT_MICROCODE_INFO));
      }

      NextFitMicrocodeEntry = NextFitMicrocodeEntry + 1;
    }

    FitMicrocodeEntry     = FitMicrocodeEntry + 1;
    NextFitMicrocodeEntry = FitMicrocodeEntry + 1;
  }
}

/**
  Initialize FIT microcode information.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.

  @return EFI_SUCCESS           FIT microcode information is initialized.
  @return EFI_OUT_OF_RESOURCES  No enough resource for the initialization.
  @return EFI_DEVICE_ERROR      There is something wrong in FIT microcode entry.
**/
EFI_STATUS
InitializeFitMicrocodeInfo (
  IN MICROCODE_FMP_PRIVATE_DATA     *MicrocodeFmpPrivate
  )
{
  UINT64                            FitPointer;
  FIRMWARE_INTERFACE_TABLE_ENTRY    *FitEntry;
  UINT32                            EntryNum;
  UINT32                            MicrocodeEntryNum;
  UINT32                            Index;
  UINTN                             Address;
  VOID                              *MicrocodePatchAddress;
  UINTN                             MicrocodePatchRegionSize;
  FIT_MICROCODE_INFO                *FitMicrocodeInfo;
  FIT_MICROCODE_INFO                *FitMicrocodeInfoNext;
  CPU_MICROCODE_HEADER              *MicrocodeEntryPoint;
  CPU_MICROCODE_HEADER              *MicrocodeEntryPointNext;
  UINTN                             FitMicrocodeIndex;
  MICROCODE_INFO                    *MicrocodeInfo;
  UINTN                             MicrocodeIndex;

  if (MicrocodeFmpPrivate->FitMicrocodeInfo != NULL) {
    FreePool (MicrocodeFmpPrivate->FitMicrocodeInfo);
    MicrocodeFmpPrivate->FitMicrocodeInfo = NULL;
    MicrocodeFmpPrivate->FitMicrocodeEntryCount = 0;
  }

  FitPointer = *(UINT64 *) (UINTN) FIT_POINTER_ADDRESS;
  if ((FitPointer == 0) ||
      (FitPointer == 0xFFFFFFFFFFFFFFFF) ||
      (FitPointer == 0xEEEEEEEEEEEEEEEE)) {
    //
    // No FIT table.
    //
    return EFI_SUCCESS;
  }
  FitEntry = (FIRMWARE_INTERFACE_TABLE_ENTRY *) (UINTN) FitPointer;
  if ((FitEntry[0].Type != FIT_TYPE_00_HEADER) ||
      (FitEntry[0].Address != FIT_TYPE_00_SIGNATURE)) {
    //
    // Invalid FIT table, treat it as no FIT table.
    //
    return EFI_SUCCESS;
  }

  EntryNum = *(UINT32 *)(&FitEntry[0].Size[0]) & 0xFFFFFF;

  //
  // Calculate microcode entry number.
  //
  MicrocodeEntryNum = 0;
  for (Index = 0; Index < EntryNum; Index++) {
    if (FitEntry[Index].Type == FIT_TYPE_01_MICROCODE) {
      MicrocodeEntryNum++;
    }
  }
  if (MicrocodeEntryNum == 0) {
    //
    // No FIT microcode entry.
    //
    return EFI_SUCCESS;
  }

  //
  // Allocate buffer.
  //
  MicrocodeFmpPrivate->FitMicrocodeInfo = AllocateZeroPool (MicrocodeEntryNum * sizeof (FIT_MICROCODE_INFO));
  if (MicrocodeFmpPrivate->FitMicrocodeInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  MicrocodeFmpPrivate->FitMicrocodeEntryCount = MicrocodeEntryNum;

  MicrocodePatchAddress = MicrocodeFmpPrivate->MicrocodePatchAddress;
  MicrocodePatchRegionSize = MicrocodeFmpPrivate->MicrocodePatchRegionSize;

  //
  // Collect microcode entry info.
  //
  MicrocodeEntryNum = 0;
  for (Index = 0; Index < EntryNum; Index++) {
    if (FitEntry[Index].Type == FIT_TYPE_01_MICROCODE) {
      Address = (UINTN) FitEntry[Index].Address;
      if ((Address < (UINTN) MicrocodePatchAddress) ||
          (Address >= ((UINTN) MicrocodePatchAddress + MicrocodePatchRegionSize))) {
        DEBUG ((
          DEBUG_ERROR,
          "InitializeFitMicrocodeInfo - Address (0x%x) is not in Microcode Region\n",
          Address
          ));
        goto ErrorExit;
      }
      FitMicrocodeInfo = &MicrocodeFmpPrivate->FitMicrocodeInfo[MicrocodeEntryNum];
      FitMicrocodeInfo->MicrocodeEntryPoint = (CPU_MICROCODE_HEADER *) Address;
      if ((*(UINT32 *) Address) == 0xFFFFFFFF) {
        //
        // It is the empty slot as long as the first dword is 0xFFFF_FFFF.
        //
        FitMicrocodeInfo->Empty = TRUE;
      } else {
        FitMicrocodeInfo->Empty = FALSE;
      }
      MicrocodeEntryNum++;
    }
  }

  //
  // Every microcode should have a FIT microcode entry.
  //
  for (MicrocodeIndex = 0; MicrocodeIndex < MicrocodeFmpPrivate->DescriptorCount; MicrocodeIndex++) {
    MicrocodeInfo = &MicrocodeFmpPrivate->MicrocodeInfo[MicrocodeIndex];
    for (FitMicrocodeIndex = 0; FitMicrocodeIndex < MicrocodeFmpPrivate->FitMicrocodeEntryCount; FitMicrocodeIndex++) {
      FitMicrocodeInfo = &MicrocodeFmpPrivate->FitMicrocodeInfo[FitMicrocodeIndex];
      if (MicrocodeInfo->MicrocodeEntryPoint == FitMicrocodeInfo->MicrocodeEntryPoint) {
        FitMicrocodeInfo->TotalSize = MicrocodeInfo->TotalSize;
        FitMicrocodeInfo->InUse = MicrocodeInfo->InUse;
        break;
      }
    }
    if (FitMicrocodeIndex >= MicrocodeFmpPrivate->FitMicrocodeEntryCount) {
      DEBUG ((
        DEBUG_ERROR,
        "InitializeFitMicrocodeInfo - There is no FIT microcode entry for Microcode (0x%x)\n",
        MicrocodeInfo->MicrocodeEntryPoint
        ));
      goto ErrorExit;
    }
  }

  SortFitMicrocodeInfo (MicrocodeFmpPrivate);

  //
  // Check overlap.
  //
  for (FitMicrocodeIndex = 0; FitMicrocodeIndex < MicrocodeFmpPrivate->FitMicrocodeEntryCount - 1; FitMicrocodeIndex++) {
    FitMicrocodeInfo = &MicrocodeFmpPrivate->FitMicrocodeInfo[FitMicrocodeIndex];
    MicrocodeEntryPoint = FitMicrocodeInfo->MicrocodeEntryPoint;
    FitMicrocodeInfoNext = &MicrocodeFmpPrivate->FitMicrocodeInfo[FitMicrocodeIndex + 1];
    MicrocodeEntryPointNext = FitMicrocodeInfoNext->MicrocodeEntryPoint;
    if ((MicrocodeEntryPoint >= MicrocodeEntryPointNext) ||
        ((FitMicrocodeInfo->TotalSize != 0) &&
         ((UINTN) MicrocodeEntryPoint + FitMicrocodeInfo->TotalSize) >
          (UINTN) MicrocodeEntryPointNext)) {
      DEBUG ((
        DEBUG_ERROR,
        "InitializeFitMicrocodeInfo - There is overlap between FIT microcode entries (0x%x 0x%x)\n",
        MicrocodeEntryPoint,
        MicrocodeEntryPointNext
        ));
      goto ErrorExit;
    }
  }

  return EFI_SUCCESS;

ErrorExit:
  FreePool (MicrocodeFmpPrivate->FitMicrocodeInfo);
  MicrocodeFmpPrivate->FitMicrocodeInfo = NULL;
  MicrocodeFmpPrivate->FitMicrocodeEntryCount = 0;
  return EFI_DEVICE_ERROR;
}

/**
  Initialize Processor Microcode Index.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.
**/
VOID
InitializedProcessorMicrocodeIndex (
  IN MICROCODE_FMP_PRIVATE_DATA *MicrocodeFmpPrivate
  )
{
  UINTN       CpuIndex;
  UINTN       MicrocodeIndex;
  UINTN       TargetCpuIndex;
  UINT32      AttemptStatus;
  EFI_STATUS  Status;

  for (CpuIndex = 0; CpuIndex < MicrocodeFmpPrivate->ProcessorCount; CpuIndex++) {
    if (MicrocodeFmpPrivate->ProcessorInfo[CpuIndex].MicrocodeIndex != (UINTN)-1) {
      continue;
    }
    for (MicrocodeIndex = 0; MicrocodeIndex < MicrocodeFmpPrivate->DescriptorCount; MicrocodeIndex++) {
      if (!MicrocodeFmpPrivate->MicrocodeInfo[MicrocodeIndex].InUse) {
        continue;
      }
      TargetCpuIndex = CpuIndex;
      Status = VerifyMicrocode(
                 MicrocodeFmpPrivate,
                 MicrocodeFmpPrivate->MicrocodeInfo[MicrocodeIndex].MicrocodeEntryPoint,
                 MicrocodeFmpPrivate->MicrocodeInfo[MicrocodeIndex].TotalSize,
                 FALSE,
                 &AttemptStatus,
                 NULL,
                 &TargetCpuIndex
                 );
      if (!EFI_ERROR(Status)) {
        MicrocodeFmpPrivate->ProcessorInfo[CpuIndex].MicrocodeIndex = MicrocodeIndex;
      }
    }
  }
}

/**
  Initialize Microcode Descriptor.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.

  @return EFI_SUCCESS           Microcode Descriptor is initialized.
  @return EFI_OUT_OF_RESOURCES  No enough resource for the initialization.
**/
EFI_STATUS
InitializeMicrocodeDescriptor (
  IN MICROCODE_FMP_PRIVATE_DATA *MicrocodeFmpPrivate
  )
{
  EFI_STATUS Status;
  UINT8      CurrentMicrocodeCount;

  CurrentMicrocodeCount = (UINT8)GetMicrocodeInfo (MicrocodeFmpPrivate, 0, NULL, NULL);

  if (CurrentMicrocodeCount > MicrocodeFmpPrivate->DescriptorCount) {
    if (MicrocodeFmpPrivate->ImageDescriptor != NULL) {
      FreePool(MicrocodeFmpPrivate->ImageDescriptor);
      MicrocodeFmpPrivate->ImageDescriptor = NULL;
    }
    if (MicrocodeFmpPrivate->MicrocodeInfo != NULL) {
      FreePool(MicrocodeFmpPrivate->MicrocodeInfo);
      MicrocodeFmpPrivate->MicrocodeInfo = NULL;
    }
  } else {
    ZeroMem(MicrocodeFmpPrivate->ImageDescriptor, MicrocodeFmpPrivate->DescriptorCount * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR));
    ZeroMem(MicrocodeFmpPrivate->MicrocodeInfo, MicrocodeFmpPrivate->DescriptorCount * sizeof(MICROCODE_INFO));
  }

  MicrocodeFmpPrivate->DescriptorCount = CurrentMicrocodeCount;

  if (MicrocodeFmpPrivate->ImageDescriptor == NULL) {
    MicrocodeFmpPrivate->ImageDescriptor = AllocateZeroPool(MicrocodeFmpPrivate->DescriptorCount * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR));
    if (MicrocodeFmpPrivate->ImageDescriptor == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }
  if (MicrocodeFmpPrivate->MicrocodeInfo == NULL) {
    MicrocodeFmpPrivate->MicrocodeInfo = AllocateZeroPool(MicrocodeFmpPrivate->DescriptorCount * sizeof(MICROCODE_INFO));
    if (MicrocodeFmpPrivate->MicrocodeInfo == NULL) {
      FreePool (MicrocodeFmpPrivate->ImageDescriptor);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  CurrentMicrocodeCount = (UINT8)GetMicrocodeInfo (MicrocodeFmpPrivate, MicrocodeFmpPrivate->DescriptorCount, MicrocodeFmpPrivate->ImageDescriptor, MicrocodeFmpPrivate->MicrocodeInfo);
  ASSERT(CurrentMicrocodeCount == MicrocodeFmpPrivate->DescriptorCount);

  InitializedProcessorMicrocodeIndex (MicrocodeFmpPrivate);

  Status = InitializeFitMicrocodeInfo (MicrocodeFmpPrivate);
  if (EFI_ERROR(Status)) {
    FreePool (MicrocodeFmpPrivate->ImageDescriptor);
    FreePool (MicrocodeFmpPrivate->MicrocodeInfo);
    DEBUG((DEBUG_ERROR, "InitializeFitMicrocodeInfo - %r\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Initialize MicrocodeFmpDriver multiprocessor information.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.

  @return EFI_SUCCESS           Processor information is initialized.
  @return EFI_OUT_OF_RESOURCES  No enough resource for the initialization.
**/
EFI_STATUS
InitializeProcessorInfo (
  IN MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate
  )
{
  EFI_STATUS                           Status;
  EFI_MP_SERVICES_PROTOCOL             *MpService;
  UINTN                                NumberOfProcessors;
  UINTN                                NumberOfEnabledProcessors;
  UINTN                                Index;
  UINTN                                BspIndex;

  Status = gBS->LocateProtocol (&gEfiMpServiceProtocolGuid, NULL, (VOID **)&MpService);
  ASSERT_EFI_ERROR(Status);

  MicrocodeFmpPrivate->MpService = MpService;
  MicrocodeFmpPrivate->ProcessorCount = 0;
  MicrocodeFmpPrivate->ProcessorInfo = NULL;

  Status = MpService->GetNumberOfProcessors (MpService, &NumberOfProcessors, &NumberOfEnabledProcessors);
  ASSERT_EFI_ERROR(Status);
  MicrocodeFmpPrivate->ProcessorCount = NumberOfProcessors;

  Status = MpService->WhoAmI (MpService, &BspIndex);
  ASSERT_EFI_ERROR(Status);
  MicrocodeFmpPrivate->BspIndex = BspIndex;

  MicrocodeFmpPrivate->ProcessorInfo = AllocateZeroPool (sizeof(PROCESSOR_INFO) * MicrocodeFmpPrivate->ProcessorCount);
  if (MicrocodeFmpPrivate->ProcessorInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < NumberOfProcessors; Index++) {
    MicrocodeFmpPrivate->ProcessorInfo[Index].CpuIndex = Index;
    MicrocodeFmpPrivate->ProcessorInfo[Index].MicrocodeIndex = (UINTN)-1;
    if (Index == BspIndex) {
      CollectProcessorInfo (&MicrocodeFmpPrivate->ProcessorInfo[Index]);
    } else {
      Status = MpService->StartupThisAP (
                            MpService,
                            CollectProcessorInfo,
                            Index,
                            NULL,
                            0,
                            &MicrocodeFmpPrivate->ProcessorInfo[Index],
                            NULL
                            );
      ASSERT_EFI_ERROR(Status);
    }
  }

  return EFI_SUCCESS;
}

/**
  Dump private information.

  @param[in] MicrocodeFmpPrivate private data structure.
**/
VOID
DumpPrivateInfo (
  IN MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate
  )
{
  UINTN                                Index;
  PROCESSOR_INFO                       *ProcessorInfo;
  MICROCODE_INFO                       *MicrocodeInfo;
  EFI_FIRMWARE_IMAGE_DESCRIPTOR        *ImageDescriptor;
  FIT_MICROCODE_INFO                   *FitMicrocodeInfo;

  DEBUG ((DEBUG_INFO, "ProcessorInfo:\n"));
  DEBUG ((DEBUG_INFO, "  ProcessorCount - 0x%x\n", MicrocodeFmpPrivate->ProcessorCount));
  DEBUG ((DEBUG_INFO, "  BspIndex - 0x%x\n", MicrocodeFmpPrivate->BspIndex));

  ProcessorInfo = MicrocodeFmpPrivate->ProcessorInfo;
  for (Index = 0; Index < MicrocodeFmpPrivate->ProcessorCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "  ProcessorInfo[0x%x] - 0x%08x, 0x%02x, 0x%08x, (0x%x)\n",
      ProcessorInfo[Index].CpuIndex,
      ProcessorInfo[Index].ProcessorSignature,
      ProcessorInfo[Index].PlatformId,
      ProcessorInfo[Index].MicrocodeRevision,
      ProcessorInfo[Index].MicrocodeIndex
      ));
  }

  DEBUG ((DEBUG_INFO, "MicrocodeInfo:\n"));
  MicrocodeInfo = MicrocodeFmpPrivate->MicrocodeInfo;
  DEBUG ((DEBUG_INFO, "  MicrocodeRegion - 0x%x - 0x%x\n", MicrocodeFmpPrivate->MicrocodePatchAddress, MicrocodeFmpPrivate->MicrocodePatchRegionSize));
  DEBUG ((DEBUG_INFO, "  MicrocodeCount - 0x%x\n", MicrocodeFmpPrivate->DescriptorCount));
  for (Index = 0; Index < MicrocodeFmpPrivate->DescriptorCount; Index++) {
    DEBUG ((
      DEBUG_INFO,
      "  MicrocodeInfo[0x%x] - 0x%08x, 0x%08x, (0x%x)\n",
      Index,
      MicrocodeInfo[Index].MicrocodeEntryPoint,
      MicrocodeInfo[Index].TotalSize,
      MicrocodeInfo[Index].InUse
      ));
  }

  ImageDescriptor = MicrocodeFmpPrivate->ImageDescriptor;
  DEBUG ((DEBUG_VERBOSE, "ImageDescriptor:\n"));
  for (Index = 0; Index < MicrocodeFmpPrivate->DescriptorCount; Index++) {
    DEBUG((DEBUG_VERBOSE, "  ImageDescriptor (%d)\n", Index));
    DEBUG((DEBUG_VERBOSE, "    ImageIndex                  - 0x%x\n", ImageDescriptor[Index].ImageIndex));
    DEBUG((DEBUG_VERBOSE, "    ImageTypeId                 - %g\n", &ImageDescriptor[Index].ImageTypeId));
    DEBUG((DEBUG_VERBOSE, "    ImageId                     - 0x%lx\n", ImageDescriptor[Index].ImageId));
    DEBUG((DEBUG_VERBOSE, "    ImageIdName                 - %s\n", ImageDescriptor[Index].ImageIdName));
    DEBUG((DEBUG_VERBOSE, "    Version                     - 0x%x\n", ImageDescriptor[Index].Version));
    DEBUG((DEBUG_VERBOSE, "    VersionName                 - %s\n", ImageDescriptor[Index].VersionName));
    DEBUG((DEBUG_VERBOSE, "    Size                        - 0x%x\n", ImageDescriptor[Index].Size));
    DEBUG((DEBUG_VERBOSE, "    AttributesSupported         - 0x%lx\n", ImageDescriptor[Index].AttributesSupported));
    DEBUG((DEBUG_VERBOSE, "    AttributesSetting           - 0x%lx\n", ImageDescriptor[Index].AttributesSetting));
    DEBUG((DEBUG_VERBOSE, "    Compatibilities             - 0x%lx\n", ImageDescriptor[Index].Compatibilities));
    DEBUG((DEBUG_VERBOSE, "    LowestSupportedImageVersion - 0x%x\n", ImageDescriptor[Index].LowestSupportedImageVersion));
    DEBUG((DEBUG_VERBOSE, "    LastAttemptVersion          - 0x%x\n", ImageDescriptor[Index].LastAttemptVersion));
    DEBUG((DEBUG_VERBOSE, "    LastAttemptStatus           - 0x%x\n", ImageDescriptor[Index].LastAttemptStatus));
    DEBUG((DEBUG_VERBOSE, "    HardwareInstance            - 0x%lx\n", ImageDescriptor[Index].HardwareInstance));
  }

  if (MicrocodeFmpPrivate->FitMicrocodeInfo != NULL) {
    DEBUG ((DEBUG_INFO, "FitMicrocodeInfo:\n"));
    FitMicrocodeInfo = MicrocodeFmpPrivate->FitMicrocodeInfo;
    DEBUG ((DEBUG_INFO, "  FitMicrocodeEntryCount - 0x%x\n", MicrocodeFmpPrivate->FitMicrocodeEntryCount));
    for (Index = 0; Index < MicrocodeFmpPrivate->FitMicrocodeEntryCount; Index++) {
      DEBUG ((
        DEBUG_INFO,
        "  FitMicrocodeInfo[0x%x] - 0x%08x, 0x%08x, (0x%x, 0x%x)\n",
        Index,
        FitMicrocodeInfo[Index].MicrocodeEntryPoint,
        FitMicrocodeInfo[Index].TotalSize,
        FitMicrocodeInfo[Index].InUse,
        FitMicrocodeInfo[Index].Empty
        ));
    }
  }
}

/**
  Initialize MicrocodeFmpDriver private data structure.

  @param[in] MicrocodeFmpPrivate private data structure to be initialized.

  @return EFI_SUCCESS private data is initialized.
**/
EFI_STATUS
InitializePrivateData (
  IN MICROCODE_FMP_PRIVATE_DATA  *MicrocodeFmpPrivate
  )
{
  EFI_STATUS       Status;
  EFI_STATUS       VarStatus;
  UINTN            VarSize;
  BOOLEAN          Result;

  MicrocodeFmpPrivate->Signature       = MICROCODE_FMP_PRIVATE_DATA_SIGNATURE;
  MicrocodeFmpPrivate->Handle          = NULL;
  CopyMem(&MicrocodeFmpPrivate->Fmp, &mFirmwareManagementProtocol, sizeof(EFI_FIRMWARE_MANAGEMENT_PROTOCOL));

  MicrocodeFmpPrivate->PackageVersion = 0x1;
  MicrocodeFmpPrivate->PackageVersionName = L"Microcode";

  MicrocodeFmpPrivate->LastAttempt.LastAttemptVersion = 0x0;
  MicrocodeFmpPrivate->LastAttempt.LastAttemptStatus = 0x0;
  VarSize = sizeof(MicrocodeFmpPrivate->LastAttempt);
  VarStatus = gRT->GetVariable(
                     MICROCODE_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                     &gEfiCallerIdGuid,
                     NULL,
                     &VarSize,
                     &MicrocodeFmpPrivate->LastAttempt
                     );
  DEBUG((DEBUG_INFO, "GetLastAttempt - %r\n", VarStatus));
  DEBUG((DEBUG_INFO, "GetLastAttempt Version - 0x%x, State - 0x%x\n", MicrocodeFmpPrivate->LastAttempt.LastAttemptVersion, MicrocodeFmpPrivate->LastAttempt.LastAttemptStatus));

  Result = GetMicrocodeRegion(&MicrocodeFmpPrivate->MicrocodePatchAddress, &MicrocodeFmpPrivate->MicrocodePatchRegionSize);
  if (!Result) {
    DEBUG((DEBUG_ERROR, "Fail to get Microcode Region\n"));
    return EFI_NOT_FOUND;
  }

  Status = InitializeProcessorInfo (MicrocodeFmpPrivate);
  if (EFI_ERROR(Status)) {
    DEBUG((DEBUG_ERROR, "InitializeProcessorInfo - %r\n", Status));
    return Status;
  }

  Status = InitializeMicrocodeDescriptor(MicrocodeFmpPrivate);
  if (EFI_ERROR(Status)) {
    FreePool (MicrocodeFmpPrivate->ProcessorInfo);
    DEBUG((DEBUG_ERROR, "InitializeMicrocodeDescriptor - %r\n", Status));
    return Status;
  }

  DumpPrivateInfo (MicrocodeFmpPrivate);

  return Status;
}

/**
  Microcode FMP module entrypoint

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @return EFI_SUCCESS Microcode FMP module is initialized.
**/
EFI_STATUS
EFIAPI
MicrocodeFmpMain (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                            Status;

  //
  // Initialize MicrocodeFmpPrivateData
  //
  mMicrocodeFmpPrivate = AllocateZeroPool (sizeof(MICROCODE_FMP_PRIVATE_DATA));
  if (mMicrocodeFmpPrivate == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = InitializePrivateData(mMicrocodeFmpPrivate);
  if (EFI_ERROR(Status)) {
    FreePool(mMicrocodeFmpPrivate);
    mMicrocodeFmpPrivate = NULL;
    return Status;
  }

  //
  // Install FMP protocol.
  //
  Status = gBS->InstallProtocolInterface (
                  &mMicrocodeFmpPrivate->Handle,
                  &gEfiFirmwareManagementProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mMicrocodeFmpPrivate->Fmp
                  );
  if (EFI_ERROR (Status)) {
    FreePool(mMicrocodeFmpPrivate);
    mMicrocodeFmpPrivate = NULL;
    return Status;
  }

  return Status;
}
