/** @file

  Provides services to convert a BMP graphics image to a GOP BLT buffer and
  from a GOP BLT buffer to a BMP graphics image.

  Caution: This module requires additional review when modified.
  This module processes external input - BMP image.
  This external input must be validated carefully to avoid security issue such
  as buffer overflow, integer overflow.

  TranslateBmpToGopBlt() receives untrusted input and performs basic validation.

  Copyright (c) 2016-2017, Microsoft Corporation
  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

  All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SafeIntLib.h>
#include <IndustryStandard/Bmp.h>

#include <Library/BmpSupportLib.h>

//
// BMP Image header for an uncompressed 24-bit per pixel BMP image.
//
const BMP_IMAGE_HEADER  mBmpImageHeaderTemplate = {
  'B',    // CharB
  'M',    // CharM
  0,      // Size will be updated at runtime
  {0, 0}, // Reserved
  sizeof (BMP_IMAGE_HEADER), // ImageOffset
  sizeof (BMP_IMAGE_HEADER) - OFFSET_OF (BMP_IMAGE_HEADER, HeaderSize), // HeaderSize
  0,      // PixelWidth will be updated at runtime
  0,      // PixelHeight will be updated at runtime
  1,      // Planes
  24,     // BitPerPixel
  0,      // CompressionType
  0,      // ImageSize will be updated at runtime
  0,      // XPixelsPerMeter
  0,      // YPixelsPerMeter
  0,      // NumberOfColors
  0       // ImportantColors
};

/**
  Translate a *.BMP graphics image to a GOP blt buffer. If a NULL Blt buffer
  is passed in a GopBlt buffer will be allocated by this routine using
  EFI_BOOT_SERVICES.AllocatePool(). If a GopBlt buffer is passed in it will be
  used if it is big enough.

  @param[in]       BmpImage      Pointer to BMP file.
  @param[in]       BmpImageSize  Number of bytes in BmpImage.
  @param[in, out]  GopBlt        Buffer containing GOP version of BmpImage.
  @param[in, out]  GopBltSize    Size of GopBlt in bytes.
  @param[out]      PixelHeight   Height of GopBlt/BmpImage in pixels.
  @param[out]      PixelWidth    Width of GopBlt/BmpImage in pixels.

  @retval  RETURN_SUCCESS            GopBlt and GopBltSize are returned.
  @retval  RETURN_INVALID_PARAMETER  BmpImage is NULL.
  @retval  RETURN_INVALID_PARAMETER  GopBlt is NULL.
  @retval  RETURN_INVALID_PARAMETER  GopBltSize is NULL.
  @retval  RETURN_INVALID_PARAMETER  PixelHeight is NULL.
  @retval  RETURN_INVALID_PARAMETER  PixelWidth is NULL.
  @retval  RETURN_UNSUPPORTED        BmpImage is not a valid *.BMP image.
  @retval  RETURN_BUFFER_TOO_SMALL   The passed in GopBlt buffer is not big
                                     enough.  The required size is returned in
                                     GopBltSize.
  @retval  RETURN_OUT_OF_RESOURCES   The GopBlt buffer could not be allocated.

**/
RETURN_STATUS
EFIAPI
TranslateBmpToGopBlt (
  IN     VOID                           *BmpImage,
  IN     UINTN                          BmpImageSize,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  **GopBlt,
  IN OUT UINTN                          *GopBltSize,
  OUT    UINTN                          *PixelHeight,
  OUT    UINTN                          *PixelWidth
  )
{
  UINT8                          *Image;
  UINT8                          *ImageHeader;
  BMP_IMAGE_HEADER               *BmpHeader;
  BMP_COLOR_MAP                  *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *Blt;
  UINT32                         BltBufferSize;
  UINTN                          Index;
  UINTN                          Height;
  UINTN                          Width;
  UINTN                          ImageIndex;
  UINT32                         DataSizePerLine;
  BOOLEAN                        IsAllocated;
  UINT32                         ColorMapNum;
  RETURN_STATUS                  Status;
  UINT32                         DataSize;
  UINT32                         Temp;

  if (BmpImage == NULL || GopBlt == NULL || GopBltSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }
  if (PixelHeight == NULL || PixelWidth == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (BmpImageSize < sizeof (BMP_IMAGE_HEADER)) {
    DEBUG ((DEBUG_ERROR, "TranslateBmpToGopBlt: BmpImageSize too small\n"));
    return RETURN_UNSUPPORTED;
  }

  BmpHeader = (BMP_IMAGE_HEADER *)BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    DEBUG ((DEBUG_ERROR, "TranslateBmpToGopBlt: BmpHeader->Char fields incorrect\n"));
    return RETURN_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
  if (BmpHeader->CompressionType != 0) {
    DEBUG ((DEBUG_ERROR, "TranslateBmpToGopBlt: Compression Type unsupported.\n"));
    return RETURN_UNSUPPORTED;
  }

  if ((BmpHeader->PixelHeight == 0) || (BmpHeader->PixelWidth == 0)) {
    DEBUG ((DEBUG_ERROR, "TranslateBmpToGopBlt: BmpHeader->PixelHeight or BmpHeader->PixelWidth is 0.\n"));
    return RETURN_UNSUPPORTED;
  }

  //
  // Only support BITMAPINFOHEADER format.
  // BITMAPFILEHEADER + BITMAPINFOHEADER = BMP_IMAGE_HEADER
  //
  if (BmpHeader->HeaderSize != sizeof (BMP_IMAGE_HEADER) - OFFSET_OF (BMP_IMAGE_HEADER, HeaderSize)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: BmpHeader->Headership is not as expected.  Headersize is 0x%x\n",
      BmpHeader->HeaderSize
      ));
    return RETURN_UNSUPPORTED;
  }

  //
  // The data size in each line must be 4 byte alignment.
  //
  Status = SafeUint32Mult (
             BmpHeader->PixelWidth,
             BmpHeader->BitPerPixel,
             &DataSizePerLine
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... PixelWidth:0x%x BitPerPixel:0x%x\n",
      BmpHeader->PixelWidth,
      BmpHeader->BitPerPixel
      ));
    return RETURN_UNSUPPORTED;
  }

  Status = SafeUint32Add (DataSizePerLine, 31, &DataSizePerLine);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... DataSizePerLine:0x%x\n",
      DataSizePerLine
      ));

    return RETURN_UNSUPPORTED;
  }

  DataSizePerLine = (DataSizePerLine >> 3) &(~0x3);
  Status = SafeUint32Mult (
             DataSizePerLine,
             BmpHeader->PixelHeight,
             &BltBufferSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... DataSizePerLine:0x%x PixelHeight:0x%x\n",
      DataSizePerLine, BmpHeader->PixelHeight
      ));

    return RETURN_UNSUPPORTED;
  }

  Status = SafeUint32Mult (
             BmpHeader->PixelHeight,
             DataSizePerLine,
             &DataSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BmpImage... PixelHeight:0x%x DataSizePerLine:0x%x\n",
      BmpHeader->PixelHeight, DataSizePerLine
      ));

    return RETURN_UNSUPPORTED;
  }

  if ((BmpHeader->Size != BmpImageSize) ||
      (BmpHeader->Size < BmpHeader->ImageOffset) ||
      (BmpHeader->Size - BmpHeader->ImageOffset != DataSize)) {

    DEBUG ((DEBUG_ERROR, "TranslateBmpToGopBlt: invalid BmpImage... \n"));
    DEBUG ((DEBUG_ERROR, "   BmpHeader->Size: 0x%x\n", BmpHeader->Size));
    DEBUG ((DEBUG_ERROR, "   BmpHeader->ImageOffset: 0x%x\n", BmpHeader->ImageOffset));
    DEBUG ((DEBUG_ERROR, "   BmpImageSize: 0x%lx\n", (UINTN)BmpImageSize));
    DEBUG ((DEBUG_ERROR, "   DataSize: 0x%lx\n", (UINTN)DataSize));

    return RETURN_UNSUPPORTED;
  }

  //
  // Calculate Color Map offset in the image.
  //
  Image = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *)(Image + sizeof (BMP_IMAGE_HEADER));
  if (BmpHeader->ImageOffset < sizeof (BMP_IMAGE_HEADER)) {
    return RETURN_UNSUPPORTED;
  }

  if (BmpHeader->ImageOffset > sizeof (BMP_IMAGE_HEADER)) {
    switch (BmpHeader->BitPerPixel) {
    case 1:
      ColorMapNum = 2;
      break;
    case 4:
      ColorMapNum = 16;
      break;
    case 8:
      ColorMapNum = 256;
      break;
    default:
      ColorMapNum = 0;
      break;
    }
    //
    // BMP file may has padding data between the bmp header section and the
    // bmp data section.
    //
    if (BmpHeader->ImageOffset - sizeof (BMP_IMAGE_HEADER) < sizeof (BMP_COLOR_MAP) * ColorMapNum) {
      return RETURN_UNSUPPORTED;
    }
  }

  //
  // Calculate graphics image data address in the image
  //
  Image = ((UINT8 *)BmpImage) + BmpHeader->ImageOffset;
  ImageHeader = Image;

  //
  // Calculate the BltBuffer needed size.
  //
  Status = SafeUint32Mult (
             BmpHeader->PixelWidth,
             BmpHeader->PixelHeight,
             &BltBufferSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BltBuffer needed size... PixelWidth:0x%x PixelHeight:0x%x\n",
      BmpHeader->PixelWidth, BmpHeader->PixelHeight
      ));

    return RETURN_UNSUPPORTED;
  }

  Temp = BltBufferSize;
  Status = SafeUint32Mult (
             BltBufferSize,
             sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL),
             &BltBufferSize
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateBmpToGopBlt: invalid BltBuffer needed size... PixelWidth x PixelHeight:0x%x struct size:0x%x\n",
      Temp, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      ));

    return RETURN_UNSUPPORTED;
  }

  IsAllocated = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    DEBUG ((DEBUG_INFO, "Bmp Support: Allocating 0x%X bytes of memory\n", BltBufferSize));
    *GopBltSize = (UINTN)BltBufferSize;
    *GopBlt = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return RETURN_OUT_OF_RESOURCES;
    }
  } else {
    //
    // GopBlt has been allocated by caller.
    //
    if (*GopBltSize < (UINTN)BltBufferSize) {
      *GopBltSize = (UINTN)BltBufferSize;
      return RETURN_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth  = BmpHeader->PixelWidth;
  *PixelHeight = BmpHeader->PixelHeight;
  DEBUG ((DEBUG_INFO, "BmpHeader->ImageOffset 0x%X\n", BmpHeader->ImageOffset));
  DEBUG ((DEBUG_INFO, "BmpHeader->PixelWidth 0x%X\n", BmpHeader->PixelWidth));
  DEBUG ((DEBUG_INFO, "BmpHeader->PixelHeight 0x%X\n", BmpHeader->PixelHeight));
  DEBUG ((DEBUG_INFO, "BmpHeader->BitPerPixel 0x%X\n", BmpHeader->BitPerPixel));
  DEBUG ((DEBUG_INFO, "BmpHeader->ImageSize 0x%X\n", BmpHeader->ImageSize));
  DEBUG ((DEBUG_INFO, "BmpHeader->HeaderSize 0x%X\n", BmpHeader->HeaderSize));
  DEBUG ((DEBUG_INFO, "BmpHeader->Size 0x%X\n", BmpHeader->Size));

  //
  // Translate image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[ (BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
      case 1:
        //
        // Translate 1-bit (2 colors) BMP to 24-bit color
        //
        for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
          Blt->Red   = BmpColorMap[ ((*Image) >> (7 - Index)) & 0x1].Red;
          Blt->Green = BmpColorMap[ ((*Image) >> (7 - Index)) & 0x1].Green;
          Blt->Blue  = BmpColorMap[ ((*Image) >> (7 - Index)) & 0x1].Blue;
          Blt++;
          Width++;
        }

        Blt--;
        Width--;
        break;

      case 4:
        //
        // Translate 4-bit (16 colors) BMP Palette to 24-bit color
        //
        Index = (*Image) >> 4;
        Blt->Red   = BmpColorMap[Index].Red;
        Blt->Green = BmpColorMap[Index].Green;
        Blt->Blue  = BmpColorMap[Index].Blue;
        if (Width < (BmpHeader->PixelWidth - 1)) {
          Blt++;
          Width++;
          Index = (*Image) & 0x0f;
          Blt->Red   = BmpColorMap[Index].Red;
          Blt->Green = BmpColorMap[Index].Green;
          Blt->Blue  = BmpColorMap[Index].Blue;
        }
        break;

      case 8:
        //
        // Translate 8-bit (256 colors) BMP Palette to 24-bit color
        //
        Blt->Red   = BmpColorMap[*Image].Red;
        Blt->Green = BmpColorMap[*Image].Green;
        Blt->Blue  = BmpColorMap[*Image].Blue;
        break;

      case 24:
        //
        // It is 24-bit BMP.
        //
        Blt->Blue  = *Image++;
        Blt->Green = *Image++;
        Blt->Red   = *Image;
        break;

      case 32:
        //
        //Conver 32 bit to 24bit bmp - just ignore the final byte of each pixel
        Blt->Blue  = *Image++;
        Blt->Green = *Image++;
        Blt->Red   = *Image++;
        break;

      default:
        //
        // Other bit format BMP is not supported.
        //
        if (IsAllocated) {
          FreePool (*GopBlt);
          *GopBlt = NULL;
        }
        DEBUG ((DEBUG_ERROR, "Bmp Bit format not supported.  0x%X\n", BmpHeader->BitPerPixel));
        return RETURN_UNSUPPORTED;
        break;
      };

    }

    ImageIndex = (UINTN)Image - (UINTN)ImageHeader;
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return RETURN_SUCCESS;
}

