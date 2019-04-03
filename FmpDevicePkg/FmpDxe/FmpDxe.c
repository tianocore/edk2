/** @file
  Produces a Firmware Management Protocol that supports updates to a firmware
  image stored in a firmware device with platform and firmware device specific
  information provided through PCDs and libraries.

  Copyright (c) 2016, Microsoft Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/FmpAuthenticationLib.h>
#include <Library/FmpDeviceLib.h>
#include <Library/FmpPayloadHeaderLib.h>
#include <Library/CapsuleUpdatePolicyLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/FirmwareManagementProgress.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/EventGroup.h>
#include "VariableSupport.h"

#define VERSION_STRING_NOT_SUPPORTED  L"VERSION STRING NOT SUPPORTED"
#define VERSION_STRING_NOT_AVAILABLE  L"VERSION STRING NOT AVAILABLE"

/**
  Check to see if any of the keys in PcdFmpDevicePkcs7CertBufferXdr matches
  the test key.  PcdFmpDeviceTestKeySha256Digest contains the SHA256 hash of
  the test key.  For each key in PcdFmpDevicePkcs7CertBufferXdr, compute the
  SHA256 hash and compare it to PcdFmpDeviceTestKeySha256Digest.  If the
  SHA256 hash matches or there is then error computing the SHA256 hash, then
  set PcdTestKeyUsed to TRUE.  Skip this check if PcdTestKeyUsed is already
  TRUE or PcdFmpDeviceTestKeySha256Digest is not exactly SHA256_DIGEST_SIZE
  bytes.
**/
VOID
DetectTestKey (
  VOID
  );

///
/// FILE_GUID from FmpDxe.inf.  When FmpDxe.inf is used in a platform, the
/// FILE_GUID must always be overridden in the <Defines> section to provide
/// the ESRT GUID value associated with the updatable firmware image.  A
/// check is made in this module's driver entry point to verify that a
/// new FILE_GUID value has been defined.
///
const EFI_GUID  mDefaultModuleFileGuid = {
  0x78ef0a56, 0x1cf0, 0x4535, { 0xb5, 0xda, 0xf6, 0xfd, 0x2f, 0x40, 0x5a, 0x11 }
};

EFI_FIRMWARE_IMAGE_DESCRIPTOR  mDesc;
BOOLEAN                        mDescriptorPopulated     = FALSE;
BOOLEAN                        mRuntimeVersionSupported = TRUE;
BOOLEAN                        mFmpInstalled            = FALSE;

///
/// Function pointer to progress function
///
EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  mProgressFunc      = NULL;
BOOLEAN                                        mProgressSupported = FALSE;

CHAR16  *mImageIdName = NULL;
UINT64  mImageId      = 0x1;
CHAR16  *mVersionName = NULL;

EFI_EVENT  mFmpDeviceLockEvent;
//
// Indicates if an attempt has been made to lock a
// FLASH storage device by calling FmpDeviceLock().
// A FLASH storage device may not support being locked,
// so this variable is set to TRUE even if FmpDeviceLock()
// returns an error.
//
BOOLEAN    mFmpDeviceLocked = FALSE;

/**
  Callback function to report the process of the firmware updating.

  Wrap the caller's version in this so that progress from the device lib is
  within the expected range.  Convert device lib 0% - 100% to 6% - 98%.

  FmpDxe        1% -   5%  for validation
  FmpDeviceLib  6% -  98%  for flashing/update
  FmpDxe       99% - 100%  finish

  @param[in] Completion  A value between 1 and 100 indicating the current
                         completion progress of the firmware update. Completion
                         progress is reported as from 1 to 100 percent. A value
                         of 0 is used by the driver to indicate that progress
                         reporting is not supported.

  @retval  EFI_SUCCESS      The progress was updated.
  @retval  EFI_UNSUPPORTED  Updating progress is not supported.

**/
EFI_STATUS
EFIAPI
FmpDxeProgress (
  IN UINTN  Completion
  )
{
  EFI_STATUS Status;

  Status = EFI_UNSUPPORTED;

  if (!mProgressSupported) {
    return Status;
  }

  if (mProgressFunc == NULL) {
    return Status;
  }

  //
  // Reserve 6% - 98% for the FmpDeviceLib.  Call the real progress function.
  //
  Status = mProgressFunc (((Completion * 92) / 100) + 6);

  if (Status == EFI_UNSUPPORTED) {
    mProgressSupported = FALSE;
    mProgressFunc = NULL;
  }

  return Status;
}

