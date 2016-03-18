/** @file
  This library provides image decoding service by managing the different
  image decoding libraries.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef __IMAGE_DECODER_LIB_H__
#define __IMAGE_DECODER_LIB_H__
#include <Protocol/PlatformLogo.h>

typedef
EFI_STATUS
(EFIAPI *DECODE_IMAGE)(
  IN  IMAGE_FORMAT                  ImageFormat,
  IN  UINT8                         *Image,
  IN  UINTN                         ImageSize,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **GopBlt,
  OUT UINTN                         *GopBltSize,
  OUT UINTN                         *PixelWidth,
  OUT UINTN                         *PixelHeight
  );

/**
  Convert a graphics image to a callee allocated GOP blt buffer.

  @param  ImageFormat   Format of the image file.
  @param  Image         Pointer to image file.
  @param  ImageSize     Number of bytes in Image.
  @param  GopBlt        Buffer containing GOP version of Image.
  @param  GopBltSize    Size of GopBlt in bytes.
  @param  PixelWidth    Width of GopBlt/Image in pixels.
  @param  PixelHeight   Height of GopBlt/Image in pixels.

  @retval EFI_SUCCESS           GopBlt and GopBltSize are returned.
  @retval EFI_INVALID_PARAMETER GopBlt or GopBltSize is NULL.
  @retval EFI_INVALID_PARAMETER Image is NULL or ImageSize is 0.
  @retval EFI_UNSUPPORTED       Image is not supported.
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate.

**/
EFI_STATUS
EFIAPI
DecodeImage (
  IN  IMAGE_FORMAT                  ImageFormat,
  IN  UINT8                         *Image,
  IN  UINTN                         ImageSize,
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL **GopBlt,
  OUT UINTN                         *GopBltSize,
  OUT UINTN                         *PixelWidth,
  OUT UINTN                         *PixelHeight
  );

/**
  Register an image decoder.

  @param Decoder  An image decoder.

  @retval EFI_SUCCESS          The decoder was successfully registered.
  @retval EFI_OUT_OF_RESOURCES No enough resource to register the decoder.

**/
EFI_STATUS
EFIAPI
RegisterImageDecoder (
  IN  DECODE_IMAGE     Decoder
  );

#endif
