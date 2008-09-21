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

/**
  This function adds the string into String Package of each language
  supported by the package list.

  If String is NULL, then ASSERT.
  If StringId is NULL, the ASSERT.
  If PackageList could not be found in the default HII database, then ASSERT.

  @param  PackageList            Handle of the package list where this string will
                                            be added.
  @param  StringId               On return, contains the new strings id, which is
                                          unique within PackageList.
  @param  String                 Points to the new null-terminated string.

  @retval EFI_SUCCESS             The new string was added successfully.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.

**/
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

    //
    // For each language supported by the package,
    // a string token is created.
    //
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


/**
  This function update the specified string in String Package of each language
  supported by the package list.

  If String is NULL, then ASSERT.
  If PackageList could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  PackageList            Handle of the package list where this string will
                                            be added.
  @param  StringId               Ths String Id to be updated.
  @param  String                 Points to the new null-terminated string.

  @retval EFI_SUCCESS            The new string was added successfully.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.

**/
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

    //
    // For each language supported by the package,
    // the string is updated.
    //
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


/**
  Get the string given the StringId and String package Producer's Guid. The caller
  is responsible to free the *String.

  If PackageList with the matching ProducerGuid is not found, then ASSERT.
  If PackageList with the matching ProducerGuid is found but no String is
  specified by StringId is found, then ASSERT.

  @param  ProducerGuid           The Guid of String package list.
  @param  StringId               The String ID.
  @param  String                 The output string.

  @retval EFI_SUCCESS            Operation is successful.
  @retval EFI_OUT_OF_RESOURCES   There is not enought memory in the system.

**/
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
    FreePool (HiiHandleBuffer);
  }
  return Status;
}

/**
  This function try to retrieve string from String package of current language.
  If fails, it try to retrieve string from String package of first language it support.

  If StringSize is NULL, then ASSERT.
  If String is NULL and *StringSize is not 0, then ASSERT.
  If PackageList could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  PackageList     The package list in the HII database to search for
                                     the specified string.
  @param  StringId          The string's id, which is unique within
                                      PackageList.
  @param  String             Points to the new null-terminated string.
  @param  StringSize       On entry, points to the size of the buffer pointed
                                 to by String, in bytes. On return, points to the
                                 length of the string, in bytes.

  @retval EFI_SUCCESS            The string was returned successfully.
  @retval EFI_NOT_FOUND          The string specified by StringId is not available.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by StringLength is too small
                                 to hold the string.

**/
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

  ASSERT (StringSize != NULL);
  ASSERT (!(*StringSize != 0 && String == NULL));
  ASSERT (IsHiiHandleRegistered (PackageList));

  GetCurrentLanguage (CurrentLang);

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
    FreePool (Languages);

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


/**
  Get string specified by StringId form the HiiHandle. The caller
  is responsible to free the *String.

  If String is NULL, then ASSERT.
  If HiiHandle could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  HiiHandle              The HII handle of package list.
  @param  StringId               The String ID.
  @param  String                 The output string.

  @retval EFI_NOT_FOUND          String is not found.
  @retval EFI_SUCCESS            Operation is successful.
  @retval EFI_OUT_OF_RESOURCES   There is not enought memory in the system.

**/
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
    FreePool (*String);
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
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR8 Iso639ToRfc3066ConversionTable[] =
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


