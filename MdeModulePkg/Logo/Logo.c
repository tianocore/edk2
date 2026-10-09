/** @file
  Logo DXE Driver, install Edkii Platform Logo protocol.

Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/HiiImageEx.h>
#include <Protocol/PlatformLogo.h>
#include <Protocol/HiiPackageList.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BmpSupportLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>

typedef struct {
  EFI_IMAGE_ID                             ImageId;
  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE    Attribute;
  INTN                                     OffsetX;
  INTN                                     OffsetY;
} LOGO_ENTRY;

STATIC VOID     *mCachedBmpImage;
STATIC UINTN    mCachedBmpImageSize;
STATIC BOOLEAN  mUseFfsLogo;

STATIC EFI_HII_IMAGE_EX_PROTOCOL  *mHiiImageEx;
STATIC EFI_HII_HANDLE             mHiiHandle;

LOGO_ENTRY  mLogos[] = {
  {
    0,  ///< ImageId set at runtime based on PcdLogoFileGuid
    EdkiiPlatformLogoDisplayAttributeCenter,
    0,
    0
  }
};

/**
  Load logo image from cached BMP and convert to GOP BLT format.

  Uses the BMP buffer cached at entry point and converts it to a GOP BLT buffer.

  @param[out] Image  Image structure to populate with converted BMP data.

  @retval EFI_SUCCESS           Logo image converted successfully.
  @retval EFI_UNSUPPORTED       BMP format not supported.
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
  @retval Others                Error returned by underlying services.
**/
STATIC
EFI_STATUS
GetFfsImage (
  OUT EFI_IMAGE_INPUT  *Image
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *GopBlt;
  UINTN                          GopBltSize;
  UINTN                          PixelHeight;
  UINTN                          PixelWidth;

  if (Image == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mCachedBmpImage == NULL) || (mCachedBmpImageSize == 0)) {
    return EFI_NOT_FOUND;
  }

  GopBlt     = NULL;
  GopBltSize = 0;

  Status = TranslateBmpToGopBlt (
             mCachedBmpImage,
             mCachedBmpImageSize,
             &GopBlt,
             &GopBltSize,
             &PixelHeight,
             &PixelWidth
             );

  if (EFI_ERROR (Status)) {
    if (GopBlt != NULL) {
      FreePool (GopBlt);
    }

    return Status;
  }

  if ((GopBlt == NULL) || (GopBltSize == 0)) {
    if (GopBlt != NULL) {
      FreePool (GopBlt);
    }

    return EFI_DEVICE_ERROR;
  }

  if ((PixelWidth == 0) ||
      (PixelHeight == 0) ||
      (PixelWidth > MAX_UINT16) ||
      (PixelHeight > MAX_UINT16))
  {
    FreePool (GopBlt);
    return EFI_UNSUPPORTED;
  }

  Image->Flags  = 0;
  Image->Width  = (UINT16)PixelWidth;
  Image->Height = (UINT16)PixelHeight;
  Image->Bitmap = GopBlt;

  return EFI_SUCCESS;
}

