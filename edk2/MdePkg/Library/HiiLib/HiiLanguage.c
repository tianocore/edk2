/** @file
  Language related HII Library implementation.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "InternalHiiLib.h"

/**
  Determine what is the current language setting. The space reserved for Lang
  must be at least RFC_3066_ENTRY_SIZE bytes;

  If Lang is NULL, then ASSERT.

  @param  Lang                   Pointer of system language. Lang will always be filled with 
                                         a valid RFC 3066 language string. If "PlatformLang" is not
                                         set in the system, the default language specifed by PcdUefiVariableDefaultPlatformLang
                                         is returned.

  @return  EFI_SUCCESS     If the EFI Variable with "PlatformLang" is set and return in Lang.
  @return  EFI_NOT_FOUND If the EFI Variable with "PlatformLang" is not set, but a valid default language is return in Lang.

**/
EFI_STATUS
EFIAPI
HiiLibGetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  ASSERT (Lang != NULL);

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
    AsciiStrCpy (Lang, (CHAR8 *) PcdGetPtr (PcdUefiVariableDefaultPlatformLang));
  }

  return Status;
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
HiiLibGetNextLanguage (
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
  This function returns the list of supported languages, in the format specified
  in UEFI specification Appendix M.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.

  @param  HiiHandle              The HII package list handle.

  @retval   !NULL  The supported languages.
  @retval   NULL    If Supported Languages can not be retrived.

**/
CHAR8 *
EFIAPI
HiiLibGetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  CHAR8       *LanguageString;

  ASSERT (IsHiiHandleRegistered (HiiHandle));
  //
  // Collect current supported Languages for given HII handle
  //
  BufferSize = 0x1000;
  LanguageString = AllocateZeroPool (BufferSize);
  if (LanguageString == NULL) {
    return NULL;
  }

  LocateHiiProtocols ();
  
  Status = mHiiStringProt->GetLanguages (mHiiStringProt, HiiHandle, LanguageString, &BufferSize);
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (LanguageString);
    LanguageString = AllocateZeroPool (BufferSize);
    if (LanguageString == NULL) {
      return NULL;
    }

    Status = mHiiStringProt->GetLanguages (mHiiStringProt, HiiHandle, LanguageString, &BufferSize);
  }

  if (EFI_ERROR (Status)) {
    LanguageString = NULL;
  }

  return LanguageString;
}


/**
  This function returns the number of supported languages on HiiHandle.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  HiiHandle              The HII package list handle.

  @return The  number of supported languages.

**/
UINT16
EFIAPI
HiiLibGetSupportedLanguageNumber (
  IN EFI_HII_HANDLE           HiiHandle
  )
{
  CHAR8   *Languages;
  CHAR8   *LanguageString;
  UINT16  LangNumber;
  CHAR8   Lang[RFC_3066_ENTRY_SIZE];

  Languages = HiiLibGetSupportedLanguages (HiiHandle);
  if (Languages == NULL) {
    return 0;
  }

  LangNumber = 0;
  LanguageString = Languages;
  while (*LanguageString != 0) {
    HiiLibGetNextLanguage (&LanguageString, Lang);
    LangNumber++;
  }
  FreePool (Languages);

  return LangNumber;
}

/**
  This function returns the list of supported 2nd languages, in the format specified
  in UEFI specification Appendix M.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  HiiHandle              The HII package list handle.
  @param  FirstLanguage          Pointer to language name buffer.
  
  @return The supported languages.

**/
CHAR8 *
EFIAPI
HiiLibGetSupportedSecondaryLanguages (
  IN EFI_HII_HANDLE           HiiHandle,
  IN CONST CHAR8              *FirstLanguage
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  CHAR8       *LanguageString;

  ASSERT (HiiHandle != NULL);
  ASSERT (IsHiiHandleRegistered (HiiHandle));
  //
  // Collect current supported 2nd Languages for given HII handle
  //
  BufferSize = 0x1000;
  LanguageString = AllocateZeroPool (BufferSize);
  if (LanguageString == NULL) {
    return NULL;
  }

  LocateHiiProtocols ();
  
  Status = mHiiStringProt->GetSecondaryLanguages (mHiiStringProt, HiiHandle, FirstLanguage, LanguageString, &BufferSize);
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (LanguageString);
    LanguageString = AllocateZeroPool (BufferSize);
    if (LanguageString == NULL) {
      return NULL;
    }

    Status = mHiiStringProt->GetSecondaryLanguages (mHiiStringProt, HiiHandle, FirstLanguage, LanguageString, &BufferSize);
  }

  if (EFI_ERROR (Status)) {
    LanguageString = NULL;
  }

  return LanguageString;
}


