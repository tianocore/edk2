/** @file
  This library is only intended to be used by PlatformBootManagerLib
  to show progress bar and LOGO.

Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/PlatformLogo.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/UgaDraw.h>
#include <Protocol/BootLogo.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/ImageDecoderLib.h>

/**
  Show LOGO on all consoles.

  @param[in]  ImageFormat Format of the image file.
  @param[in]  LogoFile    The file name of logo to display.
  @param[in]  Attribute   The display attributes of the image returned.
  @param[in]  OffsetX     The X offset of the image regarding the Attribute.
  @param[in]  OffsetY     The Y offset of the image regarding the Attribute.

  @retval EFI_SUCCESS     Logo was displayed.
  @retval EFI_UNSUPPORTED Logo was not found or cannot be displayed.
**/
EFI_STATUS
EFIAPI
BootLogoEnableLogo (
  IN  IMAGE_FORMAT                          ImageFormat,
  IN  EFI_GUID                              *Logo,
  IN  EDKII_PLATFORM_LOGO_DISPLAY_ATTRIBUTE Attribute,
  IN  INTN                                  OffsetX,
  IN  INTN                                  OffsetY
  )
{
  EFI_STATUS                    Status;
  EDKII_PLATFORM_LOGO_PROTOCOL  *PlatformLogo;
  UINT32                        SizeOfX;
  UINT32                        SizeOfY;
  INTN                          DestX;
  INTN                          DestY;
  UINT8                         *ImageData;
  UINTN                         ImageSize;
  UINTN                         BltSize;
  UINT32                        Instance;
  UINTN                         Height;
  UINTN                         Width;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Blt;
  EFI_UGA_DRAW_PROTOCOL         *UgaDraw;
  UINT32                        ColorDepth;
  UINT32                        RefreshRate;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *GraphicsOutput;
  EFI_BOOT_LOGO_PROTOCOL        *BootLogo;
  UINTN                         NumberOfLogos;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *LogoBlt;
  UINTN                         LogoDestX;
  UINTN                         LogoDestY;
  UINTN                         LogoHeight;
  UINTN                         LogoWidth;
  UINTN                         NewDestX;
  UINTN                         NewDestY;
  UINTN                         NewHeight;
  UINTN                         NewWidth;
  UINTN                         BufferSize;

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
    if (EFI_ERROR (Status)) {
      UgaDraw = NULL;
    }
  }
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  Status  = gBS->LocateProtocol (&gEdkiiPlatformLogoProtocolGuid, NULL, (VOID **) &PlatformLogo);
  if (EFI_ERROR (Status)) {
    PlatformLogo = NULL;
  }

  if ((Logo == NULL) && (PlatformLogo == NULL)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Try to open Boot Logo Protocol.
  //
  Status = gBS->LocateProtocol (&gEfiBootLogoProtocolGuid, NULL, (VOID **) &BootLogo);
  if (EFI_ERROR (Status)) {
    BootLogo = NULL;
  }

  //
  // Erase Cursor from screen
  //
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;

  } else {
    ASSERT (UgaDraw != NULL);
    Status = UgaDraw->GetMode (UgaDraw, &SizeOfX, &SizeOfY, &ColorDepth, &RefreshRate);
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  }

  Blt = NULL;
  NumberOfLogos = 0;
  LogoDestX = 0;
  LogoDestY = 0;
  LogoHeight = 0;
  LogoWidth = 0;
  NewDestX = 0;
  NewDestY = 0;
  NewHeight = 0;
  NewWidth = 0;
  Instance = 0;
  DestX = 0;
  DestY = 0;
  while (TRUE) {
    ImageData = NULL;
    ImageSize = 0;

    if (PlatformLogo != NULL) {
      //
      // Get image from OEMBadging protocol.
      //
      Status = PlatformLogo->GetImage (
                               PlatformLogo,
                               &Instance,
                               &ImageFormat,
                               &ImageData,
                               &ImageSize,
                               &Attribute,
                               &OffsetX,
                               &OffsetY
                               );
      if (EFI_ERROR (Status)) {
        break;
      }

    } else {
      //
      // Get the specified image from FV.
      //
      Status = GetSectionFromAnyFv (Logo, EFI_SECTION_RAW, 0, (VOID **) &ImageData, &ImageSize);
      if (EFI_ERROR (Status)) {
        return EFI_UNSUPPORTED;
      }
    }

    if (Blt != NULL) {
      FreePool (Blt);
    }

    Status = DecodeImage (ImageFormat, ImageData, ImageSize, &Blt, &BltSize, &Width, &Height);
    FreePool (ImageData);
    if (EFI_ERROR (Status)) {
      if (Logo != NULL) {
        //
        // Directly return failure for single LOGO
        //
        return Status;
      } else {
        continue;
      }
    }

    //
    // Calculate the display position according to Attribute.
    //
    switch (Attribute) {
    case EdkiiPlatformLogoDisplayAttributeLeftTop:
      DestX = 0;
      DestY = 0;
      break;
    case EdkiiPlatformLogoDisplayAttributeCenterTop:
      DestX = (SizeOfX - Width) / 2;
      DestY = 0;
      break;
    case EdkiiPlatformLogoDisplayAttributeRightTop:
      DestX = SizeOfX - Width;
      DestY = 0;
      break;

    case EdkiiPlatformLogoDisplayAttributeCenterLeft:
      DestX = 0;
      DestY = (SizeOfY - Height) / 2;
      break;
    case EdkiiPlatformLogoDisplayAttributeCenter:
      DestX = (SizeOfX - Width) / 2;
      DestY = (SizeOfY - Height) / 2;
      break;
    case EdkiiPlatformLogoDisplayAttributeCenterRight:
      DestX = SizeOfX - Width;
      DestY = (SizeOfY - Height) / 2;
      break;

    case EdkiiPlatformLogoDisplayAttributeLeftBottom:
      DestX = 0;
      DestY = SizeOfY - Height;
      break;
    case EdkiiPlatformLogoDisplayAttributeCenterBottom:
      DestX = (SizeOfX - Width) / 2;
      DestY = SizeOfY - Height;
      break;
    case EdkiiPlatformLogoDisplayAttributeRightBottom:
      DestX = SizeOfX - Width;
      DestY = SizeOfY - Height;
      break;

    default:
      ASSERT (FALSE);
      break;
    }

    DestX += OffsetX;
    DestY += OffsetY;

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
      } else {
        ASSERT (UgaDraw != NULL);
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
      }

      //
      // Report displayed Logo information.
      //
      if (!EFI_ERROR (Status)) {
        NumberOfLogos++;

        if (LogoWidth == 0) {
          //
          // The first Logo.
          //
          LogoDestX = (UINTN) DestX;
          LogoDestY = (UINTN) DestY;
          LogoWidth = Width;
          LogoHeight = Height;
        } else {
          //
          // Merge new logo with old one.
          //
          NewDestX = MIN ((UINTN) DestX, LogoDestX);
          NewDestY = MIN ((UINTN) DestY, LogoDestY);
          NewWidth = MAX ((UINTN) DestX + Width, LogoDestX + LogoWidth) - NewDestX;
          NewHeight = MAX ((UINTN) DestY + Height, LogoDestY + LogoHeight) - NewDestY;

          LogoDestX = NewDestX;
          LogoDestY = NewDestY;
          LogoWidth = NewWidth;
          LogoHeight = NewHeight;
        }
      }
    }

    if (PlatformLogo == NULL) {
      break;
    }
  }

  if (BootLogo == NULL || NumberOfLogos == 0) {
    //
    // No logo displayed.
    //
    if (Blt != NULL) {
      FreePool (Blt);
    }

    return Status;
  }

  //
  // Advertise displayed Logo information.
  //
  if (NumberOfLogos == 1) {
    //
    // Only one logo displayed, use its Blt buffer directly for BootLogo protocol.
    //
    LogoBlt = Blt;
    Status = EFI_SUCCESS;
  } else {
    //
    // More than one Logo displayed, get merged BltBuffer using VideoToBuffer operation. 
    //
    if (Blt != NULL) {
      FreePool (Blt);
    }

    //
    // Ensure the LogoHeight * LogoWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL) doesn't overflow
    //
    if (LogoHeight > MAX_UINTN / LogoWidth / sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) {
      return EFI_UNSUPPORTED;
    }
    BufferSize = LogoWidth * LogoHeight * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL);

    LogoBlt = AllocatePool (BufferSize);
    if (LogoBlt == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          LogoBlt,
                          EfiBltVideoToBltBuffer,
                          LogoDestX,
                          LogoDestY,
                          0,
                          0,
                          LogoWidth,
                          LogoHeight,
                          LogoWidth * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) LogoBlt,
                          EfiUgaVideoToBltBuffer,
                          LogoDestX,
                          LogoDestY,
                          0,
                          0,
                          LogoWidth,
                          LogoHeight,
                          LogoWidth * sizeof (EFI_UGA_PIXEL)
                          );
    }
  }

  if (!EFI_ERROR (Status)) {
    BootLogo->SetBootLogo (BootLogo, LogoBlt, LogoDestX, LogoDestY, LogoWidth, LogoHeight);
  }
  FreePool (LogoBlt);

  return Status;
}

