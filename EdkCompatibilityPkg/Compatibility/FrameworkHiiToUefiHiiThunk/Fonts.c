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


UINT8 mGlyphBuffer[EFI_GLYPH_WIDTH * 2 * EFI_GLYPH_HEIGHT];

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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
