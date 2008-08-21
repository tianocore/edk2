/**@file
  This file contains the Glyph related function.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"

EFI_NARROW_GLYPH mNarrowGlyphBuffer = {0, 0, {0}};

BOOLEAN                         mSysFontColorCached = FALSE;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL   mSysFGColor = {0};


/**
  This function is only called by Graphics Console module and GraphicsLib. 
  EDK II provides a UEFI Graphics Console module. ECP provides a GraphicsLib 
  complying to UEFI HII.
  
  This function will ASSERT and return EFI_UNSUPPORTED.

  @param This            N.A.
  @param Source          N.A.
  @param Index           N.A.
  @param GlyphBuffer     N.A.
  @param BitWidth        N.A.
  @param InternalStatus  N.A.

  @return EFI_UNSUPPORTED N.A.

**/
EFI_STATUS
EFIAPI
HiiGetGlyph (
  IN     EFI_HII_PROTOCOL   *This,
  IN     CHAR16             *Source,
  IN OUT UINT16             *Index,
  OUT    UINT8              **GlyphBuffer,
  OUT    UINT16             *BitWidth,
  IN OUT UINT32             *InternalStatus
  )
{
  EFI_STATUS                Status;
  EFI_IMAGE_OUTPUT          *Blt;
  EFI_FONT_DISPLAY_INFO     *FontInfo;
  UINTN                     Xpos;
  UINTN                     Ypos;
  UINTN                     BaseLine;

  if (!mSysFontColorCached) {
    //
    // Cache the system font's foreground color.
    //
    Status = mHiiFontProtocol->GetFontInfo (
                                 mHiiFontProtocol,
                                 NULL,
                                 NULL,
                                 &FontInfo,
                                 NULL
                                 );

    if (!EFI_ERROR (Status)) {
      ASSERT (StrCmp (FontInfo->FontInfo.FontName, L"sysdefault") == 0);
      mSysFGColor = FontInfo->ForegroundColor;
      FreePool (FontInfo);

      mSysFontColorCached = TRUE;
    }
    
  }

  Blt = NULL;
  Status = mHiiFontProtocol->GetGlyph (
                               mHiiFontProtocol,
                               Source[*Index],
                               NULL,
                               &Blt,
                               &BaseLine
                               );

  if (!EFI_ERROR (Status)) {
    //
    // For simplicity, we only handle Narrow Glyph.
    //
    ASSERT (Blt->Height == EFI_GLYPH_HEIGHT);
    ASSERT (Blt->Width == EFI_GLYPH_WIDTH);

    if (Blt->Height == EFI_GLYPH_HEIGHT && Blt->Width == EFI_GLYPH_WIDTH) {

      ZeroMem (&mNarrowGlyphBuffer, sizeof (mNarrowGlyphBuffer));
      mNarrowGlyphBuffer.UnicodeWeight = *Source;
      for (Ypos = 0; Ypos < EFI_GLYPH_HEIGHT; Ypos++) {
        for (Xpos = 0; Xpos < EFI_GLYPH_WIDTH; Xpos++) {
          if (CompareMem (&Blt->Image.Bitmap[Ypos * EFI_GLYPH_WIDTH + Xpos], &mSysFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) == 0) {
            mNarrowGlyphBuffer.GlyphCol1[Ypos] |= 1 << (EFI_GLYPH_WIDTH - 1 - Xpos);
          }
        }
      }

      *GlyphBuffer = (UINT8 *) &mNarrowGlyphBuffer;
      *BitWidth    = EFI_GLYPH_WIDTH;
      *Index += 1;
    } else {
      Status = EFI_NOT_FOUND;
    }

  }

  if (EFI_ERROR (Status)) {
    *GlyphBuffer = NULL;
  }
  return Status;
}

/**
  This function is only called by Graphics Console module and GraphicsLib. 
  EDK II provides a UEFI Graphics Console module. ECP provides a GraphicsLib 
  complying to UEFI HII.
  
  This function will ASSERT and return EFI_UNSUPPORTED.

  @param This            N.A.
  @param GlyphBuffer     N.A.
  @param Foreground      N.A.
  @param Background      N.A.
  @param Count           N.A.
  @param Width           N.A.
  @param Height          N.A.
  @param BltBuffer       N.A.

  @return EFI_UNSUPPORTED N.A.

**/
EFI_STATUS
EFIAPI
HiiGlyphToBlt (
  IN     EFI_HII_PROTOCOL              *This,
  IN     UINT8                         *GlyphBuffer,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Foreground,
  IN     EFI_GRAPHICS_OUTPUT_BLT_PIXEL Background,
  IN     UINTN                         Count,
  IN     UINTN                         Width,
  IN     UINTN                         Height,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer
  )
{
  UINTN X;
  UINTN Y;

  //
  // Convert Monochrome bitmap of the Glyph to BltBuffer structure
  //
  for (Y = 0; Y < Height; Y++) {
    for (X = 0; X < Width; X++) {
      if ((((EFI_NARROW_GLYPH *) GlyphBuffer)->GlyphCol1[Y] & (1 << X)) != 0) {
        BltBuffer[Y * Width * Count + (Width - X - 1)] = Foreground;
      } else {
        BltBuffer[Y * Width * Count + (Width - X - 1)] = Background;
      }
    }
  }

  return EFI_SUCCESS;
}
