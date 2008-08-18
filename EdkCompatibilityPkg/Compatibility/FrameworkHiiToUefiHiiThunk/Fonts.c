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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
