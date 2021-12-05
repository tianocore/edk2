/** @file
  System Firmware update header file.

  Copyright (c) 2016 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SYSTEM_FIRMWARE_UPDATE_H_
#define _SYSTEM_FIRMWARE_UPDATE_H_

#include <PiDxe.h>

#include <Guid/SystemResourceTable.h>
#include <Guid/FirmwareContentsSigned.h>
#include <Guid/WinCertificate.h>
#include <Guid/EdkiiSystemFmpCapsule.h>

#include <Protocol/FirmwareManagement.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DevicePathLib.h>
#include <Library/HobLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/PlatformFlashAccessLib.h>
#include <Library/EdkiiSystemCapsuleLib.h>

typedef struct {
  UINT32    LastAttemptVersion;
  UINT32    LastAttemptStatus;
} SYSTEM_FMP_LAST_ATTEMPT_VARIABLE;

#define SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_NAME  L"SystemLastAttempVar"

#define SYSTEM_FMP_LAST_ATTEMPT_VARIABLE_GUID  {0x2f564d6f, 0xcc2c, 0x4838, { 0xb9, 0xa8, 0xbe, 0x59, 0x48, 0xb0, 0x3d, 0x59 }}

#define SYSTEM_FMP_PRIVATE_DATA_SIGNATURE  SIGNATURE_32('S', 'Y', 'S', 'F')

#define SYSTEM_FMP_PROTOCOL_GUID  {0x6d16624a, 0x26a6, 0x4cb4, { 0x84, 0xfa, 0x6, 0x78, 0x5a, 0x7e, 0x82, 0x6a }}

//
// SYSTEM FMP private data structure.
//

struct _SYSTEM_FMP_PRIVATE_DATA {
  UINT32                                    Signature;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL          Fmp;
  EFI_HANDLE                                Handle;
  UINT8                                     DescriptorCount;
  EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR    *ImageDescriptor;
  SYSTEM_FMP_LAST_ATTEMPT_VARIABLE          LastAttempt;
};

typedef struct _SYSTEM_FMP_PRIVATE_DATA SYSTEM_FMP_PRIVATE_DATA;

/**
  Returns a pointer to the SYSTEM_FMP_PRIVATE_DATA structure from the input a as Fmp.

  If the signatures matches, then a pointer to the data structure that contains
  a specified field of that data structure is returned.

  @param  a              Pointer to the field specified by ServiceBinding within
                         a data structure of type SYSTEM_FMP_PRIVATE_DATA.

**/
#define SYSTEM_FMP_PRIVATE_DATA_FROM_FMP(a) \
  CR ( \
  (a), \
  SYSTEM_FMP_PRIVATE_DATA, \
  Fmp, \
  SYSTEM_FMP_PRIVATE_DATA_SIGNATURE \
  )

//
// Update data
//

typedef struct {
  UINTN    NumOfUpdates;
} CONFIG_HEADER;

typedef struct {
  UINTN                     Index;
  PLATFORM_FIRMWARE_TYPE    FirmwareType;
  FLASH_ADDRESS_TYPE        AddressType;
  EFI_GUID                  FileGuid;
  EFI_PHYSICAL_ADDRESS      BaseAddress;
  UINTN                     Length;
  UINTN                     ImageOffset;
} UPDATE_CONFIG_DATA;

//
// System Firmware Update SMM Communication
//

#define SYSTEM_FIRMWARE_UPDATE_COMMUNICATION_FUNCTION_SET_IMAGE  1

typedef struct {
  UINTN         Function;
  EFI_STATUS    ReturnStatus;
  // UINT8       Data[];
} SYSTEM_FIRMWARE_UPDATE_COMMUNICATION_HEAD;

#define  ABORT_REASON_MAX_SIZE  0x40              // UnicodeStringSize including final L'\0'

#define  CAPSULE_IMAGE_ADDITIONAL_MAX_SIZE  (0x20020 + 0xA0000) // Additional size for Capsule Header, FV block alignment + DispatchImage.

typedef struct {
  UINT8     ImageIndex;
  UINTN     ImageSize;
  UINTN     AbortReasonSize;
  UINT32    LastAttemptVersion;
  UINT32    LastAttemptStatus;
  // UINT8       Data[AbortReasonMaxSize + ImageSize];
} SYSTEM_FIRMWARE_UPDATE_COMMUNICATION_SET_IMAGE;

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
  IN EFI_FIRMWARE_MANAGEMENT_PROTOCOL      *This,
  IN OUT    UINTN                          *ImageInfoSize,
  IN OUT    EFI_FIRMWARE_IMAGE_DESCRIPTOR  *ImageInfo,
  OUT       UINT32                         *DescriptorVersion,
  OUT       UINT8                          *DescriptorCount,
  OUT       UINTN                          *DescriptorSize,
  OUT       UINT32                         *PackageVersion,
  OUT       CHAR16                         **PackageVersionName
  );

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
  );

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
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL               *This,
  IN  UINT8                                          ImageIndex,
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  OUT CHAR16                                         **AbortReason
  );

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
  );

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
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  OUT UINT32                            *PackageVersion,
  OUT CHAR16                            **PackageVersionName,
  OUT UINT32                            *PackageVersionNameMaxLen,
  OUT UINT64                            *AttributesSupported,
  OUT UINT64                            *AttributesSetting
  );

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
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  CONST VOID                        *Image,
  IN  UINTN                             ImageSize,
  IN  CONST VOID                        *VendorCode,
  IN  UINT32                            PackageVersion,
  IN  CONST CHAR16                      *PackageVersionName
  );

/**
  Initialize SystemFmpDriver private data structure.

  @param[in] SystemFmpPrivate  private data structure to be initialized.

  @return EFI_SUCCESS private data is initialized.
**/
EFI_STATUS
InitializePrivateData (
  IN SYSTEM_FMP_PRIVATE_DATA  *SystemFmpPrivate
  );

extern EFI_GUID  gSystemFmpLastAttemptVariableGuid;
extern EFI_GUID  mCurrentImageTypeId;
extern EFI_GUID  gSystemFmpProtocolGuid;

#endif
