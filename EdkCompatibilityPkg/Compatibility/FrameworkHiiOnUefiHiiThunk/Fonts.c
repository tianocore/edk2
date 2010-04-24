/** @file
  This file contains the Glyph related function.

Copyright (c) 2006 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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
  Translates a Unicode character into the corresponding font glyph.
  
  Notes:
    This function is only called by Graphics Console module and GraphicsLib. 
    Wrap the Framework HII GetGlyph function to UEFI Font Protocol.
    
    EDK II provides a UEFI Graphics Console module. ECP provides a GraphicsLib 
    complying to UEFI HII.
  
  @param This            A pointer to the EFI_HII_PROTOCOL instance.
  @param Source          A pointer to a Unicode string.
  @param Index           On input, the offset into the string from which to fetch the character. On successful completion, the 
                         index is updated to the first character past the character(s) making up the just extracted glyph. 
  @param GlyphBuffer     Pointer to an array where the glyphs corresponding to the characters in the source may be stored. 
                         GlyphBuffer is assumed to be wide enough to accept a wide glyph character.
  @param BitWidth        If EFI_SUCCESS was returned, the UINT16 pointed to by this value is filled with the length of the glyph in pixels. 
                         It is unchanged if the call was unsuccessful.
  @param InternalStatus  To save the time required to read the string from the beginning on each glyph extraction 
                         (for example, to ensure that the narrow versus wide glyph mode is correct), this value is 
                         updated each time the function is called with the status that is local to the call. The cell pointed 
                         to by this parameter must be initialized to zero prior to invoking the call the first time for any string.

  @retval EFI_SUCCESS     It worked.
  @retval EFI_NOT_FOUND   A glyph for a character was not found.
 

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

  if (!EFI_ERROR (Status) && (Status != EFI_WARN_UNKNOWN_GLYPH)) {
    //
    // For simplicity, we only handle Narrow Glyph.
    //
    if (Blt->Height == EFI_GLYPH_HEIGHT && Blt->Width == EFI_GLYPH_WIDTH) {

      ZeroMem (&mNarrowGlyphBuffer, sizeof (mNarrowGlyphBuffer));
      mNarrowGlyphBuffer.UnicodeWeight = *Source;
      for (Ypos = 0; Ypos < EFI_GLYPH_HEIGHT; Ypos++) {
        for (Xpos = 0; Xpos < EFI_GLYPH_WIDTH; Xpos++) {
          if (CompareMem (&Blt->Image.Bitmap[Ypos * EFI_GLYPH_WIDTH + Xpos], &mSysFGColor, sizeof (EFI_GRAPHICS_OUTPUT_BLT_PIXEL)) == 0) {
            mNarrowGlyphBuffer.GlyphCol1[Ypos] = (UINT8) (mNarrowGlyphBuffer.GlyphCol1[Ypos] | (1 << (EFI_GLYPH_WIDTH - 1 - Xpos)));
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

  if (EFI_ERROR (Status) || (Status == EFI_WARN_UNKNOWN_GLYPH)) {
    if (Status == EFI_WARN_UNKNOWN_GLYPH) {
      Status = EFI_NOT_FOUND;
    }
    *GlyphBuffer = NULL;
  }
  return Status;
}

/**
  Translates a glyph into the format required for input to the Universal Graphics Adapter (UGA) Block Transfer (BLT) routines.
  
  Notes:
  This function is only called by Graphics Console module and GraphicsLib. 
  EDK II provides a UEFI Graphics Console module. ECP provides a GraphicsLib 
  complying to UEFI HII.
  
  @param This         A pointer to the EFI_HII_PROTOCOL instance.
  @param GlyphBuffer  A pointer to the buffer that contains glyph data. 
  @param Foreground   The foreground setting requested to be used for the generated BltBuffer data. Type EFI_UGA_PIXEL is defined in "Related Definitions" below.
  @param Background   The background setting requested to be used for the generated BltBuffer data. 
  @param Count        The entry in the BltBuffer upon which to act.
  @param Width        The width in bits of the glyph being converted.
  @param Height       The height in bits of the glyph being converted
  @param BltBuffer    A pointer to the buffer that contains the data that is ready to be used by the UGA BLT routines. 

  @retval EFI_SUCCESS  It worked.
  @retval EFI_NOT_FOUND A glyph for a character was not found.
 

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
  UINTN Xpos;
  UINTN Ypos;

  //
  // Convert Monochrome bitmap of the Glyph to BltBuffer structure
  //
  for (Ypos = 0; Ypos < Height; Ypos++) {
    for (Xpos = 0; Xpos < Width; Xpos++) {
      if ((((EFI_NARROW_GLYPH *) GlyphBuffer)->GlyphCol1[Ypos] & (1 << Xpos)) != 0) {
        BltBuffer[Ypos * Width * Count + (Width - Xpos - 1)] = Foreground;
      } else {
        BltBuffer[Ypos * Width * Count + (Width - Xpos - 1)] = Background;
      }
    }
  }

  return EFI_SUCCESS;
}