/**
  Translate a GOP blt buffer to an uncompressed 24-bit per pixel BMP graphics
  image. If a NULL BmpImage is passed in a BmpImage buffer will be allocated by
  this routine using EFI_BOOT_SERVICES.AllocatePool(). If a BmpImage buffer is
  passed in it will be used if it is big enough.

  @param [in]      GopBlt        Pointer to GOP blt buffer.
  @param [in]      PixelHeight   Height of GopBlt/BmpImage in pixels.
  @param [in]      PixelWidth    Width of GopBlt/BmpImage in pixels.
  @param [in, out] BmpImage      Buffer containing BMP version of GopBlt.
  @param [in, out] BmpImageSize  Size of BmpImage in bytes.

  @retval RETURN_SUCCESS            BmpImage and BmpImageSize are returned.
  @retval RETURN_INVALID_PARAMETER  GopBlt is NULL.
  @retval RETURN_INVALID_PARAMETER  BmpImage is NULL.
  @retval RETURN_INVALID_PARAMETER  BmpImageSize is NULL.
  @retval RETURN_UNSUPPORTED        GopBlt cannot be converted to a *.BMP image.
  @retval RETURN_BUFFER_TOO_SMALL   The passed in BmpImage buffer is not big
                                    enough.  The required size is returned in
                                    BmpImageSize.
  @retval RETURN_OUT_OF_RESOURCES   The BmpImage buffer could not be allocated.

**/
RETURN_STATUS
EFIAPI
TranslateGopBltToBmp (
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *GopBlt,
  IN     UINT32                         PixelHeight,
  IN     UINT32                         PixelWidth,
  IN OUT VOID                           **BmpImage,
  IN OUT UINT32                         *BmpImageSize
  )
{
  RETURN_STATUS                  Status;
  UINT32                         PaddingSize;
  UINT32                         BmpSize;
  BMP_IMAGE_HEADER               *BmpImageHeader;
  UINT8                          *Image;
  UINTN                          Col;
  UINTN                          Row;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltPixel;

  if (GopBlt == NULL || BmpImage == NULL || BmpImageSize == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if ((PixelHeight == 0) || (PixelWidth == 0)) {
    return RETURN_UNSUPPORTED;
  }

  //
  // Allocate memory for BMP file.
  //
  PaddingSize = PixelWidth & 0x3;

  //
  // First check PixelWidth * 3 + PaddingSize doesn't overflow
  //
  Status = SafeUint32Mult (PixelWidth, 3, &BmpSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateGopBltToBmp: GopBlt is too large. PixelHeight:0x%x PixelWidth:0x%x\n",
      PixelHeight,
      PixelWidth
      ));
    return RETURN_UNSUPPORTED;
  }
  Status = SafeUint32Add (BmpSize, PaddingSize, &BmpSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateGopBltToBmp: GopBlt is too large. PixelHeight:0x%x PixelWidth:0x%x\n",
      PixelHeight,
      PixelWidth
      ));
    return RETURN_UNSUPPORTED;
  }

  //
  // Second check (mLogoWidth * 3 + PaddingSize) * mLogoHeight + sizeof (BMP_IMAGE_HEADER) doesn't overflow
  //
  Status = SafeUint32Mult (BmpSize, PixelHeight, &BmpSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateGopBltToBmp: GopBlt is too large. PixelHeight:0x%x PixelWidth:0x%x\n",
      PixelHeight,
      PixelWidth
      ));
    return RETURN_UNSUPPORTED;
  }
  Status = SafeUint32Add (BmpSize, sizeof (BMP_IMAGE_HEADER), &BmpSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "TranslateGopBltToBmp: GopBlt is too large. PixelHeight:0x%x PixelWidth:0x%x\n",
      PixelHeight,
      PixelWidth
      ));
    return RETURN_UNSUPPORTED;
  }

  //
  // The image should be stored in EfiBootServicesData, allowing the system to
  // reclaim the memory
  //
  if (*BmpImage == NULL) {
    *BmpImage = AllocateZeroPool (BmpSize);
    if (*BmpImage == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    *BmpImageSize = BmpSize;
  } else if (*BmpImageSize < BmpSize) {
    *BmpImageSize = BmpSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  BmpImageHeader = (BMP_IMAGE_HEADER *)*BmpImage;
  CopyMem (BmpImageHeader, &mBmpImageHeaderTemplate, sizeof (BMP_IMAGE_HEADER));
  BmpImageHeader->Size        = *BmpImageSize;
  BmpImageHeader->ImageSize   = *BmpImageSize - sizeof (BMP_IMAGE_HEADER);
  BmpImageHeader->PixelWidth  = PixelWidth;
  BmpImageHeader->PixelHeight = PixelHeight;

  //
  // Convert BLT buffer to BMP file.
  //
  Image = (UINT8 *)BmpImageHeader + sizeof (BMP_IMAGE_HEADER);
  for (Row = 0; Row < PixelHeight; Row++) {
    BltPixel = &GopBlt[(PixelHeight - Row - 1) * PixelWidth];

    for (Col = 0; Col < PixelWidth; Col++) {
      *Image++ = BltPixel->Blue;
      *Image++ = BltPixel->Green;
      *Image++ = BltPixel->Red;
      BltPixel++;
    }

    //
    // Padding for 4 byte alignment.
    //
    Image += PaddingSize;
  }

  return RETURN_SUCCESS;
}
