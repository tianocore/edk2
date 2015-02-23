/** @file
  FrameBufferBltLib - Library to perform blt operations on a frame buffer.

  Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php
  
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PiDxe.h"
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BltLib.h>
#include <Library/DebugLib.h>

#if 0
#define VDEBUG DEBUG
#else
#define VDEBUG(x)
#endif

#define MAX_LINE_BUFFER_SIZE (SIZE_4KB * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))

UINTN                           mBltLibColorDepth;
UINTN                           mBltLibWidthInBytes;
UINTN                           mBltLibBytesPerPixel;
UINTN                           mBltLibWidthInPixels;
UINTN                           mBltLibHeight;
UINT8                           mBltLibLineBuffer[MAX_LINE_BUFFER_SIZE];
UINT8                           *mBltLibFrameBuffer;
EFI_GRAPHICS_PIXEL_FORMAT       mPixelFormat;
EFI_PIXEL_BITMASK               mPixelBitMasks;
INTN                            mPixelShl[4]; // R-G-B-Rsvd
INTN                            mPixelShr[4]; // R-G-B-Rsvd


VOID
ConfigurePixelBitMaskFormat (
  IN EFI_PIXEL_BITMASK          *BitMask
  )
{
  UINTN   Loop;
  UINT32  *Masks;
  UINT32  MergedMasks;

  MergedMasks = 0;
  Masks = (UINT32*) BitMask;
  for (Loop = 0; Loop < 3; Loop++) {
    ASSERT ((Loop == 3) || (Masks[Loop] != 0));
    ASSERT ((MergedMasks & Masks[Loop]) == 0);
    mPixelShl[Loop] = HighBitSet32 (Masks[Loop]) - 23 + (Loop * 8);
    if (mPixelShl[Loop] < 0) {
      mPixelShr[Loop] = -mPixelShl[Loop];
      mPixelShl[Loop] = 0;
    } else {
      mPixelShr[Loop] = 0;
    }
    MergedMasks = (UINT32) (MergedMasks | Masks[Loop]);
    DEBUG ((EFI_D_INFO, "%d: shl:%d shr:%d mask:%x\n", Loop, mPixelShl[Loop], mPixelShr[Loop], Masks[Loop]));
  }
  MergedMasks = (UINT32) (MergedMasks | Masks[3]);

  ASSERT (MergedMasks != 0);
  mBltLibBytesPerPixel = (UINTN) ((HighBitSet32 (MergedMasks) + 7) / 8);

  DEBUG ((EFI_D_INFO, "Bytes per pixel: %d\n", mBltLibBytesPerPixel));

  CopyMem (&mPixelBitMasks, BitMask, sizeof (*BitMask));
}


/**
  Configure the FrameBufferLib instance

  @param[in] FrameBuffer      Pointer to the start of the frame buffer
  @param[in] FrameBufferInfo  Describes the frame buffer characteristics

  @retval  EFI_INVALID_PARAMETER - Invalid parameter
  @retval  EFI_UNSUPPORTED - The BltLib does not support this configuration
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibConfigure (
  IN  VOID                                 *FrameBuffer,
  IN  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *FrameBufferInfo
  )
{
  STATIC EFI_PIXEL_BITMASK  RgbPixelMasks =
    { 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };
  STATIC EFI_PIXEL_BITMASK  BgrPixelMasks =
    { 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

  switch (FrameBufferInfo->PixelFormat) {
  case PixelRedGreenBlueReserved8BitPerColor:
    ConfigurePixelBitMaskFormat (&RgbPixelMasks);
    break;
  case PixelBlueGreenRedReserved8BitPerColor:
    ConfigurePixelBitMaskFormat (&BgrPixelMasks);
    break;
  case PixelBitMask:
    ConfigurePixelBitMaskFormat (&(FrameBufferInfo->PixelInformation));
    break;
  case PixelBltOnly:
    ASSERT (FrameBufferInfo->PixelFormat != PixelBltOnly);
    return EFI_UNSUPPORTED;
  default:
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }
  mPixelFormat = FrameBufferInfo->PixelFormat;

  mBltLibFrameBuffer = (UINT8*) FrameBuffer;
  mBltLibWidthInPixels = (UINTN) FrameBufferInfo->HorizontalResolution;
  mBltLibHeight = (UINTN) FrameBufferInfo->VerticalResolution;
  mBltLibWidthInBytes = mBltLibWidthInPixels * mBltLibBytesPerPixel;

  ASSERT (mBltLibWidthInBytes < sizeof (mBltLibLineBuffer));

  return EFI_SUCCESS;
}


/**
  Performs a UEFI Graphics Output Protocol Blt operation.

  @param[in,out] BltBuffer     - The data to transfer to screen
  @param[in]     BltOperation  - The operation to perform
  @param[in]     SourceX       - The X coordinate of the source for BltOperation
  @param[in]     SourceY       - The Y coordinate of the source for BltOperation
  @param[in]     DestinationX  - The X coordinate of the destination for BltOperation
  @param[in]     DestinationY  - The Y coordinate of the destination for BltOperation
  @param[in]     Width         - The width of a rectangle in the blt rectangle in pixels
  @param[in]     Height        - The height of a rectangle in the blt rectangle in pixels
  @param[in]     Delta         - Not used for EfiBltVideoFill and EfiBltVideoToVideo operation.
                                 If a Delta of 0 is used, the entire BltBuffer will be operated on.
                                 If a subrectangle of the BltBuffer is used, then Delta represents
                                 the number of bytes in a row of the BltBuffer.

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - Blt operation success

**/
EFI_STATUS
EFIAPI
BltLibGopBlt (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer, OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION     BltOperation,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  )
{
  switch (BltOperation) {
  case EfiBltVideoToBltBuffer:
    return BltLibVideoToBltBufferEx (
             BltBuffer,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );

  case EfiBltVideoToVideo:
    return BltLibVideoToVideo (
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height
             );

  case EfiBltVideoFill:
    return BltLibVideoFill (
             BltBuffer,
             DestinationX,
             DestinationY,
             Width,
             Height
             );

  case EfiBltBufferToVideo:
    return BltLibBufferToVideoEx (
             BltBuffer,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );
  default:
    return EFI_INVALID_PARAMETER;
  }
}


