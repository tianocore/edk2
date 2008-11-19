/**@file
  Library supports diplaying graphical splash screen,
  locking of keyboard input and printing character on
  screen. These basic graphics operations are based on UEFI HII, 
  Graphics Output protocol or UGA Draw protocol.

  BugBug: Currently *.BMP files are supported. This will be replaced
          when Tiano graphics format is supported.


Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiDxe.h>

#include <Protocol/SimpleTextOut.h>
#include <Protocol/OEMBadging.h>
#include <Protocol/ConsoleControl.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/FrameworkHii.h>

#include <Guid/Bmp.h>

#include <Library/GraphicsLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

/**
  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

  @param FileNameGuid  - File Name of graphics file in the FV(s).

  @param Image         - Pointer to pointer to return graphics image.  If NULL, a
                         buffer will be allocated.

  @param ImageSize     - Size of the graphics Image in bytes. Zero if no image found.

  @retval EFI_SUCCESS          - Image and ImageSize are valid.
  @retval EFI_BUFFER_TOO_SMALL - Image not big enough. ImageSize has required size
  @retval EFI_NOT_FOUND        - FileNameGuid not found

**/

EFI_STATUS
GetGraphicsBitMapFromFV (
  IN  EFI_GUID      *FileNameGuid,
  OUT VOID          **Image,
  OUT UINTN         *ImageSize
  )
{
  EFI_STATUS                    Status;
  UINTN                         FvProtocolCount;
  EFI_HANDLE                    *FvHandles;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  UINTN                         Index;
  UINT32                        AuthenticationStatus;


  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &FvProtocolCount,
                  &FvHandles
                  );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < FvProtocolCount; Index++) {
    Status = gBS->HandleProtocol (
                    FvHandles[Index],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **) &Fv
                    );

    //
    // Assuming Image and ImageSize are correct on input.
    //
    Status = Fv->ReadSection (
                  Fv,
                  FileNameGuid,
                  EFI_SECTION_RAW,
                  0,
                  Image,
                  ImageSize,
                  &AuthenticationStatus
                  );
    if (!EFI_ERROR (Status)) {
      return EFI_SUCCESS;
    } else if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // ImageSize updated to needed size so return
      //
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Convert a *.BMP graphics image to a GOP/UGA blt buffer. If a NULL Blt buffer
  is passed in a GopBlt buffer will be allocated by this routine. If a GopBlt
  buffer is passed in it will be used if it is big enough.

  @param BmpImage      - Pointer to BMP file

  @param BmpImageSize  - Number of bytes in BmpImage

  @param GopBlt        - Buffer containing GOP version of BmpImage.

  @param GopBltSize    - Size of GopBlt in bytes.

  @param PixelHeight   - Height of GopBlt/BmpImage in pixels

  @param PixelWidth    - Width of GopBlt/BmpImage in pixels


  @retval EFI_SUCCESS           - GopBlt and GopBltSize are returned.
  @retval EFI_UNSUPPORTED       - BmpImage is not a valid *.BMP image
  @retval EFI_BUFFER_TOO_SMALL  - The passed in GopBlt buffer is not big enough.
                                  GopBltSize will contain the required size.
  @retval EFI_OUT_OF_RESOURCES  - No enough buffer to allocate

**/
EFI_STATUS
ConvertBmpToGopBlt (
  IN  VOID      *BmpImage,
  IN  UINTN     BmpImageSize,
  IN OUT VOID   **GopBlt,
  IN OUT UINTN  *GopBltSize,
  OUT UINTN     *PixelHeight,
  OUT UINTN     *PixelWidth
  )