/**
  Returns a pointer to the ImageTypeId GUID value.  An attempt is made to get
  the GUID value from the FmpDeviceLib. If the FmpDeviceLib does not provide
  a GUID value, then gEfiCallerIdGuid is returned.

  @return  The ImageTypeId GUID

**/
EFI_GUID *
GetImageTypeIdGuid (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *FmpDeviceLibGuid;

  FmpDeviceLibGuid = NULL;
  Status = FmpDeviceGetImageTypeIdGuidPtr (&FmpDeviceLibGuid);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      DEBUG ((DEBUG_ERROR, "FmpDxe: FmpDeviceLib GetImageTypeIdGuidPtr() returned invalid error %r\n", Status));
    }
    return &gEfiCallerIdGuid;
  }
  if (FmpDeviceLibGuid == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: FmpDeviceLib GetImageTypeIdGuidPtr() returned invalid GUID\n"));
    return &gEfiCallerIdGuid;
  }
  return FmpDeviceLibGuid;
}

/**
  Returns a pointer to the Null-terminated Unicode ImageIdName string.

  @return  Null-terminated Unicode ImageIdName string.

**/
CHAR16 *
GetImageTypeNameString (
  VOID
  )
{
  return mImageIdName;
}

/**
  Lowest supported version is a combo of three parts.
  1. Check if the device lib has a lowest supported version
  2. Check if we have a variable for lowest supported version (this will be updated with each capsule applied)
  3. Check Fixed at build PCD

  Take the largest value

**/
UINT32
GetLowestSupportedVersion (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      DeviceLibLowestSupportedVersion;
  UINT32      VariableLowestSupportedVersion;
  UINT32      ReturnLsv;

  //
  // Get the LowestSupportedVersion.
  //

  if (!IsLowestSupportedVersionCheckRequired ()) {
    //
    // Any Version can pass the 0 LowestSupportedVersion check.
    //
    return 0;
  }

  ReturnLsv = PcdGet32 (PcdFmpDeviceBuildTimeLowestSupportedVersion);

  //
  // Check the FmpDeviceLib
  //
  DeviceLibLowestSupportedVersion = DEFAULT_LOWESTSUPPORTEDVERSION;
  Status = FmpDeviceGetLowestSupportedVersion (&DeviceLibLowestSupportedVersion);
  if (EFI_ERROR (Status)) {
    DeviceLibLowestSupportedVersion = DEFAULT_LOWESTSUPPORTEDVERSION;
  }

  if (DeviceLibLowestSupportedVersion > ReturnLsv) {
    ReturnLsv = DeviceLibLowestSupportedVersion;
  }

  //
  // Check the lowest supported version UEFI variable for this device
  //
  VariableLowestSupportedVersion = GetLowestSupportedVersionFromVariable();
  if (VariableLowestSupportedVersion > ReturnLsv) {
    ReturnLsv = VariableLowestSupportedVersion;
  }

  //
  // Return the largest value
  //
  return ReturnLsv;
}