/**
  Load a platform logo image and return its data and attributes.

  This function retrieves logo images based on PcdLogoFileGuid:
  - If PcdLogoFileGuid is non-zero, the logo is loaded from a firmware volume.
  - If PcdLogoFileGuid is zero (default), the logo is loaded from the embedded
    HII database.

  @param This              The pointer to this protocol instance.
  @param Instance          The visible image instance is found.
  @param Image             Points to the image.
  @param Attribute         The display attributes of the image returned.
  @param OffsetX           The X offset of the image regarding the Attribute.
  @param OffsetY           The Y offset of the image regarding the Attribute.

  @retval EFI_SUCCESS      The image was fetched successfully.
  @retval EFI_NOT_FOUND    The specified image could not be found.
  @retval EFI_NOT_READY    The logo source is not initialized.
**/
EFI_STATUS
EFIAPI
GetImage (
  IN     EDKII_PLATFORM_LOGO_PROTOCOL        *This,
  IN OUT UINT32                              *Instance,
  OUT EFI_IMAGE_INPUT                        *Image,
  OUT EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE  *Attribute,
  OUT INTN                                   *OffsetX,
  OUT INTN                                   *OffsetY
  )
{
  UINT32  Current;

  if ((Instance == NULL) || (Image == NULL) ||
      (Attribute == NULL) || (OffsetX == NULL) || (OffsetY == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Current = *Instance;
  if (Current >= ARRAY_SIZE (mLogos)) {
    return EFI_NOT_FOUND;
  }

  (*Instance)++;
  *Attribute = mLogos[Current].Attribute;
  *OffsetX   = mLogos[Current].OffsetX;
  *OffsetY   = mLogos[Current].OffsetY;

  if (mUseFfsLogo) {
    return GetFfsImage (Image);
  }

  if ((mHiiImageEx == NULL) || (mHiiHandle == NULL)) {
    DEBUG ((DEBUG_ERROR, "LogoDxe: HII logo source is not ready\n"));
    return EFI_NOT_READY;
  }

  Image->Flags  = 0;
  Image->Width  = 0;
  Image->Height = 0;
  Image->Bitmap = NULL;

  return mHiiImageEx->GetImageEx (
                        mHiiImageEx,
                        mHiiHandle,
                        mLogos[Current].ImageId,
                        Image
                        );
}

EDKII_PLATFORM_LOGO_PROTOCOL  mPlatformLogo = {
  GetImage
};

/**
  Entrypoint of this module.

  This function is the entrypoint of this module. It installs the Edkii
  Platform Logo protocol.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
InitializeLogo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                   Status;
  EFI_HANDLE                   Handle;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageList;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  CONST EFI_GUID               *LogoFileGuid;

  mUseFfsLogo         = FALSE;
  mCachedBmpImage     = NULL;
  mCachedBmpImageSize = 0;
  mHiiImageEx         = NULL;
  mHiiHandle          = NULL;

  LogoFileGuid = (CONST EFI_GUID *)PcdGetPtr (PcdLogoFileGuid);
  if ((LogoFileGuid == NULL) || (PcdGetSize (PcdLogoFileGuid) != sizeof (EFI_GUID))) {
    return EFI_INVALID_PARAMETER;
  }

  if (!IsZeroGuid (LogoFileGuid)) {
    Status = GetSectionFromAnyFv (
               LogoFileGuid,
               EFI_SECTION_RAW,
               0,
               &mCachedBmpImage,
               &mCachedBmpImageSize
               );
    if (!EFI_ERROR (Status) && (mCachedBmpImage != NULL) && (mCachedBmpImageSize > 0)) {
      mUseFfsLogo = TRUE;
      DEBUG ((DEBUG_INFO, "LogoDxe: FFS logo found, will use on-demand conversion\n"));
      goto InstallProtocol;
    }

    if (mCachedBmpImage != NULL) {
      FreePool (mCachedBmpImage);
      mCachedBmpImage = NULL;
    }

    mCachedBmpImageSize = 0;
    DEBUG ((DEBUG_WARN, "LogoDxe: FFS logo not found\n"));
    return EFI_NOT_FOUND;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->LocateProtocol (
                  &gEfiHiiImageExProtocolGuid,
                  NULL,
                  (VOID **)&mHiiImageEx
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Retrieve HII package list from ImageHandle
  //
  Status = gBS->OpenProtocol (
                  ImageHandle,
                  &gEfiHiiPackageListProtocolGuid,
                  (VOID **)&PackageList,
                  ImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "HII Image Package with logo not found in PE/COFF resource section\n"));
    return Status;
  }

  //
  // Publish HII package list to HII Database.
  //
  Status = HiiDatabase->NewPackageList (
                          HiiDatabase,
                          PackageList,
                          NULL,
                          &mHiiHandle
                          );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Set ImageId for HII mode
  //
  mLogos[0].ImageId = IMAGE_TOKEN (IMG_LOGO);

InstallProtocol:
  Handle = NULL;
  return gBS->InstallMultipleProtocolInterfaces (
                &Handle,
                &gEdkiiPlatformLogoProtocolGuid,
                &mPlatformLogo,
                NULL
                );
}
