/** @file

 Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

 **/

#include <PiDxe.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/GlobalVariable.h>

#include "LcdGraphicsOutputDxe.h"

extern BOOLEAN mDisplayInitialized;

//
// Function Definitions
//

STATIC
EFI_STATUS
VideoCopyNoHorizontalOverlap (
  IN UINTN          BitsPerPixel,
  IN volatile VOID  *FrameBufferBase,
  IN UINT32         HorizontalResolution,
  IN UINTN          SourceX,
  IN UINTN          SourceY,
  IN UINTN          DestinationX,
  IN UINTN          DestinationY,
  IN UINTN          Width,
  IN UINTN          Height
)
{
  EFI_STATUS    Status;
  UINTN         SourceLine;
  UINTN         DestinationLine;
  UINTN         WidthInBytes;
  UINTN         LineCount;
  INTN          Step;
  VOID          *SourceAddr;
  VOID          *DestinationAddr;

  Status = EFI_SUCCESS;

  if( DestinationY <= SourceY ) {
    // scrolling up (or horizontally but without overlap)
    SourceLine       = SourceY;
    DestinationLine  = DestinationY;
    Step             = 1;
  } else {
    // scrolling down
    SourceLine       = SourceY + Height;
    DestinationLine  = DestinationY + Height;
    Step             = -1;
  }

  switch (BitsPerPixel) {

  case LcdBitsPerPixel_24:

    WidthInBytes = Width * 4;

    for( LineCount = 0; LineCount < Height; LineCount++ ) {
      // Update the start addresses of source & destination using 32bit pointer arithmetic
      SourceAddr      = (VOID *)((UINT32 *)FrameBufferBase + SourceLine      * HorizontalResolution + SourceX     );
      DestinationAddr = (VOID *)((UINT32 *)FrameBufferBase + DestinationLine * HorizontalResolution + DestinationX);

      // Copy the entire line Y from video ram to the temp buffer
      CopyMem( DestinationAddr, SourceAddr, WidthInBytes);

      // Update the line numbers
      SourceLine      += Step;
      DestinationLine += Step;
    }
    break;

  case LcdBitsPerPixel_16_555:
  case LcdBitsPerPixel_16_565:
  case LcdBitsPerPixel_12_444:

    WidthInBytes = Width * 2;

    for( LineCount = 0; LineCount < Height; LineCount++ ) {
      // Update the start addresses of source & destination using 16bit pointer arithmetic
      SourceAddr      = (VOID *)((UINT16 *)FrameBufferBase + SourceLine      * HorizontalResolution + SourceX     );
      DestinationAddr = (VOID *)((UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution + DestinationX);

      // Copy the entire line Y from video ram to the temp buffer
      CopyMem( DestinationAddr, SourceAddr, WidthInBytes);

      // Update the line numbers
      SourceLine      += Step;
      DestinationLine += Step;
    }
    break;

  case LcdBitsPerPixel_8:
  case LcdBitsPerPixel_4:
  case LcdBitsPerPixel_2:
  case LcdBitsPerPixel_1:
  default:
    // Can't handle this case
    DEBUG((DEBUG_ERROR, "ArmVeGraphics_Blt: EfiBltVideoToVideo: INVALID Number of Bits Per Pixel: %d\n", BitsPerPixel));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
    // break;

  }

  EXIT:
  return Status;
}

STATIC
EFI_STATUS
VideoCopyHorizontalOverlap (
  IN UINTN          BitsPerPixel,
  IN volatile VOID  *FrameBufferBase,
  UINT32            HorizontalResolution,
  IN UINTN          SourceX,
  IN UINTN          SourceY,
  IN UINTN          DestinationX,
  IN UINTN          DestinationY,
  IN UINTN          Width,
  IN UINTN          Height
)
{
  EFI_STATUS      Status;

  UINT32 *PixelBuffer32bit;
  UINT32 *SourcePixel32bit;
  UINT32 *DestinationPixel32bit;

  UINT16 *PixelBuffer16bit;
  UINT16 *SourcePixel16bit;
  UINT16 *DestinationPixel16bit;

  UINT32          SourcePixelY;
  UINT32          DestinationPixelY;
  UINTN           SizeIn32Bits;
  UINTN           SizeIn16Bits;

  Status = EFI_SUCCESS;

  switch (BitsPerPixel) {

  case LcdBitsPerPixel_24:
    // Allocate a temporary buffer

    PixelBuffer32bit = (UINT32 *) AllocatePool((Height * Width) * sizeof(UINT32));

    if (PixelBuffer32bit == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    SizeIn32Bits = Width * 4;

    // Copy from the video ram (source region) to a temp buffer
    for (SourcePixelY = SourceY, DestinationPixel32bit = PixelBuffer32bit;
         SourcePixelY < SourceY + Height;
         SourcePixelY++, DestinationPixel32bit += Width)
    {
      // Update the start address of line Y (source)
      SourcePixel32bit = (UINT32 *)FrameBufferBase + SourcePixelY * HorizontalResolution + SourceX;

      // Copy the entire line Y from video ram to the temp buffer
      CopyMem( (VOID *)DestinationPixel32bit, (CONST VOID *)SourcePixel32bit, SizeIn32Bits);
    }

    // Copy from the temp buffer to the video ram (destination region)
    for (DestinationPixelY = DestinationY, SourcePixel32bit = PixelBuffer32bit;
         DestinationPixelY < DestinationY + Height;
         DestinationPixelY++, SourcePixel32bit += Width)
    {
      // Update the start address of line Y (target)
      DestinationPixel32bit = (UINT32 *)FrameBufferBase + DestinationPixelY * HorizontalResolution + DestinationX;

      // Copy the entire line Y from the temp buffer to video ram
      CopyMem( (VOID *)DestinationPixel32bit, (CONST VOID *)SourcePixel32bit, SizeIn32Bits);
    }

    // Free up the allocated memory
    FreePool((VOID *) PixelBuffer32bit);

    break;


  case LcdBitsPerPixel_16_555:
  case LcdBitsPerPixel_16_565:
  case LcdBitsPerPixel_12_444:
    // Allocate a temporary buffer
    PixelBuffer16bit = (UINT16 *) AllocatePool((Height * Width) * sizeof(UINT16));

    if (PixelBuffer16bit == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto EXIT;
    }

    // Access each pixel inside the source area of the Video Memory and copy it to the temp buffer

    SizeIn16Bits = Width * 2;

    for (SourcePixelY = SourceY, DestinationPixel16bit = PixelBuffer16bit;
         SourcePixelY < SourceY + Height;
         SourcePixelY++, DestinationPixel16bit += Width)
    {
      // Calculate the source address:
      SourcePixel16bit = (UINT16 *)FrameBufferBase + SourcePixelY * HorizontalResolution + SourceX;

      // Copy the entire line Y from Video to the temp buffer
      CopyMem( (VOID *)DestinationPixel16bit, (CONST VOID *)SourcePixel16bit, SizeIn16Bits);
    }

    // Copy from the temp buffer into the destination area of the Video Memory

    for (DestinationPixelY = DestinationY, SourcePixel16bit = PixelBuffer16bit;
         DestinationPixelY < DestinationY + Height;
         DestinationPixelY++, SourcePixel16bit += Width)
    {
      // Calculate the target address:
      DestinationPixel16bit = (UINT16 *)FrameBufferBase + (DestinationPixelY * HorizontalResolution + DestinationX);

      // Copy the entire line Y from the temp buffer to Video
      CopyMem( (VOID *)DestinationPixel16bit, (CONST VOID *)SourcePixel16bit, SizeIn16Bits);
    }

    // Free the allocated memory
    FreePool((VOID *) PixelBuffer16bit);

    break;


  case LcdBitsPerPixel_8:
  case LcdBitsPerPixel_4:
  case LcdBitsPerPixel_2:
  case LcdBitsPerPixel_1:
  default:
    // Can't handle this case
    DEBUG((DEBUG_ERROR, "ArmVeGraphics_Blt: EfiBltVideoToVideo: INVALID Number of Bits Per Pixel: %d\n", BitsPerPixel));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
    // break;

  }

EXIT:
  return Status;
}

STATIC
EFI_STATUS
BltVideoFill (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *EfiSourcePixel,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_PIXEL_BITMASK*  PixelInformation;
  EFI_STATUS         Status;
  UINT32             HorizontalResolution;
  LCD_BPP            BitsPerPixel;
  VOID            *FrameBufferBase;
  VOID            *DestinationAddr;
  UINT16          *DestinationPixel16bit;
  UINT16          Pixel16bit;
  UINT32          DestinationPixelX;
  UINT32          DestinationLine;
  UINTN           WidthInBytes;

  Status           = EFI_SUCCESS;
  PixelInformation = &This->Mode->Info->PixelInformation;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));
  HorizontalResolution = This->Mode->Info->HorizontalResolution;

  LcdPlatformGetBpp (This->Mode->Mode,&BitsPerPixel);

  switch (BitsPerPixel) {
  case LcdBitsPerPixel_24:
    WidthInBytes = Width * 4;

    // Copy the SourcePixel into every pixel inside the target rectangle
    for (DestinationLine = DestinationY;
         DestinationLine < DestinationY + Height;
         DestinationLine++)
    {
      // Calculate the target address using 32bit pointer arithmetic:
      DestinationAddr = (VOID *)((UINT32 *)FrameBufferBase + DestinationLine * HorizontalResolution  + DestinationX);

      // Fill the entire line
      SetMem32 (DestinationAddr, WidthInBytes, *((UINT32 *)EfiSourcePixel));
    }
    break;

  case LcdBitsPerPixel_16_555:
    // Convert the EFI pixel at the start of the BltBuffer(0,0) into a video display pixel
    Pixel16bit = (UINT16) (
        ( (EfiSourcePixel->Red      <<  7) & PixelInformation->RedMask      )
      | ( (EfiSourcePixel->Green    <<  2) & PixelInformation->GreenMask    )
      | ( (EfiSourcePixel->Blue     >>  3) & PixelInformation->BlueMask     )
//      | ( 0                           & PixelInformation->ReservedMask )
     );

    // Copy the SourcePixel into every pixel inside the target rectangle
    for (DestinationLine = DestinationY;
         DestinationLine < DestinationY + Height;
         DestinationLine++)
    {
      for (DestinationPixelX = DestinationX;
           DestinationPixelX < DestinationX + Width;
           DestinationPixelX++)
      {
        // Calculate the target address:
        DestinationPixel16bit =  (UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution + DestinationPixelX;

        // Copy the pixel into the new target
        *DestinationPixel16bit = Pixel16bit;
      }
    }
    break;

  case LcdBitsPerPixel_16_565:
    // Convert the EFI pixel at the start of the BltBuffer(0,0) into a video display pixel
    Pixel16bit = (UINT16) (
        ( (EfiSourcePixel->Red      <<  8) & PixelInformation->RedMask      )
      | ( (EfiSourcePixel->Green    <<  3) & PixelInformation->GreenMask    )
      | ( (EfiSourcePixel->Blue     >>  3) & PixelInformation->BlueMask     )
     );

    // Copy the SourcePixel into every pixel inside the target rectangle
    for (DestinationLine = DestinationY;
         DestinationLine < DestinationY + Height;
         DestinationLine++)
    {
      for (DestinationPixelX = DestinationX;
           DestinationPixelX < DestinationX + Width;
           DestinationPixelX++)
      {
        // Calculate the target address:
        DestinationPixel16bit =  (UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution  + DestinationPixelX;

        // Copy the pixel into the new target
        *DestinationPixel16bit = Pixel16bit;
      }
    }
    break;

  case LcdBitsPerPixel_12_444:
    // Convert the EFI pixel at the start of the BltBuffer(0,0) into a video display pixel
    Pixel16bit = (UINT16) (
        ( (EfiSourcePixel->Red      >> 4) & PixelInformation->RedMask      )
      | ( (EfiSourcePixel->Green        ) & PixelInformation->GreenMask    )
      | ( (EfiSourcePixel->Blue     << 4) & PixelInformation->BlueMask     )
     );

    // Copy the SourcePixel into every pixel inside the target rectangle
    for (DestinationLine = DestinationY;
         DestinationLine < DestinationY + Height;
         DestinationLine++)
    {
      for (DestinationPixelX = DestinationX;
           DestinationPixelX < DestinationX + Width;
           DestinationPixelX++)
      {
        // Calculate the target address:
        DestinationPixel16bit =  (UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution  + DestinationPixelX;

        // Copy the pixel into the new target
        *DestinationPixel16bit = Pixel16bit;
      }
    }
    break;

  case LcdBitsPerPixel_8:
  case LcdBitsPerPixel_4:
  case LcdBitsPerPixel_2:
  case LcdBitsPerPixel_1:
  default:
    // Can't handle this case
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: EfiBltVideoFill: INVALID Number of Bits Per Pixel: %d\n", BitsPerPixel));
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

STATIC
EFI_STATUS
BltVideoToBltBuffer (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             HorizontalResolution;
  LCD_BPP            BitsPerPixel;
  EFI_PIXEL_BITMASK  *PixelInformation;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *EfiDestinationPixel;
  VOID   *FrameBufferBase;
  VOID            *SourceAddr;
  VOID            *DestinationAddr;
  UINT16 *SourcePixel16bit;
  UINT16          Pixel16bit;
  UINT32          SourcePixelX;
  UINT32          SourceLine;
  UINT32          DestinationPixelX;
  UINT32          DestinationLine;
  UINT32          BltBufferHorizontalResolution;
  UINTN           WidthInBytes;

  Status = EFI_SUCCESS;
  PixelInformation = &This->Mode->Info->PixelInformation;
  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  if(( Delta != 0 ) && ( Delta != Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    // Delta is not zero and it is different from the width.
    // Divide it by the size of a pixel to find out the buffer's horizontal resolution.
    BltBufferHorizontalResolution = (UINT32) (Delta / sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    BltBufferHorizontalResolution = Width;
  }

  LcdPlatformGetBpp (This->Mode->Mode,&BitsPerPixel);

  switch (BitsPerPixel) {
  case LcdBitsPerPixel_24:
    WidthInBytes = Width * 4;

    // Access each line inside the Video Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
         SourceLine < SourceY + Height;
         SourceLine++, DestinationLine++)
    {
      // Calculate the source and target addresses using 32bit pointer arithmetic:
      SourceAddr      = (VOID *)((UINT32 *)FrameBufferBase + SourceLine      * HorizontalResolution          + SourceX     );
      DestinationAddr = (VOID *)((UINT32 *)BltBuffer       + DestinationLine * BltBufferHorizontalResolution + DestinationX);

      // Copy the entire line
      CopyMem( DestinationAddr, SourceAddr, WidthInBytes);
    }
    break;

  case LcdBitsPerPixel_16_555:
    // Access each pixel inside the Video Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
         SourceLine < SourceY + Height;
         SourceLine++, DestinationLine++)
    {
      for (SourcePixelX = SourceX, DestinationPixelX = DestinationX;
           SourcePixelX < SourceX + Width;
           SourcePixelX++, DestinationPixelX++)
      {
        // Calculate the source and target addresses:
        SourcePixel16bit = (UINT16 *)FrameBufferBase + SourceLine * HorizontalResolution + SourcePixelX;
        EfiDestinationPixel = BltBuffer + DestinationLine * BltBufferHorizontalResolution + DestinationPixelX;

        // Snapshot the pixel from the video buffer once, to speed up the operation.
        // If we were dereferencing the pointer, as it is volatile, we would perform 3 memory read operations.
        Pixel16bit = *SourcePixel16bit;

        // Copy the pixel into the new target
        EfiDestinationPixel->Red      = (UINT8) ( (Pixel16bit & PixelInformation->RedMask     ) >>  7 );
        EfiDestinationPixel->Green    = (UINT8) ( (Pixel16bit & PixelInformation->GreenMask   ) >>  2);
        EfiDestinationPixel->Blue     = (UINT8) ( (Pixel16bit & PixelInformation->BlueMask    ) <<  3 );
        // EfiDestinationPixel->Reserved = (UINT8) 0;
      }
    }
    break;

  case LcdBitsPerPixel_16_565:
    // Access each pixel inside the Video Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
         SourceLine < SourceY + Height;
         SourceLine++, DestinationLine++)
    {
      for (SourcePixelX = SourceX, DestinationPixelX = DestinationX;
           SourcePixelX < SourceX + Width;
           SourcePixelX++, DestinationPixelX++)
      {
        // Calculate the source and target addresses:
        SourcePixel16bit = (UINT16 *)FrameBufferBase + SourceLine * HorizontalResolution + SourcePixelX;
        EfiDestinationPixel = BltBuffer + DestinationLine * BltBufferHorizontalResolution + DestinationPixelX;

        // Snapshot the pixel from the video buffer once, to speed up the operation.
        // If we were dereferencing the pointer, as it is volatile, we would perform 3 memory read operations.
        Pixel16bit = *SourcePixel16bit;

        // Copy the pixel into the new target
        // There is no info for the Reserved byte, so we set it to zero
        EfiDestinationPixel->Red      = (UINT8) ( (Pixel16bit & PixelInformation->RedMask     ) >> 8 );
        EfiDestinationPixel->Green    = (UINT8) ( (Pixel16bit & PixelInformation->GreenMask   ) >> 3);
        EfiDestinationPixel->Blue     = (UINT8) ( (Pixel16bit & PixelInformation->BlueMask    ) << 3 );
        // EfiDestinationPixel->Reserved = (UINT8) 0;
      }
    }
    break;

  case LcdBitsPerPixel_12_444:
    // Access each pixel inside the Video Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
         SourceLine < SourceY + Height;
         SourceLine++, DestinationLine++)
    {
      for (SourcePixelX = SourceX, DestinationPixelX = DestinationX;
           SourcePixelX < SourceX + Width;
           SourcePixelX++, DestinationPixelX++)
      {
        // Calculate the source and target addresses:
        SourcePixel16bit = (UINT16 *)FrameBufferBase + SourceLine * HorizontalResolution + SourcePixelX;
        EfiDestinationPixel = BltBuffer + DestinationLine * BltBufferHorizontalResolution + DestinationPixelX;

        // Snapshot the pixel from the video buffer once, to speed up the operation.
        // If we were dereferencing the pointer, as it is volatile, we would perform 3 memory read operations.
        Pixel16bit = *SourcePixel16bit;

        // Copy the pixel into the new target
        EfiDestinationPixel->Red      = (UINT8) ( (Pixel16bit & PixelInformation->RedMask     ) >> 4 );
        EfiDestinationPixel->Green    = (UINT8) ( (Pixel16bit & PixelInformation->GreenMask   )     );
        EfiDestinationPixel->Blue     = (UINT8) ( (Pixel16bit & PixelInformation->BlueMask    ) << 4 );
        // EfiDestinationPixel->Reserved = (UINT8) 0;
      }
    }
    break;

  case LcdBitsPerPixel_8:
  case LcdBitsPerPixel_4:
  case LcdBitsPerPixel_2:
  case LcdBitsPerPixel_1:
  default:
    // Can't handle this case
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: EfiBltVideoToBltBuffer: INVALID Number of Bits Per Pixel: %d\n", BitsPerPixel));
    Status = EFI_INVALID_PARAMETER;
    break;
  }
  return Status;
}

STATIC
EFI_STATUS
BltBufferToVideo (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             HorizontalResolution;
  LCD_BPP            BitsPerPixel;
  EFI_PIXEL_BITMASK  *PixelInformation;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *EfiSourcePixel;
  VOID   *FrameBufferBase;
  VOID            *SourceAddr;
  VOID            *DestinationAddr;
  UINT16 *DestinationPixel16bit;
  UINT32          SourcePixelX;
  UINT32          SourceLine;
  UINT32          DestinationPixelX;
  UINT32          DestinationLine;
  UINT32          BltBufferHorizontalResolution;
  UINTN           WidthInBytes;

  Status = EFI_SUCCESS;
  PixelInformation = &This->Mode->Info->PixelInformation;
  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  if(( Delta != 0 ) && ( Delta != Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    // Delta is not zero and it is different from the width.
    // Divide it by the size of a pixel to find out the buffer's horizontal resolution.
    BltBufferHorizontalResolution = (UINT32) (Delta / sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    BltBufferHorizontalResolution = Width;
  }

  LcdPlatformGetBpp (This->Mode->Mode,&BitsPerPixel);

  switch (BitsPerPixel) {
  case LcdBitsPerPixel_24:
    WidthInBytes = Width * 4;

    // Access each pixel inside the BltBuffer Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
       SourceLine < SourceY + Height;
       SourceLine++, DestinationLine++)
    {
      // Calculate the source and target addresses using 32bit pointer arithmetic:
      SourceAddr      = (VOID *)((UINT32 *)BltBuffer       + SourceLine      * BltBufferHorizontalResolution + SourceX     );
      DestinationAddr = (VOID *)((UINT32 *)FrameBufferBase + DestinationLine * HorizontalResolution          + DestinationX);

      // Copy the entire row Y
      CopyMem( DestinationAddr, SourceAddr, WidthInBytes);
    }
    break;

  case LcdBitsPerPixel_16_555:
    // Access each pixel inside the BltBuffer Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
       SourceLine < SourceY + Height;
       SourceLine++, DestinationLine++) {

      for (SourcePixelX = SourceX, DestinationPixelX = DestinationX;
           SourcePixelX < SourceX + Width;
           SourcePixelX++, DestinationPixelX++)
      {
        // Calculate the source and target addresses:
        EfiSourcePixel  = BltBuffer + SourceLine * BltBufferHorizontalResolution + SourcePixelX;
        DestinationPixel16bit = (UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution + DestinationPixelX;

        // Copy the pixel into the new target
        // Only the most significant bits will be copied across:
        // To convert from 8 bits to 5 bits per pixel we throw away the 3 least significant bits
        *DestinationPixel16bit = (UINT16) (
              ( (EfiSourcePixel->Red      <<  7) & PixelInformation->RedMask      )
            | ( (EfiSourcePixel->Green    <<  2) & PixelInformation->GreenMask    )
            | ( (EfiSourcePixel->Blue     >>  3) & PixelInformation->BlueMask     )
      //            | ( 0                                & PixelInformation->ReservedMask )
            );
      }
    }
    break;

  case LcdBitsPerPixel_16_565:
    // Access each pixel inside the BltBuffer Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
         SourceLine < SourceY + Height;
         SourceLine++, DestinationLine++) {

      for (SourcePixelX = SourceX, DestinationPixelX = DestinationX;
           SourcePixelX < SourceX + Width;
           SourcePixelX++, DestinationPixelX++)
      {
        // Calculate the source and target addresses:
        EfiSourcePixel = BltBuffer + SourceLine * BltBufferHorizontalResolution + SourcePixelX;
        DestinationPixel16bit = (UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution + DestinationPixelX;

        // Copy the pixel into the new target
        // Only the most significant bits will be copied across:
        // To convert from 8 bits to 5 or 6 bits per pixel we throw away the 3 or 2  least significant bits
        // There is no room for the Reserved byte so we ignore that completely
        *DestinationPixel16bit = (UINT16) (
              ( (EfiSourcePixel->Red      <<  8) & PixelInformation->RedMask      )
            | ( (EfiSourcePixel->Green    <<  3) & PixelInformation->GreenMask    )
            | ( (EfiSourcePixel->Blue     >>  3) & PixelInformation->BlueMask     )
           );
      }
    }
    break;

  case LcdBitsPerPixel_12_444:
    // Access each pixel inside the BltBuffer Memory
    for (SourceLine = SourceY, DestinationLine = DestinationY;
         SourceLine < SourceY + Height;
         SourceLine++, DestinationLine++) {

      for (SourcePixelX = SourceX, DestinationPixelX = DestinationX;
           SourcePixelX < SourceX + Width;
           SourcePixelX++, DestinationPixelX++)
      {
        // Calculate the source and target addresses:
        EfiSourcePixel = BltBuffer + SourceLine * BltBufferHorizontalResolution + SourcePixelX;
        DestinationPixel16bit = (UINT16 *)FrameBufferBase + DestinationLine * HorizontalResolution + DestinationPixelX;

        // Copy the pixel into the new target
        // Only the most significant bits will be copied across:
        // To convert from 8 bits to 5 bits per pixel we throw away the 3 least significant bits
        *DestinationPixel16bit = (UINT16) (
              ( (EfiSourcePixel->Red      << 4) & PixelInformation->RedMask      )
            | ( (EfiSourcePixel->Green        ) & PixelInformation->GreenMask    )
            | ( (EfiSourcePixel->Blue     >> 4) & PixelInformation->BlueMask     )
  //            | ( 0                               & PixelInformation->ReservedMask )
           );
      }
    }
    break;

  case LcdBitsPerPixel_8:
  case LcdBitsPerPixel_4:
  case LcdBitsPerPixel_2:
  case LcdBitsPerPixel_1:
  default:
    // Can't handle this case
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: EfiBltBufferToVideo: INVALID Number of Bits Per Pixel: %d\n", BitsPerPixel));
    Status = EFI_INVALID_PARAMETER;
    break;
  }
  return Status;
}

STATIC
EFI_STATUS
BltVideoToVideo (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             HorizontalResolution;
  LCD_BPP            BitsPerPixel;
  VOID   *FrameBufferBase;

  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  //
  // BltVideo to BltVideo:
  //
  //  Source is the Video Memory,
  //  Destination is the Video Memory

  LcdPlatformGetBpp (This->Mode->Mode,&BitsPerPixel);
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  // The UEFI spec currently states:
  // "There is no limitation on the overlapping of the source and destination rectangles"
  // Therefore, we must be careful to avoid overwriting the source data
  if( SourceY == DestinationY ) {
    // Copying within the same height, e.g. horizontal shift
    if( SourceX == DestinationX ) {
      // Nothing to do
      Status = EFI_SUCCESS;
    } else if( ((SourceX>DestinationX)?(SourceX - DestinationX):(DestinationX - SourceX)) < Width ) {
      // There is overlap
      Status = VideoCopyHorizontalOverlap (BitsPerPixel, FrameBufferBase, HorizontalResolution, SourceX, SourceY, DestinationX, DestinationY, Width, Height );
    } else {
      // No overlap
      Status = VideoCopyNoHorizontalOverlap (BitsPerPixel, FrameBufferBase, HorizontalResolution, SourceX, SourceY, DestinationX, DestinationY, Width, Height );
    }
  } else {
    // Copying from different heights
    Status = VideoCopyNoHorizontalOverlap (BitsPerPixel, FrameBufferBase, HorizontalResolution, SourceX, SourceY, DestinationX, DestinationY, Width, Height );
  }

  return Status;
}

/***************************************
 * GraphicsOutput Protocol function, mapping to
 * EFI_GRAPHICS_OUTPUT_PROTOCOL.Blt
 *
 * PRESUMES: 1 pixel = 4 bytes (32bits)
 *  ***************************************/
EFI_STATUS
EFIAPI
LcdGraphicsBlt (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION   BltOperation,
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             HorizontalResolution;
  UINT32             VerticalResolution;
  LCD_INSTANCE*      Instance;

  Instance = LCD_INSTANCE_FROM_GOP_THIS(This);

  // Setup the hardware if not already done
  if (!mDisplayInitialized) {
    Status = InitializeDisplay (Instance);
    if (EFI_ERROR(Status)) {
      goto EXIT;
    }
  }

  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  VerticalResolution   = This->Mode->Info->VerticalResolution;

  DEBUG((DEBUG_INFO, "LcdGraphicsBlt (BltOperation:%d,DestX:%d,DestY:%d,Width:%d,Height:%d) res(%d,%d)\n",
      BltOperation,DestinationX,DestinationY,Width,Height,HorizontalResolution,VerticalResolution));

  // Check we have reasonable parameters
  if (Width == 0 || Height == 0) {
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: ERROR - Invalid dimension: Zero size area.\n" ));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  if ((BltOperation == EfiBltVideoFill) || (BltOperation == EfiBltBufferToVideo) || (BltOperation == EfiBltVideoToBltBuffer)) {
    ASSERT( BltBuffer != NULL);
  }

  /*if ((DestinationX >= HorizontalResolution) || (DestinationY >= VerticalResolution)) {
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: ERROR - Invalid destination.\n" ));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }*/

  // If we are reading data out of the video buffer, check that the source area is within the display limits
  if ((BltOperation == EfiBltVideoToBltBuffer) || (BltOperation == EfiBltVideoToVideo)) {
    if ((SourceY + Height > VerticalResolution) || (SourceX + Width > HorizontalResolution)) {
      DEBUG((DEBUG_INFO, "LcdGraphicsBlt: ERROR - Invalid source resolution.\n" ));
      DEBUG((DEBUG_INFO, "                      - SourceY=%d + Height=%d > VerticalResolution=%d.\n", SourceY, Height, VerticalResolution ));
      DEBUG((DEBUG_INFO, "                      - SourceX=%d + Width=%d > HorizontalResolution=%d.\n", SourceX, Width, HorizontalResolution ));
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }
  }

  // If we are writing data into the video buffer, that the destination area is within the display limits
  if ((BltOperation == EfiBltVideoFill) || (BltOperation == EfiBltBufferToVideo) || (BltOperation == EfiBltVideoToVideo)) {
    if ((DestinationY + Height > VerticalResolution) || (DestinationX + Width > HorizontalResolution)) {
      DEBUG((DEBUG_INFO, "LcdGraphicsBlt: ERROR - Invalid destination resolution.\n" ));
      DEBUG((DEBUG_INFO, "                      - DestinationY=%d + Height=%d > VerticalResolution=%d.\n", DestinationY, Height, VerticalResolution ));
      DEBUG((DEBUG_INFO, "                      - DestinationX=%d + Width=%d > HorizontalResolution=%d.\n", DestinationX, Width, HorizontalResolution ));
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }
  }

  //
  // Perform the Block Transfer Operation
  //

  switch (BltOperation) {
  case EfiBltVideoFill:
    Status = BltVideoFill (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    break;

  case EfiBltVideoToBltBuffer:
    Status = BltVideoToBltBuffer (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    break;

  case EfiBltBufferToVideo:
    Status = BltBufferToVideo (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    break;

  case EfiBltVideoToVideo:
    Status = BltVideoToVideo (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    break;

  case EfiGraphicsOutputBltOperationMax:
  default:
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: Invalid Operation\n"));
    Status = EFI_INVALID_PARAMETER;
    break;
  }

EXIT:
  return Status;
}
