/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UefiIfrString.c

Abstract:

  Common Library Routines to assist to handle String and Language.

--*/

#include "UefiIfrLibrary.h"

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

EFI_STATUS
ConvertRfc3066LanguageToIso639Language (
  CHAR8   *LanguageRfc3066,
  CHAR8   *LanguageIso639
  )
/*++

Routine Description:
  Convert language code from RFC3066 to ISO639-2.

Arguments:
  LanguageRfc3066 - RFC3066 language code.
  LanguageIso639  - ISO639-2 language code.

Returns:
  EFI_SUCCESS   - Language code converted.
  EFI_NOT_FOUND - Language code not found.

--*/
{
  UINTN  Index;

  if ((LanguageRfc3066[2] != '-') && (LanguageRfc3066[2] != 0)) {
    EfiCopyMem (LanguageIso639, LanguageRfc3066, 3);
    return EFI_SUCCESS;
  }

  for (Index = 0; Iso639ToRfc3066ConversionTable[Index] != 0; Index += 5) {
    if (EfiCompareMem (LanguageRfc3066, &Iso639ToRfc3066ConversionTable[Index + 3], 2) == 0) {
      EfiCopyMem (LanguageIso639, &Iso639ToRfc3066ConversionTable[Index], 3);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

CHAR8 *
Rfc3066ToIso639 (
  CHAR8  *SupportedLanguages
  )
/*++

Routine Description:
  Convert language code list from RFC3066 to ISO639-2, e.g. "en-US;fr-FR" will
  be converted to "engfra".

Arguments:
  SupportedLanguages - The RFC3066 language list.

Returns:
  The ISO639-2 language list.

--*/
{
  CHAR8       *Languages;
  CHAR8       *ReturnValue;
  CHAR8       *LangCodes;
  CHAR8       LangRfc3066[RFC_3066_ENTRY_SIZE];
  CHAR8       LangIso639[ISO_639_2_ENTRY_SIZE];
  EFI_STATUS  Status;

  ReturnValue = EfiLibAllocateZeroPool (EfiAsciiStrSize (SupportedLanguages));
  if (ReturnValue == NULL) {
    return ReturnValue;
  }

  Languages = ReturnValue;
  LangCodes = SupportedLanguages;
  while (*LangCodes != 0) {
    GetNextLanguage (&LangCodes, LangRfc3066);

    Status = ConvertRfc3066LanguageToIso639Language (LangRfc3066, LangIso639);
    if (!EFI_ERROR (Status)) {
      EfiCopyMem (Languages, LangIso639, 3);
      Languages = Languages + 3;
    }
  }

  return ReturnValue;
}

EFI_STATUS
GetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
/*++

Routine Description:
  Determine what is the current language setting

Arguments:
  Lang      - Pointer of system language

Returns:
  Status code

--*/
{
  EFI_STATUS  Status;
  UINTN       Size;

  //
  // Get current language setting
  //
  Size = RFC_3066_ENTRY_SIZE;
  Status = gRT->GetVariable (
                  L"PlatformLang",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &Size,
                  Lang
                  );

  if (EFI_ERROR (Status)) {
    EfiAsciiStrCpy (Lang, (CHAR8 *) "en-US");
  }

  return Status;
}

VOID
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
/*++

Routine Description:
  Get next language from language code list (with separator ';').

Arguments:
  LangCode - On input: point to first language in the list. On output: point to
             next language in the list, or NULL if no more language in the list.
  Lang     - The first language in the list.

Returns:
  None.

--*/
{
  UINTN  Index;
  CHAR8  *StringPtr;

  if (LangCode == NULL || *LangCode == NULL) {
    *Lang = 0;
    return;
  }

  Index = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  EfiCopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

CHAR8 *
GetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  This function returns the list of supported languages, in the format specified
  in UEFI specification Appendix M.

Arguments:
  HiiHandle  - The HII package list handle.

Returns:
  The supported languages.

--*/
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  CHAR8       *LanguageString;

  LocateHiiProtocols ();

  //
  // Collect current supported Languages for given HII handle
  //
  BufferSize = 0x1000;
  LanguageString = EfiLibAllocatePool (BufferSize);
  Status = gIfrLibHiiString->GetLanguages (gIfrLibHiiString, HiiHandle, LanguageString, &BufferSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (LanguageString);
    LanguageString = EfiLibAllocatePool (BufferSize);
    Status = gIfrLibHiiString->GetLanguages (gIfrLibHiiString, HiiHandle, LanguageString, &BufferSize);
  }

  if (EFI_ERROR (Status)) {
    LanguageString = NULL;
  }

  return LanguageString;
}

UINT16
GetSupportedLanguageNumber (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  This function returns the number of supported languages

Arguments:
  HiiHandle  - The HII package list handle.

Returns:
  The  number of supported languages.

--*/
{
  CHAR8   *Languages;
  CHAR8   *LanguageString;
  UINT16  LangNumber;
  CHAR8   Lang[RFC_3066_ENTRY_SIZE];

  Languages = GetSupportedLanguages (HiiHandle);
  if (Languages == NULL) {
    return 0;
  }

  LangNumber = 0;
  LanguageString = Languages;
  while (*LanguageString != 0) {
    GetNextLanguage (&LanguageString, Lang);
    LangNumber++;
  }
  gBS->FreePool (Languages);

  return LangNumber;
}

EFI_STATUS
GetStringFromHandle (
  IN  EFI_HII_HANDLE                  HiiHandle,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
/*++

Routine Description:
  Get string specified by StringId form the HiiHandle.

Arguments:
  HiiHandle     - The HII handle of package list.
  StringId      - The String ID.
  String        - The output string.

Returns:
  EFI_NOT_FOUND         - String is not found.
  EFI_SUCCESS           - Operation is successful.
  EFI_OUT_OF_RESOURCES  - There is not enought memory in the system.
  EFI_INVALID_PARAMETER - The String is NULL.

--*/
{
  EFI_STATUS                          Status;
  UINTN                               StringSize;

  if (String == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringSize = IFR_LIB_DEFAULT_STRING_SIZE;
  *String    = EfiLibAllocateZeroPool (StringSize);
  if (*String == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = IfrLibGetString (HiiHandle, StringId, *String, &StringSize);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (*String);
    *String = EfiLibAllocateZeroPool (StringSize);
    if (*String == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Status = IfrLibGetString (HiiHandle, StringId, *String, &StringSize);
  }

  return Status;
}

EFI_STATUS
GetStringFromToken (
  IN  EFI_GUID                        *ProducerGuid,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
/*++

Routine Description:
  Get the string given the StringId and String package Producer's Guid.

Arguments:
  ProducerGuid  - The Guid of String package list.
  StringId      - The String ID.
  String        - The output string.

Returns:
  EFI_NOT_FOUND         - String is not found.
  EFI_SUCCESS           - Operation is successful.
  EFI_OUT_OF_RESOURCES  - There is not enought memory in the system.

--*/
{
  EFI_STATUS      Status;
  UINTN           Index;
  UINTN           HandleBufferLen;
  EFI_HII_HANDLE  *HiiHandleBuffer;
  EFI_GUID        Guid;

  HiiHandleBuffer = NULL;
  Status = GetHiiHandles (&HandleBufferLen, &HiiHandleBuffer);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  for (Index = 0; Index < (HandleBufferLen / sizeof (EFI_HII_HANDLE)); Index++) {
    Status = ExtractGuidFromHiiHandle (HiiHandleBuffer[Index], &Guid);
    if (EFI_ERROR(Status)) {
      return Status;
    }
    if (EfiCompareGuid (&Guid, ProducerGuid) == TRUE) {
      break;
    }
  }

  if (Index >= (HandleBufferLen / sizeof (EFI_HII_HANDLE))) {
    Status = EFI_NOT_FOUND;
    goto Out;
  }

  Status = GetStringFromHandle (HiiHandleBuffer[Index], StringId, String);

Out:
  if (HiiHandleBuffer != NULL) {
    gBS->FreePool (HiiHandleBuffer);
  }
  return Status;
}

EFI_STATUS
IfrLibNewString (
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST EFI_STRING                String
  )
/*++

  Routine Description:
    This function adds the string into String Package of each language.

  Arguments:
    PackageList       - Handle of the package list where this string will be added.
    StringId          - On return, contains the new strings id, which is unique within PackageList.
    String            - Points to the new null-terminated string.

  Returns:
    EFI_SUCCESS            - The new string was added successfully.
    EFI_NOT_FOUND          - The specified PackageList could not be found in database.
    EFI_OUT_OF_RESOURCES   - Could not add the string due to lack of resources.
    EFI_INVALID_PARAMETER  - String is NULL or StringId is NULL is NULL.

--*/
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];

  Status = EFI_SUCCESS;

  LocateHiiProtocols ();

  Languages = GetSupportedLanguages (PackageList);
  if (Languages == NULL) {
    return EFI_NOT_FOUND;
  }
  
  if (StringId == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *StringId = 0;

  LangStrings = Languages;
  while (*LangStrings != 0) {
    GetNextLanguage (&LangStrings, Lang);

    if (*StringId == 0) {
      Status = gIfrLibHiiString->NewString (
                                   gIfrLibHiiString,
                                   PackageList,
                                   StringId,
                                   Lang,
                                   NULL,
                                   String,
                                   NULL
                                   );
    } else {
      Status = gIfrLibHiiString->SetString (
                                   gIfrLibHiiString,
                                   PackageList,
                                   *StringId,
                                   Lang,
                                   String,
                                   NULL
                                   );
    }

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  gBS->FreePool (Languages);

  return Status;
}

EFI_STATUS
IfrLibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  )
/*++

  Routine Description:
    This function try to retrieve string from String package of current language.
    If fail, it try to retrieve string from String package of first language it support.

  Arguments:
    PackageList       - The package list in the HII database to search for the specified string.
    StringId          - The string's id, which is unique within PackageList.
    String            - Points to the new null-terminated string.
    StringSize        - On entry, points to the size of the buffer pointed to by String, in bytes. On return,
                        points to the length of the string, in bytes.

  Returns:
    EFI_SUCCESS            - The string was returned successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not available.
    EFI_BUFFER_TOO_SMALL   - The buffer specified by StringLength is too small to hold the string.
    EFI_INVALID_PARAMETER  - The String or StringSize was NULL.

--*/
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];
  CHAR8       CurrentLang[RFC_3066_ENTRY_SIZE];

  LocateHiiProtocols ();

  GetCurrentLanguage (CurrentLang);

  Status = gIfrLibHiiString->GetString (
                               gIfrLibHiiString,
                               CurrentLang,
                               PackageList,
                               StringId,
                               String,
                               StringSize,
                               NULL
                               );

  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    Languages = GetSupportedLanguages (PackageList);
    LangStrings = Languages;
    GetNextLanguage (&LangStrings, Lang);
    gBS->FreePool (Languages);

    Status = gIfrLibHiiString->GetString (
                                 gIfrLibHiiString,
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
IfrLibSetString (
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_STRING_ID                    StringId,
  IN CONST EFI_STRING                 String
  )
/*++

  Routine Description:
    This function updates the string in String package of each language.

  Arguments:
    PackageList       - The package list containing the strings.
    StringId          - The string's id, which is unique within PackageList.
    String            - Points to the new null-terminated string.

  Returns:
    EFI_SUCCESS            - The string was updated successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not in the database.
    EFI_INVALID_PARAMETER  - The String was NULL.
    EFI_OUT_OF_RESOURCES   - The system is out of resources to accomplish the task.

--*/
{
  EFI_STATUS  Status;
  CHAR8       *Languages;
  CHAR8       *LangStrings;
  CHAR8       Lang[RFC_3066_ENTRY_SIZE];

  Status = EFI_SUCCESS;

  LocateHiiProtocols ();

  Languages = GetSupportedLanguages (PackageList);
  if (Languages == NULL) {
    return EFI_NOT_FOUND;
  }

  LangStrings = Languages;
  while (*LangStrings != 0) {
    GetNextLanguage (&LangStrings, Lang);

    Status = gIfrLibHiiString->SetString (
                                 gIfrLibHiiString,
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

  gBS->FreePool (Languages);

  return Status;
}

