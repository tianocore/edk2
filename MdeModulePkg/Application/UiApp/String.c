/** @file
  String support

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ui.h"
#include "FrontPage.h"

EFI_HII_HANDLE gStringPackHandle;

EFI_GUID mUiStringPackGuid = {
  0x136a3048, 0x752a, 0x4bf6, { 0xa7, 0x57, 0x9, 0x36, 0x11, 0x95, 0x38, 0xed }
};

EFI_GUID  mFontPackageGuid = {
  0x78941450, 0x90ab, 0x4fb1, {0xb7, 0x5f, 0x58, 0x92, 0x14, 0xe2, 0x4a, 0xc}
};

#define NARROW_GLYPH_NUMBER 8
#define WIDE_GLYPH_NUMBER   75

typedef struct {
  ///
  /// This 4-bytes total array length is required by HiiAddPackages()
  ///
  UINT32                 Length;

  //
  // This is the Font package definition
  //
  EFI_HII_PACKAGE_HEADER Header;
  UINT16                 NumberOfNarrowGlyphs;
  UINT16                 NumberOfWideGlyphs;
  EFI_NARROW_GLYPH       NarrowArray[NARROW_GLYPH_NUMBER];
  EFI_WIDE_GLYPH         WideArray[WIDE_GLYPH_NUMBER];
} FONT_PACK_BIN;

FONT_PACK_BIN mFontBin = {
  sizeof (FONT_PACK_BIN),
  {
    sizeof (FONT_PACK_BIN) - sizeof (UINT32),
    EFI_HII_PACKAGE_SIMPLE_FONTS,
  },
  NARROW_GLYPH_NUMBER,
  0,
  {     // Narrow Glyphs
    {
      0x05d0,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x4E,
        0x6E,
        0x62,
        0x32,
        0x32,
        0x3C,
        0x68,
        0x4C,
        0x4C,
        0x46,
        0x76,
        0x72,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x05d1,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x78,
        0x7C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x7E,
        0x7E,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x05d2,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x78,
        0x7C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x1C,
        0x3E,
        0x66,
        0x66,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x05d3,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x7E,
        0x7E,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x05d4,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x7C,
        0x7E,
        0x06,
        0x06,
        0x06,
        0x06,
        0x66,
        0x66,
        0x66,
        0x66,
        0x66,
        0x66,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x05d5,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x3C,
        0x3C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x0C,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x05d6,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x38,
        0x38,
        0x1E,
        0x1E,
        0x18,
        0x18,
        0x18,
        0x18,
        0x18,
        0x18,
        0x18,
        0x18,
        0x00,
        0x00,
        0x00,
        0x00
      }
    },
    {
      0x0000,
      0x00,
      {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00
      }
    }
  }
};

/**
  Initialize HII global accessor for string support.

**/
VOID
InitializeStringSupport (
  VOID
  )
{
  gStringPackHandle = HiiAddPackages (
                         &mUiStringPackGuid,
                         gImageHandle,
                         UiAppStrings,
                         NULL
                         );
  ASSERT (gStringPackHandle != NULL);
}

/**
  Remove the string package.

**/
VOID
UninitializeStringSupport (
  VOID
  )
{
  HiiRemovePackages (gStringPackHandle);
}

/**
  Get string by string id from HII Interface


  @param Id              String ID.

  @retval  CHAR16 *  String from ID.
  @retval  NULL      If error occurs.

**/
CHAR16 *
GetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (gStringPackHandle, Id, NULL);
}

/**
  Routine to export glyphs to the HII database.  This is in addition to whatever is defined in the Graphics Console driver.

**/
EFI_HII_HANDLE
ExportFonts (
  VOID
  )
{
  return HiiAddPackages (
           &mFontPackageGuid,
           gImageHandle,
           &mFontBin,
           NULL
           );
}