{
  UINT8             *Image;
  UINT8             *ImageHeader;
  BMP_IMAGE_HEADER  *BmpHeader;
  BMP_COLOR_MAP     *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINTN             BltBufferSize;
  UINTN             Index;
  UINTN             Height;
  UINTN             Width;
  UINTN             ImageIndex;
  BOOLEAN           IsAllocated;

  BmpHeader = (BMP_IMAGE_HEADER *) BmpImage;
  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    return EFI_UNSUPPORTED;
  }

  if (BmpHeader->CompressionType != 0) {
    return EFI_UNSUPPORTED;
  }

  //
  // Calculate Color Map offset in the image.
  //
  Image       = BmpImage;
  BmpColorMap = (BMP_COLOR_MAP *) (Image + sizeof (BMP_IMAGE_HEADER));

  //
  // Calculate graphics image data address in the image
  //
  Image         = ((UINT8 *) BmpImage) + BmpHeader->ImageOffset;
  ImageHeader   = Image;

  BltBufferSize = BmpHeader->PixelWidth * BmpHeader->PixelHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  IsAllocated   = FALSE;
  if (*GopBlt == NULL) {
    *GopBltSize = BltBufferSize;
    *GopBlt     = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    if (*GopBltSize < BltBufferSize) {
      *GopBltSize = BltBufferSize;
      return EFI_BUFFER_TOO_SMALL;
    }
  }

  *PixelWidth   = BmpHeader->PixelWidth;
  *PixelHeight  = BmpHeader->PixelHeight;

  //
  // Convert image from BMP to Blt buffer format
  //
  BltBuffer = *GopBlt;
  for (Height = 0; Height < BmpHeader->PixelHeight; Height++) {
    Blt = &BltBuffer[(BmpHeader->PixelHeight - Height - 1) * BmpHeader->PixelWidth];
    for (Width = 0; Width < BmpHeader->PixelWidth; Width++, Image++, Blt++) {
      switch (BmpHeader->BitPerPixel) {
      case 1:
        //
        // Convert 1bit BMP to 24-bit color
        //
        for (Index = 0; Index < 8 && Width < BmpHeader->PixelWidth; Index++) {
          Blt->Red    = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Red;
          Blt->Green  = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Green;
          Blt->Blue   = BmpColorMap[((*Image) >> (7 - Index)) & 0x1].Blue;
          Blt++;
          Width++;
        }

        Blt --;
        Width --;
        break;

      case 4:
        //
        // Convert BMP Palette to 24-bit color
        //
        Index       = (*Image) >> 4;
        Blt->Red    = BmpColorMap[Index].Red;
        Blt->Green  = BmpColorMap[Index].Green;
        Blt->Blue   = BmpColorMap[Index].Blue;
        if (Width < (BmpHeader->PixelWidth - 1)) {
          Blt++;
          Width++;
          Index       = (*Image) & 0x0f;
          Blt->Red    = BmpColorMap[Index].Red;
          Blt->Green  = BmpColorMap[Index].Green;
          Blt->Blue   = BmpColorMap[Index].Blue;
        }
        break;

      case 8:
        //
        // Convert BMP Palette to 24-bit color
        //
        Blt->Red    = BmpColorMap[*Image].Red;
        Blt->Green  = BmpColorMap[*Image].Green;
        Blt->Blue   = BmpColorMap[*Image].Blue;
        break;

      case 24:
        Blt->Blue   = *Image++;
        Blt->Green  = *Image++;
        Blt->Red    = *Image;
        break;

      default:
        if (IsAllocated) {
          gBS->FreePool (*GopBlt);
          *GopBlt = NULL;
        }
        return EFI_UNSUPPORTED;
        break;
      };

    }

    ImageIndex = (UINTN) (Image - ImageHeader);
    if ((ImageIndex % 4) != 0) {
      //
      // Bmp Image starts each row on a 32-bit boundary!
      //
      Image = Image + (4 - (ImageIndex % 4));
    }
  }

  return EFI_SUCCESS;
}

/**
  Use Console Control Protocol to lock the Console In Spliter virtual handle.
  This is the ConInHandle and ConIn handle in the EFI system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param  Password - Password used to lock ConIn device

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully..
  @retval EFI_UNSUPPORTED password not found

**/
EFI_STATUS
LockKeyboards (
  IN  CHAR16    *Password
  )
{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  Status = gBS->LocateProtocol (&gEfiConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status = ConsoleControl->LockStdIn (ConsoleControl, Password);
  return Status;
}

/**
  Use Console Control to turn off UGA based Simple Text Out consoles from going
  to the UGA device. Put up LogoFile on every UGA device that is a console

  @param LogoFile - File name of logo to display on the center of the screen.

  @retval EFI_SUCCESS           - ConsoleControl has been flipped to graphics and logo
                                  displayed.
  @retval EFI_UNSUPPORTED       - Logo not found

**/
EFI_STATUS
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  )

