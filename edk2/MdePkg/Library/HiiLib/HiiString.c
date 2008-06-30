/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "InternalHiiLib.h"
EFI_STATUS
EFIAPI
HiiLibNewString (
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST EFI_STRING                String
  )
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];

  ASSERT (String != NULL);
  ASSERT (StringId != NULL);

  Status = EFI_SUCCESS;

  Languages = HiiLibGetSupportedLanguages (PackageList);

  LangStrings = Languages;
  while (*LangStrings != 0) {
    HiiLibGetNextLanguage (&LangStrings, Lang);

    Status = mHiiStringProt->NewString (
                                 mHiiStringProt,
                                 PackageList,
                                 StringId,
                                 Lang,
                                 NULL,
                                 String,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (Languages);

  return Status;
  
}

EFI_STATUS
EFIAPI
HiiLibSetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  IN  CONST EFI_STRING                String
  )
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];

  ASSERT (IsHiiHandleRegistered (PackageList));

  Status = EFI_SUCCESS;

  Languages = HiiLibGetSupportedLanguages (PackageList);
  ASSERT (Languages != NULL);

  LangStrings = Languages;
  while (*LangStrings != 0) {
    HiiLibGetNextLanguage (&LangStrings, Lang);

    Status = mHiiStringProt->SetString (
                                 mHiiStringProt,
                                 PackageList,
                                 StringId,
                                 Lang,
                                 String,
                                 NULL
                                 );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  FreePool (Languages);

  return Status;
}


EFI_STATUS
EFIAPI
HiiLibGetStringFromToken (
  IN  EFI_GUID                        *ProducerGuid,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
{
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           HandleBufferLen;
  EFI_HII_HANDLE  *HiiHandleBuffer;
  EFI_GUID        Guid;

  Status = HiiLibGetHiiHandles (&HandleBufferLen, &HiiHandleBuffer);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  for (Index = 0; Index < (HandleBufferLen / sizeof (EFI_HII_HANDLE)); Index++) {
    Status = HiiLibExtractGuidFromHiiHandle (HiiHandleBuffer[Index], &Guid);
    if (EFI_ERROR(Status)) {
      return Status;
    }
    if (CompareGuid (&Guid, ProducerGuid)) {
      break;
    }
  }

  if (Index >= (HandleBufferLen / sizeof (EFI_HII_HANDLE))) {
    //
    // If PackageList with the matching ProducerGuid is not found, then ASSERT.
    //
    ASSERT (FALSE);
    Status = EFI_NOT_FOUND;
    goto Out;
  }

  Status = HiiLibGetStringFromHandle (HiiHandleBuffer[Index], StringId, String);

Out:
  if (HiiHandleBuffer != NULL) {
    gBS->FreePool (HiiHandleBuffer);
  }
  return Status;
}

EFI_STATUS
EFIAPI
HiiLibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  )
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];
  CHAR8       CurrentLang[RFC_3066_ENTRY_SIZE];

  ASSERT (String != NULL);
  ASSERT (StringSize != NULL);
  ASSERT (IsHiiHandleRegistered (PackageList));

  HiiLibGetCurrentLanguage (CurrentLang);

  Status = mHiiStringProt->GetString (
                               mHiiStringProt,
                               CurrentLang,
                               PackageList,
                               StringId,
                               String,
                               StringSize,
                               NULL
                               );

  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    Languages = HiiLibGetSupportedLanguages (PackageList);
    ASSERT (Languages != NULL);
    
    LangStrings = Languages;
    HiiLibGetNextLanguage (&LangStrings, Lang);
    gBS->FreePool (Languages);

    Status = mHiiStringProt->GetString (
                                 mHiiStringProt,
                                 Lang,
                                 PackageList,
                                 StringId,
                                 String,
                                 StringSize,
                                 NULL
                                 );
  }

  return Status;
}


