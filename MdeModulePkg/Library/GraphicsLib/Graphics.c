/** @file
  Library supports diplaying graphical splash screen,
  locking of keyboard input and printing character on
  screen. These basic graphics operations are based on UEFI HII, 
  Graphics Output protocol or UGA Draw protocol.

Copyright (c) 2006 - 2008, Intel Corporation
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
#include <Protocol/UgaDraw.h>
#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>

#include <Guid/Bmp.h>

#include <Library/GraphicsLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxePiLib.h>
#include <Library/PcdLib.h>

STATIC EFI_GRAPHICS_OUTPUT_BLT_PIXEL mEfiColors[16] = {
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
  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

  @param  FileNameGuid          File Name of graphics file in the FV(s).
  @param  Image                 Pointer to pointer to return graphics image.  If NULL, a 
                                buffer will be allocated.
  @param  ImageSize             Size of the graphics Image in bytes. Zero if no image found.

  @retval EFI_SUCCESS           Image and ImageSize are valid. 
  @retval EFI_BUFFER_TOO_SMALL  Image not big enough. ImageSize has required size
  @retval EFI_NOT_FOUND         FileNameGuid not found

**/
EFI_STATUS
EFIAPI
GetGraphicsBitMapFromFV (
  IN  EFI_GUID      *FileNameGuid,
  OUT VOID          **Image,
  OUT UINTN         *ImageSize
  )
{
  return GetGraphicsBitMapFromFVEx (NULL, FileNameGuid, Image, ImageSize);
}

/**
  Return the graphics image file named FileNameGuid into Image and return it's
  size in ImageSize. All Firmware Volumes (FV) in the system are searched for the
  file name.

  @param  ImageHandle           The driver image handle of the caller. The parameter is used to
                                optimize the loading of the image file so that the FV from which
                                the driver image is loaded will be tried first. 
  @param  FileNameGuid          File Name of graphics file in the FV(s).
  @param  Image                 Pointer to pointer to return graphics image.  If NULL, a 
                                buffer will be allocated.
  @param  ImageSize             Size of the graphics Image in bytes. Zero if no image found.

  @retval EFI_SUCCESS           Image and ImageSize are valid. 
  @retval EFI_BUFFER_TOO_SMALL  Image not big enough. ImageSize has required size
  @retval EFI_NOT_FOUND         FileNameGuid not found

**/
EFI_STATUS
EFIAPI
GetGraphicsBitMapFromFVEx (
  IN  EFI_HANDLE    ImageHandle,
  IN  EFI_GUID      *FileNameGuid,
  OUT VOID          **Image,
  OUT UINTN         *ImageSize
  )
{
  return PiLibGetSectionFromAnyFv (
           FileNameGuid,
           EFI_SECTION_RAW,
           0,
           Image,
           ImageSize
           );
}