/**
  Populates the EFI_FIRMWARE_IMAGE_DESCRIPTOR structure in the module global
  variable mDesc.

**/
VOID
PopulateDescriptor (
  VOID
  )
{
  EFI_STATUS  Status;

  mDesc.ImageIndex = 1;
  CopyGuid (&mDesc.ImageTypeId, GetImageTypeIdGuid());
  mDesc.ImageId = mImageId;
  mDesc.ImageIdName = GetImageTypeNameString();

  //
  // Get the version.  Some devices don't support getting the firmware version
  // at runtime.  If FmpDeviceLib does not support returning a version, then
  // it is stored in a UEFI variable.
  //
  Status = FmpDeviceGetVersion (&mDesc.Version);
  if (Status == EFI_UNSUPPORTED) {
    mRuntimeVersionSupported = FALSE;
    mDesc.Version = GetVersionFromVariable();
  } else if (EFI_ERROR (Status)) {
    //
    // Unexpected error.   Use default version.
    //
    DEBUG ((DEBUG_ERROR, "FmpDxe: GetVersion() from FmpDeviceLib (%s) returned %r\n", GetImageTypeNameString(), Status));
    mDesc.Version = DEFAULT_VERSION;
  }

  //
  // Free the current version name.  Shouldn't really happen but this populate
  // function could be called multiple times (to refresh).
  //
  if (mVersionName != NULL) {
    FreePool (mVersionName);
    mVersionName = NULL;
  }

  //
  // Attempt to get the version string from the FmpDeviceLib
  //
  Status = FmpDeviceGetVersionString (&mVersionName);
  if (Status == EFI_UNSUPPORTED) {
    DEBUG ((DEBUG_INFO, "FmpDxe: GetVersionString() unsupported in FmpDeviceLib.\n"));
    mVersionName = AllocateCopyPool (
                     sizeof (VERSION_STRING_NOT_SUPPORTED),
                     VERSION_STRING_NOT_SUPPORTED
                     );
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "FmpDxe: GetVersionString() not available in FmpDeviceLib.\n"));
    mVersionName = AllocateCopyPool (
                     sizeof (VERSION_STRING_NOT_AVAILABLE),
                     VERSION_STRING_NOT_AVAILABLE
                     );
  }

  mDesc.VersionName = mVersionName;

  mDesc.LowestSupportedImageVersion = GetLowestSupportedVersion();

  //
  // Get attributes from the FmpDeviceLib
  //
  FmpDeviceGetAttributes (&mDesc.AttributesSupported, &mDesc.AttributesSetting);

  //
  // Force set the updatable bits in the attributes;
  //
  mDesc.AttributesSupported |= IMAGE_ATTRIBUTE_IMAGE_UPDATABLE;
  mDesc.AttributesSetting   |= IMAGE_ATTRIBUTE_IMAGE_UPDATABLE;

  //
  // Force set the authentication bits in the attributes;
  //
  mDesc.AttributesSupported |= (IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);
  mDesc.AttributesSetting   |= (IMAGE_ATTRIBUTE_AUTHENTICATION_REQUIRED);

  mDesc.Compatibilities = 0;

  //
  // Get the size of the firmware image from the FmpDeviceLib
  //
  Status = FmpDeviceGetSize (&mDesc.Size);
  if (EFI_ERROR (Status)) {
    mDesc.Size = 0;
  }

  mDesc.LastAttemptVersion = GetLastAttemptVersionFromVariable ();
  mDesc.LastAttemptStatus  = GetLastAttemptStatusFromVariable ();

  mDescriptorPopulated = TRUE;
}

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
GetTheImageInfo (
  IN     EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN OUT UINTN                             *ImageInfoSize,
  IN OUT EFI_FIRMWARE_IMAGE_DESCRIPTOR     *ImageInfo,
  OUT    UINT32                            *DescriptorVersion,
  OUT    UINT8                             *DescriptorCount,
  OUT    UINTN                             *DescriptorSize,
  OUT    UINT32                            *PackageVersion,
  OUT    CHAR16                            **PackageVersionName
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  //
  // Check for valid pointer
  //
  if (ImageInfoSize == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: GetImageInfo() - ImageInfoSize is NULL.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  //
  // Check the buffer size
  // NOTE: Check this first so caller can get the necessary memory size it must allocate.
  //
  if (*ImageInfoSize < (sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR))) {
    *ImageInfoSize = sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR);
    DEBUG ((DEBUG_VERBOSE, "FmpDxe: GetImageInfo() - ImageInfoSize is to small.\n"));
    Status = EFI_BUFFER_TOO_SMALL;
    goto cleanup;
  }

  //
  // Confirm that buffer isn't null
  //
  if ( (ImageInfo == NULL) || (DescriptorVersion == NULL) || (DescriptorCount == NULL) || (DescriptorSize == NULL)
       || (PackageVersion == NULL)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: GetImageInfo() - Pointer Parameter is NULL.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  //
  // Set the size to whatever we need
  //
  *ImageInfoSize = sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR);


  if (!mDescriptorPopulated) {
    PopulateDescriptor();
  }

  //
  // Copy the image descriptor
  //
  CopyMem (ImageInfo, &mDesc, sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR));

  *DescriptorVersion = EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION;
  *DescriptorCount = 1;
  *DescriptorSize = sizeof (EFI_FIRMWARE_IMAGE_DESCRIPTOR);
  //
  // means unsupported
  //
  *PackageVersion = 0xFFFFFFFF;

  //
  // Do not update PackageVersionName since it is not supported in this instance.
  //

