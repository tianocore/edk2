/** @file
  Language settings

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
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
  Check if lang is in supported language codes according to language string.

  This code is used to check if lang is in in supported language codes. It can handle
  RFC4646 and ISO639 language tags.
  In ISO639 language tags, take 3-characters as a delimitation to find matched string.
  In RFC4646 language tags, take semicolon as a delimitation to find matched string.

  For example:
    SupportedLang  = "engfraengfra"
    Iso639Language = TRUE
    Lang           = "eng", the return value is "TRUE", or
    Lang           = "chs", the return value is "FALSE".
  Another example:
    SupportedLang  = "en;fr;en-US;fr-FR"
    Iso639Language = FALSE
    Lang           = "en", the return value is "TRUE", or
    Lang           = "zh", the return value is "FALSE".

  @param  SupportedLang               Platform supported language codes.
  @param  Lang                        Configured language.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646.

  @retval TRUE  lang is in supported language codes.
  @retval FALSE lang is not in supported language codes.

**/
BOOLEAN
IsLangInSupportedLangCodes(
  IN  CHAR8            *SupportedLang,
  IN  CHAR8            *Lang,
  IN  BOOLEAN          Iso639Language
  ) 
{
  UINTN    Index;
  UINTN    CompareLength;
  UINTN    LanguageLength;

  if (Iso639Language) {
    CompareLength = ISO_639_2_ENTRY_SIZE;
    for (Index = 0; Index < AsciiStrLen (SupportedLang); Index += CompareLength) {
      if (AsciiStrnCmp (Lang, SupportedLang + Index, CompareLength) == 0) {
        //
        // Successfully find the Lang string in SupportedLang string.
        //
        return TRUE;
      }
    }
    return FALSE;
  } else {
    //
    // Compare RFC4646 language code
    //
    for (LanguageLength = 0; Lang[LanguageLength] != '\0'; LanguageLength++);

    for (; *SupportedLang != '\0'; SupportedLang += CompareLength) {
      //
      // Skip ';' characters in SupportedLang
      //
      for (; *SupportedLang != '\0' && *SupportedLang == ';'; SupportedLang++);
      //
      // Determine the length of the next language code in SupportedLang
      //
      for (CompareLength = 0; SupportedLang[CompareLength] != '\0' && SupportedLang[CompareLength] != ';'; CompareLength++);
      
      if ((CompareLength == LanguageLength) && 
          (AsciiStrnCmp (Lang, SupportedLang, CompareLength) == 0)) {
        //
        // Successfully find the Lang string in SupportedLang string.
        //
        return TRUE;
      }
    }
    return FALSE;
  }
}

/**
  Initialize Lang or PlatformLang variable, if Lang or PlatformLang variable is not found,
  or it has been set to an unsupported value(not one of platform supported language codes),
  set the default language code to it.

  @param  LangName                    Language name, L"Lang" or L"PlatformLang".
  @param  SupportedLang               Platform supported language codes.
  @param  DefaultLang                 Default language code.
  @param  Iso639Language              A bool value to signify if the handler is operated on ISO639 or RFC4646,
                                      TRUE for L"Lang" LangName or FALSE for L"PlatformLang" LangName.

**/
VOID
InitializeLangVariable (
  IN CHAR16     *LangName,
  IN CHAR8      *SupportedLang,
  IN CHAR8      *DefaultLang,     
  IN BOOLEAN    Iso639Language
  )
{
  CHAR8       *Lang;

  //
  // Find current Lang or PlatformLang from EFI Variable.
  //
  GetEfiGlobalVariable2 (LangName, (VOID **) &Lang, NULL);
  //
  // If Lang or PlatformLang variable is not found,
  // or it has been set to an unsupported value(not one of the supported language codes),
  // set the default language code to it.
  //
  if ((Lang == NULL) || !IsLangInSupportedLangCodes (SupportedLang, Lang, Iso639Language)) {
    //
    // The default language code should be one of the supported language codes.
    //
    ASSERT (IsLangInSupportedLangCodes (SupportedLang, DefaultLang, Iso639Language));
    BdsDxeSetVariableAndReportStatusCodeOnError (
                    LangName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (DefaultLang),
                    DefaultLang
                    );
  }

  if (Lang != NULL) {
    FreePool (Lang);
  }
}

/**
  Determine the current language that will be used
  based on language related EFI Variables.

  @param LangCodesSettingRequired - If required to set LangCodes variable

**/
VOID
InitializeLanguage (
  BOOLEAN LangCodesSettingRequired
  )
{
  EFI_STATUS  Status;
  CHAR8       *LangCodes;
  CHAR8       *PlatformLangCodes;

  ExportFonts ();

  LangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultLangCodes);
  PlatformLangCodes = (CHAR8 *)PcdGetPtr (PcdUefiVariableDefaultPlatformLangCodes);
  if (LangCodesSettingRequired) {
    if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
      //
      // UEFI 2.0 depricated this variable so we support turning it off
      //
      Status = gRT->SetVariable (
                      L"LangCodes",
                      &gEfiGlobalVariableGuid,
                      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      AsciiStrSize (LangCodes),
                      LangCodes
                      );
      //
      // Platform needs to make sure setting volatile variable before calling 3rd party code shouldn't fail.
      //
      ASSERT_EFI_ERROR (Status);
    }

    Status = gRT->SetVariable (
                    L"PlatformLangCodes",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    AsciiStrSize (PlatformLangCodes),
                    PlatformLangCodes
                    );
    //
    // Platform needs to make sure setting volatile variable before calling 3rd party code shouldn't fail.
    //
    ASSERT_EFI_ERROR (Status);
  }

  if (!FeaturePcdGet (PcdUefiVariableDefaultLangDeprecate)) {
    //
    // UEFI 2.0 depricated this variable so we support turning it off
    //
    InitializeLangVariable (L"Lang", LangCodes, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultLang), TRUE);
  }
  InitializeLangVariable (L"PlatformLang", PlatformLangCodes, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang), FALSE);
}