/**
  Convert a *.BMP graphics image to a GOP blt buffer. If a NULL Blt buffer
  is passed in a GopBlt buffer will be allocated by this routine. If a GopBlt
  buffer is passed in it will be used if it is big enough.

  @param  BmpImage      Pointer to BMP file
  @param  BmpImageSize  Number of bytes in BmpImage
  @param  GopBlt        Buffer containing GOP version of BmpImage.
  @param  GopBltSize    Size of GopBlt in bytes.
  @param  PixelHeight   Height of GopBlt/BmpImage in pixels
  @param  PixelWidth    Width of GopBlt/BmpImage in pixels

  @retval EFI_SUCCESS           GopBlt and GopBltSize are returned. 
  @retval EFI_UNSUPPORTED       BmpImage is not a valid *.BMP image
  @retval EFI_BUFFER_TOO_SMALL  The passed in GopBlt buffer is not big enough.
                                GopBltSize will contain the required size.
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate.

**/
EFI_STATUS
EFIAPI
ConvertBmpToGopBlt (
  IN     VOID      *BmpImage,
  IN     UINTN     BmpImageSize,
  IN OUT VOID      **GopBlt,
  IN OUT UINTN     *GopBltSize,
     OUT UINTN     *PixelHeight,
     OUT UINTN     *PixelWidth
  )
{
  UINT8                         *Image;
  UINT8                         *ImageHeader;
  BMP_IMAGE_HEADER              *BmpHeader;
  BMP_COLOR_MAP                 *BmpColorMap;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  UINTN                         BltBufferSize;
  UINTN                         Index;
  UINTN                         Height;
  UINTN                         Width;
  UINTN                         ImageIndex;
  BOOLEAN                       IsAllocated;

  BmpHeader = (BMP_IMAGE_HEADER *) BmpImage;

  if (BmpHeader->CharB != 'B' || BmpHeader->CharM != 'M') {
    return EFI_UNSUPPORTED;
  }

  //
  // Doesn't support compress.
  //
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

  //
  // Calculate the BltBuffer needed size.
  //
  BltBufferSize = BmpHeader->PixelWidth * BmpHeader->PixelHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  IsAllocated   = FALSE;
  if (*GopBlt == NULL) {
    //
    // GopBlt is not allocated by caller.
    //
    *GopBltSize = BltBufferSize;
    *GopBlt     = AllocatePool (*GopBltSize);
    IsAllocated = TRUE;
    if (*GopBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    //
    // GopBlt has been allocated by caller.
    //
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
        // Convert 1-bit (2 colors) BMP to 24-bit color
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
        // Convert 4-bit (16 colors) BMP Palette to 24-bit color
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
        // Convert 8-bit (256 colors) BMP Palette to 24-bit color
        //
        Blt->Red    = BmpColorMap[*Image].Red;
        Blt->Green  = BmpColorMap[*Image].Green;
        Blt->Blue   = BmpColorMap[*Image].Blue;
        break;

      case 24:
        //
        // It is 24-bit BMP.
        //
        Blt->Blue   = *Image++;
        Blt->Green  = *Image++;
        Blt->Red    = *Image;
        break;

      default:
        //
        // Other bit format BMP is not supported.
        //
        if (IsAllocated) {
          FreePool (*GopBlt);
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

  @param  Password        Password used to lock ConIn device.

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found.

**/
EFI_STATUS
EFIAPI
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
  to the UGA device. Put up LogoFile on every UGA device that is a console.

  @param  LogoFile        File name of logo to display on the center of the screen.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found.

**/
EFI_STATUS
EFIAPI
EnableQuietBoot (
  IN  EFI_GUID  *LogoFile
  )
{
  return EnableQuietBootEx (LogoFile, NULL);
}

/**
  Use Console Control to turn off UGA based Simple Text Out consoles from going
  to the UGA device. Put up LogoFile on every UGA device that is a console

  @param  LogoFile    File name of logo to display on the center of the screen.
  @param  ImageHandle The driver image handle of the caller. The parameter is used to
                      optimize the loading of the logo file so that the FV from which
                      the driver image is loaded will be tried first.

  @retval EFI_SUCCESS     ConsoleControl has been flipped to graphics and logo displayed.
  @retval EFI_UNSUPPORTED Logo not found.

**/
EFI_STATUS
EFIAPI
EnableQuietBootEx (
  IN  EFI_GUID    *LogoFile,
  IN  EFI_HANDLE  ImageHandle
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
  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
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

  //
  // Set console control to graphics mode.
  //
  Status = ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenGraphics);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

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
      //
      // Get image from OEMBadging protocol.
      //
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
      // Currently only support BMP format.
      //
      if (Format != EfiBadgingFormatBMP) {
        SafeFreePool (ImageData);
        continue;
      }
    } else {
      //
      // Get the specified image from FV.
      //
      Status = GetGraphicsBitMapFromFVEx (ImageHandle, LogoFile, (VOID **) &ImageData, &ImageSize);
      if (EFI_ERROR (Status)) {
        return EFI_UNSUPPORTED;
      }

      CoordinateX = 0;
      CoordinateY = 0;
      Attribute   = EfiBadgingDisplayAttributeCenter;
    }

    Blt = NULL;
    Status = ConvertBmpToGopBlt (
              ImageData,
              ImageSize,
              (VOID **) &Blt,
              &BltSize,
              &Height,
              &Width
              );
    if (EFI_ERROR (Status)) {
      SafeFreePool (ImageData);
      if (Badging == NULL) {
        return Status;
      } else {
        continue;
      }
    }

    //
    // Caculate the display position according to Attribute.
    //
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

    SafeFreePool (ImageData);
    SafeFreePool (Blt);

    if (Badging == NULL) {
      break;
    }
  }

  return Status;
}

/**
  Use Console Control to turn on UGA based Simple Text Out consoles. The UGA 
  Simple Text Out screens will now be synced up with all non UGA output devices

  @retval EFI_SUCCESS          UGA devices are back in text mode and synced up.
  @retval EFI_UNSUPPORTED      Logo not found

**/
EFI_STATUS
EFIAPI
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

  //
  // Set console control to text mode.
  //
  return ConsoleControl->SetMode (ConsoleControl, EfiConsoleControlScreenText);
}

