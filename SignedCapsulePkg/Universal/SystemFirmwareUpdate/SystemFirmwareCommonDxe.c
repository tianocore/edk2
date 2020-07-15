/** @file
  Produce FMP instance for system firmware.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SystemFirmwareDxe.h"

EFI_GUID gSystemFmpLastAttemptVariableGuid = SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_GUID;
EFI_GUID gSystemFmpProtocolGuid = SYSTEM_FMP_PROTOCOL_GUID;

EFI_FIRMWARE_MANAGEMENT_PROTOCOL mFirmwareManagementProtocol = {
  FmpGetImageInfo,
  FmpGetImage,
  FmpSetImage,
  FmpCheckImage,
  FmpGetPackageInfo,
  FmpSetPackageInfo
};

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
  IN        EFI_FIRMWARE_MANAGEMENT_PROTOCOL *This,
  IN OUT    UINTN                            *ImageInfoSize,
  IN OUT    EFI_FIRMWARE_IMAGE_DESCRIPTOR    *ImageInfo,
  OUT       UINT32                           *DescriptorVersion,
  OUT       UINT8                            *DescriptorCount,
  OUT       UINTN                            *DescriptorSize,
  OUT       UINT32                           *PackageVersion,
  OUT       CHAR16                           **PackageVersionName
  )
{
  SYSTEM_FMP_PRIVATE_DATA                *SystemFmpPrivate;
  EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR *ImageDescriptor;

  SystemFmpPrivate = SYSTEM_FMP_PRIVATE_DATA_FROM_FMP(This);

  if(ImageInfoSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (*ImageInfoSize < sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * SystemFmpPrivate->DescriptorCount) {
    *ImageInfoSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * SystemFmpPrivate->DescriptorCount;
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

  *ImageInfoSize      = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR) * SystemFmpPrivate->DescriptorCount;
  *DescriptorSize     = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);
  *DescriptorCount    = SystemFmpPrivate->DescriptorCount;
  *DescriptorVersion  = EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION;

  //
  // supports 1 ImageInfo descriptor
  //
  ImageDescriptor = SystemFmpPrivate->ImageDescriptor;
  ImageInfo->ImageIndex = ImageDescriptor->ImageIndex;
  CopyGuid (&ImageInfo->ImageTypeId, &ImageDescriptor->ImageTypeId);
  ImageInfo->ImageId = ImageDescriptor->ImageId;
  if (ImageDescriptor->ImageIdNameStringOffset != 0) {
    ImageInfo->ImageIdName = (CHAR16 *)((UINTN)ImageDescriptor + ImageDescriptor->ImageIdNameStringOffset);
  } else {
    ImageInfo->ImageIdName = NULL;
  }
  ImageInfo->Version = ImageDescriptor->Version;
  if (ImageDescriptor->VersionNameStringOffset != 0) {
    ImageInfo->VersionName = (CHAR16 *)((UINTN)ImageDescriptor + ImageDescriptor->VersionNameStringOffset);
  } else {
    ImageInfo->VersionName = NULL;
  }
  ImageInfo->Size = (UINTN)ImageDescriptor->Size;
  ImageInfo->AttributesSupported = ImageDescriptor->AttributesSupported;
  ImageInfo->AttributesSetting = ImageDescriptor->AttributesSetting;
  ImageInfo->Compatibilities = ImageDescriptor->Compatibilities;
  ImageInfo->LowestSupportedImageVersion = ImageDescriptor->LowestSupportedImageVersion;
  ImageInfo->LastAttemptVersion = SystemFmpPrivate->LastAttempt.LastAttemptVersion;
  ImageInfo->LastAttemptStatus = SystemFmpPrivate->LastAttempt.LastAttemptStatus;
  ImageInfo->HardwareInstance = ImageDescriptor->HardwareInstance;

  //
  // package version
  //
  *PackageVersion = ImageDescriptor->PackageVersion;
  if (ImageDescriptor->PackageVersionNameStringOffset != 0) {
    *PackageVersionName = (VOID *)((UINTN)ImageDescriptor + ImageDescriptor->PackageVersionNameStringOffset);
    *PackageVersionName = AllocateCopyPool(StrSize(*PackageVersionName), *PackageVersionName);
  } else {
    *PackageVersionName = NULL;
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
  return EFI_UNSUPPORTED;
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
  Initialize SystemFmpDriver private data structure.

  @param[in] SystemFmpPrivate  private data structure to be initialized.

  @return EFI_SUCCESS private data is initialized.
**/
EFI_STATUS
InitializePrivateData (
  IN SYSTEM_FMP_PRIVATE_DATA  *SystemFmpPrivate
  )
{
  EFI_STATUS       VarStatus;
  UINTN            VarSize;

  SystemFmpPrivate->Signature       = SYSTEM_FMP_PRIVATE_DATA_SIGNATURE;
  SystemFmpPrivate->Handle          = NULL;
  SystemFmpPrivate->DescriptorCount = 1;
  CopyMem(&SystemFmpPrivate->Fmp, &mFirmwareManagementProtocol, sizeof(EFI_FIRMWARE_MANAGEMENT_PROTOCOL));

  SystemFmpPrivate->ImageDescriptor = PcdGetPtr(PcdEdkiiSystemFirmwareImageDescriptor);

  SystemFmpPrivate->LastAttempt.LastAttemptVersion = 0x0;
  SystemFmpPrivate->LastAttempt.LastAttemptStatus = 0x0;
  VarSize = sizeof(SystemFmpPrivate->LastAttempt);
  VarStatus = gRT->GetVariable(
                     SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_NAME,
                     &gSystemFmpLastAttemptVariableGuid,
                     NULL,
                     &VarSize,
                     &SystemFmpPrivate->LastAttempt
                     );
  DEBUG((DEBUG_INFO, "GetLastAttempt - %r\n", VarStatus));
  DEBUG((DEBUG_INFO, "GetLastAttempt Version - 0x%x, State - 0x%x\n", SystemFmpPrivate->LastAttempt.LastAttemptVersion, SystemFmpPrivate->LastAttempt.LastAttemptStatus));

  return EFI_SUCCESS;
}


