/** @file
  This library provides image decoding service by managing the different
  image decoding libraries.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/ImageDecoderLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

typedef struct {
  UINT32       Signature;
  DECODE_IMAGE Decoder;
  LIST_ENTRY   Link;
} IMAGE_DECODER_ENTRY;
#define IMAGE_DECODER_ENTRY_SIGNATURE  SIGNATURE_32 ('i', 'm', 'g', 'd')
#define IMAGE_DECODER_ENTRY_FROM_LINK(Link) \
        CR (Link, IMAGE_DECODER_ENTRY, Link, IMAGE_DECODER_ENTRY_SIGNATURE)

LIST_ENTRY mImageDecoderLibDecoders = INITIALIZE_LIST_HEAD_VARIABLE (mImageDecoderLibDecoders);

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
  )
{
  IMAGE_DECODER_ENTRY               *Entry;
  LIST_ENTRY                        *Link;
  EFI_STATUS                        Status;

  if ((GopBlt == NULL) || (GopBltSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Image == NULL) || (ImageSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for ( Link = GetFirstNode (&mImageDecoderLibDecoders)
      ; !IsNull (&mImageDecoderLibDecoders, Link)
      ; Link = GetNextNode (&mImageDecoderLibDecoders, Link)
      ) {
    Entry = IMAGE_DECODER_ENTRY_FROM_LINK (Link);
    Status = Entry->Decoder (ImageFormat, Image, ImageSize, GopBlt, GopBltSize, PixelWidth, PixelHeight);
    if (!EFI_ERROR (Status)) {
      break;
    }
  }

  if (IsNull (&mImageDecoderLibDecoders, Link)) {
    return EFI_UNSUPPORTED;
  } else {
    return EFI_SUCCESS;
  }
}

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
  )
{
  IMAGE_DECODER_ENTRY  *Entry;

  Entry = AllocatePool (sizeof (IMAGE_DECODER_ENTRY));
  if (Entry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Entry->Signature = IMAGE_DECODER_ENTRY_SIGNATURE;
  Entry->Decoder   = Decoder;
  InsertTailList (&mImageDecoderLibDecoders, &Entry->Link);

  return EFI_SUCCESS;
}