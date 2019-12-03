/** @file

Provides services to convert a BMP graphics image to a GOP BLT buffer
and to convert a GOP BLT buffer to a BMP graphics image.

Copyright (c) 2016, Microsoft Corporation
Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>

All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BMP_SUPPORT_LIB_H__
#define __BMP_SUPPORT_LIB_H__

#include <Protocol/GraphicsOutput.h>

/**
  Translate a *.BMP graphics image to a GOP blt buffer. If a NULL Blt buffer
  is passed in a GopBlt buffer will be allocated by this routine using
  EFI_BOOT_SERVICES.AllocatePool(). If a GopBlt buffer is passed in it will be
  used if it is big enough.

  @param [in]      BmpImage      Pointer to BMP file.
  @param [in]      BmpImageSize  Number of bytes in BmpImage.
  @param [in, out] GopBlt        Buffer containing GOP version of BmpImage.
  @param [in, out] GopBltSize    Size of GopBlt in bytes.
  @param [out]     PixelHeight   Height of GopBlt/BmpImage in pixels.
  @param [out]     PixelWidth    Width of GopBlt/BmpImage in pixels.

  @retval RETURN_SUCCESS            GopBlt and GopBltSize are returned.
  @retval RETURN_INVALID_PARAMETER  BmpImage is NULL.
  @retval RETURN_INVALID_PARAMETER  GopBlt is NULL.
  @retval RETURN_INVALID_PARAMETER  GopBltSize is NULL.
  @retval RETURN_INVALID_PARAMETER  PixelHeight is NULL.
  @retval RETURN_INVALID_PARAMETER  PixelWidth is NULL.
  @retval RETURN_UNSUPPORTED        BmpImage is not a valid *.BMP image.
  @retval RETURN_BUFFER_TOO_SMALL   The passed in GopBlt buffer is not big
                                    enough.  The required size is returned in
                                    GopBltSize.
  @retval RETURN_OUT_OF_RESOURCES   The GopBlt buffer could not be allocated.

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
  );

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
  );

#endif
