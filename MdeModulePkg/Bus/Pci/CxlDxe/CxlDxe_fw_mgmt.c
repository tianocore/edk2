/** @file
  Firmware Management Protocol - supports Get Fw Info, Sending/Receiving FMP commands
  supports Set Fw Image, Activate Fw image
  SetPkgInfo, GetPkgInfo, CheckImg
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "CxlDxe.h"
extern void strCpy_c16(CHAR16 *st1, CHAR16 *st2);
extern void strCpy_const16(CHAR16 *st1, CONST CHAR16 *st2);

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
CxlFirmwareMgmtGetImageInfo (
  IN EFI_FIRMWARE_MANAGEMENT_PROTOCOL        *This,
  IN OUT    UINTN                            *ImageInfoSize,
  IN OUT    EFI_FIRMWARE_IMAGE_DESCRIPTOR    *ImageInfo,
  OUT       UINT32                           *DescriptorVersion,
  OUT       UINT8                            *DescriptorCount,
  OUT       UINTN                            *DescriptorSize,
  OUT       UINT32                           *PackageVersion,
  OUT       CHAR16                           **PackageVersionName
  )
{
  DEBUG((EFI_D_INFO, "CxlFirmwareMgmtGetImageInfo: Entering CxlFirmwareMgmtGetImageInfo...\n"));
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(This);

  if(((Private->slotInfo.num_slots) * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR)) > *ImageInfoSize){
    *ImageInfoSize = (Private->slotInfo.num_slots * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR));
    return EFI_BUFFER_TOO_SMALL;
  }

  if (NULL == ImageInfoSize || NULL == ImageInfo || NULL == DescriptorVersion || NULL == DescriptorCount || \
    NULL == PackageVersionName || NULL == DescriptorSize || NULL == PackageVersion) {
    DEBUG((EFI_D_ERROR, "CxlFirmwareMgmtGetImageInfo: EFI Invalid param...\n"));
    return EFI_INVALID_PARAMETER;
  }

  *PackageVersionName = AllocateZeroPool(CXL_STRING_BUFFER_WIDTH);
  if (NULL == *PackageVersionName) {
    DEBUG((EFI_D_ERROR, "CxlFirmwareMgmtGetImageInfo: EFI Out of resources...\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  CopyMem(ImageInfo, &Private->slotInfo.FwImageDescriptor, Private->slotInfo.num_slots * sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR));

  *DescriptorVersion = EFI_FIRMWARE_IMAGE_DESCRIPTOR_VERSION;
  *DescriptorCount = Private->slotInfo.num_slots;
  *DescriptorSize = sizeof(EFI_FIRMWARE_IMAGE_DESCRIPTOR);

  strCpy_c16(*PackageVersionName, Private->PackageVersionName);
  *PackageVersion = Private->PackageVersion;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CxlFirmwareMgmtGetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *This,
  IN  UINT8                             ImageIndex,
  IN  OUT  VOID                         *Image,
  IN  OUT  UINTN                        *ImageSize
  )
{
  EFI_STATUS  Status = EFI_SUCCESS;
  CXL_CONTROLLER_PRIVATE_DATA *Private = NULL;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(This);
  if (ImageIndex > Private->slotInfo.num_slots || Private->slotInfo.isSetImageDone[ImageIndex] != TRUE)
  {
    DEBUG((EFI_D_ERROR, "[%a]: ImageIndex = %d, num_slots = %d, isSetImageDone = %d \n", __func__, ImageIndex, Private->slotInfo.num_slots, Private->slotInfo.isSetImageDone[ImageIndex]));
    return EFI_INVALID_PARAMETER;
  }

  if (Private->slotInfo.imageFileSize[ImageIndex] > *ImageSize) {
    *ImageSize = Private->slotInfo.imageFileSize[ImageIndex];
    return EFI_BUFFER_TOO_SMALL;
  }

  if (Image != NULL) {
    CopyMem(Image, Private->slotInfo.imageFileBuffer[ImageIndex], sizeof(*ImageSize));
  } else {
      *ImageSize = Private->slotInfo.imageFileSize[ImageIndex];
      return EFI_BUFFER_TOO_SMALL;
  }
  return Status;
}

EFI_STATUS
EFIAPI
CxlFirmwareMgmtSetImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL                 *This,
  IN  UINT8                                            ImageIndex,
  IN  CONST VOID                                       *Image,
  IN  UINTN                                            ImageSize,
  IN  CONST VOID                                       *VendorCode,
  IN  EFI_FIRMWARE_MANAGEMENT_UPDATE_IMAGE_PROGRESS    Progress,
  OUT CHAR16                                           **AbortReason
  )
{
  EFI_STATUS Status;
  CXL_CONTROLLER_PRIVATE_DATA *Private = NULL;
  UINT32 written;

  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(This);
  if (ImageIndex > Private->slotInfo.num_slots) {
    DEBUG((EFI_D_ERROR, "CxlFirmwareMgmtSetImage: ImageIndex = %d is greater then Total slots = %d \n", ImageIndex, Private->slotInfo.num_slots));
    Status = EFI_INVALID_PARAMETER;
    return Status;
  }

  Status = cxl_mem_transfer_fw(Private, ImageIndex, Image, 0, ImageSize, &written);

  if (Status != EFI_SUCCESS) {
    DEBUG((EFI_D_ERROR, "CxlFirmwareMgmtSetImage: UEFI Driver Transfer FW (Opcode 0201h) Failed!\n"));
    return Status;
  }

  Private->slotInfo.imageFileBuffer[ImageIndex] = AllocateZeroPool(ImageSize);
  if (Private->slotInfo.imageFileBuffer[ImageIndex] == NULL) {
    DEBUG((EFI_D_ERROR, "CxlFirmwareMgmtSetImage: AllocateZeroPool failed!\n"));
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  Private->slotInfo.imageFileSize[ImageIndex] = ImageSize;
  CopyMem(Private->slotInfo.imageFileBuffer[ImageIndex], Image, sizeof(ImageSize));
  Private->slotInfo.isSetImageDone[ImageIndex] = TRUE;

  DEBUG((EFI_D_INFO, "[CxlFirmwareMgmtSetImage] SetImage returns...%r\n", Status));
  return Status;
}

EFI_STATUS
EFIAPI
CxlFirmwareMgmtCheckImage (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL    *This,
  IN  UINT8                               ImageIndex,
  IN  CONST VOID                          *Image,
  IN  UINTN                               ImageSize,
  OUT UINT32                              *ImageUpdatable
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
CxlFirmwareMgmtGetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL    *This,
  OUT UINT32                              *PackageVersion,
  OUT CHAR16                              **PackageVersionName,
  OUT UINT32                              *PackageVersionNameMaxLen,
  OUT UINT64                              *AttributesSupported,
  OUT UINT64                              *AttributesSetting
  )
{
  DEBUG((EFI_D_INFO, "CxlFirmwareMgmtGetPackageInfo: Entering CxlFirmwareMgmtGetPackageInfo...\n"));
  CXL_CONTROLLER_PRIVATE_DATA  *Private;
  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(This);

  if (Private->PackageVersionName != NULL) {
    strCpy_c16(*PackageVersionName, Private->PackageVersionName);
  }

  *PackageVersion = Private->PackageVersion;
  *AttributesSupported = Private->slotInfo.FwImageDescriptor[0].AttributesSupported;
  *AttributesSetting = Private->slotInfo.FwImageDescriptor[0].AttributesSetting;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
CxlFirmwareMgmtSetPackageInfo (
  IN  EFI_FIRMWARE_MANAGEMENT_PROTOCOL    *This,
  IN  CONST VOID                          *Image,
  IN  UINTN                               ImageSize,
  IN  CONST VOID                          *VendorCode,
  IN  UINT32                              PackageVersion,
  IN  CONST CHAR16                        *PackageVersionName
  )
{
  DEBUG((EFI_D_INFO, "CxlFirmwareMgmtSetPackageInfo: Entering CxlFirmwareMgmtSetPackageInfo...\n"));
  CXL_CONTROLLER_PRIVATE_DATA *Private;
  Private = CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(This);
  strCpy_const16(Private->PackageVersionName, PackageVersionName);
  Private->PackageVersion = PackageVersion;
  return EFI_SUCCESS;
}

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_FIRMWARE_MANAGEMENT_PROTOCOL gCxlFirmwareManagement = {
  CxlFirmwareMgmtGetImageInfo,
  CxlFirmwareMgmtGetImage,
  CxlFirmwareMgmtSetImage,
  CxlFirmwareMgmtCheckImage,
  CxlFirmwareMgmtGetPackageInfo,
  CxlFirmwareMgmtSetPackageInfo
};
