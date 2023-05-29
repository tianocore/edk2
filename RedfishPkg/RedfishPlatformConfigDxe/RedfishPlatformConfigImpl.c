/** @file
  The implementation of EDKII Redfish Platform Config Protocol.

  (C) Copyright 2021-2022 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

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
    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "???"));
    return EFI_INVALID_PARAMETER;
  }

  String = HiiGetString (HiiHandle, StringId, NULL);
  if (String == NULL) {
    return EFI_NOT_FOUND;
  }

  DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%s", String));
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

    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "  [%d] form: %d title: ", ++Index, HiiFormPrivate->Id));
    DumpHiiString (FormsetPrivate->HiiHandle, HiiFormPrivate->Title);
    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "\n"));

    HiiStatementLink = GetFirstNode (&HiiFormPrivate->StatementList);
    while (!IsNull (&HiiFormPrivate->StatementList, HiiStatementLink)) {
      HiiStatementPrivate  = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (HiiStatementLink);
      HiiNextStatementLink = GetNextNode (&HiiFormPrivate->StatementList, HiiStatementLink);

      DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "    QID: 0x%x Prompt: ", HiiStatementPrivate->QuestionId));
      DumpHiiString (FormsetPrivate->HiiHandle, HiiStatementPrivate->Description);
      DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "\n"));

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
    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: Empty formset list\n", __func__));
    return EFI_SUCCESS;
  }

  Index          = 0;
  HiiFormsetLink = GetFirstNode (FormsetList);
  while (!IsNull (FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "[%d] HII Handle: 0x%x formset: %g at %s\n", ++Index, HiiFormsetPrivate->HiiHandle, &HiiFormsetPrivate->Guid, HiiFormsetPrivate->DevicePathStr));
    DumpFormset (HiiFormsetPrivate);

    HiiFormsetLink = HiiFormsetNextLink;
  }

  return EFI_SUCCESS;
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
  Get string from HII database in English language. The returned string is allocated
  using AllocatePool(). The caller is responsible for freeing the allocated buffer using
  FreePool().

  @param[in]  HiiHandle         A handle that was previously registered in the HII Database.
  @param[in]  StringId          The identifier of the string to retrieved from the string
                                package associated with HiiHandle.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
HiiGetEnglishString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId
  )
{
  return HiiGetRedfishString (HiiHandle, ENGLISH_LANGUAGE_CODE, StringId);
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

        if ((HiiStatementPrivate->Description != 0) && !HiiStatementPrivate->Suppressed) {
          TmpString = HiiGetRedfishString (HiiFormsetPrivate->HiiHandle, Schema, HiiStatementPrivate->Description);
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

            FreePool (TmpString);
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

  if ((FormsetList == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (ConfigureLang)) {
    return NULL;
  }

  if (IsListEmpty (FormsetList)) {
    return NULL;
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

        DEBUG_CODE (
          STATIC UINTN Index = 0;
          DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: [%d] search %s in QID: 0x%x form: 0x%x formset: %g\n", __func__, ++Index, ConfigureLang, HiiStatementPrivate->QuestionId, HiiFormPrivate->Id, &HiiFormsetPrivate->Guid));
          );

        if (HiiStatementPrivate->Description != 0) {
          TmpString = HiiGetRedfishString (HiiFormsetPrivate->HiiHandle, Schema, HiiStatementPrivate->Description);
          if (TmpString != NULL) {
            if (StrCmp (TmpString, ConfigureLang) == 0) {
              FreePool (TmpString);
              return HiiStatementPrivate;
            }

            FreePool (TmpString);
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
  Release formset and all the forms and statements that belong to this formset.

  @param[in]      FormsetPrivate Pointer to HP_HII_FORM_SET_PRIVATE

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

      if (HiiStatementPrivate->DesStringCache != NULL) {
        FreePool (HiiStatementPrivate->DesStringCache);
        HiiStatementPrivate->DesStringCache = NULL;
      }

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

  return NewFormsetPrivate;
}

/**
  Load the HII formset from the given HII handle.

  @param[in]  HiiHandle       Target HII handle to load.
  @param[out] FormsetPrivate  The formset private data.

  @retval EFI_STATUS

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

  if ((HiiHandle == NULL) || (FormsetPrivate == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HiiFormSet = AllocateZeroPool (sizeof (HII_FORMSET));
  if (HiiFormSet == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Find HII formset by the given HII handle.
  //
  ZeroMem (&ZeroGuid, sizeof (ZeroGuid));
  Status = CreateFormSetFromHiiHandle (HiiHandle, &ZeroGuid, HiiFormSet);
  if (EFI_ERROR (Status) || IsListEmpty (&HiiFormSet->FormListHead)) {
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
    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: No schema from HII handle: 0x%x found: %r\n", __func__, FormsetPrivate->HiiHandle, Status));
  }

  HiiFormLink = GetFirstNode (&HiiFormSet->FormListHead);
  while (!IsNull (&HiiFormSet->FormListHead, HiiFormLink)) {
    HiiForm = HII_FORM_FROM_LINK (HiiFormLink);

    HiiFormPrivate = AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_FORM_PRIVATE));
    if (HiiFormPrivate == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
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

      //
      // Attach to statement list.
      //
      InsertTailList (&HiiFormPrivate->StatementList, &HiiStatementPrivate->Link);
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
    DEBUG ((DEBUG_ERROR, "%a: failed to load formset: %r\n", __func__, Status));
    FreePool (FormsetPrivate);
    return Status;
  }

  //
  // Attach to cache list.
  //
  InsertTailList (FormsetList, &FormsetPrivate->Link);

  DEBUG_CODE (
    DumpFormsetList (FormsetList);
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
      DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: HII handle: 0x%x is updated\n", __func__, HiiHandle));
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
    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: HII handle: 0x%x is created\n", __func__, HiiHandle));
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
      DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: HII handle: 0x%x is updated and deleted\n", __func__, HiiHandle));
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
    DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: HII handle: 0x%x is deleted\n", __func__, HiiHandle));
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
        DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: formset: %g is removed because driver release HII resource it already\n", __func__, FormsetPrivate->Guid));
        RemoveEntryList (&FormsetPrivate->Link);
        ReleaseFormset (FormsetPrivate);
        FreePool (FormsetPrivate);
      } else {
        DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: formset on HII handle 0x%x was removed already\n", __func__, Target->HiiHandle));
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
        DEBUG ((REDFISH_PLATFORM_CONFIG_DEBUG, "%a: formset: %g is updated. Release current formset\n", __func__, &FormsetPrivate->Guid));
        RemoveEntryList (&FormsetPrivate->Link);
        ReleaseFormset (FormsetPrivate);
        FreePool (FormsetPrivate);
      }

      Status = LoadFormsetList (Target->HiiHandle, FormsetList);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: load formset from HII handle: 0x%x failed: %r\n", __func__, Target->HiiHandle, Status));
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
