/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  language.c

Abstract:

 Language settings

Revision History

--*/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "String.h"
#include "Language.h"

#define NARROW_GLYPH_NUMBER 8
#define WIDE_GLYPH_NUMBER   75

//
// Default language code, currently is English
//
CHAR8 *mDefaultLangCode = "eng";

typedef struct {
  EFI_HII_FONT_PACK FixedLength;
  EFI_NARROW_GLYPH  NarrowArray[NARROW_GLYPH_NUMBER];
  EFI_WIDE_GLYPH    WideArray[WIDE_GLYPH_NUMBER];
} FONT_PACK;

FONT_PACK mFontPack = {
  sizeof (EFI_HII_FONT_PACK) + (NARROW_GLYPH_NUMBER * sizeof (EFI_NARROW_GLYPH)) + (WIDE_GLYPH_NUMBER * sizeof (EFI_WIDE_GLYPH)),
  EFI_HII_FONT,
  NARROW_GLYPH_NUMBER,
  WIDE_GLYPH_NUMBER,
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
  },
  {     // Wide Glyphs
    {
      0x0020,
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
      },
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
      },
      {
        0x00,
        0x00,
        0x00
      }
    },  //
  }
};

VOID
ExportFonts (
  VOID
  )
/*++

Routine Description:
  Routine to export glyphs to the HII database.  This is in addition to whatever is defined in the Graphics Console driver.

Arguments:
  None

Returns:

--*/
{
  EFI_HII_HANDLE    HiiHandle;
  EFI_HII_PACKAGES  *PackageList;

  PackageList = PreparePackages (1, NULL, &mFontPack);
  //
  // Register our Fonts into the global database
  //
  HiiHandle = 0;
  Hii->NewPack (Hii, PackageList, &HiiHandle);

  FreePool (PackageList);
}

VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
/*++

Routine Description:
  Determine the current language that will be used
  based on language related EFI Variables

Arguments:
  LangCodesSettingRequired - If required to set LangCode variable

Returns:

--*/
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       Size;
  CHAR8       LangCode[ISO_639_2_ENTRY_SIZE];
  CHAR8       *LangCodes;
  CHAR16      *LanguageString;

  LanguageString  = NULL;
  LangCodes       = NULL;

  ExportFonts ();

  //
  // Collect the languages from what our current Language support is based on our VFR
  //
  Hii->GetPrimaryLanguages (Hii, gStringPackHandle, &LanguageString);

  LangCodes = AllocatePool (StrLen (LanguageString));
  ASSERT (LangCodes);

  //
  // Convert LanguageString from Unicode to EFI defined ASCII LangCodes
  //
  for (Index = 0; LanguageString[Index] != 0x0000; Index++) {
    LangCodes[Index] = (CHAR8) LanguageString[Index];
  }

  LangCodes[Index] = 0;

  if (LangCodesSettingRequired) {
    Status = gRT->SetVariable (
                    L"LangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrLen (LangCodes),
                    LangCodes
                    );
  }
  //
  // Find current LangCode from Lang NV Variable
  //
  Size = ISO_639_2_ENTRY_SIZE;
  Status = gRT->GetVariable (
                  L"Lang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  &LangCode
                  );

  if (!EFI_ERROR (Status)) {
    Status = EFI_NOT_FOUND;
    for (Index = 0; LangCodes[Index] != 0; Index += ISO_639_2_ENTRY_SIZE) {
      if (CompareMem (&LangCodes[Index], LangCode, ISO_639_2_ENTRY_SIZE) == 0) {
        Status = EFI_SUCCESS;
        break;
      }
    }
  }
  //
  // If we cannot get language code from Lang variable,
  // or LangCode cannot be found from language table,
  // set the mDefaultLangCode to Lang variable.
  //
  if (EFI_ERROR (Status)) {
    Status = gRT->SetVariable (
                    L"Lang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    ISO_639_2_ENTRY_SIZE,
                    mDefaultLangCode
                    );
  }

  if (LangCodes) {
    FreePool (LangCodes);
  }

  if (LanguageString != NULL) {
    FreePool (LanguageString);
  }

}