{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;
  EFI_OEM_BADGING_PROTOCOL      *Badging;
  UINT32                        SizeOfX;
  UINT32                        SizeOfY;
  INTN                          DestX;
  INTN                          DestY;
  UINT8                         *ImageData;
  UINTN                         ImageSize;
  UINTN                         BltSize;
  UINT32                        Instance;
  EFI_BADGING_FORMAT            Format;
  EFI_BADGING_DISPLAY_ATTRIBUTE Attribute;
  UINTN                         CoordinateX;
  UINTN                         CoordinateY;
  UINTN                         Height;
  UINTN                         Width;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  UINT32                        ColorDepth;
  UINT32                        RefreshRate;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;

  Status = gBS->LocateProtocol (&gEfiConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  UgaDraw = NULL;
  //
  // Try to open GOP first
  //
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
  if (EFI_ERROR(Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    GraphicsOutput = NULL;
    //
    // Open GOP failed, try to open UGA
    //
    Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiUgaDrawProtocolGuid, (VOID **) &UgaDraw);
  }
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Badging = NULL;
  Status  = gBS->LocateProtocol (&gEfiOEMBadgingProtocolGuid, NULL, (VOID **) &Badging);

  ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenGraphics);

  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    Status = UgaDraw->GetMode (UgaDraw, &SizeOfX, &SizeOfY, &ColorDepth, &RefreshRate);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  Instance = 0;
  while (1) {
    ImageData = NULL;
    ImageSize = 0;

    if (Badging != NULL) {
      Status = Badging->GetImage (
                          Badging,
                          &Instance,
                          &Format,
                          &ImageData,
                          &ImageSize,
                          &Attribute,
                          &CoordinateX,
                          &CoordinateY
                          );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Currently only support BMP format
      //
      if (Format != EfiBadgingFormatBMP) {
        gBS->FreePool (ImageData);
        continue;
      }
    } else {
      Status = GetGraphicsBitMapFromFV (LogoFile, (VOID **) &ImageData, &ImageSize);
      if (EFI_ERROR (Status)) {
        return EFI_UNSUPPORTED;
      }

      CoordinateX = 0;
      CoordinateY = 0;
      Attribute   = EfiBadgingDisplayAttributeCenter;
    }

    Blt = NULL;
    BltSize = 0;
    Status = ConvertBmpToGopBlt (
              ImageData,
              ImageSize,
              (VOID**)&Blt,
              &BltSize,
              &Height,
              &Width
              );
    if (EFI_ERROR (Status)) {
      gBS->FreePool (ImageData);
      if (Badging == NULL) {
        return Status;
      } else {
        continue;
      }
    }

    switch (Attribute) {
    case EfiBadgingDisplayAttributeLeftTop:
      DestX = CoordinateX;
      DestY = CoordinateY;
      break;

    case EfiBadgingDisplayAttributeCenterTop:
      DestX = (SizeOfX - Width) / 2;
      DestY = CoordinateY;
      break;

    case EfiBadgingDisplayAttributeRightTop:
      DestX = (SizeOfX - Width - CoordinateX);
      DestY = CoordinateY;;
      break;

    case EfiBadgingDisplayAttributeCenterRight:
      DestX = (SizeOfX - Width - CoordinateX);
      DestY = (SizeOfY - Height) / 2;
      break;

    case EfiBadgingDisplayAttributeRightBottom:
      DestX = (SizeOfX - Width - CoordinateX);
      DestY = (SizeOfY - Height - CoordinateY);
      break;

    case EfiBadgingDisplayAttributeCenterBottom:
      DestX = (SizeOfX - Width) / 2;
      DestY = (SizeOfY - Height - CoordinateY);
      break;

    case EfiBadgingDisplayAttributeLeftBottom:
      DestX = CoordinateX;
      DestY = (SizeOfY - Height - CoordinateY);
      break;

    case EfiBadgingDisplayAttributeCenterLeft:
      DestX = CoordinateX;
      DestY = (SizeOfY - Height) / 2;
      break;

    case EfiBadgingDisplayAttributeCenter:
      DestX = (SizeOfX - Width) / 2;
      DestY = (SizeOfY - Height) / 2;
      break;

    default:
      DestX = CoordinateX;
      DestY = CoordinateY;
      break;
    }

    if ((DestX >= 0) && (DestY >= 0)) {
      if (GraphicsOutput != NULL) {
        Status = GraphicsOutput->Blt (
                            GraphicsOutput,
                            Blt,
                            EfiBltBufferToVideo,
                            0,
                            0,
                            (UINTN) DestX,
                            (UINTN) DestY,
                            Width,
                            Height,
                            Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                            );
      } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
        Status = UgaDraw->Blt (
                            UgaDraw,
                            (EFI_UGA_PIXEL *) Blt,
                            EfiUgaBltBufferToVideo,
                            0,
                            0,
                            (UINTN) DestX,
                            (UINTN) DestY,
                            Width,
                            Height,
                            Width * sizeof (EFI_UGA_PIXEL)
                            );
      } else {
        Status = EFI_UNSUPPORTED;
      }
    }

    gBS->FreePool (ImageData);
    gBS->FreePool (Blt);

    if (Badging == NULL) {
      break;
    }
  }

  return Status;
}

