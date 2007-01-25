/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Fonts.c

Abstract:

  This file contains the Glyph/Font processing code to the HII database.

--*/


#include "HiiDatabase.h"

//
// We only need to define a wide glyph, since we will seed the narrow glyph with EFI_NARROW_GLYPH size of
// this data structure
//
UINT8 mUnknownGlyph[38] = {
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xAA,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xaa,
  0x55,
  0xAA
};

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
/*++

Routine Description:
  Translates a Unicode character into the corresponding font glyph.  
  If the Source was pointing to a non-spacing character, the next Source[*Index]
  character will be parsed and OR'd to the GlyphBuffer until a spacing character
  is found in the Source.  Since non-spacing characters are considered to be the
  same pixel width as a regular character their BitWidth will be reflected correctly
  however due to their special attribute, they are considered to be zero advancing width.
  This basically means that the cursor would not advance, thus the character that follows
  it would overlay the non-spacing character.  The Index is modified to reflect both the
  incoming array entry into the Source string but also the outgoing array entry after having
  parsed the equivalent of a single Glyph's worth of data.

Arguments:

Returns: 

--*/
{
  EFI_HII_GLOBAL_DATA *GlobalData;
  EFI_HII_DATA        *HiiData;
  UINTN               Count;
  BOOLEAN             Narrow;
  UINTN               Location;
  UINTN               SearchLocation;
  UINTN               Value;
  CHAR16              Character;
  UINTN               Attributes;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiData         = EFI_HII_DATA_FROM_THIS (This);

  GlobalData      = HiiData->GlobalData;
  Count           = sizeof (GlobalData->NarrowGlyphs->GlyphCol1);

  Location        = *Index;
  SearchLocation  = *Index;
  Narrow          = TRUE;

  if (Source[Location] == NARROW_CHAR || Source[Location] == WIDE_CHAR) {
    *InternalStatus = 0;
  }
  //
  // We don't know what glyph database to look in - let's figure it out
  //
  if (*InternalStatus == 0) {
    //
    // Determine if we are looking for narrow or wide glyph data
    //
    do {
      if (Source[SearchLocation] == NARROW_CHAR || Source[SearchLocation] == WIDE_CHAR) {
        //
        // We found something that identifies what glyph database to look in
        //
        if (Source[SearchLocation] == WIDE_CHAR) {
          Narrow          = FALSE;
          *BitWidth       = WIDE_WIDTH;
          *InternalStatus = WIDE_CHAR;
          Location++;
          break;
        } else {
          Narrow          = TRUE;
          *BitWidth       = NARROW_WIDTH;
          *InternalStatus = NARROW_CHAR;
          Location++;
          break;
        }
      }
    } while (SearchLocation-- > 0);
  }

  if (*InternalStatus == NARROW_CHAR) {
    Narrow    = TRUE;
    *BitWidth = NARROW_WIDTH;
  } else if (*InternalStatus == WIDE_CHAR) {
    Narrow    = FALSE;
    *BitWidth = WIDE_WIDTH;
  } else {
    //
    // Without otherwise knowing what the width is narrow (e.g. someone passed in a string with index of 0
    // we wouldn't be able to determine the width of the data.)
    // BUGBUG - do we go to wide database and if exist, ignore narrow?  Check Unicode spec....
    //
    Narrow    = TRUE;
    *BitWidth = NARROW_WIDTH;
  }

  Character = Source[Location];

  if (Narrow) {
    if (GlobalData->NarrowGlyphs[Character].UnicodeWeight != 0x0000) {
      *GlyphBuffer  = (UINT8 *) (&GlobalData->NarrowGlyphs[Character]);
      Attributes    = GlobalData->NarrowGlyphs[Character].Attributes & EFI_GLYPH_NON_SPACING;
    } else {
      //
      // Glyph is uninitialized - return an error, but hand back the glyph
      //
      *GlyphBuffer  = (UINT8 *) (&GlobalData->NarrowGlyphs[Character]);
      *Index        = (UINT16) (Location + 1);
      return EFI_NOT_FOUND;
    }
  } else {
    //
    // Wide character
    //
    if (GlobalData->WideGlyphs[Character].UnicodeWeight != 0x0000) {
      *GlyphBuffer  = (UINT8 *) (&GlobalData->WideGlyphs[Character]);
      Attributes    = GlobalData->WideGlyphs[Character].Attributes & EFI_GLYPH_NON_SPACING;
    } else {
      //
      // Glyph is uninitialized - return an error, but hand back the glyph
      //
      *GlyphBuffer  = (UINT8 *) (&GlobalData->WideGlyphs[Character]);
      *Index        = (UINT16) (Location + 1);
      return EFI_NOT_FOUND;
    }
  }
  //
  // This is a non-spacing character.  It will be followed by either more non-spacing
  // characters or a regular character.  We need to OR together the data associated with each.
  //
  for (; Attributes != 0; Location++) {
    //
    // Character is the Unicode value which is the index into the Glyph array.
    //
    Character = Source[Location];

    if (Narrow) {
      for (Value = 0; Value != Count; Value++) {
        *GlyphBuffer[Location + Value] = (UINT8) (*GlyphBuffer[Location + Value] |
                                                  GlobalData->NarrowGlyphs[Character].GlyphCol1[Value]);
      }

      Attributes = GlobalData->NarrowGlyphs[Character].Attributes & EFI_GLYPH_NON_SPACING;
    } else {
      for (Value = 0; Value != Count; Value++) {
        *GlyphBuffer[Location + Value] = (UINT8) (*GlyphBuffer[Location + Value] | 
                                                  GlobalData->WideGlyphs[Character].GlyphCol1[Value]);
        *GlyphBuffer[Location + Value + Count] = (UINT8) (*GlyphBuffer[Location + Value + Count] |
                                                          GlobalData->WideGlyphs[Character].GlyphCol2[Value]);
      }

      Attributes = GlobalData->WideGlyphs[Character].Attributes & EFI_GLYPH_NON_SPACING;
    }
  }
  //
  // Source[*Index] should point to the next character to process
  //
  *Index = (UINT16) (Location + 1);
  return EFI_SUCCESS;
}

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