/**
  Internal display string worker function.

  @param GraphicsOutput   Graphics output protocol interface.
  @param UgaDraw          UGA draw protocol interface.
  @param Sto              Simple text out protocol interface.
  @param X                X coordinate to start printing.
  @param Y                Y coordinate to start printing.
  @param Foreground       Foreground color.
  @param Background       Background color.
  @param fmt              Format string.
  @param args             Print arguments.

  @return  Number of Characters printed. Zero means no any character 
           displayed successfully.

**/
UINTN
Print (
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
  UINTN                          Index;
  CHAR16                         *UnicodeWeight;
  UINT32                         HorizontalResolution;
  UINT32                         VerticalResolution;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  UINTN                          BufferLen;
  UINTN                          LineBufferLen;
  EFI_HII_FONT_PROTOCOL          *HiiFont;
  EFI_IMAGE_OUTPUT               *Blt;
  EFI_FONT_DISPLAY_INFO          *FontInfo;
  EFI_HII_ROW_INFO               *RowInfoArray;
  UINTN                          RowInfoArraySize;
  UINTN                          PrintNum;     

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  if (Buffer == NULL) {
    return 0;
  }

  HorizontalResolution  = 0;
  VerticalResolution    = 0;
  Blt                   = NULL;
  FontInfo              = NULL;
  PrintNum              = 0;

  if (GraphicsOutput != NULL) {
    HorizontalResolution = GraphicsOutput->Mode->Info->HorizontalResolution;
    VerticalResolution = GraphicsOutput->Mode->Info->VerticalResolution;
  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    UgaDraw->GetMode (UgaDraw, &HorizontalResolution, &VerticalResolution, &ColorDepth, &RefreshRate);
  } else {
    Status = EFI_UNSUPPORTED;
    goto Error;
  }

  ASSERT ((HorizontalResolution != 0) && (VerticalResolution !=0));

  Status = gBS->LocateProtocol (&gEfiHiiFontProtocolGuid, NULL, (VOID **) &HiiFont);
  if (EFI_ERROR (Status)) {
    goto Error;
  }

  PrintNum = UnicodeVSPrint (Buffer, 0x10000, fmt, args);

  UnicodeWeight = (CHAR16 *) Buffer;

  for (Index = 0; UnicodeWeight[Index] != 0; Index++) {
    if (UnicodeWeight[Index] == CHAR_BACKSPACE ||
        UnicodeWeight[Index] == CHAR_LINEFEED  ||
        UnicodeWeight[Index] == CHAR_CARRIAGE_RETURN) {
      UnicodeWeight[Index] = 0;
    }
  }

  BufferLen = StrLen (Buffer);

  LineBufferLen = sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * HorizontalResolution * EFI_GLYPH_HEIGHT;
  if (EFI_GLYPH_WIDTH * EFI_GLYPH_HEIGHT * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * BufferLen > LineBufferLen) {
     Status = EFI_INVALID_PARAMETER;
     goto Error;
  }

  Blt = (EFI_IMAGE_OUTPUT *) AllocateZeroPool (sizeof (EFI_IMAGE_OUTPUT));
  if (Blt == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }

  Blt->Width        = (UINT16) (HorizontalResolution);
  Blt->Height       = (UINT16) (VerticalResolution);

  FontInfo = (EFI_FONT_DISPLAY_INFO *) AllocateZeroPool (sizeof (EFI_FONT_DISPLAY_INFO));
  if (FontInfo == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Error;
  }
  if (Foreground != NULL) {
    CopyMem (&FontInfo->ForegroundColor, Foreground, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    CopyMem (
      &FontInfo->ForegroundColor,
      &mEfiColors[Sto->Mode->Attribute & 0x0f],
      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      );
  }
  if (Background != NULL) {
    CopyMem (&FontInfo->BackgroundColor, Background, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    CopyMem (
      &FontInfo->BackgroundColor,
      &mEfiColors[Sto->Mode->Attribute >> 4],
      sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
      );
  }

  if (GraphicsOutput != NULL) {
    Blt->Image.Screen = GraphicsOutput;
    
    Status = HiiFont->StringToImage (
                         HiiFont,
                         EFI_HII_IGNORE_IF_NO_GLYPH | EFI_HII_DIRECT_TO_SCREEN,
                         Buffer,
                         FontInfo,
                         &Blt,
                         X,
                         Y,
                         NULL,
                         NULL,
                         NULL
                         );

  } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
    ASSERT (UgaDraw!= NULL);

    Blt->Image.Bitmap = AllocateZeroPool (Blt->Width * Blt->Height * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    if (Blt->Image.Bitmap == NULL) {
      SafeFreePool (Blt);
      SafeFreePool (Buffer);
      return EFI_OUT_OF_RESOURCES;
    }

    RowInfoArray = NULL;
    //
    //  StringToImage only support blt'ing image to device using GOP protocol. If GOP is not supported in this platform,
    //  we ask StringToImage to print the string to blt buffer, then blt to device using UgaDraw.
    //
    Status = HiiFont->StringToImage (
                         HiiFont,
                         EFI_HII_IGNORE_IF_NO_GLYPH,
                         Buffer,
                         FontInfo,
                         &Blt,
                         X,
                         Y,
                         &RowInfoArray,
                         &RowInfoArraySize,
                         NULL
                         );

    if (!EFI_ERROR (Status)) {
      //
      // Line breaks are handled by caller of DrawUnicodeWeightAtCursorN, so the updated parameter RowInfoArraySize by StringToImage will
      // always be 1 or 0 (if there is no valid Unicode Char can be printed). ASSERT here to make sure.
      //
      ASSERT (RowInfoArraySize <= 1);

      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) Blt->Image.Bitmap,
                          EfiUgaBltBufferToVideo,
                          X,
                          Y,
                          X,
                          Y,
                          RowInfoArray[0].LineWidth,
                          RowInfoArray[0].LineHeight,
                          Blt->Width * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    }

    SafeFreePool (RowInfoArray);
    SafeFreePool (Blt->Image.Bitmap);
  } else {
    Status = EFI_UNSUPPORTED;
  }

Error:
  SafeFreePool (Blt);
  SafeFreePool (FontInfo);
  FreePool (Buffer);

  if (EFI_ERROR (Status)) {
    return PrintNum;
  } else {
    return 0;
  }
}

/**
  Print Unicode string to graphics screen at the given X,Y coordinates of the graphics screen.
  see definition of Print to find rules for constructing Fmt.

  @param  X            Row to start printing at.
  @param  Y            Column to start printing at.
  @param  ForeGround   Foreground color.
  @param  BackGround   background color.
  @param  Fmt          Print format sting. See definition of Print.
  @param  ...          Argumnet stream defined by Fmt string.

  @return  Number of Characters printed. Zero means no any character 
           displayed successfully.

**/
UINTN
EFIAPI
PrintXY (
  IN UINTN                            X,
  IN UINTN                            Y,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *ForeGround, OPTIONAL
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL    *BackGround, OPTIONAL
  IN CHAR16                           *Fmt,
  ...
  )
{
  EFI_HANDLE                       Handle;
  EFI_GRAPHICS_OUTPUT_PROTOCOL     *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL            *UgaDraw;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *Sto;
  EFI_STATUS                       Status;
  VA_LIST                          Args;

  VA_START (Args, Fmt);

  Handle = gST->ConsoleOutHandle;

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiGraphicsOutputProtocolGuid,
                  (VOID **) &GraphicsOutput
                  );

  UgaDraw = NULL;
  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    //
    // If no GOP available, try to open UGA Draw protocol if supported.
    //
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (
                    Handle,
                    &gEfiUgaDrawProtocolGuid,
                    (VOID **) &UgaDraw
                    );
  }
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleTextOutProtocolGuid,
                  (VOID **) &Sto
                  );

  if (EFI_ERROR (Status)) {
    return 0;
  }

  return Print (GraphicsOutput, UgaDraw, Sto, X, Y, ForeGround, BackGround, Fmt, Args);
}

