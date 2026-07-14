/** @file
  Logo DXE driver that loads the platform logo from a firmware volume file.

  Installs EDKII_PLATFORM_LOGO_PROTOCOL by loading a BMP image from a
  FREEFORM FFS file via GetSectionFromAnyFv(). No HII framework dependency.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BmpSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/PlatformLogo.h>

STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *mLogoBitmap = NULL;
STATIC UINTN                          mLogoWidth   = 0;
STATIC UINTN                          mLogoHeight  = 0;

/**
  Return the platform logo image.

  @param[in]      This       Protocol instance pointer.
  @param[in, out] Instance   On input, the index of the logo to return.
                             On output, incremented to the next index.
  @param[out]     Image      Returned logo image data. Caller frees Bitmap.
  @param[out]     Attribute  Display attribute for the logo.
  @param[out]     OffsetX    X offset from the display attribute origin.
  @param[out]     OffsetY    Y offset from the display attribute origin.

  @retval EFI_SUCCESS            Logo returned.
  @retval EFI_NOT_FOUND          No logo at the requested index.
  @retval EFI_INVALID_PARAMETER  A required output pointer is NULL.
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory.
**/
STATIC
EFI_STATUS
EFIAPI
LogoFvFileGetImage (
  IN     EDKII_PLATFORM_LOGO_PROTOCOL           *This,
  IN OUT UINT32                                 *Instance,
  OUT    EFI_IMAGE_INPUT                        *Image,
  OUT    EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE  *Attribute,
  OUT    INTN                                   *OffsetX,
  OUT    INTN                                   *OffsetY
  )
{
  UINTN  BitmapSize;

  if ((Instance == NULL) || (Image == NULL) ||
      (Attribute == NULL) || (OffsetX == NULL) || (OffsetY == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (*Instance >= 1) {
    return EFI_NOT_FOUND;
  }

  (*Instance)++;

  *Attribute    = EdkiiPlatformLogoDisplayAttributeCenter;
  *OffsetX      = 0;
  *OffsetY      = 0;
  BitmapSize    = mLogoWidth * mLogoHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  Image->Flags  = 0;
  Image->Width  = (UINT16)mLogoWidth;
  Image->Height = (UINT16)mLogoHeight;
  Image->Bitmap = AllocateCopyPool (BitmapSize, mLogoBitmap);
  if (Image->Bitmap == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

STATIC EDKII_PLATFORM_LOGO_PROTOCOL  mPlatformLogo = {
  LogoFvFileGetImage
};

/**
  Load raw BMP data from a FREEFORM FFS file in the firmware volume.

  @param[in]  LogoGuid     GUID of the FREEFORM FFS file (RAW section = BMP).
  @param[out] BmpData      Pointer to the allocated BMP data buffer.
  @param[out] BmpDataSize  Size of the BMP data buffer in bytes.

  @retval EFI_SUCCESS  BMP data loaded into BmpData and BmpDataSize.
  @retval other        File not found in the firmware volume.
**/
STATIC
EFI_STATUS
LoadLogoFromFv (
  IN  CONST EFI_GUID  *LogoGuid,
  OUT VOID            **BmpData,
  OUT UINTN           *BmpDataSize
  )
{
  return GetSectionFromAnyFv (
           LogoGuid,
           EFI_SECTION_RAW,
           0,
           BmpData,
           BmpDataSize
           );
}

/**
  Entry point for the LogoFvFileDxe driver.

  Skips installation if gEdkiiPlatformLogoProtocolGuid is already present.
  Loads the logo BMP from the FV file identified by PcdLogoFile and installs
  the protocol. If the logo cannot be found or decoded, boot continues without
  a logo.

  @param[in] ImageHandle  Handle for this driver image.
  @param[in] SystemTable  Pointer to the EFI System Table.

  @retval EFI_SUCCESS          Protocol installed, or no logo found.
  @retval EFI_ALREADY_STARTED  Protocol already installed by another driver.
  @retval other                Protocol installation failed.
**/
EFI_STATUS
EFIAPI
LogoFvFileDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                     Status;
  VOID                           *ExistingLogo;
  VOID                           *BmpData;
  UINTN                          BmpDataSize;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *GopBlt;
  UINTN                          GopBltSize;
  UINTN                          PixelHeight;
  UINTN                          PixelWidth;
  EFI_HANDLE                     Handle;

  Status = gBS->LocateProtocol (
                  &gEdkiiPlatformLogoProtocolGuid,
                  NULL,
                  &ExistingLogo
                  );
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: already installed, skipping.\n", __func__));
    return EFI_ALREADY_STARTED;
  }

  Status = LoadLogoFromFv (PcdGetPtr (PcdLogoFile), &BmpData, &BmpDataSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: logo not found (Status=%r). "
      "Add FILE FREEFORM = <PcdLogoFile GUID> { SECTION RAW = <logo.bmp> } to the FDF.\n",
      __func__,
      Status
      ));
    return EFI_SUCCESS;
  }

  //
  // TranslateBmpToGopBlt() treats a non-NULL *GopBlt as a caller-supplied
  // buffer to reuse. Initialise to NULL/0 so it always allocates a fresh one.
  //
  GopBlt     = NULL;
  GopBltSize = 0;

  Status = TranslateBmpToGopBlt (
             BmpData,
             BmpDataSize,
             &GopBlt,
             &GopBltSize,
             &PixelHeight,
             &PixelWidth
             );
  FreePool (BmpData);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: BMP decode failed (Status=%r).\n", __func__, Status));
    return EFI_SUCCESS;
  }

  mLogoBitmap = GopBlt;
  mLogoWidth  = PixelWidth;
  mLogoHeight = PixelHeight;

  DEBUG ((
    DEBUG_INFO,
    "%a: logo loaded %u x %u.\n",
    __func__,
    mLogoWidth,
    mLogoHeight
    ));

  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEdkiiPlatformLogoProtocolGuid,
                  &mPlatformLogo,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: install failed (Status=%r).\n", __func__, Status));
    FreePool (mLogoBitmap);
    mLogoBitmap = NULL;
    mLogoWidth  = 0;
    mLogoHeight = 0;
  }

  return Status;
}
