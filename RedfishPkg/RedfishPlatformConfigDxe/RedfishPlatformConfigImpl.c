/** @file
  The implementation of EDKII Redfish Platform Config Protocol.

  (C) Copyright 2021-2022 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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
  EFI_STATUS  Status;
  UINTN       StringSize;
  CHAR16      TempString;
  EFI_STRING  String;

  if ((mRedfishPlatformConfigPrivate->HiiString == NULL) || (HiiHandle == NULL) || (StringId == 0) || IS_EMPTY_STRING (Language)) {
    ASSERT (FALSE);
    return NULL;
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
    DEBUG ((DEBUG_ERROR, "%a: Can not find string ID: 0x%x with %a\n", __func__, StringId, Language));
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
    return EFI_NOT_FOUND;
  }

  SupportedSchema->Count      = Count;
  SupportedSchema->SchemaList = AllocatePool (sizeof (CHAR8 *) * Count);
  if (SupportedSchema->SchemaList == NULL) {
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

  if (FormsetPrivate->HiiPackageListHeader != NULL) {
    FreePool (FormsetPrivate->HiiPackageListHeader);
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
  UINTN                       BufferSize;
  EFI_HII_PACKAGE_HEADER      *PackageHeader;
  UINTN                       EndingPackageAddress;
  EFI_HII_STRING_PACKAGE_HDR  *HiiStringPackageHeader;
  UINTN                       SupportedSchemaLangCount;
  CHAR8                       **SupportedSchemaLang;
  BOOLEAN                     StringIdMapIsBuilt;
  UINTN                       TotalStringsAdded;
  UINTN                       NumberPackageStrings;

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: Building x-UEFI-redfish string database, HII Formset GUID - %g.\n", __func__, FormsetPrivate->Guid));

  BufferSize = 0;
  Status     = mRedfishPlatformConfigPrivate->HiiDatabase->ExportPackageLists (
                                                             mRedfishPlatformConfigPrivate->HiiDatabase,
                                                             FormsetPrivate->HiiHandle,
                                                             &BufferSize,
                                                             FormsetPrivate->HiiPackageListHeader
                                                             );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    DEBUG ((DEBUG_ERROR, "  Failed to export package list.\n"));
    return;
  }

  FormsetPrivate->HiiPackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *)AllocateZeroPool (BufferSize);
  if (FormsetPrivate->HiiPackageListHeader == NULL) {
    DEBUG ((DEBUG_ERROR, "  Failed to allocate memory for the exported package list.\n"));
    return;
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
  if (EFI_ERROR (Status) || IsListEmpty (&HiiFormSet->FormListHead)) {
    DEBUG ((DEBUG_ERROR, "%a: Formset not found by HII handle - %g\n", __func__, FormsetPrivate->Guid));
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
      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: No x-UEFI-redfish configuration found on the formset - %g\n", __func__, FormsetPrivate->Guid));
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
    DEBUG ((DEBUG_ERROR, "%a: Formset is not loaded for edk2 redfish: %r\n", __func__, Status));
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
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: formset: %g is removed because driver release HII resource it already\n", __func__, FormsetPrivate->Guid));
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
          DEBUG ((DEBUG_ERROR, "  The formset has no x-UEFI-redfish configurations.\n"));
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
