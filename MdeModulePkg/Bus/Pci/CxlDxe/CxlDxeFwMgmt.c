/** @file
  Firmware Management Protocol - supports Get Fw Info
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
    DEBUG ((EFI_D_ERROR, "CxlFirmwareMgmtGetImageInfo: EFI Out of resources...\n"));
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

//
// Template of the private context structure for the Firmware Management Protocol instance
//
GLOBAL_REMOVE_IF_UNREFERENCED
EFI_FIRMWARE_MANAGEMENT_PROTOCOL  gCxlFirmwareManagement = {
  CxlFirmwareMgmtGetImageInfo,
};

