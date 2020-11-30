/** @file
  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

/**
  Get next language from language code list (with separator ';').

  @param  LangCode       Input: point to first language in the list. On
                         Otput: point to next language in the list, or
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

  if (LangCode == NULL || *LangCode == NULL || Lang == NULL) {
    return;
  }

  Index     = 0;
  StringPtr = *LangCode;
  while (StringPtr[Index] != 0 && StringPtr[Index] != ';') {
    Index++;
  }

  (VOID)CopyMem (Lang, StringPtr, Index);
  Lang[Index] = 0;

  if (StringPtr[Index] == ';') {
    Index++;
  }
  *LangCode = StringPtr + Index;
}

/**
  This function returns the number of supported languages on HiiHandle.

  @param   HiiHandle    The HII package list handle.

  @retval  The number of supported languages.

**/
UINT16
EFIAPI
GetSupportedLanguageNumber (
  IN EFI_HII_HANDLE    HiiHandle
  )
{
  CHAR8   *Lang;
  CHAR8   *Languages;
  CHAR8   *LanguageString;
  UINT16  LangNumber;

  Languages = HiiGetSupportedLanguages (HiiHandle);
  if (Languages == NULL) {
    return 0;
  }

  LangNumber = 0;
  Lang = AllocatePool (AsciiStrSize (Languages));
  if (Lang != NULL) {
    LanguageString = Languages;
    while (*LanguageString != 0) {
      GetNextLanguage (&LanguageString, Lang);
      LangNumber++;
    }
    FreePool (Lang);
  }
  FreePool (Languages);
  return LangNumber;
}


/**
  This function makes boot time changes to the contents of the
  MiscNumberOfInstallableLanguages (Type 13).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscNumberOfInstallableLanguages)
{
  UINTN                                     LangStrLen;
  CHAR8                                     CurrentLang[SMBIOS_STRING_MAX_LENGTH + 1];
  CHAR8                                     *OptionalStrStart;
  EFI_STATUS                                Status;
  EFI_SMBIOS_HANDLE                         SmbiosHandle;
  SMBIOS_TABLE_TYPE13                       *SmbiosRecord;
  SMBIOS_TABLE_TYPE13                       *InputData = NULL;;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE13 *)RecordData;

  InputData->InstallableLanguages = GetSupportedLanguageNumber (mHiiHandle);

  //
  // Try to check if current langcode matches with the langcodes in installed languages
  //
  ZeroMem (CurrentLang, SMBIOS_STRING_MAX_LENGTH - 1);
  (VOID)AsciiStrCpyS (CurrentLang, SMBIOS_STRING_MAX_LENGTH - 1, "en|US|iso8859-1");
  LangStrLen = AsciiStrLen (CurrentLang);

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE13) + LangStrLen + 1 + 1);
  if (SmbiosRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE13));

  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE13);

  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  (VOID)AsciiStrCpyS (OptionalStrStart, SMBIOS_STRING_MAX_LENGTH - 1, CurrentLang);
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = LogSmbiosData ((UINT8*)SmbiosRecord, &SmbiosHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]:[%dL] Smbios Type13 Table Log Failed! %r \n",
            __FUNCTION__, __LINE__, Status));
  }

  FreePool (SmbiosRecord);
  return Status;
}