/**
  Use SystemTable Conout to turn on video based Simple Text Out consoles. The 
  Simple Text Out screens will now be synced up with all non video output devices

  @retval EFI_SUCCESS     UGA devices are back in text mode and synced up.

**/
EFI_STATUS
EFIAPI
BootLogoDisableLogo (
  VOID
  )
{

  //
  // Enable Cursor on Screen
  //
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  return EFI_SUCCESS;
}


/**

  Update progress bar with title above it. It only works in Graphics mode.

  @param TitleForeground Foreground color for Title.
  @param TitleBackground Background color for Title.
  @param Title           Title above progress bar.
  @param ProgressColor   Progress bar color.
  @param Progress        Progress (0-100)
  @param PreviousValue   The previous value of the progress.

  @retval  EFI_STATUS       Success update the progress bar

**/
EFI_STATUS
EFIAPI
BootLogoUpdateProgress (
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleForeground,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL TitleBackground,
  IN CHAR16                        *Title,
  IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL ProgressColor,
  IN UINTN                         Progress,
  IN UINTN                         PreviousValue
  )
{
  EFI_STATUS                     Status;
  EFI_GRAPHICS_OUTPUT_PROTOCOL   *GraphicsOutput;
  EFI_UGA_DRAW_PROTOCOL          *UgaDraw;
  UINT32                         SizeOfX;
  UINT32                         SizeOfY;
  UINT32                         ColorDepth;
  UINT32                         RefreshRate;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Color;
  UINTN                          BlockHeight;
  UINTN                          BlockWidth;
  UINTN                          BlockNum;
  UINTN                          PosX;
  UINTN                          PosY;
  UINTN                          Index;

  if (Progress > 100) {
    return EFI_INVALID_PARAMETER;
  }

  UgaDraw = NULL;
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **) &GraphicsOutput);
  if (EFI_ERROR (Status) && FeaturePcdGet (PcdUgaConsumeSupport)) {
    GraphicsOutput = NULL;

    Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiUgaDrawProtocolGuid, (VOID **) &UgaDraw);
    if (EFI_ERROR (Status)) {
      UgaDraw = NULL;
    }
  }
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  SizeOfX = 0;
  SizeOfY = 0;
  if (GraphicsOutput != NULL) {
    SizeOfX = GraphicsOutput->Mode->Info->HorizontalResolution;
    SizeOfY = GraphicsOutput->Mode->Info->VerticalResolution;
  } else if (UgaDraw != NULL) {
    Status = UgaDraw->GetMode (
                        UgaDraw,
                        &SizeOfX,
                        &SizeOfY,
                        &ColorDepth,
                        &RefreshRate
                        );
    if (EFI_ERROR (Status)) {
      return EFI_UNSUPPORTED;
    }
  } else {
    return EFI_UNSUPPORTED;
  }

  BlockWidth  = SizeOfX / 100;
  BlockHeight = SizeOfY / 50;

  BlockNum    = Progress;

  PosX        = 0;
  PosY        = SizeOfY * 48 / 50;

  if (BlockNum == 0) {
    //
    // Clear progress area
    //
    SetMem (&Color, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0x0);

    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          &Color,
                          EfiBltVideoFill,
                          0,
                          0,
                          0,
                          PosY - EFI_GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &Color,
                          EfiUgaVideoFill,
                          0,
                          0,
                          0,
                          PosY - EFI_GLYPH_HEIGHT - 1,
                          SizeOfX,
                          SizeOfY - (PosY - EFI_GLYPH_HEIGHT - 1),
                          SizeOfX * sizeof (EFI_UGA_PIXEL)
                          );
    } else {
      return EFI_UNSUPPORTED;
    }
  }
  //
  // Show progress by drawing blocks
  //
  for (Index = PreviousValue; Index < BlockNum; Index++) {
    PosX = Index * BlockWidth;
    if (GraphicsOutput != NULL) {
      Status = GraphicsOutput->Blt (
                          GraphicsOutput,
                          &ProgressColor,
                          EfiBltVideoFill,
                          0,
                          0,
                          PosX,
                          PosY,
                          BlockWidth - 1,
                          BlockHeight,
                          (BlockWidth) * sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)
                          );
    } else if (FeaturePcdGet (PcdUgaConsumeSupport)) {
      Status = UgaDraw->Blt (
                          UgaDraw,
                          (EFI_UGA_PIXEL *) &ProgressColor,
                          EfiUgaVideoFill,
                          0,
                          0,
                          PosX,
                          PosY,
                          BlockWidth - 1,
                          BlockHeight,
                          (BlockWidth) * sizeof (EFI_UGA_PIXEL)
                          );
    } else {
      return EFI_UNSUPPORTED;
    }
  }

  PrintXY (
    (SizeOfX - StrLen (Title) * EFI_GLYPH_WIDTH) / 2,
    PosY - EFI_GLYPH_HEIGHT - 1,
    &TitleForeground,
    &TitleBackground,
    Title
    );

  return EFI_SUCCESS;
}