/**
  Use Console Control to turn on GOP/UGA based Simple Text Out consoles. The GOP/UGA
  Simple Text Out screens will now be synced up with all non GOP/UGA output devices

  @retval EFI_SUCCESS           - GOP/UGA devices are back in text mode and synced up.
  @retval EFI_UNSUPPORTED       - Logo not found
**/
EFI_STATUS
DisableQuietBoot (
  VOID
  )

{
  EFI_STATUS                    Status;
  EFI_CONSOLE_CONTROL_PROTOCOL  *ConsoleControl;

  Status = gBS->LocateProtocol (&gEfiConsoleControlProtocolGuid, NULL, (VOID **) &ConsoleControl);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  return ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenText);
}

EFI_GRAPHICS_OUTPUT_BLT_PIXEL mEfiColors[16] = {
  { 0x00, 0x00, 0x00, 0x00 },
  { 0x98, 0x00, 0x00, 0x00 },
  { 0x00, 0x98, 0x00, 0x00 },
  { 0x98, 0x98, 0x00, 0x00 },
  { 0x00, 0x00, 0x98, 0x00 },
  { 0x98, 0x00, 0x98, 0x00 },
  { 0x00, 0x98, 0x98, 0x00 },
  { 0x98, 0x98, 0x98, 0x00 },
  { 0x10, 0x10, 0x10, 0x00 },
  { 0xff, 0x10, 0x10, 0x00 },
  { 0x10, 0xff, 0x10, 0x00 },
  { 0xff, 0xff, 0x10, 0x00 },
  { 0x10, 0x10, 0xff, 0x00 },
  { 0xf0, 0x10, 0xff, 0x00 },
  { 0x10, 0xff, 0xff, 0x00 },
  { 0xff, 0xff, 0xff, 0x00 }
};

/**
  Display string worker for: Print, PrintAt, IPrint, IPrintAt

  @param GraphicsOutput  - Graphics output protocol interface

  @param UgaDraw         - UGA draw protocol interface

  @param Sto             - Simple text out protocol interface

  @param X               - X coordinate to start printing

  @param Y               - Y coordinate to start printing

  @param Foreground      - Foreground color

  @param Background      - Background color

  @param fmt             - Format string

  @param args            - Print arguments


  @retval EFI_SUCCESS             -  success
  @retval EFI_OUT_OF_RESOURCES    -  out of resources

**/
UINTN
_IPrint (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput,
  IN EFI_UGA_DRAW_PROTOCOL            *UgaDraw,
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Sto,
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Foreground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *Background,
  IN CHAR16                           *fmt,
  IN VA_LIST                          args
  )