cleanup:

  return Status;
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
GetTheImage (
  IN     EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN     UINT8                             ImageIndex,
  IN OUT VOID                              *Image,
  IN OUT UINTN                             *ImageSize
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  Status = EFI_SUCCESS;

  //
  // Check to make sure index is 1 (only 1 image for this device)
  //
  if (ImageIndex != 1) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: GetImage() - Image Index Invalid.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  if (ImageSize == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: GetImage() - ImageSize Pointer Parameter is NULL.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  //
  // Check the buffer size
  //
  Status = FmpDeviceGetSize (&Size);
  if (EFI_ERROR (Status)) {
    Size = 0;
  }
  if (*ImageSize < Size) {
    *ImageSize = Size;
    DEBUG ((DEBUG_VERBOSE, "FmpDxe: GetImage() - ImageSize is to small.\n"));
    Status = EFI_BUFFER_TOO_SMALL;
    goto cleanup;
  }

  if (Image == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: GetImage() - Image Pointer Parameter is NULL.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  Status = FmpDeviceGetImage (Image, ImageSize);
cleanup:

  return Status;
}

/**
  Helper function to safely retrieve the FMP header from
  within an EFI_FIRMWARE_IMAGE_AUTHENTICATION structure.

  @param[in]   Image        Pointer to the image.
  @param[in]   ImageSize    Size of the image.
  @param[out]  PayloadSize

  @retval  !NULL  Valid pointer to the header.
  @retval  NULL   Structure is bad and pointer cannot be found.

**/
VOID *
GetFmpHeader (
  IN  CONST EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN  CONST UINTN                              ImageSize,
  OUT UINTN                                    *PayloadSize
  )
{
  //
  // Check to make sure that operation can be safely performed.
  //
  if (((UINTN)Image + sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) < (UINTN)Image || \
      ((UINTN)Image + sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength) >= (UINTN)Image + ImageSize) {
    //
    // Pointer overflow. Invalid image.
    //
    return NULL;
  }

  *PayloadSize = ImageSize - (sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength);
  return (VOID *)((UINT8 *)Image + sizeof (Image->MonotonicCount) + Image->AuthInfo.Hdr.dwLength);
}

/**
  Helper function to safely calculate the size of all headers
  within an EFI_FIRMWARE_IMAGE_AUTHENTICATION structure.

  @param[in]  Image                 Pointer to the image.
  @param[in]  AdditionalHeaderSize  Size of any headers that cannot be calculated by this function.

  @retval  UINT32>0  Valid size of all the headers.
  @retval  0         Structure is bad and size cannot be found.

**/
UINT32
GetAllHeaderSize (
  IN CONST EFI_FIRMWARE_IMAGE_AUTHENTICATION  *Image,
  IN UINT32                                   AdditionalHeaderSize
  )
{
  UINT32  CalculatedSize;

  CalculatedSize = sizeof (Image->MonotonicCount) +
                   AdditionalHeaderSize +
                   Image->AuthInfo.Hdr.dwLength;

  //
  // Check to make sure that operation can be safely performed.
  //
  if (CalculatedSize < sizeof (Image->MonotonicCount) ||
      CalculatedSize < AdditionalHeaderSize           ||
      CalculatedSize < Image->AuthInfo.Hdr.dwLength      ) {
    //
    // Integer overflow. Invalid image.
    //
    return 0;
  }

  return CalculatedSize;
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
  @retval EFI_ABORTED            The operation is aborted.
  @retval EFI_INVALID_PARAMETER  The Image was NULL.
  @retval EFI_UNSUPPORTED        The operation is not supported.
  @retval EFI_SECURITY_VIOLATION The operation could not be performed due to an authentication failure.

**/
EFI_STATUS
EFIAPI
CheckTheImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  UINT8                             ImageIndex,
  IN  CONST VOID                        *Image,
  IN  UINTN                             ImageSize,
  OUT UINT32                            *ImageUpdatable
  )
{
  EFI_STATUS  Status;
  UINTN       RawSize;
  VOID        *FmpPayloadHeader;
  UINTN       FmpPayloadSize;
  UINT32      Version;
  UINT32      FmpHeaderSize;
  UINTN       AllHeaderSize;
  UINT32      Index;
  VOID        *PublicKeyData;
  UINTN       PublicKeyDataLength;
  UINT8       *PublicKeyDataXdr;
  UINT8       *PublicKeyDataXdrEnd;

  Status           = EFI_SUCCESS;
  RawSize          = 0;
  FmpPayloadHeader = NULL;
  FmpPayloadSize   = 0;
  Version          = 0;
  FmpHeaderSize    = 0;
  AllHeaderSize    = 0;

  //
  // make sure the descriptor has already been loaded
  //
  if (!mDescriptorPopulated) {
    PopulateDescriptor();
  }

  if (ImageUpdatable == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckImage() - ImageUpdatable Pointer Parameter is NULL.\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  //
  //Set to valid and then if any tests fail it will update this flag.
  //
  *ImageUpdatable = IMAGE_UPDATABLE_VALID;

  if (Image == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckImage() - Image Pointer Parameter is NULL.\n"));
    //
    // not sure if this is needed
    //
    *ImageUpdatable = IMAGE_UPDATABLE_INVALID;
    return EFI_INVALID_PARAMETER;
  }

  PublicKeyDataXdr    = PcdGetPtr (PcdFmpDevicePkcs7CertBufferXdr);
  PublicKeyDataXdrEnd = PublicKeyDataXdr + PcdGetSize (PcdFmpDevicePkcs7CertBufferXdr);

  if (PublicKeyDataXdr == NULL || (PublicKeyDataXdr == PublicKeyDataXdrEnd)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: Invalid certificate, skipping it.\n"));
    Status = EFI_ABORTED;
  } else {
    //
    // Try each key from PcdFmpDevicePkcs7CertBufferXdr
    //
    for (Index = 1; PublicKeyDataXdr < PublicKeyDataXdrEnd; Index++) {
      Index++;
      DEBUG (
        (DEBUG_INFO,
        "FmpDxe: Certificate #%d [%p..%p].\n",
        Index,
        PublicKeyDataXdr,
        PublicKeyDataXdrEnd
        )
        );

      if ((PublicKeyDataXdr + sizeof (UINT32)) > PublicKeyDataXdrEnd) {
        //
        // Key data extends beyond end of PCD
        //
        DEBUG ((DEBUG_ERROR, "FmpDxe: Certificate size extends beyond end of PCD, skipping it.\n"));
        Status = EFI_ABORTED;
        break;
      }
      //
      // Read key length stored in big-endian format
      //
      PublicKeyDataLength = SwapBytes32 (*(UINT32 *)(PublicKeyDataXdr));
      //
      // Point to the start of the key data
      //
      PublicKeyDataXdr += sizeof (UINT32);
      if (PublicKeyDataXdr + PublicKeyDataLength > PublicKeyDataXdrEnd) {
        //
        // Key data extends beyond end of PCD
        //
        DEBUG ((DEBUG_ERROR, "FmpDxe: Certificate extends beyond end of PCD, skipping it.\n"));
        Status = EFI_ABORTED;
        break;
      }
      PublicKeyData = PublicKeyDataXdr;
      Status = AuthenticateFmpImage (
                 (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image,
                 ImageSize,
                 PublicKeyData,
                 PublicKeyDataLength
                 );
      if (!EFI_ERROR (Status)) {
        break;
      }
      PublicKeyDataXdr += PublicKeyDataLength;
      PublicKeyDataXdr = (UINT8 *)ALIGN_POINTER (PublicKeyDataXdr, sizeof (UINT32));
    }
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckTheImage() - Authentication Failed %r.\n", Status));
    goto cleanup;
  }

  //
  // Check to make sure index is 1
  //
  if (ImageIndex != 1) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckImage() - Image Index Invalid.\n"));
    *ImageUpdatable = IMAGE_UPDATABLE_INVALID_TYPE;
    Status = EFI_SUCCESS;
    goto cleanup;
  }


  //
  // Check the FmpPayloadHeader
  //
  FmpPayloadHeader = GetFmpHeader ( (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image, ImageSize, &FmpPayloadSize );
  if (FmpPayloadHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckTheImage() - GetFmpHeader failed.\n"));
    Status = EFI_ABORTED;
    goto cleanup;
  }
  Status = GetFmpPayloadHeaderVersion (FmpPayloadHeader, FmpPayloadSize, &Version);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckTheImage() - GetFmpPayloadHeaderVersion failed %r.\n", Status));
    *ImageUpdatable = IMAGE_UPDATABLE_INVALID;
    Status = EFI_SUCCESS;
    goto cleanup;
  }

  //
  // Check the lowest supported version
  //
  if (Version < mDesc.LowestSupportedImageVersion) {
    DEBUG (
      (DEBUG_ERROR,
      "FmpDxe: CheckTheImage() - Version Lower than lowest supported version. 0x%08X < 0x%08X\n",
      Version, mDesc.LowestSupportedImageVersion)
      );
    *ImageUpdatable = IMAGE_UPDATABLE_INVALID_OLD;
    Status = EFI_SUCCESS;
    goto cleanup;
  }

  //
  // Get the FmpHeaderSize so we can determine the real payload size
  //
  Status = GetFmpPayloadHeaderSize (FmpPayloadHeader, FmpPayloadSize, &FmpHeaderSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckTheImage() - GetFmpPayloadHeaderSize failed %r.\n", Status));
    *ImageUpdatable = IMAGE_UPDATABLE_INVALID;
    Status = EFI_SUCCESS;
    goto cleanup;
  }

  //
  // Call FmpDevice Lib Check Image on the
  // Raw payload.  So all headers need stripped off
  //
  AllHeaderSize = GetAllHeaderSize ( (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image, FmpHeaderSize );
  if (AllHeaderSize == 0) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckTheImage() - GetAllHeaderSize failed.\n"));
    Status = EFI_ABORTED;
    goto cleanup;
  }
  RawSize = ImageSize - AllHeaderSize;

  //
  // FmpDeviceLib CheckImage function to do any specific checks
  //
  Status = FmpDeviceCheckImage ((((UINT8 *)Image) + AllHeaderSize), RawSize, ImageUpdatable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: CheckTheImage() - FmpDeviceLib CheckImage failed. Status = %r\n", Status));
  }

cleanup:
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
SetTheImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL               *This,
  IN  UINT8                                          ImageIndex,
  IN  CONST VOID                                     *Image,
  IN  UINTN                                          ImageSize,
  IN  CONST VOID                                     *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS  Progress,
  OUT CHAR16                                         **AbortReason
  )
{
  EFI_STATUS  Status;
  UINT32      Updateable;
  BOOLEAN     BooleanValue;
  UINT32      FmpHeaderSize;
  VOID        *FmpHeader;
  UINTN       FmpPayloadSize;
  UINT32      AllHeaderSize;
  UINT32      IncommingFwVersion;
  UINT32      LastAttemptStatus;
  UINT32      Version;
  UINT32      LowestSupportedVersion;

  Status             = EFI_SUCCESS;
  Updateable         = 0;
  BooleanValue       = FALSE;
  FmpHeaderSize      = 0;
  FmpHeader          = NULL;
  FmpPayloadSize     = 0;
  AllHeaderSize      = 0;
  IncommingFwVersion = 0;
  LastAttemptStatus  = LAST_ATTEMPT_STATUS_ERROR_UNSUCCESSFUL;


  SetLastAttemptVersionInVariable (IncommingFwVersion); //set to 0 to clear any previous results.

  //
  // if we have locked the device, then skip the set operation.
  // it should be blocked by hardware too but we can catch here even faster
  //
  if (mFmpDeviceLocked) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - Device is already locked.  Can't update.\n"));
    Status = EFI_UNSUPPORTED;
    goto cleanup;
  }

  //
  // Call check image to verify the image
  //
  Status = CheckTheImage (This, ImageIndex, Image, ImageSize, &Updateable);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - Check The Image failed with %r.\n", Status));
    if (Status == EFI_SECURITY_VIOLATION) {
      LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_AUTH_ERROR;
    }
    goto cleanup;
  }

  //
  // No functional error in CheckTheImage.  Attempt to get the Version to
  // support better error reporting.
  //
  FmpHeader = GetFmpHeader ( (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image, ImageSize, &FmpPayloadSize );
  if (FmpHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - GetFmpHeader failed.\n"));
    Status = EFI_ABORTED;
    goto cleanup;
  }
  Status = GetFmpPayloadHeaderVersion (FmpHeader, FmpPayloadSize, &IncommingFwVersion);
  if (!EFI_ERROR (Status)) {
    //
    // Set to actual value
    //
    SetLastAttemptVersionInVariable (IncommingFwVersion);
  }


  if (Updateable != IMAGE_UPDATABLE_VALID) {
    DEBUG (
      (DEBUG_ERROR,
      "FmpDxed: SetTheImage() - Check The Image returned that the Image was not valid for update.  Updatable value = 0x%X.\n",
      Updateable)
      );
    Status = EFI_ABORTED;
    goto cleanup;
  }

  if (Progress == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - Invalid progress callback\n"));
    Status = EFI_INVALID_PARAMETER;
    goto cleanup;
  }

  mProgressFunc = Progress;
  mProgressSupported = TRUE;

  //
  // Checking the image is at least 1%
  //
  Status = Progress (1);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - Progress Callback failed with Status %r.\n", Status));
  }

  //
  //Check System Power
  //
  Status = CheckSystemPower (&BooleanValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - CheckSystemPower - API call failed %r.\n", Status));
    goto cleanup;
  }
  if (!BooleanValue) {
    Status = EFI_ABORTED;
    DEBUG (
      (DEBUG_ERROR,
      "FmpDxe: SetTheImage() - CheckSystemPower - returned False.  Update not allowed due to System Power.\n")
      );
    LastAttemptStatus = LAST_ATTEMPT_STATUS_ERROR_PWR_EVT_BATT;
    goto cleanup;
  }

  Progress (2);

  //
  //Check System Thermal
  //
  Status = CheckSystemThermal (&BooleanValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - CheckSystemThermal - API call failed %r.\n", Status));
    goto cleanup;
  }
  if (!BooleanValue) {
    Status = EFI_ABORTED;
    DEBUG (
      (DEBUG_ERROR,
      "FmpDxe: SetTheImage() - CheckSystemThermal - returned False.  Update not allowed due to System Thermal.\n")
      );
    goto cleanup;
  }

  Progress (3);

  //
  //Check System Environment
  //
  Status = CheckSystemEnvironment (&BooleanValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - CheckSystemEnvironment - API call failed %r.\n", Status));
    goto cleanup;
  }
  if (!BooleanValue) {
    Status = EFI_ABORTED;
    DEBUG (
      (DEBUG_ERROR,
      "FmpDxe: SetTheImage() - CheckSystemEnvironment - returned False.  Update not allowed due to System Environment.\n")
      );
    goto cleanup;
  }

  Progress (4);

  //
  // Save LastAttemptStatus as error so that if SetImage never returns the error
  // state is recorded.
  //
  SetLastAttemptStatusInVariable (LastAttemptStatus);

  //
  // Strip off all the headers so the device can process its firmware
  //
  Status = GetFmpPayloadHeaderSize (FmpHeader, FmpPayloadSize, &FmpHeaderSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - GetFmpPayloadHeaderSize failed %r.\n", Status));
    goto cleanup;
  }

  AllHeaderSize = GetAllHeaderSize ( (EFI_FIRMWARE_IMAGE_AUTHENTICATION *)Image, FmpHeaderSize );
  if (AllHeaderSize == 0) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() - GetAllHeaderSize failed.\n"));
    Status = EFI_ABORTED;
    goto cleanup;
  }

  //
  // Indicate that control is handed off to FmpDeviceLib
  //
  Progress (5);

  //
  //Copy the requested image to the firmware using the FmpDeviceLib
  //
  Status = FmpDeviceSetImage (
             (((UINT8 *)Image) + AllHeaderSize),
             ImageSize - AllHeaderSize,
             VendorCode,
             FmpDxeProgress,
             IncommingFwVersion,
             AbortReason
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: SetTheImage() SetImage from FmpDeviceLib failed. Status =  %r.\n", Status));
    goto cleanup;
  }


  //
  // Finished the update without error
  // Indicate that control has been returned from FmpDeviceLib
  //
  Progress (99);

  //
  // Update the version stored in variable
  //
  if (!mRuntimeVersionSupported) {
    Version = DEFAULT_VERSION;
    GetFmpPayloadHeaderVersion (FmpHeader, FmpPayloadSize, &Version);
    SetVersionInVariable (Version);
  }

  //
  // Update lowest supported variable
  //
  {
    LowestSupportedVersion = DEFAULT_LOWESTSUPPORTEDVERSION;
    GetFmpPayloadHeaderLowestSupportedVersion (FmpHeader, FmpPayloadSize, &LowestSupportedVersion);
    SetLowestSupportedVersionInVariable (LowestSupportedVersion);
  }

  LastAttemptStatus = LAST_ATTEMPT_STATUS_SUCCESS;

cleanup:
  mProgressFunc = NULL;
  mProgressSupported = FALSE;
  SetLastAttemptStatusInVariable (LastAttemptStatus);

  if (Progress != NULL) {
    //
    // Set progress to 100 after everything is done including recording Status.
    //
    Progress (100);
  }

  //
  // Need repopulate after SetImage is called to
  // update LastAttemptVersion and LastAttemptStatus.
  //
  mDescriptorPopulated = FALSE;

  return Status;
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
GetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  OUT UINT32                            *PackageVersion,
  OUT CHAR16                            **PackageVersionName,
  OUT UINT32                            *PackageVersionNameMaxLen,
  OUT UINT64                            *AttributesSupported,
  OUT UINT64                            *AttributesSetting
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
SetPackageInfo (
  IN EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN CONST VOID                        *Image,
  IN UINTN                             ImageSize,
  IN CONST VOID                        *VendorCode,
  IN UINT32                            PackageVersion,
  IN CONST CHAR16                      *PackageVersionName
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Event notification function that is invoked when the event GUID specified by
  PcdFmpDeviceLockEventGuid is signaled.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  The pointer to the notification function's context,
                      which is implementation-dependent.
**/
VOID
EFIAPI
FmpDxeLockEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  if (!mFmpDeviceLocked) {
    //
    // Lock the firmware device
    //
    Status = FmpDeviceLock();
    if (EFI_ERROR (Status)) {
      if (Status != EFI_UNSUPPORTED) {
        DEBUG ((DEBUG_ERROR, "FmpDxe: FmpDeviceLock() returned error.  Status = %r\n", Status));
      } else {
        DEBUG ((DEBUG_WARN, "FmpDxe: FmpDeviceLock() returned error.  Status = %r\n", Status));
      }
    }
    mFmpDeviceLocked = TRUE;
  }
}

/**
  Function to install FMP instance.

  @param[in]  Handle  The device handle to install a FMP instance on.

  @retval  EFI_SUCCESS            FMP Installed
  @retval  EFI_INVALID_PARAMETER  Handle was invalid
  @retval  other                  Error installing FMP

**/
EFI_STATUS
EFIAPI
InstallFmpInstance (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                                   Status;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL             *Fmp;
  EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL  *FmpProgress;

  Status      = EFI_SUCCESS;
  Fmp         = NULL;
  FmpProgress = NULL;

  //
  // Only allow a single FMP Protocol instance to be installed
  //
  if (mFmpInstalled) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Allocate FMP Protocol instance
  //
  Fmp = AllocateZeroPool (sizeof (EFI_FIRMWARE_MANAGEMENT_PROTOCOL));
  if (Fmp == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: Failed to allocate memory for FMP Protocol instance.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto cleanup;
  }

  //
  // Allocate FMP Progress Protocol instance
  //
  FmpProgress = AllocateZeroPool (sizeof (EDKII_FIRMWARE_MANAGEMENT_PROGRESS_PROTOCOL));
  if (FmpProgress == NULL) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: Failed to allocate memory for FMP Progress Protocol instance.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    FreePool (Fmp);
    goto cleanup;
  }

  //
  // Set up FMP Protocol function pointers
  //
  Fmp->GetImageInfo   = GetTheImageInfo;
  Fmp->GetImage       = GetTheImage;
  Fmp->SetImage       = SetTheImage;
  Fmp->CheckImage     = CheckTheImage;
  Fmp->GetPackageInfo = GetPackageInfo;
  Fmp->SetPackageInfo = SetPackageInfo;

  //
  // Fill in FMP Progress Protocol fields for Version 1
  //
  FmpProgress->Version                        = 1;
  FmpProgress->ProgressBarForegroundColor.Raw = PcdGet32 (PcdFmpDeviceProgressColor);
  FmpProgress->WatchdogSeconds                = PcdGet8 (PcdFmpDeviceProgressWatchdogTimeInSeconds);

  //
  // Install FMP Protocol and FMP Progress Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiFirmwareManagementProtocolGuid,
                  Fmp,
                  &gEdkiiFirmwareManagementProgressProtocolGuid,
                  FmpProgress,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: FMP Protocol install error. Status = %r.\n", Status));
    FreePool (Fmp);
    goto cleanup;
  }

  DEBUG ((DEBUG_INFO, "FmpDxe: FMP Protocol Installed!\n"));
  mFmpInstalled = TRUE;

