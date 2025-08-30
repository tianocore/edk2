/** @file
  Firmware Management Protocol - supports Get Fw Info, Sending/Receiving FMP commands
  supports Set Fw Image, Activate Fw image
  SetPkgInfo, GetPkgInfo, CheckImg
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"

/**
  Returns information about the current firmware image(s) of the device.

  This function allows a copy of the current firmware image to be created and saved.
  The saved copy could later been used, for example, in firmware image recovery or rollback.

  @param[in]      This               A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in, out] ImageInfoSize      A pointer to the Size, in bytes, of the ImageInfo buffer.
                                     On input, this is the Size of the buffer allocated by the caller.
                                     On output, it is the Size of the buffer returned by the firmware
                                     if the buffer was large enough, or the Size of the buffer needed
                                     to contain the image(s) information if the buffer was too small.
  @param[in, out] ImageInfo          A pointer to the buffer in which firmware places the current image(s)
                                     information. The information is an array of EFI_FIRMWARE_IMAGE_DESCRIPTORs.
  @param[out]     DescriptorVersion  A pointer to the location in which firmware returns the version number
                                     associated with the EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]     DescriptorCount    A pointer to the location in which firmware returns the number of
                                     descriptors or firmware images within this device.
  @param[out]     DescriptorSize     A pointer to the location in which firmware returns the Size, in bytes,
                                     of an individual EFI_FIRMWARE_IMAGE_DESCRIPTOR.
  @param[out]     PackageVersion     A version number that represents all the firmware images in the device.
                                     The format is Vendor specific and new version must have a greater Value
                                     than the old version. If PackageVersion is not supported, the Value is
                                     0xFFFFFFFF. A Value of 0xFFFFFFFE indicates that package version comparison
                                     is to be performed using PackageVersionName. A Value of 0xFFFFFFFD indicates
                                     that package version update is in progress.
  @param[out]     PackageVersionName A pointer to a pointer to a null-terminated string representing the
                                     package version name. The buffer is allocated by this function with
                                     AllocatePool(), and it is the caller's responsibility to free it with a call
                                     to FreePool().

  @retval EFI_SUCCESS                The device was successfully updated with the new image.
  @retval EFI_BUFFER_TOO_SMALL       The ImageInfo buffer was too small. The current buffer Size
                                     needed to hold the image(s) information is returned in ImageInfoSize.
  @retval EFI_INVALID_PARAMETER      ImageInfoSize is NULL.
  @retval EFI_DEVICE_ERROR           Valid information could not be returned. Possible corrupted image.

**/
EFI_STATUS
EFIAPI
CxlFirmwareMgmtGetImageInfo (
  IN EFI_FIRMWARE_MANAGEMENT_PROTOCOL      *This,
  IN OUT    UINTN                          *ImageInfoSize,
  IN OUT    EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageInfo,
  OUT       UINT32                         *DescriptorVersion,
  OUT       UINT8                          *DescriptorCount,
  OUT       UINTN                          *DescriptorSize,
  OUT       UINT32                         *PackageVersion,
  OUT       CHAR16                         **PackageVersionName
  )
{
  DEBUG ((EFI_D_INFO, "CxlFirmwareMgmtGetImageInfo: Entering CxlFirmwareMgmtGetImageInfo...\n"));
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (This);

  if (((Private->SlotInfo.NumberOfSlots) * sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR)) > *ImageInfoSize) {
    *ImageInfoSize = (Private->SlotInfo.NumberOfSlots * sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR));
    return EFI_BUFFER_TOO_SMALL;
  }

  *PackageVersionName = AllocateZeroPool (CXL_STRING_BUFFER_WIDTH);
  if (NULL == *PackageVersionName) {
    DEBUG ((DEBUG_ERROR, "CxlFirmwareMgmtGetImageInfo: EFI Out of resources...\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem (ImageInfo, &Private->SlotInfo.FwImageDescriptor, Private->SlotInfo.NumberOfSlots * sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR));

  *DescriptorVersion = EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION;
  *DescriptorCount   = Private->SlotInfo.NumberOfSlots;
  *DescriptorSize    = sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR);

  StrCpyS (*PackageVersionName, CXL_STRING_BUFFER_WIDTH, Private->PackageVersionName);
  *PackageVersion = Private->PackageVersion;
  return EFI_SUCCESS;
}

/**
  Retrieves a copy of the current firmware image of the device.

  This function allows a copy of the current firmware image to be created and saved.
  The saved copy could later been used, for example, in firmware image recovery or rollback.

  @param[in]      This           A pointer to the EFI_FIRMWARE_MANAGEMENT_PROTOCOL instance.
  @param[in]      ImageIndex     A unique number identifying the firmware image(s) within the device.
                                 The number is between 1 and DescriptorCount.
  @param[in, out] Image          Points to the buffer where the current image is copied to.
  @param[in, out] ImageSize      On entry, points to the size of the buffer pointed to by Image, in bytes.
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
CxlFirmwareMgmtGetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  UINT8                             ImageIndex,
  IN  OUT  VOID                         *Image,
  IN  OUT  UINTN                        *ImageSize
  )
{
  EFI_STATUS                   Status   = EFI_SUCCESS;
  CXL_CONTROLLER_PRIVATE_DATA  *Private = NULL;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (This);
  if ((ImageIndex > Private->SlotInfo.NumberOfSlots) || (Private->SlotInfo.IsSetImageDone[ImageIndex] != TRUE)) {
    DEBUG (
      (DEBUG_ERROR, "[%a]: ImageIndex = %d, NumberOfSlots = %d, IsSetImageDone = %d \n",
       __func__, ImageIndex, Private->SlotInfo.NumberOfSlots, Private->SlotInfo.IsSetImageDone[ImageIndex])
      );
    return EFI_INVALID_PARAMETER;
  }

  if (Private->SlotInfo.ImageFileSize[ImageIndex] > *ImageSize) {
    *ImageSize = Private->SlotInfo.ImageFileSize[ImageIndex];
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Image != NULL) {
    CopyMem (Image, Private->SlotInfo.ImageFileBuffer[ImageIndex], sizeof (*ImageSize));
  } else {
    *ImageSize = Private->SlotInfo.ImageFileSize[ImageIndex];
    return EFI_BUFFER_TOO_SMALL;
  }

  return Status;
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
CxlFirmwareMgmtSetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL               *This,
  IN  UINT8                                          ImageIndex,
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  OUT CHAR16                                         **AbortReason
  )
{
  EFI_STATUS                   Status;
  CXL_CONTROLLER_PRIVATE_DATA  *Private = NULL;
  UINT32                       Written;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (This);
  if (ImageIndex > Private->SlotInfo.NumberOfSlots) {
    DEBUG (
      (DEBUG_ERROR, "CxlFirmwareMgmtSetImage: ImageIndex = %d is greater then Total slots = %d \n",
       ImageIndex, Private->SlotInfo.NumberOfSlots)
      );
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  Status = CxlMemTransferFw (Private, ImageIndex, Image, 0, (UINT32)ImageSize, &Written);

  if (Status != EFI_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "CxlFirmwareMgmtSetImage: UEFI Driver Transfer FW (Opcode 0201h) Failed!\n"));
    return Status;
  }

  Private->SlotInfo.ImageFileBuffer[ImageIndex] = AllocateZeroPool (ImageSize);
  if (Private->SlotInfo.ImageFileBuffer[ImageIndex] == NULL) {
    DEBUG ((DEBUG_ERROR, "CxlFirmwareMgmtSetImage: AllocateZeroPool failed!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  Private->SlotInfo.ImageFileSize[ImageIndex] = ImageSize;
  CopyMem (Private->SlotInfo.ImageFileBuffer[ImageIndex], Image, sizeof (ImageSize));
  Private->SlotInfo.IsSetImageDone[ImageIndex] = TRUE;

  DEBUG ((EFI_D_INFO, "[CxlFirmwareMgmtSetImage] SetImage returns...%r\n", Status));
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

  @retval EFI_UNSUPPORTED        The operation is not supported.

**/
EFI_STATUS
EFIAPI
CxlFirmwareMgmtCheckImage (
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
CxlFirmwareMgmtGetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  OUT UINT32                            *PackageVersion,
  OUT CHAR16                            **PackageVersionName,
  OUT UINT32                            *PackageVersionNameMaxLen,
  OUT UINT64                            *AttributesSupported,
  OUT UINT64                            *AttributesSetting
  )
{
  DEBUG ((EFI_D_INFO, "CxlFirmwareMgmtGetPackageInfo: Entering CxlFirmwareMgmtGetPackageInfo...\n"));
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (This);

  if (Private->PackageVersionName != NULL) {
    StrCpyS (*PackageVersionName, CXL_STRING_BUFFER_WIDTH, Private->PackageVersionName);
  }

  *PackageVersion      = Private->PackageVersion;
  *AttributesSupported = Private->SlotInfo.FwImageDescriptor[0].AttributesSupported;
  *AttributesSetting   = Private->SlotInfo.FwImageDescriptor[0].AttributesSetting;

  return EFI_SUCCESS;
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
CxlFirmwareMgmtSetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  CONST VOID                        *Image,
  IN  UINTN                             ImageSize,
  IN  CONST VOID                        *VendorCode,
  IN  UINT32                            PackageVersion,
  IN  CONST CHAR16                      *PackageVersionName
  )
{
  DEBUG ((EFI_D_INFO, "CxlFirmwareMgmtSetPackageInfo: Entering CxlFirmwareMgmtSetPackageInfo...\n"));
  CXL_CONTROLLER_PRIVATE_DATA  *Private;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT (This);
  StrCpyS (Private->PackageVersionName, CXL_STRING_BUFFER_WIDTH, PackageVersionName);
  Private->PackageVersion = PackageVersion;
  return EFI_SUCCESS;
}

//
// Template of the private context structure for the Firmware Management Protocol instance
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_FIRMWARE_MANAGEMENT_PROTOCOL  gCxlFirmwareManagement = {
  CxlFirmwareMgmtGetImageInfo,
  CxlFirmwareMgmtGetImage,
  CxlFirmwareMgmtSetImage,
  CxlFirmwareMgmtCheckImage,
  CxlFirmwareMgmtGetPackageInfo,
  CxlFirmwareMgmtSetPackageInfo
};