{
  VOID                           *Buffer;
  EFI_STATUS                     Status;
  UINT16                         GlyphWidth;
  UINT32                         GlyphStatus;
  UINT16                         StringIndex;
  UINTN                          Index;
  CHAR16                         *UnicodeWeight;
  EFI_NARROW_GLYPH               *Glyph;
  EFI_HII_PROTOCOL               *Hii;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *LineBuffer;
  UINT32                         HorizontalResolution;
  UINT32                         VerticalResolution;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  UINTN                          BufferLen;
  UINTN                          LineBufferLen;
  UINTN                          BufferGlyphWidth;

  GlyphStatus = 0;
  HorizontalResolution = 0;
  VerticalResolution = 0;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (GraphicsOutput != NULL) {
    HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
    VerticalResolution   = GraphicsOutput->Mode->Info->VerticalResolution;
  } else if (FeaturePcdGet (PcdUgaConsumeSupport))  {
    //
    // Get the current mode information from the UGA Draw Protocol
    //
    UgaDraw->GetMode (UgaDraw, &HorizontalResolution, &VerticalResolution, &ColorDepth, &RefreshRate);
  }
  ASSERT ((HorizontalResolution != 0) && (VerticalResolution !=0));

  LineBufferLen = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution * GLYPH_HEIGHT;
  LineBuffer = AllocatePool (LineBufferLen);
  if (LineBuffer == NULL) {
    gBS->FreePool (Buffer);
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->LocateProtocol (&gEfiHiiProtocolGuid, NULL, (VOID **) &Hii);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  UnicodeVSPrint (Buffer, 0x10000, fmt, args);

  UnicodeWeight = (CHAR16 *) Buffer;

  for (Index = 0; UnicodeWeight[Index] != 0; Index++) {
    if (UnicodeWeight[Index] == CHAR_BACKSPACE ||
        UnicodeWeight[Index] == CHAR_LINEFEED  ||
        UnicodeWeight[Index] == CHAR_CARRIAGE_RETURN) {
      UnicodeWeight[Index] = 0;
    }
  }

  BufferLen = StrLen (Buffer);

  if (GLYPH_WIDTH * GLYPH_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * BufferLen > LineBufferLen) {
     Status = EFI_INVALID_PARAMETER;
     goto Error;
  }

  for (Index = 0; Index < BufferLen; Index++) {
    StringIndex = (UINT16) Index;
    Status      = Hii->GetGlyph (Hii, UnicodeWeight, &StringIndex, (UINT8 **) &Glyph, &GlyphWidth, &GlyphStatus);
    if (EFI_ERROR (Status)) {
      goto Error;
    }

    if (Foreground == NULL || Background == NULL) {
      Status = Hii->GlyphToBlt (
                      Hii,
                      (UINT8 *) Glyph,
                      mEfiColors[Sto->Mode->Attribute & 0x0f],
                      mEfiColors[Sto->Mode->Attribute >> 4],
                      BufferLen,
                      GlyphWidth,
                      GLYPH_HEIGHT,
                      &LineBuffer[Index * GLYPH_WIDTH]
                      );
    } else {
      Status = Hii->GlyphToBlt (
                      Hii,
                      (UINT8 *) Glyph,
                      *Foreground,
                      *Background,
                      BufferLen,
                      GlyphWidth,
                      GLYPH_HEIGHT,
                      &LineBuffer[Index * GLYPH_WIDTH]
                      );
    }
  }

  //
  // Blt a character to the screen
  //
  BufferGlyphWidth = GLYPH_WIDTH * BufferLen;
  if (GraphicsOutput != NULL) {
    Status = GraphicsOutput->Blt (
                        GraphicsOutput,
                        LineBuffer,
                        EfiBltBufferToVideo,
                        0,
                        0,
                        X,
                        Y,
                        BufferGlyphWidth,
                        GLYPH_HEIGHT,
                        BufferGlyphWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                        );
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    Status = UgaDraw->Blt (
                        UgaDraw,
                        (EFI_UGA_PIXEL *) (UINTN) LineBuffer,
                        EfiUgaBltBufferToVideo,
                        0,
                        0,
                        X,
                        Y,
                        BufferGlyphWidth,
                        GLYPH_HEIGHT,
                        BufferGlyphWidth * sizeof (EFI_UGA_PIXEL)
                        );
  } else {
    Status = EFI_UNSUPPORTED;
  }

Error:
  gBS->FreePool (LineBuffer);
  gBS->FreePool (Buffer);
  return Status;
}

/**
  Prints a formatted unicode string to the default console

  @param X           - X coordinate to start printing

  @param Y           - Y coordinate to start printing

  @param ForeGround  - Foreground color

  @param BackGround  - Background color

  @param Fmt         - Format string

  @param ...           Variable argument list whose contents are accessed based on 
                       the format string specified by Format.


  @retval Length of string printed to the console

**/
UINTN
PrintXY (
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround, OPTIONAL
  IN CHAR16                           *Fmt,
  ...
  )
{
  EFI_HANDLE                    Handle;

  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Sto;
  EFI_STATUS                    Status;
  VA_LIST                       Args;

  VA_START (Args, Fmt);

  UgaDraw = NULL;

  Handle = gST->ConsoleOutHandle;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );

  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw
                    );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **) &Sto
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return _IPrint (GraphicsOutput, UgaDraw, Sto, X, Y, ForeGround, BackGround, Fmt, Args);
}

