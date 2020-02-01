/** @file
  RISC-V logos on POST screen.

  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "RiscvBadging.h"

/**

  Load an OEM badge image and return its data and attributes.

  @param This              The pointer to this protocol instance.
  @param Instance          The visible image instance is found.
  @param Format            The format of the image. Examples: BMP, JPEG.
  @param ImageData         The image data for the badge file. Currently only 
                           supports the .bmp file format. 
  @param ImageSize         The size of the image returned.
  @param Attribute         The display attributes of the image returned.
  @param CoordinateX       The X coordinate of the image.
  @param CoordinateY       The Y coordinate of the image.

  @retval EFI_SUCCESS      The image was fetched successfully.
  @retval EFI_NOT_FOUND    The specified image could not be found.

**/

EFI_STATUS
RiscvBadgingGetImage (
  IN  EFI_OEM_BADGING_PROTOCOL          *This,
  IN  OUT UINT32                         *Instance,
  OUT EFI_BADGING_FORMAT                *Format,
  OUT UINT8                             **ImageData,
  OUT UINTN                             *ImageSize,
  OUT EFI_BADGING_DISPLAY_ATTRIBUTE     *Attribute,
  OUT UINTN                             *CoordinateX,
  OUT UINTN                             *CoordinateY
)
{
  EFI_STATUS Status;

  if (*Instance == 0) {
    //
    // Tiano logo
    //
    *Attribute = EfiBadgingDisplayAttributeCustomized;
    *CoordinateX = 187;
    *CoordinateY = 271;
    Status = GetSectionFromAnyFv (PcdGetPtr(PcdLogoFile), EFI_SECTION_RAW, 0, (VOID **) ImageData, ImageSize);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }  
    DEBUG ((DEBUG_INFO, "Disaply Tiano logo\n"));
  } else if (*Instance == 1) {
    //
    // RISC-V logo
    //
    *Attribute = EfiBadgingDisplayAttributeCustomized;
    *CoordinateX = 420;
    *CoordinateY = 271;
    Status = GetSectionFromAnyFv (PcdGetPtr(PcdRiscvUEfiLogoFile), EFI_SECTION_RAW, 0, (VOID **) ImageData, ImageSize);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }  
    DEBUG ((DEBUG_INFO, "Disaply RISC-V logo\n"));
  } else {
    return EFI_NOT_FOUND;
  }

  *Format = EfiBadgingFormatBMP;
  *Instance = *Instance + 1;
  return EFI_SUCCESS; 
}

EFI_OEM_BADGING_PROTOCOL mRiscvBadging = {
    RiscvBadgingGetImage
};

EFI_STATUS
EFIAPI
RiscvBadgingEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install OEM Badging protocol.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiOEMBadgingProtocolGuid,
                  &mRiscvBadging,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