EFI_STATUS
EFIAPI
HiiLibGetStringFromHandle (
  IN  EFI_HII_HANDLE                  HiiHandle,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
{
  EFI_STATUS                          Status;
  UINTN                               StringSize;

  ASSERT (String != NULL);

  StringSize = HII_LIB_DEFAULT_STRING_SIZE;
  *String    = AllocateZeroPool (StringSize);
  if (*String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = HiiLibGetString (HiiHandle, StringId, *String, &StringSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (*String);
    *String = AllocateZeroPool (StringSize);
    if (*String == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = HiiLibGetString (HiiHandle, StringId, *String, &StringSize);
  }

  return Status;
}



//
// Lookup table of ISO639-2 3 character language codes to ISO 639-1 2 character language codes
// Each entry is 5 CHAR8 values long.  The first 3 CHAR8 values are the ISO 639-2 code.
// The last 2 CHAR8 values are the ISO 639-1 code.
//
CHAR8 Iso639ToRfc3066ConversionTable[] =
"\
aaraa\
abkab\
afraf\
amham\
araar\
asmas\
aymay\
azeaz\
bakba\
belbe\
benbn\
bihbh\
bisbi\
bodbo\
brebr\
bulbg\
catca\
cescs\
corkw\
cosco\
cymcy\
danda\
deude\
dzodz\
ellel\
engen\
epoeo\
estet\
euseu\
faofo\
fasfa\
fijfj\
finfi\
frafr\
fryfy\
gaiga\
gdhgd\
glggl\
grngn\
gujgu\
hauha\
hebhe\
hinhi\
hrvhr\
hunhu\
hyehy\
ikuiu\
ileie\
inaia\
indid\
ipkik\
islis\
itait\
jawjw\
jpnja\
kalkl\
kankn\
kasks\
katka\
kazkk\
khmkm\
kinrw\
kirky\
korko\
kurku\
laolo\
latla\
lavlv\
linln\
litlt\
ltzlb\
malml\
marmr\
mkdmk\
mlgmg\
mltmt\
molmo\
monmn\
mrimi\
msams\
myamy\
nauna\
nepne\
nldnl\
norno\
ocioc\
ormom\
panpa\
polpl\
porpt\
pusps\
quequ\
rohrm\
ronro\
runrn\
rusru\
sagsg\
sansa\
sinsi\
slksk\
slvsl\
smise\
smosm\
snasn\
sndsd\
somso\
sotst\
spaes\
sqisq\
srpsr\
sswss\
sunsu\
swasw\
swesv\
tamta\
tattt\
telte\
tgktg\
tgltl\
thath\
tsnts\
tuktk\
twitw\
uigug\
ukruk\
urdur\
uzbuz\
vievi\
volvo\
wolwo\
xhoxh\
yidyi\
zhaza\
zhozh\
zulzu\
";


/**
  Convert language code from RFC3066 to ISO639-2.

  @param  LanguageRfc3066        RFC3066 language code.
  @param  LanguageIso639         ISO639-2 language code.

  @retval EFI_SUCCESS            Language code converted.
  @retval EFI_NOT_FOUND          Language code not found.

**/
EFI_STATUS
EFIAPI
ConvertRfc3066LanguageToIso639Language (
  CHAR8   *LanguageRfc3066,
  CHAR8   *LanguageIso639
  )
{
  UINTN  Index;

  if ((LanguageRfc3066[2] != '-') && (LanguageRfc3066[2] != 0)) {
    CopyMem (LanguageIso639, LanguageRfc3066, 3);
    return EFI_SUCCESS;
  }

  for (Index = 0; Iso639ToRfc3066ConversionTable[Index] != 0; Index += 5) {
    if (CompareMem (LanguageRfc3066, &Iso639ToRfc3066ConversionTable[Index + 3], 2) == 0) {
      CopyMem (LanguageIso639, &Iso639ToRfc3066ConversionTable[Index], 3);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Convert language code from ISO639-2 to RFC3066.

  LanguageIso639 contain a single ISO639-2 code such as
  "eng" or "fra".

  The LanguageRfc3066 must be a buffer large enough
  for RFC_3066_ENTRY_SIZE characters.

  If LanguageIso639 is NULL, then ASSERT.
  If LanguageRfc3066 is NULL, then ASSERT.

  @param  LanguageIso639         ISO639-2 language code.
  @param  LanguageRfc3066        RFC3066 language code.

  @retval EFI_SUCCESS            Language code converted.
  @retval EFI_NOT_FOUND          Language code not found.

**/
EFI_STATUS
EFIAPI
ConvertIso639LanguageToRfc3066Language (
  IN  CONST CHAR8   *LanguageIso639,
  OUT CHAR8         *LanguageRfc3066
  )
{
  UINTN Index;
  
  for (Index = 0; Iso639ToRfc3066ConversionTable[Index] != 0; Index += 5) {
    if (CompareMem (LanguageIso639, &Iso639ToRfc3066ConversionTable[Index], 3) == 0) {
      CopyMem (LanguageRfc3066, &Iso639ToRfc3066ConversionTable[Index + 3], 2);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Convert language code list from RFC3066 to ISO639-2, e.g. "en-US;fr-FR" will
  be converted to "engfra".

  @param  SupportedLanguages     The RFC3066 language list.

  @return The ISO639-2 language list.

**/
CHAR8 *
EFIAPI
Rfc3066ToIso639 (
  CHAR8  *SupportedLanguages
  )
{
  CHAR8       *Languages;
  CHAR8       *ReturnValue;
  CHAR8       *LangCodes;
  CHAR8       LangRfc3066[RFC_3066_ENTRY_SIZE];
  CHAR8       LangIso639[ISO_639_2_ENTRY_SIZE];
  EFI_STATUS  Status;

  ReturnValue = AllocateZeroPool (AsciiStrSize (SupportedLanguages));
  if (ReturnValue == NULL) {
    return ReturnValue;
  }

  Languages = ReturnValue;
  LangCodes = SupportedLanguages;
  while (*LangCodes != 0) {
    HiiLibGetNextLanguage (&LangCodes, LangRfc3066);

    Status = ConvertRfc3066LanguageToIso639Language (LangRfc3066, LangIso639);
    if (!EFI_ERROR (Status)) {
      CopyMem (Languages, LangIso639, 3);
      Languages = Languages + 3;
    }
  }

  return ReturnValue;
}