/**
  Performs a UEFI Graphics Output Protocol Blt Video Fill.

  @param[in]  Color         Color to fill the region with
  @param[in]  DestinationX  X location to start fill operation
  @param[in]  DestinationY  Y location to start fill operation
  @param[in]  Width         Width (in pixels) to fill
  @param[in]  Height        Height to fill

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibVideoFill (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *Color,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  UINTN                           DstY;
  VOID                            *BltMemDst;
  UINTN                           X;
  UINT8                           Uint8;
  UINT32                          Uint32;
  UINT64                          WideFill;
  BOOLEAN                         UseWideFill;
  BOOLEAN                         LineBufferReady;
  UINTN                           Offset;
  UINTN                           WidthInBytes;
  UINTN                           SizeInBytes;

  //
  // BltBuffer to Video: Source is BltBuffer, destination is Video
  //
  if (DestinationY + Height > mBltLibHeight) {
    DEBUG ((EFI_D_INFO, "VideoFill: Past screen (Y)\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (DestinationX + Width > mBltLibWidthInPixels) {
    DEBUG ((EFI_D_INFO, "VideoFill: Past screen (X)\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    DEBUG ((EFI_D_INFO, "VideoFill: Width or Height is 0\n"));
    return EFI_INVALID_PARAMETER;
  }

  WidthInBytes = Width * mBltLibBytesPerPixel;

  Uint32 = *(UINT32*) Color;
  WideFill =
    (UINT32) (
        (((Uint32 << mPixelShl[0]) >> mPixelShr[0]) & mPixelBitMasks.RedMask) |
        (((Uint32 << mPixelShl[1]) >> mPixelShr[1]) & mPixelBitMasks.GreenMask) |
        (((Uint32 << mPixelShl[2]) >> mPixelShr[2]) & mPixelBitMasks.BlueMask)
      );
  VDEBUG ((EFI_D_INFO, "VideoFill: color=0x%x, wide-fill=0x%x\n", Uint32, WideFill));

  //
  // If the size of the pixel data evenly divides the sizeof
  // WideFill, then a wide fill operation can be used
  //
  UseWideFill = TRUE;
  if ((sizeof (WideFill) % mBltLibBytesPerPixel) == 0) {
    for (X = mBltLibBytesPerPixel; X < sizeof (WideFill); X++) {
      ((UINT8*)&WideFill)[X] = ((UINT8*)&WideFill)[X % mBltLibBytesPerPixel];
    }
  } else {
    //
    // If all the bytes in the pixel are the same value, then use
    // a wide fill operation.
    //
    for (
      X = 1, Uint8 = ((UINT8*)&WideFill)[0];
      X < mBltLibBytesPerPixel;
      X++) {
      if (Uint8 != ((UINT8*)&WideFill)[X]) {
        UseWideFill = FALSE;
        break;
      }
    }
    if (UseWideFill) {
      SetMem ((VOID*) &WideFill, sizeof (WideFill), Uint8);
    }
  }

  if (UseWideFill && (DestinationX == 0) && (Width == mBltLibWidthInPixels)) {
    VDEBUG ((EFI_D_INFO, "VideoFill (wide, one-shot)\n"));
    Offset = DestinationY * mBltLibWidthInPixels;
    Offset = mBltLibBytesPerPixel * Offset;
    BltMemDst = (VOID*) (mBltLibFrameBuffer + Offset);
    SizeInBytes = WidthInBytes * Height;
    if (SizeInBytes >= 8) {
      SetMem32 (BltMemDst, SizeInBytes & ~3, (UINT32) WideFill);
      SizeInBytes = SizeInBytes & 3;
    }
    if (SizeInBytes > 0) {
      SetMem (BltMemDst, SizeInBytes, (UINT8)(UINTN) WideFill);
    }
  } else {
    LineBufferReady = FALSE;
    for (DstY = DestinationY; DstY < (Height + DestinationY); DstY++) {
      Offset = (DstY * mBltLibWidthInPixels) + DestinationX;
      Offset = mBltLibBytesPerPixel * Offset;
      BltMemDst = (VOID*) (mBltLibFrameBuffer + Offset);

      if (UseWideFill && (((UINTN) BltMemDst & 7) == 0)) {
        VDEBUG ((EFI_D_INFO, "VideoFill (wide)\n"));
        SizeInBytes = WidthInBytes;
        if (SizeInBytes >= 8) {
          SetMem64 (BltMemDst, SizeInBytes & ~7, WideFill);
          SizeInBytes = SizeInBytes & 7;
        }
        if (SizeInBytes > 0) {
          CopyMem (BltMemDst, (VOID*) &WideFill, SizeInBytes);
        }
      } else {
        VDEBUG ((EFI_D_INFO, "VideoFill (not wide)\n"));
        if (!LineBufferReady) {
          CopyMem (mBltLibLineBuffer, &WideFill, mBltLibBytesPerPixel);
          for (X = 1; X < Width; ) {
            CopyMem(
              (mBltLibLineBuffer + (X * mBltLibBytesPerPixel)),
              mBltLibLineBuffer,
              MIN (X, Width - X) * mBltLibBytesPerPixel
              );
            X = X + MIN (X, Width - X);
          }
          LineBufferReady = TRUE;
        }
        CopyMem (BltMemDst, mBltLibLineBuffer, WidthInBytes);
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation.

  @param[out] BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibVideoToBltBuffer (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  return BltLibVideoToBltBufferEx (
           BltBuffer,
           SourceX,
           SourceY,
           0,
           0,
           Width,
           Height,
           0
           );
}


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Buffer operation
  with extended parameters.

  @param[out] BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  DestinationX  X location within BltBuffer
  @param[in]  DestinationY  Y location within BltBuffer
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height
  @param[in]  Delta         Number of bytes in a row of BltBuffer

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibVideoToBltBufferEx (
  OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  )
{
  UINTN                           DstY;
  UINTN                           SrcY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Blt;
  VOID                            *BltMemSrc;
  VOID                            *BltMemDst;
  UINTN                           X;
  UINT32                          Uint32;
  UINTN                           Offset;
  UINTN                           WidthInBytes;

  //
  // Video to BltBuffer: Source is Video, destination is BltBuffer
  //
  if (SourceY + Height > mBltLibHeight) {
    return EFI_INVALID_PARAMETER;
  }

  if (SourceX + Width > mBltLibWidthInPixels) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  WidthInBytes = Width * mBltLibBytesPerPixel;

  //
  // Video to BltBuffer: Source is Video, destination is BltBuffer
  //
  for (SrcY = SourceY, DstY = DestinationY; DstY < (Height + DestinationY); SrcY++, DstY++) {

    Offset = (SrcY * mBltLibWidthInPixels) + SourceX;
    Offset = mBltLibBytesPerPixel * Offset;
    BltMemSrc = (VOID *) (mBltLibFrameBuffer + Offset);

    if (mPixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
      BltMemDst =
        (VOID *) (
            (UINT8 *) BltBuffer +
            (DstY * Delta) +
            (DestinationX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))
          );
    } else {
      BltMemDst = (VOID *) mBltLibLineBuffer;
    }

    CopyMem (BltMemDst, BltMemSrc, WidthInBytes);

    if (mPixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
      for (X = 0; X < Width; X++) {
        Blt         = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) ((UINT8 *) BltBuffer + (DstY * Delta) + (DestinationX + X) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
        Uint32 = *(UINT32*) (mBltLibLineBuffer + (X * mBltLibBytesPerPixel));
        *(UINT32*) Blt =
          (UINT32) (
              (((Uint32 & mPixelBitMasks.RedMask)   >> mPixelShl[0]) << mPixelShr[0]) |
              (((Uint32 & mPixelBitMasks.GreenMask) >> mPixelShl[1]) << mPixelShr[1]) |
              (((Uint32 & mPixelBitMasks.BlueMask)  >> mPixelShl[2]) << mPixelShr[2])
            );
      }
    }
  }

  return EFI_SUCCESS;
}


/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation.

  @param[in]  BltBuffer     Output buffer for pixel color data
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibBufferToVideo (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  return BltLibBufferToVideoEx (
           BltBuffer,
           0,
           0,
           DestinationX,
           DestinationY,
           Width,
           Height,
           0
           );
}


/**
  Performs a UEFI Graphics Output Protocol Blt Buffer to Video operation
  with extended parameters.

  @param[in]  BltBuffer     Output buffer for pixel color data
  @param[in]  SourceX       X location within BltBuffer
  @param[in]  SourceY       Y location within BltBuffer
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height
  @param[in]  Delta         Number of bytes in a row of BltBuffer

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibBufferToVideoEx (
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL         *BltBuffer,
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height,
  IN  UINTN                                 Delta
  )
{
  UINTN                           DstY;
  UINTN                           SrcY;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *Blt;
  VOID                            *BltMemSrc;
  VOID                            *BltMemDst;
  UINTN                           X;
  UINT32                          Uint32;
  UINTN                           Offset;
  UINTN                           WidthInBytes;

  //
  // BltBuffer to Video: Source is BltBuffer, destination is Video
  //
  if (DestinationY + Height > mBltLibHeight) {
    return EFI_INVALID_PARAMETER;
  }

  if (DestinationX + Width > mBltLibWidthInPixels) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If Delta is zero, then the entire BltBuffer is being used, so Delta
  // is the number of bytes in each row of BltBuffer.  Since BltBuffer is Width pixels size,
  // the number of bytes in each row can be computed.
  //
  if (Delta == 0) {
    Delta = Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  }

  WidthInBytes = Width * mBltLibBytesPerPixel;

  for (SrcY = SourceY, DstY = DestinationY; SrcY < (Height + SourceY); SrcY++, DstY++) {

    Offset = (DstY * mBltLibWidthInPixels) + DestinationX;
    Offset = mBltLibBytesPerPixel * Offset;
    BltMemDst = (VOID*) (mBltLibFrameBuffer + Offset);

    if (mPixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
      BltMemSrc = (VOID *) ((UINT8 *) BltBuffer + (SrcY * Delta));
    } else {
      for (X = 0; X < Width; X++) {
        Blt =
          (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *) (
              (UINT8 *) BltBuffer +
              (SrcY * Delta) +
              ((SourceX + X) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL))
            );
        Uint32 = *(UINT32*) Blt;
        *(UINT32*) (mBltLibLineBuffer + (X * mBltLibBytesPerPixel)) =
          (UINT32) (
              (((Uint32 << mPixelShl[0]) >> mPixelShr[0]) & mPixelBitMasks.RedMask) |
              (((Uint32 << mPixelShl[1]) >> mPixelShr[1]) & mPixelBitMasks.GreenMask) |
              (((Uint32 << mPixelShl[2]) >> mPixelShr[2]) & mPixelBitMasks.BlueMask)
            );
      }
      BltMemSrc = (VOID *) mBltLibLineBuffer;
    }

    CopyMem (BltMemDst, BltMemSrc, WidthInBytes);
  }

  return EFI_SUCCESS;
}


/**
  Performs a UEFI Graphics Output Protocol Blt Video to Video operation

  @param[in]  SourceX       X location within video
  @param[in]  SourceY       Y location within video
  @param[in]  DestinationX  X location within video
  @param[in]  DestinationY  Y location within video
  @param[in]  Width         Width (in pixels)
  @param[in]  Height        Height

  @retval  EFI_DEVICE_ERROR - A hardware error occured
  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibVideoToVideo (
  IN  UINTN                                 SourceX,
  IN  UINTN                                 SourceY,
  IN  UINTN                                 DestinationX,
  IN  UINTN                                 DestinationY,
  IN  UINTN                                 Width,
  IN  UINTN                                 Height
  )
{
  VOID                            *BltMemSrc;
  VOID                            *BltMemDst;
  UINTN                           Offset;
  UINTN                           WidthInBytes;
  INTN                            LineStride;

  //
  // Video to Video: Source is Video, destination is Video
  //
  if (SourceY + Height > mBltLibHeight) {
    return EFI_INVALID_PARAMETER;
  }

  if (SourceX + Width > mBltLibWidthInPixels) {
    return EFI_INVALID_PARAMETER;
  }

  if (DestinationY + Height > mBltLibHeight) {
    return EFI_INVALID_PARAMETER;
  }

  if (DestinationX + Width > mBltLibWidthInPixels) {
    return EFI_INVALID_PARAMETER;
  }

  if (Width == 0 || Height == 0) {
    return EFI_INVALID_PARAMETER;
  }

  WidthInBytes = Width * mBltLibBytesPerPixel;

  Offset = (SourceY * mBltLibWidthInPixels) + SourceX;
  Offset = mBltLibBytesPerPixel * Offset;
  BltMemSrc = (VOID *) (mBltLibFrameBuffer + Offset);

  Offset = (DestinationY * mBltLibWidthInPixels) + DestinationX;
  Offset = mBltLibBytesPerPixel * Offset;
  BltMemDst = (VOID *) (mBltLibFrameBuffer + Offset);

  LineStride = mBltLibWidthInBytes;
  if ((UINTN) BltMemDst > (UINTN) BltMemSrc) {
    LineStride = -LineStride;
  }

  while (Height > 0) {
    CopyMem (BltMemDst, BltMemSrc, WidthInBytes);

    BltMemSrc = (VOID*) ((UINT8*) BltMemSrc + LineStride);
    BltMemDst = (VOID*) ((UINT8*) BltMemDst + LineStride);
    Height--;
  }

  return EFI_SUCCESS;
}


/**
  Returns the sizes related to the video device

  @param[out]  Width   Width (in pixels)
  @param[out]  Height  Height (in pixels)

  @retval  EFI_INVALID_PARAMETER - Invalid parameter passed in
  @retval  EFI_SUCCESS - The sizes were returned

**/
EFI_STATUS
EFIAPI
BltLibGetSizes (
  OUT UINTN                                 *Width,  OPTIONAL
  OUT UINTN                                 *Height  OPTIONAL
  )
{
  if (Width != NULL) {
    *Width = mBltLibWidthInPixels;
  }
  if (Height != NULL) {
    *Height = mBltLibHeight;
  }

  return EFI_SUCCESS;
}

