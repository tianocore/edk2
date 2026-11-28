/** @file
  The implementation of EDKII Redfish Platform Config Protocol.

  (C) Copyright 2021-2022 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "RedfishPlatformConfigDxe.h"
#include "RedfishPlatformConfigImpl.h"

extern REDFISH_PLATFORM_CONFIG_PRIVATE  *mRedfishPlatformConfigPrivate;

/**
  Debug dump HII string.

  @param[in]  HiiHandle   HII handle instance
  @param[in]  StringId    HII string to dump

  @retval EFI_SUCCESS       Dump HII string successfully
  @retval Others            Errors occur

**/
EFI_STATUS
DumpHiiString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  )
{
  EFI_STRING  String;

  if ((HiiHandle == NULL) || (StringId == 0)) {
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "???"));
    return EFI_INVALID_PARAMETER;
  }

  String = HiiGetString (HiiHandle, StringId, NULL);
  if (String == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%s", String));
  FreePool (String);

  return EFI_SUCCESS;
}

/**
  Debug dump HII form-set data.

  @param[in]  FormsetPrivate    HII form-set private instance.

  @retval EFI_SUCCESS       Dump form-set successfully
  @retval Others            Errors occur

**/
EFI_STATUS
DumpFormset (
  IN REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  LIST_ENTRY                                 *HiiFormLink;
  LIST_ENTRY                                 *HiiNextFormLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE       *HiiFormPrivate;
  LIST_ENTRY                                 *HiiStatementLink;
  LIST_ENTRY                                 *HiiNextStatementLink;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *HiiStatementPrivate;
  UINTN                                      Index;

  if (FormsetPrivate == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Index       = 0;
  HiiFormLink = GetFirstNode (&FormsetPrivate->HiiFormList);
  while (!IsNull (&FormsetPrivate->HiiFormList, HiiFormLink)) {
    HiiFormPrivate  = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (HiiFormLink);
    HiiNextFormLink = GetNextNode (&FormsetPrivate->HiiFormList, HiiFormLink);

    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "  [%d] form: %d title: ", ++Index, HiiFormPrivate->Id));
    DumpHiiString (FormsetPrivate->HiiHandle, HiiFormPrivate->Title);
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "\n"));

    HiiStatementLink = GetFirstNode (&HiiFormPrivate->StatementList);
    while (!IsNull (&HiiFormPrivate->StatementList, HiiStatementLink)) {
      HiiStatementPrivate  = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (HiiStatementLink);
      HiiNextStatementLink = GetNextNode (&HiiFormPrivate->StatementList, HiiStatementLink);

      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "    QID: 0x%x Prompt: ", HiiStatementPrivate->QuestionId));
      DumpHiiString (FormsetPrivate->HiiHandle, HiiStatementPrivate->Description);
      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "\n"));

      HiiStatementLink = HiiNextStatementLink;
    }

    HiiFormLink = HiiNextFormLink;
  }

  return EFI_SUCCESS;
}

/**
  Debug dump HII form-set list.

  @param[in]  FormsetList   Form-set list instance

  @retval EFI_SUCCESS       Dump list successfully
  @retval Others            Errors occur

**/
EFI_STATUS
DumpFormsetList (
  IN  LIST_ENTRY  *FormsetList
  )
{
  LIST_ENTRY                                *HiiFormsetLink;
  LIST_ENTRY                                *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *HiiFormsetPrivate;
  UINTN                                     Index;

  if (FormsetList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (FormsetList)) {
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: Empty formset list\n", __func__));
    return EFI_SUCCESS;
  }

  Index          = 0;
  HiiFormsetLink = GetFirstNode (FormsetList);
  while (!IsNull (FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "[%d] HII Handle: 0x%x formset: %g at %s\n", ++Index, HiiFormsetPrivate->HiiHandle, &HiiFormsetPrivate->Guid, HiiFormsetPrivate->DevicePathStr));
    DumpFormset (HiiFormsetPrivate);

    HiiFormsetLink = HiiFormsetNextLink;
  }

  return EFI_SUCCESS;
}

/**
  Return the HII string length. We don't check word alignment
  of the input string as same as the checking in StrLen
  function, because the HII string in the database is compact
  at the byte alignment.

  @param[in]  String  Input UCS format string.

  @retval Length of the string.

**/
UINTN
EFIAPI
HiiStrLen (
  IN  CONST CHAR16  *String
  )
{
  UINTN  Length;

  ASSERT (String != NULL);

  for (Length = 0; *String != L'\0'; String++, Length++) {
  }

  return Length;
}

/**
  Return the HII string size. We don't check word alignment
  of the input string as same as the checking in StrLen
  function, because the HII string in the database is compact
  at the byte alignment.

  @param[in]  String  Input UCS format string.

  @retval Size of the string.

**/
UINTN
EFIAPI
HiiStrSize (
  IN      CONST CHAR16  *String
  )
{
  return (HiiStrLen (String) + 1) * sizeof (*String);
}

/**
  Compare two HII strings. We don't check word alignment
  of the input string as same as the checking in StrLen
  function, because the HII string in the database is compact
  at the byte alignment.

  @param[in]  FirstString   Input UCS format of string to search.
  @param[in]  SecondString  Input UCS format of string to look for in
                            FirstString;

  @retval 0   The strings are identical.
          !0  The strings are not identical.

**/
INTN
EFIAPI
HiiStrCmp (
  IN      CONST CHAR16  *FirstString,
  IN      CONST CHAR16  *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength
  //
  ASSERT (HiiStrSize (FirstString) != 0);
  ASSERT (HiiStrSize (SecondString) != 0);

  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }

  return *FirstString - *SecondString;
}

/**
  Delete a string from HII Package List by given HiiHandle.

  @param[in]  StringId           Id of the string in HII database.
  @param[in]  HiiHandle          The HII package list handle.

  @retval EFI_SUCCESS            The string was deleted successfully.
  @retval EFI_INVALID_PARAMETER  StringId is zero.

**/
EFI_STATUS
HiiDeleteString (
  IN  EFI_STRING_ID   StringId,
  IN  EFI_HII_HANDLE  HiiHandle
  )
{
  CHAR16  NullChar;

  if (StringId == 0x00) {
    return EFI_INVALID_PARAMETER;
  }

  NullChar = CHAR_NULL;
  HiiSetString (HiiHandle, StringId, &NullChar, NULL);

  return EFI_SUCCESS;
}

