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


CHAR8 *
EFIAPI
HiiLibGetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;
  CHAR8       *LanguageString;

  ASSERT (HiiHandle != NULL);
  ASSERT (IsHiiHandleRegistered (HiiHandle));
  //
  // Collect current supported Languages for given HII handle
  //
  BufferSize = 0x1000;
  LanguageString = AllocateZeroPool (BufferSize);
  if (LanguageString == NULL) {
    return NULL;
  }
  
  Status = mHiiStringProt->GetLanguages (mHiiStringProt, HiiHandle, LanguageString, &BufferSize);
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (LanguageString);
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
  gBS->FreePool (Languages);

  return LangNumber;
}

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
  Status = mHiiStringProt->GetSecondaryLanguages (mHiiStringProt, HiiHandle, FirstLanguage, LanguageString, &BufferSize);
  
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (LanguageString);
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