cleanup:

  return Status;
}

/**
  Main entry for this driver/library.

  @param[in] ImageHandle  Image handle this driver.
  @param[in] SystemTable  Pointer to SystemTable.

**/
EFI_STATUS
EFIAPI
FmpDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_GUID    *LockGuid;

  //
  // Verify that a new FILE_GUID value has been provided in the <Defines>
  // section of this module.  The FILE_GUID is the ESRT GUID that must be
  // unique for each updatable firmware image.
  //
  if (CompareGuid (&mDefaultModuleFileGuid, &gEfiCallerIdGuid)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: Use of default FILE_GUID detected.  FILE_GUID must be set to a unique value.\n"));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  //
  // Get the ImageIdName value for the EFI_FIRMWARE_IMAGE_DESCRIPTOR from a PCD.
  //
  mImageIdName = (CHAR16 *) PcdGetPtr (PcdFmpDeviceImageIdName);
  if (PcdGetSize (PcdFmpDeviceImageIdName) <= 2 || mImageIdName[0] == 0) {
    //
    // PcdFmpDeviceImageIdName must be set to a non-empty Unicode string
    //
    DEBUG ((DEBUG_ERROR, "FmpDxe: FmpDeviceLib PcdFmpDeviceImageIdName is an empty string.\n"));
    ASSERT (FALSE);
  }

  //
  // Detects if PcdFmpDevicePkcs7CertBufferXdr contains a test key.
  //
  DetectTestKey ();

  if (IsLockFmpDeviceAtLockEventGuidRequired ()) {
    //
    // Lock all UEFI Variables used by this module.
    //
    Status = LockAllFmpVariables ();
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe: Failed to lock variables.  Status = %r.\n", Status));
    } else {
      DEBUG ((DEBUG_INFO, "FmpDxe: All variables locked\n"));
    }

    //
    // Register notify function to lock the FMP device.
    // The lock event GUID is retrieved from PcdFmpDeviceLockEventGuid.
    // If PcdFmpDeviceLockEventGuid is not the size of an EFI_GUID, then
    // gEfiEndOfDxeEventGroupGuid is used.
    //
    LockGuid = &gEfiEndOfDxeEventGroupGuid;
    if (PcdGetSize (PcdFmpDeviceLockEventGuid) == sizeof (EFI_GUID)) {
      LockGuid = (EFI_GUID *)PcdGetPtr (PcdFmpDeviceLockEventGuid);
    }
    DEBUG ((DEBUG_INFO, "FmpDxe: Lock GUID: %g\n", LockGuid));

    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    FmpDxeLockEventNotify,
                    NULL,
                    LockGuid,
                    &mFmpDeviceLockEvent
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "FmpDxe: Failed to register notification.  Status = %r\n", Status));
    }
    ASSERT_EFI_ERROR (Status);
  } else {
    DEBUG ((DEBUG_VERBOSE, "FmpDxe: Not registering notification to call FmpDeviceLock() because mfg mode\n"));
  }

  //
  // Register with library the install function so if the library uses
  // UEFI driver model/driver binding protocol it can install FMP on its device handle
  // If library is simple lib that does not use driver binding then it should return
  // unsupported and this will install the FMP instance on the ImageHandle
  //
  Status = RegisterFmpInstaller (InstallFmpInstance);
  if (Status == EFI_UNSUPPORTED) {
    DEBUG ((DEBUG_INFO, "FmpDxe: FmpDeviceLib registration returned EFI_UNSUPPORTED.  Installing single FMP instance.\n"));
    Status = InstallFmpInstance (ImageHandle);
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "FmpDxe: FmpDeviceLib registration returned %r.  No FMP installed.\n", Status));
  } else {
    DEBUG ((
      DEBUG_INFO,
      "FmpDxe: FmpDeviceLib registration returned EFI_SUCCESS.  Expect FMP to be installed during the BDS/Device connection phase.\n"
      ));
  }

  return Status;
}