/**
  Retrieves a unicode string from a string package in a given language. The
  returned string is allocated using AllocatePool().  The caller is responsible
  for freeing the allocated buffer using FreePool().

  If HiiHandle is NULL, then ASSERT().
  If StringId is 0, then ASSET.

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  Language          The specified configure language to get string.
  @param[in]  StringId          The identifier of the string to retrieved from the string
                                package associated with HiiHandle.

  @param[in]  AllowCache        TRUE to serve exact-language hits from the per-formset
                                string cache; FALSE to always read through the HII
                                string protocol.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
STATIC
EFI_STRING
HiiGetRedfishStringEx (
  IN EFI_HII_HANDLE  HiiHandle,
  IN CHAR8           *Language,
  IN EFI_STRING_ID   StringId,
  IN BOOLEAN         AllowCache
  )
{
  EFI_STATUS                                Status;
  UINTN                                     StringSize;
  CHAR16                                    TempString;
  EFI_STRING                                String;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate;
  REDFISH_HII_STRING_CACHE                  *StringCache;
  CHAR16                                    *CachedString;

  if ((mRedfishPlatformConfigPrivate->HiiString == NULL) || (HiiHandle == NULL) || (StringId == 0) || IS_EMPTY_STRING (Language)) {
    ASSERT (FALSE);
    return NULL;
  }

  //
  // Fast path: serve the string from the per-formset, per-language cache.
  // EFI_HII_STRING_PROTOCOL.GetString() walks the string package block by
  // block from string ID 1 on every call, which dominates attribute
  // enumeration on platforms with thousands of strings.  The cache only
  // covers an exact language match; anything else (best-language fallback,
  // packages not yet exported, string IDs outside the package) takes the
  // original path below.
  //
  FormsetPrivate = AllowCache ? GetFormsetPrivateByHiiHandle (HiiHandle, &mRedfishPlatformConfigPrivate->FormsetList) : NULL;
  if (FormsetPrivate != NULL) {
    StringCache = GetOrBuildHiiStringCache (FormsetPrivate, Language);
    if ((StringCache != NULL) && ((UINTN)StringId < StringCache->Count)) {
      CachedString = StringCache->Strings[StringId];
      if (CachedString != NULL) {
        //
        // The cached pointer is into the exported package and is only byte
        // aligned, so copy with the HII-aware size helper.
        //
        StringSize = HiiStrSize (CachedString);
        String     = AllocateZeroPool (StringSize);
        if (String == NULL) {
          return NULL;
        }

        CopyMem (String, CachedString, StringSize);
        return String;
      }
    }
  }

  //
  // Retrieve the size of the string in the string package for the BestLanguage
  //
  StringSize = 0;
  Status     = mRedfishPlatformConfigPrivate->HiiString->GetString (
                                                           mRedfishPlatformConfigPrivate->HiiString,
                                                           Language,
                                                           HiiHandle,
                                                           StringId,
                                                           &TempString,
                                                           &StringSize,
                                                           NULL
                                                           );
  //
  // If GetString() returns EFI_SUCCESS for a zero size,
  // then there are no supported languages registered for HiiHandle.  If GetString()
  // returns an error other than EFI_BUFFER_TOO_SMALL, then HiiHandle is not present
  // in the HII Database
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return NULL;
  }

  //
  // Allocate a buffer for the return string
  //
  String = AllocateZeroPool (StringSize);
  if (String == NULL) {
    return NULL;
  }

  //
  // Retrieve the string from the string package
  //
  Status = mRedfishPlatformConfigPrivate->HiiString->GetString (
                                                       mRedfishPlatformConfigPrivate->HiiString,
                                                       Language,
                                                       HiiHandle,
                                                       StringId,
                                                       String,
                                                       &StringSize,
                                                       NULL
                                                       );
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the supported languages can not be retrieved.
    //
    FreePool (String);
    String = NULL;
  }

  //
  // Return the Null-terminated Unicode string
  //
  return String;
}

/**
  Retrieves a unicode string from a string package in a given language. The
  returned string is allocated using AllocatePool().  The caller is responsible
  for freeing the allocated buffer using FreePool().  Exact-language hits are
  served from the per-formset string cache; see HiiGetRedfishStringUncached()
  for strings that may change at runtime.

  If HiiHandle is NULL, then ASSERT().
  If StringId is 0, then ASSET.

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  Language          The specified configure language to get string.
  @param[in]  StringId          The identifier of the string to retrieved from the string
                                package associated with HiiHandle.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
HiiGetRedfishString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN CHAR8           *Language,
  IN EFI_STRING_ID   StringId
  )
{
  return HiiGetRedfishStringEx (HiiHandle, Language, StringId, TRUE);
}

/**
  Same as HiiGetRedfishString() but always reads through
  EFI_HII_STRING_PROTOCOL, bypassing the per-formset string cache.

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  Language          The specified configure language to get string.
  @param[in]  StringId          The identifier of the string to retrieve.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
HiiGetRedfishStringUncached (
  IN EFI_HII_HANDLE  HiiHandle,
  IN CHAR8           *Language,
  IN EFI_STRING_ID   StringId
  )
{
  return HiiGetRedfishStringEx (HiiHandle, Language, StringId, FALSE);
}

/**
  ASCII variant of HiiGetRedfishStringUncached().

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  Language          The specified configure language to get string.
  @param[in]  StringId          The identifier of the string to retrieve.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
CHAR8 *
HiiGetRedfishAsciiStringUncached (
  IN EFI_HII_HANDLE  HiiHandle,
  IN CHAR8           *Language,
  IN EFI_STRING_ID   StringId
  )
{
  EFI_STRING  HiiString;
  CHAR8       *AsciiString;

  HiiString = HiiGetRedfishStringUncached (HiiHandle, Language, StringId);
  if (HiiString == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Can not find string ID: 0x%x with %a\n", __func__, StringId, Language));
    return NULL;
  }

  AsciiString = StrToAsciiStr (HiiString);
  FreePool (HiiString);

  return AsciiString;
}

/**
  English ASCII variant of HiiGetRedfishStringUncached().

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  StringId          The identifier of the string to retrieve.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
CHAR8 *
HiiGetEnglishAsciiStringUncached (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  )
{
  return HiiGetRedfishAsciiStringUncached (HiiHandle, ENGLISH_LANGUAGE_CODE, StringId);
}

/**
  Retrieves a ASCII string from a string package in a given language. The
  returned string is allocated using AllocatePool().  The caller is responsible
  for freeing the allocated buffer using FreePool().

  If HiiHandle is NULL, then ASSERT().
  If StringId is 0, then ASSET.

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  Language          The specified configure language to get string.
  @param[in]  StringId          The identifier of the string to retrieved from the string
                                package associated with HiiHandle.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
CHAR8 *
HiiGetRedfishAsciiString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN CHAR8           *Language,
  IN EFI_STRING_ID   StringId
  )
{
  EFI_STRING  HiiString;
  CHAR8       *AsciiString;

  HiiString = HiiGetRedfishString (HiiHandle, Language, StringId);
  if (HiiString == NULL) {
    return NULL;
  }

  AsciiString = StrToAsciiStr (HiiString);
  FreePool (HiiString);

  return AsciiString;
}

/**
  Get ASCII string from HII database in English language. The returned string is allocated
  using AllocatePool(). The caller is responsible for freeing the allocated buffer using
  FreePool().

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  StringId          The identifier of the string to retrieved from the string
                                package associated with HiiHandle.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
CHAR8 *
HiiGetEnglishAsciiString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  )
{
  EFI_STRING  HiiString;
  CHAR8       *AsciiString;

  HiiString = HiiGetRedfishString (HiiHandle, ENGLISH_LANGUAGE_CODE, StringId);
  if (HiiString == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Can not find string ID: 0x%x with %a\n", __func__, StringId, ENGLISH_LANGUAGE_CODE));
    return NULL;
  }

  AsciiString = StrToAsciiStr (HiiString);
  FreePool (HiiString);

  return AsciiString;
}

/**
  Check and see if this is supported schema or not.

  @param[in]  SupportedSchema   The list of supported schema.
  @param[in]  Schema            Schema string to be checked.

  @retval BOOLEAN               TRUE if this is supported schema. FALSE otherwise.

**/
BOOLEAN
CheckSupportedSchema (
  IN REDFISH_PLATFORM_CONFIG_SCHEMA  *SupportedSchema,
  IN CHAR8                           *Schema
  )
{
  UINTN  Index;

  if ((SupportedSchema == NULL) || IS_EMPTY_STRING (Schema)) {
    return FALSE;
  }

  if (SupportedSchema->Count == 0) {
    return FALSE;
  }

  for (Index = 0; Index < SupportedSchema->Count; Index++) {
    if (AsciiStrCmp (SupportedSchema->SchemaList[Index], Schema) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  Get the list of supported schema from the given HII handle.

  @param[in]  HiiHandle         HII handle instance.
  @param[out] SupportedSchema   Supported schema on this HII handle.

  @retval EFI_SUCCESS           Schema list is returned.
  @retval EFI_INVALID_PARAMETER HiiHandle is NULL or SupportedSchema is NULL.
  @retval EFI_NOT_FOUND         No supported schema found.
  @retval EFI_OUT_OF_RESOURCES  System is out of memory.

**/
EFI_STATUS
GetSupportedSchema (
  IN  EFI_HII_HANDLE                  HiiHandle,
  OUT REDFISH_PLATFORM_CONFIG_SCHEMA  *SupportedSchema
  )
{
  CHAR8  *SupportedLanguages;
  UINTN  Index;
  UINTN  LangIndex;
  UINTN  Count;
  UINTN  StrSize;
  UINTN  ListIndex;

  if ((HiiHandle == NULL) || (SupportedSchema == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SupportedSchema->Count = 0;

  SupportedLanguages = HiiGetSupportedLanguages (HiiHandle);
  if (SupportedLanguages == NULL) {
    return EFI_NOT_FOUND;
  }

  Index     = 0;
  LangIndex = 0;
  Count     = 0;
  while (TRUE) {
    if ((SupportedLanguages[Index] == ';') || (SupportedLanguages[Index] == '\0')) {
      if (AsciiStrnCmp (&SupportedLanguages[LangIndex], X_UEFI_SCHEMA_PREFIX, AsciiStrLen (X_UEFI_SCHEMA_PREFIX)) == 0) {
        ++Count;
      }

      LangIndex = Index + 1;
    }

    if (SupportedLanguages[Index] == '\0') {
      break;
    }

    ++Index;
  }

  if (Count == 0) {
    FreePool (SupportedLanguages);

    return EFI_NOT_FOUND;
  }

  SupportedSchema->Count      = Count;
  SupportedSchema->SchemaList = AllocatePool (sizeof (CHAR8 *) * Count);
  if (SupportedSchema->SchemaList == NULL) {
    FreePool (SupportedLanguages);

    return EFI_OUT_OF_RESOURCES;
  }

  Index     = 0;
  LangIndex = 0;
  ListIndex = 0;
  while (TRUE) {
    if ((SupportedLanguages[Index] == ';') || (SupportedLanguages[Index] == '\0')) {
      if (AsciiStrnCmp (&SupportedLanguages[LangIndex], X_UEFI_SCHEMA_PREFIX, AsciiStrLen (X_UEFI_SCHEMA_PREFIX)) == 0) {
        StrSize                                         = Index - LangIndex;
        SupportedSchema->SchemaList[ListIndex]          = AllocateCopyPool ((StrSize + 1), &SupportedLanguages[LangIndex]);
        SupportedSchema->SchemaList[ListIndex][StrSize] = '\0';
        ++ListIndex;
      }

      LangIndex = Index + 1;
    }

    if (SupportedLanguages[Index] == '\0') {
      break;
    }

    ++Index;
  }

  FreePool (SupportedLanguages);

  return EFI_SUCCESS;
}

/**
  Search and find statement private instance by given regular expression pattern
  which describes the Configure Language.

  @param[in]  RegularExpressionProtocol   Regular express protocol.
  @param[in]  FormsetList                 Form-set list to search.
  @param[in]  Schema                      Schema to be matched.
  @param[in]  Pattern                     Regular expression pattern.
  @param[out] StatementList               Statement list that match above pattern.

  @retval EFI_SUCCESS             Statement list is returned.
  @retval EFI_INVALID_PARAMETER   Input parameter is NULL.
  @retval EFI_NOT_READY           Regular express protocol is NULL.
  @retval EFI_NOT_FOUND           No statement is found.
  @retval EFI_OUT_OF_RESOURCES    System is out of memory.

**/
EFI_STATUS
GetStatementPrivateByConfigureLangRegex (
  IN  EFI_REGULAR_EXPRESSION_PROTOCOL                 *RegularExpressionProtocol,
  IN  LIST_ENTRY                                      *FormsetList,
  IN  CHAR8                                           *Schema,
  IN  EFI_STRING                                      Pattern,
  OUT REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST  *StatementList
  )
{
  LIST_ENTRY                                     *HiiFormsetLink;
  LIST_ENTRY                                     *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE       *HiiFormsetPrivate;
  LIST_ENTRY                                     *HiiFormLink;
  LIST_ENTRY                                     *HiiNextFormLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE           *HiiFormPrivate;
  LIST_ENTRY                                     *HiiStatementLink;
  LIST_ENTRY                                     *HiiNextStatementLink;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE      *HiiStatementPrivate;
  EFI_STRING                                     TmpString;
  UINTN                                          CaptureCount;
  BOOLEAN                                        IsMatch;
  EFI_STATUS                                     Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF  *StatementRef;

  if ((FormsetList == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Pattern) || (StatementList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (RegularExpressionProtocol == NULL) {
    return EFI_NOT_READY;
  }

  StatementList->Count = 0;
  InitializeListHead (&StatementList->StatementList);

  if (IsListEmpty (FormsetList)) {
    return EFI_NOT_FOUND;
  }

  HiiFormsetLink = GetFirstNode (FormsetList);
  while (!IsNull (FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    //
    // Performance check.
    // If there is no desired Redfish schema found, skip this formset.
    //
    if (!CheckSupportedSchema (&HiiFormsetPrivate->SupportedSchema, Schema)) {
      HiiFormsetLink = HiiFormsetNextLink;
      continue;
    }

    HiiFormLink = GetFirstNode (&HiiFormsetPrivate->HiiFormList);
    while (!IsNull (&HiiFormsetPrivate->HiiFormList, HiiFormLink)) {
      HiiNextFormLink = GetNextNode (&HiiFormsetPrivate->HiiFormList, HiiFormLink);
      HiiFormPrivate  = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (HiiFormLink);

      HiiStatementLink = GetFirstNode (&HiiFormPrivate->StatementList);
      while (!IsNull (&HiiFormPrivate->StatementList, HiiStatementLink)) {
        HiiNextStatementLink = GetNextNode (&HiiFormPrivate->StatementList, HiiStatementLink);
        HiiStatementPrivate  = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (HiiStatementLink);

        if ((HiiStatementPrivate->Description != 0) &&
            (RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_ALLOW_SUPPRESSED) || !HiiStatementPrivate->Suppressed))
        {
          TmpString = HiiStatementPrivate->XuefiRedfishStr;
          if (TmpString != NULL) {
            Status = RegularExpressionProtocol->MatchString (
                                                  RegularExpressionProtocol,
                                                  TmpString,
                                                  Pattern,
                                                  &gEfiRegexSyntaxTypePerlGuid,
                                                  &IsMatch,
                                                  NULL,
                                                  &CaptureCount
                                                  );
            if (EFI_ERROR (Status)) {
              DEBUG ((DEBUG_ERROR, "%a: MatchString \"%s\" failed: %r\n", __func__, Pattern, Status));
              ASSERT (FALSE);
              return Status;
            }

            //
            // Found
            //
            if (IsMatch) {
              StatementRef = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF));
              if (StatementRef == NULL) {
                return EFI_OUT_OF_RESOURCES;
              }

              StatementRef->Statement = HiiStatementPrivate;
              InsertTailList (&StatementList->StatementList, &StatementRef->Link);
              ++StatementList->Count;
            }
          } else {
            if (!RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_BUILD_MENU_PATH)) {
              DEBUG ((DEBUG_ERROR, "%a: HiiStatementPrivate->XuefiRedfishStr is NULL, x-UEFI-string has something wrong.\n", __func__));
              ASSERT (FALSE);
            }
          }
        }

        HiiStatementLink = HiiNextStatementLink;
      }

      HiiFormLink = HiiNextFormLink;
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  return EFI_SUCCESS;
}

/**
  Get statement private instance by the given configure language.

  @param[in]  FormsetList                 Form-set list to search.
  @param[in]  Schema                      Schema to be matched.
  @param[in]  ConfigureLang               Configure language.

  @retval REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *   Pointer to statement private instance.

**/
REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *
GetStatementPrivateByConfigureLang (
  IN  LIST_ENTRY  *FormsetList,
  IN  CHAR8       *Schema,
  IN  EFI_STRING  ConfigureLang
  )
{
  LIST_ENTRY                                 *HiiFormsetLink;
  LIST_ENTRY                                 *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE   *HiiFormsetPrivate;
  LIST_ENTRY                                 *HiiFormLink;
  LIST_ENTRY                                 *HiiNextFormLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE       *HiiFormPrivate;
  LIST_ENTRY                                 *HiiStatementLink;
  LIST_ENTRY                                 *HiiNextStatementLink;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *HiiStatementPrivate;
  EFI_STRING                                 TmpString;
  UINTN                                      Index;

  if ((FormsetList == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (ConfigureLang)) {
    return NULL;
  }

  if (IsListEmpty (FormsetList)) {
    return NULL;
  }

  Index          = 0;
  HiiFormsetLink = GetFirstNode (FormsetList);
  while (!IsNull (FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    //
    // Performance check.
    // If there is no desired Redfish schema found, skip this formset.
    //
    if (!CheckSupportedSchema (&HiiFormsetPrivate->SupportedSchema, Schema)) {
      HiiFormsetLink = HiiFormsetNextLink;
      continue;
    }

    //
    // Fast path: search the per-formset index built in LoadFormset().  It
    // returns the same statement the linear walk below would (first visible in
    // load order).  If the index could not be built (allocation failure at
    // load), fall through to the linear walk for this formset only.
    //
    if (HiiFormsetPrivate->ConfigLangIndexBuilt) {
      HiiStatementPrivate = SearchConfigureLangIndex (HiiFormsetPrivate, ConfigureLang);
      if (HiiStatementPrivate != NULL) {
        DEBUG_REDFISH_THIS_MODULE (
          REDFISH_PLATFORM_CONFIG_DEBUG_CONFIG_LANG_SEARCH,
          "%a: indexed hit %s in QID: 0x%x formset: %g\n",
          __func__,
          ConfigureLang,
          HiiStatementPrivate->QuestionId,
          &HiiFormsetPrivate->Guid
          );
        return HiiStatementPrivate;
      }

      HiiFormsetLink = HiiFormsetNextLink;
      continue;
    }

    HiiFormLink = GetFirstNode (&HiiFormsetPrivate->HiiFormList);
    while (!IsNull (&HiiFormsetPrivate->HiiFormList, HiiFormLink)) {
      HiiNextFormLink = GetNextNode (&HiiFormsetPrivate->HiiFormList, HiiFormLink);
      HiiFormPrivate  = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (HiiFormLink);

      HiiStatementLink = GetFirstNode (&HiiFormPrivate->StatementList);
      while (!IsNull (&HiiFormPrivate->StatementList, HiiStatementLink)) {
        HiiNextStatementLink = GetNextNode (&HiiFormPrivate->StatementList, HiiStatementLink);
        HiiStatementPrivate  = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (HiiStatementLink);

        if ((HiiStatementPrivate->Description != 0) &&
            (RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_ALLOW_SUPPRESSED) || !HiiStatementPrivate->Suppressed))
        {
          TmpString = HiiStatementPrivate->XuefiRedfishStr;
          if (TmpString != NULL) {
            Index++;
            DEBUG_REDFISH_THIS_MODULE (
              REDFISH_PLATFORM_CONFIG_DEBUG_CONFIG_LANG_SEARCH,
              "%a: [%d] check %s in QID: 0x%x form: 0x%x formset: %g\n",
              __func__,
              Index,
              ConfigureLang,
              HiiStatementPrivate->QuestionId,
              HiiFormPrivate->Id,
              &HiiFormsetPrivate->Guid
              );
            if (HiiStrCmp (TmpString, ConfigureLang) == 0) {
              return HiiStatementPrivate;
            }
          } else {
            if (!RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_BUILD_MENU_PATH)) {
              DEBUG ((DEBUG_ERROR, "%a: HiiStatementPrivate->XuefiRedfishStr is NULL, x-UEFI-string has something wrong.\n", __func__));
              ASSERT (FALSE);
            }
          }
        }

        HiiStatementLink = HiiNextStatementLink;
      }

      HiiFormLink = HiiNextFormLink;
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  return NULL;
}

/**
  Get form-set private instance by the given HII handle.

  @param[in]  HiiHandle       HII handle instance.
  @param[in]  FormsetList     Form-set list to search.

  @retval REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE *   Pointer to form-set private instance.

**/
REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE *
GetFormsetPrivateByHiiHandle (
  IN  EFI_HII_HANDLE  HiiHandle,
  IN  LIST_ENTRY      *FormsetList
  )
{
  LIST_ENTRY                                *HiiFormsetLink;
  LIST_ENTRY                                *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *HiiFormsetPrivate;

  if ((HiiHandle == NULL) || (FormsetList == NULL)) {
    return NULL;
  }

  if (IsListEmpty (FormsetList)) {
    return NULL;
  }

  HiiFormsetLink = GetFirstNode (FormsetList);
  while (!IsNull (FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    if (HiiFormsetPrivate->HiiHandle == HiiHandle) {
      return HiiFormsetPrivate;
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  return NULL;
}

/**
  Export the HII package list of the formset into FormsetPrivate->HiiPackageListHeader
  if it has not been exported yet.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.

  @retval EFI_SUCCESS         Package list is available in FormsetPrivate->HiiPackageListHeader.
  @retval Others              Export failed; HiiPackageListHeader is NULL.

**/
EFI_STATUS
ExportFormsetPackageList (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  EFI_STATUS  Status;
  UINTN       BufferSize;

  if (FormsetPrivate == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (FormsetPrivate->HiiPackageListHeader != NULL) {
    return EFI_SUCCESS;
  }

  if (mRedfishPlatformConfigPrivate->HiiDatabase == NULL) {
    return EFI_NOT_READY;
  }

  BufferSize = 0;
  Status     = mRedfishPlatformConfigPrivate->HiiDatabase->ExportPackageLists (
                                                             mRedfishPlatformConfigPrivate->HiiDatabase,
                                                             FormsetPrivate->HiiHandle,
                                                             &BufferSize,
                                                             NULL
                                                             );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to export package list: %r\n", __func__, Status));
    return EFI_ERROR (Status) ? Status : EFI_DEVICE_ERROR;
  }

  FormsetPrivate->HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *)AllocateZeroPool (BufferSize);
  if (FormsetPrivate->HiiPackageListHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory for the exported package list.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  Status = mRedfishPlatformConfigPrivate->HiiDatabase->ExportPackageLists (
                                                         mRedfishPlatformConfigPrivate->HiiDatabase,
                                                         FormsetPrivate->HiiHandle,
                                                         &BufferSize,
                                                         FormsetPrivate->HiiPackageListHeader
                                                         );
  if (EFI_ERROR (Status)) {
    FreePool (FormsetPrivate->HiiPackageListHeader);
    FormsetPrivate->HiiPackageListHeader = NULL;
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Record one UCS-2 string pointer in the cache array if it fits.

  @param[in]  Strings   Output array (may be NULL during the counting pass).
  @param[in]  Capacity  Number of entries in Strings.
  @param[in]  StringId  String ID of the text.
  @param[in]  Text      Pointer to the UCS-2 text inside the package.

**/
STATIC
VOID
RecordCachedString (
  IN  CHAR16         **Strings OPTIONAL,
  IN  UINTN          Capacity,
  IN  EFI_STRING_ID  StringId,
  IN  UINT8          *Text
  )
{
  if ((Strings != NULL) && ((UINTN)StringId < Capacity)) {
    Strings[StringId] = (CHAR16 *)Text;
  }
}

/**
  Walk the string blocks of one HII string package.  When Strings is NULL the
  walk only counts, returning the highest string ID seen plus one in *Count.
  When Strings is provided (sized *Count), every UCS-2 string block is recorded
  as Strings[StringId] = pointer to its text inside the package.  SCSU-encoded
  blocks are not recorded (callers fall back to GetString for those).

  @param[in]      HiiStringPackageHeader  HII string package to walk.
  @param[in]      Strings                 Optional output array indexed by string ID.
  @param[in,out]  Count                   In: capacity of Strings when non-NULL.
                                          Out: highest string ID + 1 observed.

**/
STATIC
VOID
WalkStringPackageForCache (
  IN      EFI_HII_STRING_PACKAGE_HDR  *HiiStringPackageHeader,
  IN      CHAR16                      **Strings OPTIONAL,
  IN OUT  UINTN                       *Count
  )
{
  UINT8                    *BlockHdr;
  UINT8                    *StringBlockInfo;
  UINT8                    *StringTextPtr;
  UINTN                    BlockSize;
  UINTN                    Offset;
  UINTN                    Index;
  UINTN                    Capacity;
  UINTN                    PackageEnd;
  EFI_STRING_ID            CurrentStringId;
  EFI_STRING_ID            DuplicateId;
  UINT16                   StringCount;
  UINT16                   SkipCount;
  UINT8                    Length8;
  EFI_HII_SIBT_EXT2_BLOCK  Ext2;
  UINT32                   Length32;

  Capacity        = (Strings != NULL) ? *Count : 0;
  StringBlockInfo = (UINT8 *)((UINTN)HiiStringPackageHeader + HiiStringPackageHeader->StringInfoOffset);
  PackageEnd      = (UINTN)HiiStringPackageHeader + HiiStringPackageHeader->Header.Length;
  BlockHdr        = StringBlockInfo;
  BlockSize       = 0;
  CurrentStringId = 1;

  //
  // Never read past the package even if the END block is missing.
  //
  while (((UINTN)BlockHdr < PackageEnd) && (*BlockHdr != EFI_HII_SIBT_END)) {
    switch (*BlockHdr) {
      case EFI_HII_SIBT_STRING_SCSU:
        Offset        = sizeof (EFI_HII_STRING_BLOCK);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset + AsciiStrSize ((CHAR8 *)StringTextPtr);
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRING_SCSU_FONT:
        Offset        = sizeof (EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK) - sizeof (UINT8);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset + AsciiStrSize ((CHAR8 *)StringTextPtr);
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRINGS_SCSU:
        CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
        StringTextPtr = (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_BLOCK) - sizeof (UINT8));
        BlockSize    += StringTextPtr - BlockHdr;
        for (Index = 0; Index < StringCount; Index++) {
          BlockSize    += AsciiStrSize ((CHAR8 *)StringTextPtr);
          StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_STRINGS_SCSU_FONT:
        CopyMem (&StringCount, (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)), sizeof (UINT16));
        StringTextPtr = (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK) - sizeof (UINT8));
        BlockSize    += StringTextPtr - BlockHdr;
        for (Index = 0; Index < StringCount; Index++) {
          BlockSize    += AsciiStrSize ((CHAR8 *)StringTextPtr);
          StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_STRING_UCS2:
        Offset        = sizeof (EFI_HII_STRING_BLOCK);
        StringTextPtr = BlockHdr + Offset;
        RecordCachedString (Strings, Capacity, CurrentStringId, StringTextPtr);
        BlockSize += (Offset + HiiStrSize ((CHAR16 *)StringTextPtr));
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRING_UCS2_FONT:
        Offset        = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK) - sizeof (CHAR16);
        StringTextPtr = BlockHdr + Offset;
        RecordCachedString (Strings, Capacity, CurrentStringId, StringTextPtr);
        BlockSize += (Offset + HiiStrSize ((CHAR16 *)StringTextPtr));
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRINGS_UCS2:
        Offset        = sizeof (EFI_HII_SIBT_STRINGS_UCS2_BLOCK) - sizeof (CHAR16);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset;
        CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
        for (Index = 0; Index < StringCount; Index++) {
          RecordCachedString (Strings, Capacity, CurrentStringId, StringTextPtr);
          BlockSize    += HiiStrSize ((CHAR16 *)StringTextPtr);
          StringTextPtr = StringTextPtr + HiiStrSize ((CHAR16 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_STRINGS_UCS2_FONT:
        Offset        = sizeof (EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK) - sizeof (CHAR16);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset;
        CopyMem (&StringCount, (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)), sizeof (UINT16));
        for (Index = 0; Index < StringCount; Index++) {
          RecordCachedString (Strings, Capacity, CurrentStringId, StringTextPtr);
          BlockSize    += HiiStrSize ((CHAR16 *)StringTextPtr);
          StringTextPtr = StringTextPtr + HiiStrSize ((CHAR16 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_DUPLICATE:
        //
        // Resolve to the referenced string if it was already recorded.
        //
        CopyMem (&DuplicateId, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (EFI_STRING_ID));
        if ((Strings != NULL) && ((UINTN)DuplicateId < Capacity) && ((UINTN)CurrentStringId < Capacity)) {
          Strings[CurrentStringId] = Strings[DuplicateId];
        }

        BlockSize += sizeof (EFI_HII_SIBT_DUPLICATE_BLOCK);
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_SKIP1:
        SkipCount       = (UINT16)(*(UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK)));
        CurrentStringId = (UINT16)(CurrentStringId + SkipCount);
        BlockSize      += sizeof (EFI_HII_SIBT_SKIP1_BLOCK);
        break;

      case EFI_HII_SIBT_SKIP2:
        CopyMem (&SkipCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
        CurrentStringId = (UINT16)(CurrentStringId + SkipCount);
        BlockSize      += sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
        break;

      case EFI_HII_SIBT_EXT1:
        CopyMem (&Length8, (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)), sizeof (UINT8));
        if (Length8 == 0) {
          *Count = CurrentStringId;
          return;
        }

        BlockSize += Length8;
        break;

      case EFI_HII_SIBT_EXT2:
        CopyMem (&Ext2, BlockHdr, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
        if (Ext2.Length == 0) {
          *Count = CurrentStringId;
          return;
        }

        BlockSize += Ext2.Length;
        break;

      case EFI_HII_SIBT_EXT4:
        CopyMem (&Length32, (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)), sizeof (UINT32));
        if (Length32 == 0) {
          *Count = CurrentStringId;
          return;
        }

        BlockSize += Length32;
        break;

      default:
        //
        // Unknown block type: stop here rather than mis-parse the rest.
        //
        *Count = CurrentStringId;
        return;
    }

    BlockHdr = (UINT8 *)(StringBlockInfo + BlockSize);
  }

  *Count = CurrentStringId;
}

/**
  Return the string cache for the given language, building it on first use from
  the formset's exported HII package list.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.
  @param[in]  Language        HII language of the string package to index.

  @retval REDFISH_HII_STRING_CACHE *  Cache for this language. Count is 0 when the
                                      language is not present in the package list.
  @retval NULL                        Out of resources or package list unavailable.

**/
REDFISH_HII_STRING_CACHE *
GetOrBuildHiiStringCache (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN  CHAR8                                     *Language
  )
{
  EFI_STATUS                  Status;
  LIST_ENTRY                  *Link;
  REDFISH_HII_STRING_CACHE    *Cache;
  EFI_HII_PACKAGE_HEADER      *PackageHeader;
  EFI_HII_STRING_PACKAGE_HDR  *HiiStringPackageHeader;
  UINTN                       EndingPackageAddress;
  UINTN                       Count;

  if ((FormsetPrivate == NULL) || IS_EMPTY_STRING (Language)) {
    return NULL;
  }

  //
  // Existing cache for this language?
  //
  Link = GetFirstNode (&FormsetPrivate->HiiStringCacheList);
  while (!IsNull (&FormsetPrivate->HiiStringCacheList, Link)) {
    Cache = REDFISH_HII_STRING_CACHE_FROM_LINK (Link);
    if (AsciiStriCmp (Cache->Language, Language) == 0) {
      return Cache;
    }

    Link = GetNextNode (&FormsetPrivate->HiiStringCacheList, Link);
  }

  Status = ExportFormsetPackageList (FormsetPrivate);
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  Cache = AllocateZeroPool (sizeof (REDFISH_HII_STRING_CACHE));
  if (Cache == NULL) {
    return NULL;
  }

  Cache->Language = AllocateCopyPool (AsciiStrSize (Language), Language);
  if (Cache->Language == NULL) {
    FreePool (Cache);
    return NULL;
  }

  //
  // Find the string package for this language.  A negative result is kept in
  // the list too (Count == 0) so the packages are not re-walked per request.
  //
  EndingPackageAddress = (UINTN)FormsetPrivate->HiiPackageListHeader + FormsetPrivate->HiiPackageListHeader->PackageLength;
  PackageHeader        = (EFI_HII_PACKAGE_HEADER *)(FormsetPrivate->HiiPackageListHeader + 1);
  while ((UINTN)PackageHeader < EndingPackageAddress) {
    if (PackageHeader->Type == EFI_HII_PACKAGE_STRINGS) {
      HiiStringPackageHeader = (EFI_HII_STRING_PACKAGE_HDR *)PackageHeader;
      if (AsciiStriCmp (HiiStringPackageHeader->Language, Language) == 0) {
        Count = 0;
        WalkStringPackageForCache (HiiStringPackageHeader, NULL, &Count);
        if (Count > 0) {
          Cache->Strings = AllocateZeroPool (Count * sizeof (CHAR16 *));
          if (Cache->Strings == NULL) {
            FreePool (Cache->Language);
            FreePool (Cache);
            return NULL;
          }

          Cache->Count = Count;
          WalkStringPackageForCache (HiiStringPackageHeader, Cache->Strings, &Count);
        }

        break;
      }
    }

    PackageHeader = (EFI_HII_PACKAGE_HEADER *)((UINTN)PackageHeader + PackageHeader->Length);
  }

  DEBUG ((
    DEBUG_REDFISH_PLATFORM_CONFIG,
    "%a: formset %g language %a: %u string IDs cached\n",
    __func__,
    &FormsetPrivate->Guid,
    Language,
    (UINT32)Cache->Count
    ));

  InsertTailList (&FormsetPrivate->HiiStringCacheList, &Cache->Link);
  return Cache;
}

/**
  Release every per-language string cache attached to the formset.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.

**/
VOID
ReleaseHiiStringCacheList (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  LIST_ENTRY                *Link;
  LIST_ENTRY                *NextLink;
  REDFISH_HII_STRING_CACHE  *Cache;

  if (FormsetPrivate == NULL) {
    return;
  }

  Link = GetFirstNode (&FormsetPrivate->HiiStringCacheList);
  while (!IsNull (&FormsetPrivate->HiiStringCacheList, Link)) {
    NextLink = GetNextNode (&FormsetPrivate->HiiStringCacheList, Link);
    Cache    = REDFISH_HII_STRING_CACHE_FROM_LINK (Link);
    RemoveEntryList (&Cache->Link);
    if (Cache->Strings != NULL) {
      FreePool (Cache->Strings);
    }

    if (Cache->Language != NULL) {
      FreePool (Cache->Language);
    }

    FreePool (Cache);
    Link = NextLink;
  }
}

/**
  Order two configure-language index entries: by x-UEFI-redfish string, then by
  load order so equal keys keep the order the statements were parsed in.

  @param[in]  Entry1  First entry.
  @param[in]  Entry2  Second entry.

  @retval <0, 0, >0    Entry1 sorts before, equal to, after Entry2.

**/
STATIC
INTN
CompareConfigureLangIndexEntry (
  IN CONST REDFISH_CONFIG_LANG_INDEX_ENTRY  *Entry1,
  IN CONST REDFISH_CONFIG_LANG_INDEX_ENTRY  *Entry2
  )
{
  INTN  Result;

  Result = HiiStrCmp (Entry1->ConfigureLang, Entry2->ConfigureLang);
  if (Result != 0) {
    return Result;
  }

  if (Entry1->Order < Entry2->Order) {
    return -1;
  }

  return (Entry1->Order > Entry2->Order) ? 1 : 0;
}

/**
  In-place heapsort of the configure-language index.

  BaseLib QuickSort() recurses once per partition with a last-element pivot, so
  its depth is O(n) for already sorted or duplicate-heavy input.  Generated VFR
  can easily present thousands of attributes in alphabetical order, which would
  exhaust the DXE stack.  Heapsort is O(n log n) with O(1) extra space and no
  recursion; the Order tie-break in the comparator makes the result
  deterministic despite heapsort not being stable.

  @param[in,out]  Entries  Array to sort.
  @param[in]      Count    Number of entries.

**/
STATIC
VOID
HeapSortConfigureLangIndex (
  IN OUT REDFISH_CONFIG_LANG_INDEX_ENTRY  *Entries,
  IN     UINTN                            Count
  )
{
  UINTN                            Start;
  UINTN                            End;
  UINTN                            Root;
  UINTN                            Child;
  REDFISH_CONFIG_LANG_INDEX_ENTRY  Temp;

  if (Count < 2) {
    return;
  }

  //
  // Build a max-heap, then repeatedly move the largest element to the end.
  //
  for (Start = Count / 2; Start > 0; Start--) {
    Root = Start - 1;
    while ((Child = 2 * Root + 1) < Count) {
      if (((Child + 1) < Count) && (CompareConfigureLangIndexEntry (&Entries[Child], &Entries[Child + 1]) < 0)) {
        Child++;
      }

      if (CompareConfigureLangIndexEntry (&Entries[Root], &Entries[Child]) >= 0) {
        break;
      }

      Temp           = Entries[Root];
      Entries[Root]  = Entries[Child];
      Entries[Child] = Temp;
      Root           = Child;
    }
  }

  for (End = Count - 1; End > 0; End--) {
    Temp         = Entries[0];
    Entries[0]   = Entries[End];
    Entries[End] = Temp;

    Root = 0;
    while ((Child = 2 * Root + 1) < End) {
      if (((Child + 1) < End) && (CompareConfigureLangIndexEntry (&Entries[Child], &Entries[Child + 1]) < 0)) {
        Child++;
      }

      if (CompareConfigureLangIndexEntry (&Entries[Root], &Entries[Child]) >= 0) {
        break;
      }

      Temp           = Entries[Root];
      Entries[Root]  = Entries[Child];
      Entries[Child] = Temp;
      Root           = Child;
    }
  }
}

/**
  Build the sorted configure-language index over all statements of the formset
  that carry an x-UEFI-redfish string. Must be called after all forms and
  statements have been attached to FormsetPrivate.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.

  @retval EFI_SUCCESS           Index built (possibly empty).
  @retval EFI_OUT_OF_RESOURCES  Out of memory; lookups fall back to a linear search.

**/
EFI_STATUS
BuildConfigureLangIndex (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  LIST_ENTRY                                 *FormLink;
  LIST_ENTRY                                 *StatementLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE       *FormPrivate;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *StatementPrivate;
  UINTN                                      Count;
  UINTN                                      Index;

  if (FormsetPrivate == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ReleaseConfigureLangIndex (FormsetPrivate);

  //
  // Count statements that have a configure language.
  //
  Count    = 0;
  FormLink = GetFirstNode (&FormsetPrivate->HiiFormList);
  while (!IsNull (&FormsetPrivate->HiiFormList, FormLink)) {
    FormPrivate   = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (FormLink);
    StatementLink = GetFirstNode (&FormPrivate->StatementList);
    while (!IsNull (&FormPrivate->StatementList, StatementLink)) {
      StatementPrivate = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (StatementLink);
      if (StatementPrivate->XuefiRedfishStr != NULL) {
        Count++;
      }

      StatementLink = GetNextNode (&FormPrivate->StatementList, StatementLink);
    }

    FormLink = GetNextNode (&FormsetPrivate->HiiFormList, FormLink);
  }

  if (Count > 0) {
    FormsetPrivate->ConfigLangIndex = AllocateZeroPool (Count * sizeof (REDFISH_CONFIG_LANG_INDEX_ENTRY));
    if (FormsetPrivate->ConfigLangIndex == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Index    = 0;
    FormLink = GetFirstNode (&FormsetPrivate->HiiFormList);
    while (!IsNull (&FormsetPrivate->HiiFormList, FormLink)) {
      FormPrivate   = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (FormLink);
      StatementLink = GetFirstNode (&FormPrivate->StatementList);
      while (!IsNull (&FormPrivate->StatementList, StatementLink)) {
        StatementPrivate = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (StatementLink);
        if ((StatementPrivate->XuefiRedfishStr != NULL) && (Index < Count)) {
          FormsetPrivate->ConfigLangIndex[Index].ConfigureLang = StatementPrivate->XuefiRedfishStr;
          FormsetPrivate->ConfigLangIndex[Index].Order         = Index;
          FormsetPrivate->ConfigLangIndex[Index].Statement     = StatementPrivate;
          Index++;
        }

        StatementLink = GetNextNode (&FormPrivate->StatementList, StatementLink);
      }

      FormLink = GetNextNode (&FormsetPrivate->HiiFormList, FormLink);
    }

    FormsetPrivate->ConfigLangIndexCount = Index;
    HeapSortConfigureLangIndex (FormsetPrivate->ConfigLangIndex, FormsetPrivate->ConfigLangIndexCount);
  }

  //
  // An empty index is still a valid index: lookups on this formset can stop
  // without walking its statements.
  //
  FormsetPrivate->ConfigLangIndexBuilt = TRUE;

  DEBUG ((
    DEBUG_REDFISH_PLATFORM_CONFIG,
    "%a: formset %g: %u configure languages indexed\n",
    __func__,
    &FormsetPrivate->Guid,
    (UINT32)FormsetPrivate->ConfigLangIndexCount
    ));

  return EFI_SUCCESS;
}

/**
  Release the configure-language index of the formset.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.

**/
VOID
ReleaseConfigureLangIndex (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  if (FormsetPrivate == NULL) {
    return;
  }

  if (FormsetPrivate->ConfigLangIndex != NULL) {
    FreePool (FormsetPrivate->ConfigLangIndex);
    FormsetPrivate->ConfigLangIndex = NULL;
  }

  FormsetPrivate->ConfigLangIndexCount = 0;
  FormsetPrivate->ConfigLangIndexBuilt = FALSE;
}

/**
  Look up a configure language in the index of one formset.

  Equal keys are ordered by load order, so this returns the same statement the
  linear walk of forms and statements would have returned: the first one, in
  load order, that carries the configure language and is visible under the
  current REDFISH_PLATFORM_CONFIG_ALLOW_SUPPRESSED policy.

  @param[in]  FormsetPrivate  Formset whose index to search.
  @param[in]  ConfigureLang   Configure language to find.

  @retval Statement pointer, or NULL when no visible statement matches.

**/
REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *
SearchConfigureLangIndex (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN  EFI_STRING                                ConfigureLang
  )
{
  REDFISH_CONFIG_LANG_INDEX_ENTRY            *Entries;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *Statement;
  UINTN                                      Low;
  UINTN                                      High;
  UINTN                                      Mid;
  UINTN                                      Index;
  INTN                                       Result;
  BOOLEAN                                    AllowSuppressed;

  if ((FormsetPrivate == NULL) || IS_EMPTY_STRING (ConfigureLang)) {
    return NULL;
  }

  Entries = FormsetPrivate->ConfigLangIndex;
  if ((Entries == NULL) || (FormsetPrivate->ConfigLangIndexCount == 0)) {
    return NULL;
  }

  //
  // Lower bound: first entry whose key is >= ConfigureLang.
  //
  Low  = 0;
  High = FormsetPrivate->ConfigLangIndexCount;
  while (Low < High) {
    Mid    = Low + (High - Low) / 2;
    Result = HiiStrCmp (Entries[Mid].ConfigureLang, ConfigureLang);
    if (Result < 0) {
      Low = Mid + 1;
    } else {
      High = Mid;
    }
  }

  AllowSuppressed = RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_ALLOW_SUPPRESSED);

  //
  // Scan the run of equal keys in load order for the first visible statement.
  //
  for (Index = Low; Index < FormsetPrivate->ConfigLangIndexCount; Index++) {
    if (HiiStrCmp (Entries[Index].ConfigureLang, ConfigureLang) != 0) {
      break;
    }

    Statement = Entries[Index].Statement;
    if ((Statement->Description != 0) && (AllowSuppressed || !Statement->Suppressed)) {
      return Statement;
    }
  }

  return NULL;
}

/**
  Release x-UEFI-string related information.

  @param[in]      FormsetPrivate Pointer to HII form-set private instance.

  @retval         EFI_STATUS

**/
EFI_STATUS
ReleaseXuefiStringDatabase (
  IN REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  REDFISH_X_UEFI_STRING_DATABASE  *ThisDatabase;
  REDFISH_X_UEFI_STRING_DATABASE  *PreDatabase;
  REDFISH_X_UEFI_STRINGS_ARRAY    *ThisStringArray;
  REDFISH_X_UEFI_STRINGS_ARRAY    *PreStringArray;
  BOOLEAN                         EndDatabase;
  BOOLEAN                         EndArray;

  //
  // The per-language string caches point into the exported package list, so
  // they must go before it does, regardless of which caller releases it.
  //
  ReleaseHiiStringCacheList (FormsetPrivate);

  if (FormsetPrivate->HiiPackageListHeader != NULL) {
    FreePool (FormsetPrivate->HiiPackageListHeader);
    FormsetPrivate->HiiPackageListHeader = NULL;
  }

  // Walk through x-UEFI-redfish string database.
  if (!IsListEmpty (&FormsetPrivate->XuefiRedfishStringDatabase)) {
    EndDatabase  = FALSE;
    ThisDatabase = (REDFISH_X_UEFI_STRING_DATABASE *)GetFirstNode (&FormsetPrivate->XuefiRedfishStringDatabase);
    while (!EndDatabase) {
      // Walk through string arrays.
      if (!IsListEmpty (&ThisDatabase->XuefiRedfishStringArrays)) {
        EndArray        = FALSE;
        ThisStringArray = (REDFISH_X_UEFI_STRINGS_ARRAY *)GetFirstNode (&ThisDatabase->XuefiRedfishStringArrays);
        while (!EndArray) {
          // Remove this array
          FreePool (ThisStringArray->ArrayEntryAddress);
          EndArray       = IsNodeAtEnd (&ThisDatabase->XuefiRedfishStringArrays, &ThisStringArray->NextArray);
          PreStringArray = ThisStringArray;
          if (!EndArray) {
            ThisStringArray = (REDFISH_X_UEFI_STRINGS_ARRAY *)GetNextNode (&ThisDatabase->XuefiRedfishStringArrays, &ThisStringArray->NextArray);
          }

          RemoveEntryList (&PreStringArray->NextArray);
          FreePool (PreStringArray);
        }
      }

      //
      // Remove this database
      //
      EndDatabase = IsNodeAtEnd (&FormsetPrivate->XuefiRedfishStringDatabase, &ThisDatabase->NextXuefiRedfishLanguage);
      PreDatabase = ThisDatabase;
      if (!EndDatabase) {
        ThisDatabase = (REDFISH_X_UEFI_STRING_DATABASE *)GetNextNode (&FormsetPrivate->XuefiRedfishStringDatabase, &ThisDatabase->NextXuefiRedfishLanguage);
      }

      RemoveEntryList (&PreDatabase->NextXuefiRedfishLanguage);
      FreePool (PreDatabase);
    }
  }

  return EFI_SUCCESS;
}

/**
  Release formset and all the forms and statements that belong to this formset.

  @param[in]      FormsetPrivate Pointer to HII form-set private instance.

  @retval         EFI_STATUS

**/
EFI_STATUS
ReleaseFormset (
  IN REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  LIST_ENTRY                                 *HiiFormLink;
  LIST_ENTRY                                 *HiiNextFormLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE       *HiiFormPrivate;
  LIST_ENTRY                                 *HiiStatementLink;
  LIST_ENTRY                                 *HiiNextStatementLink;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *HiiStatementPrivate;
  UINTN                                      Index;

  if (FormsetPrivate == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HiiFormLink = GetFirstNode (&FormsetPrivate->HiiFormList);
  while (!IsNull (&FormsetPrivate->HiiFormList, HiiFormLink)) {
    HiiFormPrivate  = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (HiiFormLink);
    HiiNextFormLink = GetNextNode (&FormsetPrivate->HiiFormList, HiiFormLink);

    HiiStatementLink = GetFirstNode (&HiiFormPrivate->StatementList);
    while (!IsNull (&HiiFormPrivate->StatementList, HiiStatementLink)) {
      HiiStatementPrivate  = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (HiiStatementLink);
      HiiNextStatementLink = GetNextNode (&HiiFormPrivate->StatementList, HiiStatementLink);

      //
      // HiiStatementPrivate->HiiStatement will be released in DestroyFormSet().
      //
      RemoveEntryList (&HiiStatementPrivate->Link);
      FreePool (HiiStatementPrivate);
      HiiStatementLink = HiiNextStatementLink;
    }

    //
    // HiiStatementPrivate->HiiForm will be released in DestroyFormSet().
    //

    RemoveEntryList (&HiiFormPrivate->Link);
    FreePool (HiiFormPrivate);
    HiiFormLink = HiiNextFormLink;
  }

  if (FormsetPrivate->HiiFormSet != NULL) {
    DestroyFormSet (FormsetPrivate->HiiFormSet);
    FormsetPrivate->HiiFormSet = NULL;
  }

  if (FormsetPrivate->DevicePathStr != NULL) {
    FreePool (FormsetPrivate->DevicePathStr);
  }

  //
  // Release schema list
  //
  if (FormsetPrivate->SupportedSchema.SchemaList != NULL) {
    for (Index = 0; Index < FormsetPrivate->SupportedSchema.Count; Index++) {
      FreePool (FormsetPrivate->SupportedSchema.SchemaList[Index]);
    }

    FreePool (FormsetPrivate->SupportedSchema.SchemaList);
    FormsetPrivate->SupportedSchema.SchemaList = NULL;
    FormsetPrivate->SupportedSchema.Count      = 0;
  }

  //
  // The index points into the statements released above; the string caches
  // point into the exported package list and are released together with it
  // inside ReleaseXuefiStringDatabase().
  //
  ReleaseConfigureLangIndex (FormsetPrivate);
  ReleaseXuefiStringDatabase (FormsetPrivate);

  return EFI_SUCCESS;
}

/**
  Create new form-set instance.

  @retval REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE *   Pointer to newly created form-set private instance.

**/
REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *
NewFormsetPrivate (
  VOID
  )
{
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *NewFormsetPrivate;

  NewFormsetPrivate = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE));
  if (NewFormsetPrivate == NULL) {
    return NULL;
  }

  //
  // Initial newly created formset private data.
  //
  InitializeListHead (&NewFormsetPrivate->HiiFormList);
  InitializeListHead (&NewFormsetPrivate->XuefiRedfishStringDatabase);
  InitializeListHead (&NewFormsetPrivate->HiiStringCacheList);

  return NewFormsetPrivate;
}

/**
  Create new x-UEFI-redfish string array.

  @param[in]      XuefiRedfishStringDatabase  The x-UEFI-redfish string database.

  @retval         EFI_OUT_OF_RESOURCES  Not enough memory for creating a new array.
                  EFI_SUCCESS           New array is created successfully.

**/
EFI_STATUS
NewRedfishXuefiStringArray (
  IN  REDFISH_X_UEFI_STRING_DATABASE  *XuefiRedfishStringDatabase
  )
{
  REDFISH_X_UEFI_STRINGS_ARRAY  *ArrayAddress;

  // Initial first REDFISH_X_UEFI_STRINGS_ARRAY memory.
  ArrayAddress = (REDFISH_X_UEFI_STRINGS_ARRAY *)AllocateZeroPool (sizeof (REDFISH_X_UEFI_STRINGS_ARRAY));
  if (ArrayAddress == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate REDFISH_X_UEFI_STRINGS_ARRAY.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&ArrayAddress->NextArray);

  // Allocate memory buffer for REDFISH_X_UEFI_STRINGS_ARRAY_ELEMENT elements.
  ArrayAddress->ArrayEntryAddress = \
    (REDFISH_X_UEFI_STRINGS_ARRAY_ELEMENT *)AllocateZeroPool (sizeof (REDFISH_X_UEFI_STRINGS_ARRAY_ELEMENT) * X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER);
  if (ArrayAddress->ArrayEntryAddress == NULL) {
    FreePool (ArrayAddress);
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate array for REDFISH_X_UEFI_STRINGS_ARRAY_ELEMENTs.\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  XuefiRedfishStringDatabase->StringsArrayBlocks++;
  InsertTailList (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays, &ArrayAddress->NextArray);
  return EFI_SUCCESS;
}

/**
  Get the pointer of x-UEFI-redfish database or create a new database.

  @param[in]      FormsetPrivate          Pointer to HII form-set private instance.
  @param[in]      HiiStringPackageHeader  HII string package header.

  @retval         Pointer to REDFISH_X_UEFI_STRING_DATABASE.
                  If NULL, it fails to obtain x-UEFI-redfish database.

**/
REDFISH_X_UEFI_STRING_DATABASE *
GetExistOrCreateXuefiStringDatabase (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN  EFI_HII_STRING_PACKAGE_HDR                *HiiStringPackageHeader
  )
{
  EFI_STATUS                      Status;
  BOOLEAN                         CreateNewOne;
  REDFISH_X_UEFI_STRING_DATABASE  *XuefiRedfishStringDatabase;

  CreateNewOne               = TRUE;
  XuefiRedfishStringDatabase = NULL;
  if (!IsListEmpty (&FormsetPrivate->XuefiRedfishStringDatabase)) {
    XuefiRedfishStringDatabase = (REDFISH_X_UEFI_STRING_DATABASE *)GetFirstNode (&FormsetPrivate->XuefiRedfishStringDatabase);

    while (TRUE) {
      if (AsciiStriCmp (XuefiRedfishStringDatabase->XuefiRedfishLanguage, HiiStringPackageHeader->Language) == 0) {
        CreateNewOne = FALSE;
        break;
      }

      if (IsNodeAtEnd (&FormsetPrivate->XuefiRedfishStringDatabase, &XuefiRedfishStringDatabase->NextXuefiRedfishLanguage)) {
        break;
      }

      XuefiRedfishStringDatabase = \
        (REDFISH_X_UEFI_STRING_DATABASE *)GetNextNode (&FormsetPrivate->XuefiRedfishStringDatabase, &XuefiRedfishStringDatabase->NextXuefiRedfishLanguage);
    }
  }

  if (CreateNewOne) {
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "  Creating x-UEFI-redfish (%a) string database...\n", HiiStringPackageHeader->Language));
    XuefiRedfishStringDatabase = (REDFISH_X_UEFI_STRING_DATABASE *)AllocateZeroPool (sizeof (REDFISH_X_UEFI_STRING_DATABASE));
    if (XuefiRedfishStringDatabase == NULL) {
      DEBUG ((DEBUG_ERROR, "  Failed to allocate REDFISH_X_UEFI_STRING_DATABASE.\n"));
      return NULL;
    }

    InitializeListHead (&XuefiRedfishStringDatabase->NextXuefiRedfishLanguage);
    InitializeListHead (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays);
    XuefiRedfishStringDatabase->StringsArrayBlocks   = 0;
    XuefiRedfishStringDatabase->XuefiRedfishLanguage = HiiStringPackageHeader->Language;

    Status = NewRedfishXuefiStringArray (XuefiRedfishStringDatabase);
    if (EFI_ERROR (Status)) {
      FreePool (XuefiRedfishStringDatabase);
      return NULL;
    }

    DEBUG ((
      DEBUG_REDFISH_PLATFORM_CONFIG,
      "  x-UEFI-redfish (%a):\n    String array is added to XuefiRedfishStringDatabase, total %d arrays now.\n",
      XuefiRedfishStringDatabase->XuefiRedfishLanguage,
      XuefiRedfishStringDatabase->StringsArrayBlocks
      ));

    // Link string database to FormsetPrivate.
    InsertTailList (&FormsetPrivate->XuefiRedfishStringDatabase, &XuefiRedfishStringDatabase->NextXuefiRedfishLanguage);
  }

  return XuefiRedfishStringDatabase;
}

/**
  Check and allocate a new x-UEFI-redfish array if it is insufficient for the
  newly added x-UEFI-redfish string.

  @param[in]      FormsetPrivate              Pointer to HII form-set private instance.
  @param[in]      XuefiRedfishStringDatabase  Pointer to the x-UEFI-redfish database.
  @param[in]      StringId                    String ID added to database.

  @retval         EFI_SUCCESS                 The size of x-UEFI-string array is adjusted or
                                              is not required to be adjusted.
                  Otherwise, refer to the error code returned from NewRedfishXuefiStringArray().

**/
EFI_STATUS
RedfishXuefiStringAdjustArrays (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN  REDFISH_X_UEFI_STRING_DATABASE            *XuefiRedfishStringDatabase,
  IN  EFI_STRING_ID                             StringId
  )
{
  EFI_STATUS  Status;

  while (((StringId + X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER) / X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER) > (UINT16)XuefiRedfishStringDatabase->StringsArrayBlocks) {
    Status = NewRedfishXuefiStringArray (XuefiRedfishStringDatabase);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to adjust x-UEFI-string array", __func__));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  Insert a x-UEFI-redfish string to database.

  @param[in]      FormsetPrivate          Pointer to HII form-set private instance.
  @param[in]      HiiStringPackageHeader  Pointer to HII string package.
  @param[in]      StringId                The HII string ID
  @param[in]      StringTextPtr           Pointer to HII string text.

  @retval         EFI_SUCCESS             The HII string is added to database.
                  EFI_LOAD_ERROR          Something wrong when insert an HII string
                                          to database.

**/
EFI_STATUS
RedfishXuefiStringInsertDatabase (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN  EFI_HII_STRING_PACKAGE_HDR                *HiiStringPackageHeader,
  IN  EFI_STRING_ID                             StringId,
  IN  CHAR16                                    *StringTextPtr
  )
{
  EFI_STATUS                      Status;
  UINTN                           StringIdOffset;
  REDFISH_X_UEFI_STRING_DATABASE  *XuefiRedfishStringDatabase;
  REDFISH_X_UEFI_STRINGS_ARRAY    *ThisArray;

  XuefiRedfishStringDatabase = GetExistOrCreateXuefiStringDatabase (FormsetPrivate, HiiStringPackageHeader);
  if (XuefiRedfishStringDatabase == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get REDFISH_X_UEFI_STRING_DATABASE of x-UEFI-redfish language %a.\n", __func__, HiiStringPackageHeader->Language));
    ReleaseXuefiStringDatabase (FormsetPrivate);
    return EFI_LOAD_ERROR;
  }

  Status = RedfishXuefiStringAdjustArrays (FormsetPrivate, XuefiRedfishStringDatabase, StringId);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to adjust x-UEFI-redfish string array.\n", __func__));
    ReleaseXuefiStringDatabase (FormsetPrivate);
    return EFI_LOAD_ERROR;
  }

  // Insert string to x-UEFI-redfish string array.
  StringIdOffset = (UINTN)StringId;
  ThisArray      = (REDFISH_X_UEFI_STRINGS_ARRAY *)GetFirstNode (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays);
  while (StringIdOffset >= X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER) {
    ThisArray       = (REDFISH_X_UEFI_STRINGS_ARRAY *)GetNextNode (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays, &ThisArray->NextArray);
    StringIdOffset -= X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER;
  }

  // Insert string
  (ThisArray->ArrayEntryAddress + StringIdOffset)->StringId  = StringId;
  (ThisArray->ArrayEntryAddress + StringIdOffset)->UcsString = StringTextPtr;

  DEBUG_REDFISH_THIS_MODULE (
    REDFISH_PLATFORM_CONFIG_DEBUG_STRING_DATABASE,
    "  Insert string ID: (%d) to database\n    x-UEFI-string: \"%s\"\n    Language: %a.\n",
    StringId,
    StringTextPtr,
    HiiStringPackageHeader->Language
    );
  return EFI_SUCCESS;
}

/**
  Get x-UEFI-redfish string and language by string ID.

  @param[in]       FormsetPrivate          Pointer to HII form-set private instance.
  @param[in]       HiiStringPackageHeader  HII string package header.
  @param[out]      TotalStringAdded        Return the total strings added to database.

  @retval  TRUE   x-UEFI-redfish string and ID map is inserted to database.
           FALSE  Something is wrong when insert x-UEFI-redfish string and ID map.

**/
BOOLEAN
CreateXuefiLanguageStringIdMap (
  IN   REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN   EFI_HII_STRING_PACKAGE_HDR                *HiiStringPackageHeader,
  OUT  UINTN                                     *TotalStringAdded
  )
{
  EFI_STATUS               Status;
  UINT8                    *BlockHdr;
  EFI_STRING_ID            CurrentStringId;
  UINTN                    BlockSize;
  UINTN                    Index;
  UINT8                    *StringTextPtr;
  UINTN                    Offset;
  UINT16                   StringCount;
  UINT16                   SkipCount;
  UINT8                    Length8;
  EFI_HII_SIBT_EXT2_BLOCK  Ext2;
  UINT32                   Length32;
  UINT8                    *StringBlockInfo;
  UINTN                    StringsAdded;

  StringsAdded = 0;

  //
  // Parse the string blocks to get the string text and font.
  //
  StringBlockInfo = (UINT8 *)((UINTN)HiiStringPackageHeader + HiiStringPackageHeader->StringInfoOffset);
  BlockHdr        = StringBlockInfo;
  BlockSize       = 0;
  Offset          = 0;
  CurrentStringId = 1;
  while (*BlockHdr != EFI_HII_SIBT_END) {
    switch (*BlockHdr) {
      case EFI_HII_SIBT_STRING_SCSU:
        Offset        = sizeof (EFI_HII_STRING_BLOCK);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset + AsciiStrSize ((CHAR8 *)StringTextPtr);
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRING_SCSU_FONT:
        Offset        = sizeof (EFI_HII_SIBT_STRING_SCSU_FONT_BLOCK) - sizeof (UINT8);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset + AsciiStrSize ((CHAR8 *)StringTextPtr);
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRINGS_SCSU:
        CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
        StringTextPtr = (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_BLOCK) - sizeof (UINT8));
        BlockSize    += StringTextPtr - BlockHdr;

        for (Index = 0; Index < StringCount; Index++) {
          BlockSize    += AsciiStrSize ((CHAR8 *)StringTextPtr);
          StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_STRINGS_SCSU_FONT:
        CopyMem (
          &StringCount,
          (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
          sizeof (UINT16)
          );
        StringTextPtr = (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_SIBT_STRINGS_SCSU_FONT_BLOCK) - sizeof (UINT8));
        BlockSize    += StringTextPtr - BlockHdr;

        for (Index = 0; Index < StringCount; Index++) {
          BlockSize    += AsciiStrSize ((CHAR8 *)StringTextPtr);
          StringTextPtr = StringTextPtr + AsciiStrSize ((CHAR8 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_STRING_UCS2:
        Offset        = sizeof (EFI_HII_STRING_BLOCK);
        StringTextPtr = BlockHdr + Offset;

        // x-UEFI-redfish string is always encoded as UCS and started with '/'.
        if (*StringTextPtr == (UINT16)'/') {
          Status = RedfishXuefiStringInsertDatabase (
                     FormsetPrivate,
                     HiiStringPackageHeader,
                     CurrentStringId,
                     (CHAR16 *)StringTextPtr
                     );
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "%a: Failed to insert x-UEFI-redfish string %s.\n", __func__, StringTextPtr));
            return FALSE;
          }

          StringsAdded++;
        }

        BlockSize += (Offset + HiiStrSize ((CHAR16 *)StringTextPtr));
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRING_UCS2_FONT:
        Offset        = sizeof (EFI_HII_SIBT_STRING_UCS2_FONT_BLOCK)  - sizeof (CHAR16);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += (Offset + HiiStrSize ((CHAR16 *)StringTextPtr));
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_STRINGS_UCS2:
        Offset        = sizeof (EFI_HII_SIBT_STRINGS_UCS2_BLOCK) - sizeof (CHAR16);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset;
        CopyMem (&StringCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
        for (Index = 0; Index < StringCount; Index++) {
          BlockSize    += HiiStrSize ((CHAR16 *)StringTextPtr);
          StringTextPtr = StringTextPtr + HiiStrSize ((CHAR16 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_STRINGS_UCS2_FONT:
        Offset        = sizeof (EFI_HII_SIBT_STRINGS_UCS2_FONT_BLOCK) - sizeof (CHAR16);
        StringTextPtr = BlockHdr + Offset;
        BlockSize    += Offset;
        CopyMem (
          &StringCount,
          (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
          sizeof (UINT16)
          );
        for (Index = 0; Index < StringCount; Index++) {
          BlockSize    += HiiStrSize ((CHAR16 *)StringTextPtr);
          StringTextPtr = StringTextPtr + HiiStrSize ((CHAR16 *)StringTextPtr);
          CurrentStringId++;
        }

        break;

      case EFI_HII_SIBT_DUPLICATE:
        BlockSize += sizeof (EFI_HII_SIBT_DUPLICATE_BLOCK);
        CurrentStringId++;
        break;

      case EFI_HII_SIBT_SKIP1:
        SkipCount       = (UINT16)(*(UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK)));
        CurrentStringId = (UINT16)(CurrentStringId + SkipCount);
        BlockSize      +=  sizeof (EFI_HII_SIBT_SKIP1_BLOCK);
        break;

      case EFI_HII_SIBT_SKIP2:
        CopyMem (&SkipCount, BlockHdr + sizeof (EFI_HII_STRING_BLOCK), sizeof (UINT16));
        CurrentStringId = (UINT16)(CurrentStringId + SkipCount);
        BlockSize      +=  sizeof (EFI_HII_SIBT_SKIP2_BLOCK);
        break;

      case EFI_HII_SIBT_EXT1:
        CopyMem (
          &Length8,
          (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
          sizeof (UINT8)
          );
        BlockSize += Length8;
        break;

      case EFI_HII_SIBT_EXT2:
        CopyMem (&Ext2, BlockHdr, sizeof (EFI_HII_SIBT_EXT2_BLOCK));
        BlockSize += Ext2.Length;
        break;

      case EFI_HII_SIBT_EXT4:
        CopyMem (
          &Length32,
          (UINT8 *)((UINTN)BlockHdr + sizeof (EFI_HII_STRING_BLOCK) + sizeof (UINT8)),
          sizeof (UINT32)
          );

        BlockSize += Length32;
        break;

      default:
        break;
    }

    BlockHdr = (UINT8 *)(StringBlockInfo + BlockSize);
  }

  *TotalStringAdded = StringsAdded;
  return TRUE;
}

/**
  Get x-UEFI-redfish string and language by string ID.

  @param[in]      FormsetPrivate       Pointer to HII form-set private instance.
  @param[in]      StringId             The HII string ID.
  @param[out]     String               Optionally return USC string.
  @param[out]     Language             Optionally return x-UEFI-redfish language.
  @param[out]     XuefiStringDatabase  Optionally return x-UEFI-redfish database.

  @retval  EFI_SUCCESS            String information is returned.
           EFI_INVALID_PARAMETER  One of the given parameters to this function is
                                  invalid.
           EFI_NOT_FOUND          String is not found.

**/
EFI_STATUS
GetXuefiStringAndLangByStringId (
  IN   REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN   EFI_STRING_ID                             StringId,
  OUT  CHAR16                                    **String OPTIONAL,
  OUT  CHAR8                                     **Language OPTIONAL,
  OUT  REDFISH_X_UEFI_STRING_DATABASE            **XuefiStringDatabase OPTIONAL
  )
{
  REDFISH_X_UEFI_STRING_DATABASE  *XuefiRedfishStringDatabase;
  REDFISH_X_UEFI_STRINGS_ARRAY    *StringArray;
  UINT16                          StringIndex;

  if ((String == NULL) && (Language == NULL) && (XuefiStringDatabase == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid parameters for this function.\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (&FormsetPrivate->XuefiRedfishStringDatabase)) {
    return EFI_NOT_FOUND;
  }

  XuefiRedfishStringDatabase = (REDFISH_X_UEFI_STRING_DATABASE *)GetFirstNode (&FormsetPrivate->XuefiRedfishStringDatabase);
  while (TRUE) {
    if (Language != NULL) {
      *Language = XuefiRedfishStringDatabase->XuefiRedfishLanguage;
    }

    StringArray = (REDFISH_X_UEFI_STRINGS_ARRAY *)GetFirstNode (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays);

    // Loop to the correct string array.
    StringIndex = StringId;
    while (StringIndex >= X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER) {
      if (IsNodeAtEnd (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays, &StringArray->NextArray)) {
        goto ErrorExit;
      }

      StringArray  = (REDFISH_X_UEFI_STRINGS_ARRAY *)GetNextNode (&XuefiRedfishStringDatabase->XuefiRedfishStringArrays, &StringArray->NextArray);
      StringIndex -= X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER;
    }

    //
    // NOTE: The string ID in the formset is a unique number.
    //       If the string in the array is NULL, then the matched string ID
    //       should be in another x-UEFI-redfish database.
    //
    if ((StringArray->ArrayEntryAddress + StringIndex)->UcsString != NULL) {
      //
      // String ID is belong to this x-uef-redfish language database.
      //
      if (String != NULL) {
        *String = (StringArray->ArrayEntryAddress + StringIndex)->UcsString;
      }

      if (XuefiStringDatabase != NULL) {
        *XuefiStringDatabase = XuefiRedfishStringDatabase;
      }

      return EFI_SUCCESS;
    }

    if (IsNodeAtEnd (&FormsetPrivate->XuefiRedfishStringDatabase, &XuefiRedfishStringDatabase->NextXuefiRedfishLanguage)) {
      return EFI_NOT_FOUND;
    }

    XuefiRedfishStringDatabase = (REDFISH_X_UEFI_STRING_DATABASE *)GetNextNode (
                                                                     &FormsetPrivate->XuefiRedfishStringDatabase,
                                                                     &XuefiRedfishStringDatabase->NextXuefiRedfishLanguage
                                                                     );
  }

ErrorExit:;
  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: String ID (%d) is not in any x-uef-redfish string databases.\n", __func__, StringId));
  return EFI_NOT_FOUND;
}

/**
  Build a x-UEFI-redfish database for the newly added x-UEFI-redfish language.

  @param[in]      FormsetPrivate          Pointer to HII form-set private instance.

**/
VOID
BuildXUefiRedfishStringDatabase (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  EFI_STATUS                  Status;
  EFI_HII_PACKAGE_HEADER      *PackageHeader;
  UINTN                       EndingPackageAddress;
  EFI_HII_STRING_PACKAGE_HDR  *HiiStringPackageHeader;
  UINTN                       SupportedSchemaLangCount;
  CHAR8                       **SupportedSchemaLang;
  BOOLEAN                     StringIdMapIsBuilt;
  UINTN                       TotalStringsAdded;
  UINTN                       NumberPackageStrings;

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: Building x-UEFI-redfish string database, HII Formset GUID - %g.\n", __func__, &FormsetPrivate->Guid));

  Status = ExportFormsetPackageList (FormsetPrivate);
  if (EFI_ERROR (Status)) {
    return;
  }

  TotalStringsAdded = 0;
  //
  // Finding the string package.
  //
  EndingPackageAddress = (UINTN)FormsetPrivate->HiiPackageListHeader + FormsetPrivate->HiiPackageListHeader->PackageLength;
  PackageHeader        = (EFI_HII_PACKAGE_HEADER *)(FormsetPrivate->HiiPackageListHeader + 1);
  SupportedSchemaLang  = FormsetPrivate->SupportedSchema.SchemaList;
  while ((UINTN)PackageHeader < EndingPackageAddress) {
    switch (PackageHeader->Type) {
      case EFI_HII_PACKAGE_STRINGS:
        StringIdMapIsBuilt     = FALSE;
        HiiStringPackageHeader = (EFI_HII_STRING_PACKAGE_HDR *)PackageHeader;

        // Check if this is the string package for x-UEFI-redfish
        for (SupportedSchemaLangCount = 0;
             SupportedSchemaLangCount < FormsetPrivate->SupportedSchema.Count;
             SupportedSchemaLangCount++
             )
        {
          if (AsciiStrnCmp (
                *(SupportedSchemaLang + SupportedSchemaLangCount),
                HiiStringPackageHeader->Language,
                AsciiStrLen (HiiStringPackageHeader->Language)
                ) == 0)
          {
            StringIdMapIsBuilt = CreateXuefiLanguageStringIdMap (FormsetPrivate, HiiStringPackageHeader, &NumberPackageStrings);
            if (StringIdMapIsBuilt) {
              TotalStringsAdded += NumberPackageStrings;
            }

            break;
          }
        }

        if (StringIdMapIsBuilt == FALSE) {
          if (AsciiStrStr (HiiStringPackageHeader->Language, X_UEFI_SCHEMA_PREFIX) == NULL) {
            DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "  No need to build x-UEFI-redfish string ID map for HII language %a\n", HiiStringPackageHeader->Language));
          } else {
            DEBUG ((DEBUG_ERROR, "  Failed to build x-UEFI-redfish string ID map of HII language %a\n", HiiStringPackageHeader->Language));
          }
        }

      default:
        PackageHeader = (EFI_HII_PACKAGE_HEADER *)((UINTN)PackageHeader + PackageHeader->Length);
    }
  }

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "  Total %d x-UEFI-redfish config language are added.\n", TotalStringsAdded));
}

/**
  Load the HII formset from the given HII handle.

  @param[in]  HiiHandle       Target HII handle to load.
  @param[out] FormsetPrivate  The formset private data.

  @retval EFI_STATUS          The formset is loaded successfully.
  @retval EFI_UNSUPPORTED     This formset doesn't have any x-UEFI-redfish configuration.

**/
EFI_STATUS
LoadFormset (
  IN  EFI_HII_HANDLE                            HiiHandle,
  OUT REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  )
{
  EFI_STATUS                                 Status;
  HII_FORMSET                                *HiiFormSet;
  HII_FORM                                   *HiiForm;
  LIST_ENTRY                                 *HiiFormLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE       *HiiFormPrivate;
  HII_STATEMENT                              *HiiStatement;
  LIST_ENTRY                                 *HiiStatementLink;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *HiiStatementPrivate;
  EFI_GUID                                   ZeroGuid;
  EXPRESS_RESULT                             ExpressionResult;
  CHAR16                                     *String;

  if ((HiiHandle == NULL) || (FormsetPrivate == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HiiFormSet = AllocateZeroPool (sizeof (HII_FORMSET));
  if (HiiFormSet == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No memory resource for HII_FORMSET - %g\n", __func__, FormsetPrivate->Guid));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Find HII formset by the given HII handle.
  //
  ZeroMem (&ZeroGuid, sizeof (ZeroGuid));
  Status = CreateFormSetFromHiiHandle (HiiHandle, &ZeroGuid, HiiFormSet);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "%a: Cannot create formset from HII handle (0x%x): %r\n", __func__, HiiHandle, Status));
    }

    goto ErrorExit;
  } else if (IsListEmpty (&HiiFormSet->FormListHead)) {
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: there is no form in HII handle: 0x%x\n", __func__, HiiHandle));
    Status = EFI_NOT_FOUND;
    goto ErrorExit;
  }

  //
  // Initialize formset
  //
  InitializeFormSet (HiiFormSet);

  //
  // Initialize formset private data.
  //
  FormsetPrivate->HiiFormSet = HiiFormSet;
  FormsetPrivate->HiiHandle  = HiiHandle;
  CopyGuid (&FormsetPrivate->Guid, &HiiFormSet->Guid);
  FormsetPrivate->DevicePathStr = ConvertDevicePathToText (HiiFormSet->DevicePath, FALSE, FALSE);
  Status                        = GetSupportedSchema (FormsetPrivate->HiiHandle, &FormsetPrivate->SupportedSchema);
  if (EFI_ERROR (Status)) {
    if (!RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_BUILD_MENU_PATH)) {
      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: No x-UEFI-redfish configuration found on the formset - %g\n", __func__, &FormsetPrivate->Guid));
      //
      // If there is no x-UEFI-redfish language in this form-set, we don't add formset
      // since we don't need to build menu path for attribute registry.
      //
      return EFI_UNSUPPORTED;
    }
  } else {
    // Building x-UEFI-redfish string database
    BuildXUefiRedfishStringDatabase (FormsetPrivate);
  }

  HiiFormLink = GetFirstNode (&HiiFormSet->FormListHead);
  while (!IsNull (&HiiFormSet->FormListHead, HiiFormLink)) {
    HiiForm = HII_FORM_FROM_LINK (HiiFormLink);

    HiiFormPrivate = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_FORM_PRIVATE));
    if (HiiFormPrivate == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "%a: No memory resource for REDFISH_PLATFORM_CONFIG_FORM_PRIVATE.\n", __func__));
      goto ErrorExit;
    }

    //
    // Initialize form private data.
    //
    HiiFormPrivate->HiiForm       = HiiForm;
    HiiFormPrivate->Id            = HiiForm->FormId;
    HiiFormPrivate->Title         = HiiForm->FormTitle;
    HiiFormPrivate->ParentFormset = FormsetPrivate;
    HiiFormPrivate->Suppressed    = FALSE;
    InitializeListHead (&HiiFormPrivate->StatementList);

    if ((HiiForm->SuppressExpression != NULL) &&
        (EvaluateExpressionList (HiiForm->SuppressExpression, TRUE, HiiFormSet, HiiForm) == ExpressSuppress))
    {
      HiiFormPrivate->Suppressed = TRUE;
    }

    HiiStatementLink = GetFirstNode (&HiiForm->StatementListHead);
    while (!IsNull (&HiiForm->StatementListHead, HiiStatementLink)) {
      HiiStatement = HII_STATEMENT_FROM_LINK (HiiStatementLink);

      HiiStatementPrivate = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE));
      if (HiiStatementPrivate == NULL) {
        DEBUG ((DEBUG_ERROR, "%a: No memory resource for REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE.\n", __func__));
        Status = EFI_OUT_OF_RESOURCES;
        goto ErrorExit;
      }

      //
      // Initialize statement private data.
      //
      HiiStatementPrivate->HiiStatement             = HiiStatement;
      HiiStatementPrivate->QuestionId               = HiiStatement->QuestionId;
      HiiStatementPrivate->Description              = HiiStatement->Prompt;
      HiiStatementPrivate->Help                     = HiiStatement->Help;
      HiiStatementPrivate->ParentForm               = HiiFormPrivate;
      HiiStatementPrivate->Flags                    = HiiStatement->QuestionFlags;
      HiiStatementPrivate->StatementData.NumMaximum = HiiStatement->ExtraData.NumData.Maximum;
      HiiStatementPrivate->StatementData.NumMinimum = HiiStatement->ExtraData.NumData.Minimum;
      HiiStatementPrivate->StatementData.NumStep    = HiiStatement->ExtraData.NumData.Step;
      HiiStatementPrivate->StatementData.StrMaxSize = HiiStatement->ExtraData.StrData.MaxSize;
      HiiStatementPrivate->StatementData.StrMinSize = HiiStatement->ExtraData.StrData.MinSize;
      HiiStatementPrivate->Suppressed               = FALSE;
      HiiStatementPrivate->GrayedOut                = FALSE;

      //
      // Expression
      //
      if (HiiFormPrivate->Suppressed) {
        HiiStatementPrivate->Suppressed = TRUE;
      } else {
        if (HiiStatement->ExpressionList != NULL) {
          ExpressionResult =  EvaluateExpressionList (HiiStatement->ExpressionList, TRUE, HiiFormSet, HiiForm);
          if (ExpressionResult == ExpressGrayOut) {
            HiiStatementPrivate->GrayedOut = TRUE;
          } else if (ExpressionResult == ExpressSuppress) {
            HiiStatementPrivate->Suppressed = TRUE;
          }
        }
      }

      // Get x-UEFI-redfish string using String ID.
      Status = GetXuefiStringAndLangByStringId (FormsetPrivate, HiiStatementPrivate->Description, &String, NULL, NULL);
      if (!EFI_ERROR (Status)) {
        HiiStatementPrivate->XuefiRedfishStr = String;
        //
        // Attach to statement list.
        //
        InsertTailList (&HiiFormPrivate->StatementList, &HiiStatementPrivate->Link);
      } else {
        if (!RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_BUILD_MENU_PATH)) {
          //
          // If there is no x-UEFI-redfish language for this statement, we don't add this statement
          // since we don't need to build menu path for attribute registry.
          //
          FreePool (HiiStatementPrivate);
        } else {
          //
          // This is not x-UEFI-redfish string and we don't cache its string for searching Redfish configure language.
          // When caller wants the string, we will read English string by calling HiiGetString().
          //
          HiiStatementPrivate->XuefiRedfishStr = NULL;
          //
          // Attach to statement list.
          //
          InsertTailList (&HiiFormPrivate->StatementList, &HiiStatementPrivate->Link);
        }
      }

      HiiStatementLink = GetNextNode (&HiiForm->StatementListHead, HiiStatementLink);
    }

    //
    // Attach to form list.
    //
    InsertTailList (&FormsetPrivate->HiiFormList, &HiiFormPrivate->Link);
    HiiFormLink = GetNextNode (&HiiFormSet->FormListHead, HiiFormLink);
  }

  //
  // All statements are attached; build the configure-language index so that
  // per-attribute lookups are a binary search instead of a walk of every
  // statement.  Failure is not fatal: lookups fall back to the linear walk.
  //
  Status = BuildConfigureLangIndex (FormsetPrivate);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: configure-language index not built for %g: %r\n", __func__, &FormsetPrivate->Guid, Status));
  }

  return EFI_SUCCESS;

ErrorExit:

  //
  // Release HiiFormSet if HiiFormSet is not linked to FormsetPrivate yet.
  //
  if ((HiiFormSet != NULL) && (FormsetPrivate->HiiFormSet != HiiFormSet)) {
    DestroyFormSet (HiiFormSet);
  }

  //
  // Release resource when error happens.
  //
  ReleaseFormset (FormsetPrivate);

  return Status;
}

/**
  Load formset list on given HII handle.

  @param[in]  HiiHandle     HII handle to load formset list.
  @param[out] FormsetList   Pointer to formset list returned on given handle.

  @retval     EFI_STATUS

**/
EFI_STATUS
LoadFormsetList (
  IN   EFI_HII_HANDLE  *HiiHandle,
  OUT  LIST_ENTRY      *FormsetList
  )
{
  EFI_STATUS                                Status;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate;

  if ((HiiHandle == NULL) || (FormsetList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  FormsetPrivate = GetFormsetPrivateByHiiHandle (HiiHandle, FormsetList);
  if (FormsetPrivate != NULL) {
    return EFI_ALREADY_STARTED;
  }

  FormsetPrivate =  NewFormsetPrivate ();
  if (FormsetPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: out of resource\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Load formset on the given HII handle.
  //
  Status = LoadFormset (HiiHandle, FormsetPrivate);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_NOT_FOUND) {
      DEBUG ((DEBUG_ERROR, "%a: Formset is not loaded for edk2 redfish: %r\n", __func__, Status));
    }

    FreePool (FormsetPrivate);
    return Status;
  }

  //
  // Attach to cache list.
  //
  InsertTailList (FormsetList, &FormsetPrivate->Link);

  DEBUG_CODE (
    if (RedfishPlatformConfigDebugProp (REDFISH_PLATFORM_CONFIG_DEBUG_DUMP_FORMSET)) {
    DumpFormsetList (FormsetList);
  }

    );

  return EFI_SUCCESS;
}

/**
  Release formset list and all the forms that belong to this formset.

  @param[in]      FormsetList   Pointer to formset list that needs to be
                                released.

  @retval         EFI_STATUS

**/
EFI_STATUS
ReleaseFormsetList (
  IN  LIST_ENTRY  *FormsetList
  )
{
  LIST_ENTRY                                *HiiFormsetLink;
  LIST_ENTRY                                *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *HiiFormsetPrivate;

  if (FormsetList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (FormsetList)) {
    return EFI_SUCCESS;
  }

  HiiFormsetLink = GetFirstNode (FormsetList);
  while (!IsNull (FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    //
    // Detach from list.
    //
    RemoveEntryList (&HiiFormsetPrivate->Link);
    ReleaseFormset (HiiFormsetPrivate);
    FreePool (HiiFormsetPrivate);
    HiiFormsetLink = HiiFormsetNextLink;
  }

  return EFI_SUCCESS;
}

/**
  Get all pending list.

  @param[in]  HiiHandle   HII handle instance.
  @param[in]  PendingList Pending list to keep pending data.

  @retval REDFISH_PLATFORM_CONFIG_PENDING_LIST *   Pointer to pending list data.

**/
REDFISH_PLATFORM_CONFIG_PENDING_LIST *
GetPendingList (
  IN  EFI_HII_HANDLE  *HiiHandle,
  IN  LIST_ENTRY      *PendingList
  )
{
  LIST_ENTRY                            *PendingListLink;
  REDFISH_PLATFORM_CONFIG_PENDING_LIST  *Target;

  if ((HiiHandle == NULL) || (PendingList == NULL)) {
    return NULL;
  }

  if (IsListEmpty (PendingList)) {
    return NULL;
  }

  PendingListLink = GetFirstNode (PendingList);
  while (!IsNull (PendingList, PendingListLink)) {
    Target = REDFISH_PLATFORM_CONFIG_PENDING_LIST_FROM_LINK (PendingListLink);

    if (Target->HiiHandle == HiiHandle) {
      return Target;
    }

    PendingListLink = GetNextNode (PendingList, PendingListLink);
  }

  return NULL;
}

/**
  When HII database is updated. Keep updated HII handle into pending list so
  we can process them later.

  @param[in]  HiiHandle   HII handle instance.
  @param[in]  PendingList Pending list to keep HII handle which is recently updated.

  @retval EFI_SUCCESS             HII handle is saved in pending list.
  @retval EFI_INVALID_PARAMETER   HiiHandle is NULL or PendingList is NULL.
  @retval EFI_OUT_OF_RESOURCES    System is out of memory.

**/
EFI_STATUS
NotifyFormsetUpdate (
  IN  EFI_HII_HANDLE  *HiiHandle,
  IN  LIST_ENTRY      *PendingList
  )
{
  REDFISH_PLATFORM_CONFIG_PENDING_LIST  *TargetPendingList;

  if ((HiiHandle == NULL) || (PendingList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check and see if this HII handle is processed already.
  //
  TargetPendingList = GetPendingList (HiiHandle, PendingList);
  if (TargetPendingList != NULL) {
    TargetPendingList->IsDeleted = FALSE;
    DEBUG_CODE (
      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: HII handle: 0x%x is updated\n", __func__, HiiHandle));
      );
    return EFI_SUCCESS;
  }

  TargetPendingList = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_PENDING_LIST));
  if (TargetPendingList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TargetPendingList->HiiHandle = HiiHandle;
  TargetPendingList->IsDeleted = FALSE;

  InsertTailList (PendingList, &TargetPendingList->Link);

  DEBUG_CODE (
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: HII handle: 0x%x is created\n", __func__, HiiHandle));
    );

  return EFI_SUCCESS;
}

/**
  When HII database is updated and form-set is deleted. Keep deleted HII handle into pending list so
  we can process them later.

  @param[in]  HiiHandle   HII handle instance.
  @param[in]  PendingList Pending list to keep HII handle which is recently updated.

  @retval EFI_SUCCESS             HII handle is saved in pending list.
  @retval EFI_INVALID_PARAMETER   HiiHandle is NULL or PendingList is NULL.
  @retval EFI_OUT_OF_RESOURCES    System is out of memory.

**/
EFI_STATUS
NotifyFormsetDeleted (
  IN  EFI_HII_HANDLE  *HiiHandle,
  IN  LIST_ENTRY      *PendingList
  )
{
  REDFISH_PLATFORM_CONFIG_PENDING_LIST  *TargetPendingList;

  if ((HiiHandle == NULL) || (PendingList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check and see if this HII handle is processed already.
  //
  TargetPendingList = GetPendingList (HiiHandle, PendingList);
  if (TargetPendingList != NULL) {
    TargetPendingList->IsDeleted = TRUE;
    DEBUG_CODE (
      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: HII handle: 0x%x is updated and deleted\n", __func__, HiiHandle));
      );
    return EFI_SUCCESS;
  }

  TargetPendingList = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_PENDING_LIST));
  if (TargetPendingList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TargetPendingList->HiiHandle = HiiHandle;
  TargetPendingList->IsDeleted = TRUE;

  InsertTailList (PendingList, &TargetPendingList->Link);

  DEBUG_CODE (
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: HII handle: 0x%x is deleted\n", __func__, HiiHandle));
    );

  return EFI_SUCCESS;
}

/**
  There are HII database update and we need to process them accordingly so that we
  won't use stale data. This function will parse updated HII handle again in order
  to get updated data-set.

  @param[in]  FormsetList   List to keep HII form-set.
  @param[in]  PendingList   List to keep HII handle that is updated.

  @retval EFI_SUCCESS             HII handle is saved in pending list.
  @retval EFI_INVALID_PARAMETER   FormsetList is NULL or PendingList is NULL.

**/
EFI_STATUS
ProcessPendingList (
  IN  LIST_ENTRY  *FormsetList,
  IN  LIST_ENTRY  *PendingList
  )
{
  LIST_ENTRY                                *PendingListLink;
  LIST_ENTRY                                *PendingListNextLink;
  REDFISH_PLATFORM_CONFIG_PENDING_LIST      *Target;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate;
  EFI_STATUS                                Status;

  if ((FormsetList == NULL) || (PendingList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (PendingList)) {
    return EFI_SUCCESS;
  }

  PendingListLink = GetFirstNode (PendingList);
  while (!IsNull (PendingList, PendingListLink)) {
    PendingListNextLink = GetNextNode (PendingList, PendingListLink);
    Target              = REDFISH_PLATFORM_CONFIG_PENDING_LIST_FROM_LINK (PendingListLink);

    if (Target->IsDeleted) {
      //
      // The HII resource on this HII handle is removed. Release the formset.
      //
      FormsetPrivate = GetFormsetPrivateByHiiHandle (Target->HiiHandle, FormsetList);
      if (FormsetPrivate != NULL) {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: formset: %g is removed because driver release HII resource it already\n", __func__, &FormsetPrivate->Guid));
        RemoveEntryList (&FormsetPrivate->Link);
        ReleaseFormset (FormsetPrivate);
        FreePool (FormsetPrivate);
      } else {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: formset on HII handle 0x%x was removed already\n", __func__, Target->HiiHandle));
      }
    } else {
      //
      // The HII resource on this HII handle is updated/removed.
      //
      FormsetPrivate = GetFormsetPrivateByHiiHandle (Target->HiiHandle, FormsetList);
      if (FormsetPrivate != NULL) {
        //
        // HII formset already exist, release it and query again.
        //
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: formset: %g is updated. Release current formset\n", __func__, &FormsetPrivate->Guid));
        RemoveEntryList (&FormsetPrivate->Link);
        ReleaseFormset (FormsetPrivate);
        FreePool (FormsetPrivate);
      }

      Status = LoadFormsetList (Target->HiiHandle, FormsetList);
      if (EFI_ERROR (Status)) {
        if (Status == EFI_UNSUPPORTED) {
          DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "  The formset has no x-UEFI-redfish configurations.\n"));
        } else if (Status == EFI_NOT_FOUND) {
          DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "  There is no formset or form on HII handle: 0x%x.\n", Target->HiiHandle));
        } else {
          DEBUG ((DEBUG_ERROR, "  load formset from HII handle: 0x%x failed: %r\n", Target->HiiHandle, Status));
        }
      }
    }

    //
    // Detach it from list first.
    //
    RemoveEntryList (&Target->Link);
    FreePool (Target);

    PendingListLink = PendingListNextLink;
  }

  return EFI_SUCCESS;
}

/**
  Release all resource in statement list.

  @param[in]  StatementList   Statement list to be released.

  @retval EFI_SUCCESS             All resource are released.
  @retval EFI_INVALID_PARAMETER   StatementList is NULL.

**/
EFI_STATUS
ReleaseStatementList (
  IN  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST  *StatementList
  )
{
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF  *StatementRef;
  LIST_ENTRY                                     *NextLink;

  if (StatementList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (IsListEmpty (&StatementList->StatementList)) {
    return EFI_SUCCESS;
  }

  NextLink = GetFirstNode (&StatementList->StatementList);
  while (!IsNull (&StatementList->StatementList, NextLink)) {
    StatementRef = REDFISH_PLATFORM_CONFIG_STATEMENT_REF_FROM_LINK (NextLink);
    NextLink     = GetNextNode (&StatementList->StatementList, NextLink);

    RemoveEntryList (&StatementRef->Link);
    FreePool (StatementRef);
  }

  return EFI_SUCCESS;
}
