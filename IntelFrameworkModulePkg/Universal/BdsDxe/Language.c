/** @file
  Language settings

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Language.h"
#include "FrontPage.h"

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
  Routine to export glyphs to the HII database.  This is in addition to whatever is defined in the Graphics Console driver.

**/
VOID
ExportFonts (
  VOID
  )
{
  EFI_HII_HANDLE               HiiHandle;

  HiiHandle = HiiAddPackages (
                &mFontPackageGuid,
                gImageHandle,
                &mFontBin,
                NULL
                );
  ASSERT (HiiHandle != NULL);
}

/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
EFIAPI
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
{
  UINTN  Index;
  CHAR8  *StringPtr;

  ASSERT (LangCode != NULL);
  ASSERT (*LangCode != NULL);
  ASSERT (Lang != NULL);

  Index = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  CopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

/**
  Determine the current language that will be used
  based on language related EFI Variables.

  @param LangCodesSettingRequired - If required to set LangCode variable

**/
VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
{
  EFI_STATUS  Status;
  UINTN       Size;
  CHAR8       *Lang;
  CHAR8       LangCode[ISO_639_2_ENTRY_SIZE + 1];
  CHAR8       *LangCodes;
  CHAR8       *PlatformLang;
  CHAR8       *PlatformLangCodes;
  UINTN       Index;
  BOOLEAN     Invalid;

  ExportFonts ();

  LangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLangCodes);
  if (LangCodesSettingRequired) {
    if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
      //
      // UEFI 2.1 depricated this variable so we support turning it off
      //
      Status = gRT->SetVariable (
                      L"LangCodes",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrSize (LangCodes),
                      LangCodes
                      );
    }


    PlatformLangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes);
    Status = gRT->SetVariable (
                    L"PlatformLangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLangCodes),
                    PlatformLangCodes
                    );
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
    //
    // UEFI 2.1 depricated this variable so we support turning it off
    //

    //
    // Find current LangCode from Lang NV Variable
    //
    Size = ISO_639_2_ENTRY_SIZE + 1;
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
      Lang = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLang);
      Status = gRT->SetVariable (
                      L"Lang",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      ISO_639_2_ENTRY_SIZE + 1,
                      Lang
                      );
    }
  }

  Invalid = FALSE;
  PlatformLang = BdsLibGetVariableAndSize (L"PlatformLang", &gEfiGlobalVariableGuid, &Size);
  if (PlatformLang != NULL) {
    //
    // Check Current PlatformLang value against PlatformLangCode. Need a library that is TBD
    // Set Invalid based on state of PlatformLang.
    //

    FreePool (PlatformLang);
  } else {
    // No valid variable is set
    Invalid = TRUE;
  }

  if (Invalid) {
    PlatformLang = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLang);
    Status = gRT->SetVariable (
                    L"PlatformLang",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLang),
                    PlatformLang
                    );
  }
}
