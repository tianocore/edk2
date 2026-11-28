/** @file
  This file defines the EDKII Redfish Platform Config Protocol private structure.

  (C) Copyright 2021-2022 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Uefi.h>
#include <RedfishCommon.h>

//
// Libraries
//
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HiiUtilityLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#define ENGLISH_LANGUAGE_CODE  "en-US"
#define X_UEFI_SCHEMA_PREFIX   "x-UEFI-redfish-"

#define MAX_X_UEFI_REDFISH_STRING_SIZE  (128 * 2)// 128 character in UCS.

typedef struct _REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE;

//
// Definition of REDFISH_PLATFORM_CONFIG_PRIVATE.
//
typedef struct {
  LIST_ENTRY        Link;
  EFI_HII_HANDLE    HiiHandle;
  BOOLEAN           IsDeleted;
} REDFISH_PLATFORM_CONFIG_PENDING_LIST;

#define REDFISH_PLATFORM_CONFIG_PENDING_LIST_FROM_LINK(a)  BASE_CR (a, REDFISH_PLATFORM_CONFIG_PENDING_LIST, Link)

typedef struct {
  UINTN    Count;                               // Number of schema in list
  CHAR8    **SchemaList;                        // Schema list
} REDFISH_PLATFORM_CONFIG_SCHEMA;

// Defines the number of elements in array
#define X_UEFI_REDFISH_STRING_ARRAY_ENTRY_NUMBER  1024

//
// Definition of x-UEFI-redfish string element.
//
typedef struct {
  EFI_STRING_ID    StringId;
  CHAR16           *UcsString;
} REDFISH_X_UEFI_STRINGS_ARRAY_ELEMENT;

//
// Discrete string array buffer, each has X_UEFI_REDFISH_STRING_ARRAY_NUMBER element.
//
typedef struct {
  LIST_ENTRY                              NextArray;
  REDFISH_X_UEFI_STRINGS_ARRAY_ELEMENT    *ArrayEntryAddress;
} REDFISH_X_UEFI_STRINGS_ARRAY;

//
// x-UEFI-redfish string database, x-UEFI-redfish language based.
//
typedef struct {
  LIST_ENTRY    NextXuefiRedfishLanguage;                                     // Link to the next suppoted x-UEFI-Redfish language.
  CHAR8         *XuefiRedfishLanguage;                                        // x-UEFI-redfish language.
  UINTN         StringsArrayBlocks;                                           // Number of the array blocks that accommodate X_UEFI_REDFISH_STRING_ARRAY_NUMBER
                                                                              // elements in each.
  LIST_ENTRY    XuefiRedfishStringArrays;                                     // Link entry of x-UEFI-redfish string array.
} REDFISH_X_UEFI_STRING_DATABASE;

//
// Per-language HII string cache.
//
// Maps a string ID to the UCS-2 text of that string in one HII string package
// of the formset's exported package list.  Built lazily the first time a
// string of that language is requested through HiiGetRedfishString(), so any
// language present in the package list benefits, not just "en-US".  Entries
// point into HiiPackageListHeader and are therefore byte aligned; they are
// released together with the exported package list when the formset is
// re-parsed.  A Count of zero records that the language is not present in the
// package list so repeated misses do not re-walk the packages.
//
// The cache is a snapshot: HiiSetString()/NewString() on an existing ID do not
// notify this driver, so only use it for metadata strings (prompts, help,
// option labels, form titles).  Question values must go through the
// *Uncached helpers.  The snapshot is refreshed whenever the formset is
// re-parsed (form package add/remove notification).
//
typedef struct {
  LIST_ENTRY    Link;
  CHAR8         *Language;      // Language of the string package that was indexed (owned).
  UINTN         Count;          // Number of entries in Strings (highest string ID + 1).
  CHAR16        **Strings;      // Strings[StringId] -> UCS-2 text in the package, or NULL.
} REDFISH_HII_STRING_CACHE;

#define REDFISH_HII_STRING_CACHE_FROM_LINK(a)  BASE_CR (a, REDFISH_HII_STRING_CACHE, Link)

//
// One entry of the per-formset configure-language index.  Sorted by
// ConfigureLang, then by Order (position in load order), so equal keys keep
// the order in which the statements were parsed.
//
typedef struct {
  CHAR16                                       *ConfigureLang;   // Statement's x-UEFI-redfish string (not owned).
  UINTN                                        Order;            // Load-order sequence number.
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE    *Statement;
} REDFISH_CONFIG_LANG_INDEX_ENTRY;

//
// Definition of REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE
//
typedef struct {
  LIST_ENTRY                         Link;
  HII_FORMSET                        *HiiFormSet;                          // Pointer to HII formset data.
  EFI_GUID                           Guid;                                 // Formset GUID.
  EFI_HII_HANDLE                     HiiHandle;                            // Hii Handle of this formset.
  EFI_HII_PACKAGE_LIST_HEADER        *HiiPackageListHeader;                // Hii Package list header.
  LIST_ENTRY                         HiiFormList;                          // Form list that keep form data under this formset.
  CHAR16                             *DevicePathStr;                       // Device path of this formset.
  REDFISH_PLATFORM_CONFIG_SCHEMA     SupportedSchema;                      // Schema that is supported in this formset.
  LIST_ENTRY                         XuefiRedfishStringDatabase;           // x-UEFI-redfish string/Id data base;
  LIST_ENTRY                         HiiStringCacheList;                   // REDFISH_HII_STRING_CACHE per requested language.
  REDFISH_CONFIG_LANG_INDEX_ENTRY    *ConfigLangIndex;                     // Statements with an x-UEFI-redfish string, sorted by that string.
  UINTN                              ConfigLangIndexCount;                 // Number of entries in ConfigLangIndex.
  BOOLEAN                            ConfigLangIndexBuilt;                 // TRUE once the index (possibly empty) is valid.
} REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE;

#define REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK(a)  BASE_CR (a, REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE, Link)

//
// Definition of REDFISH_PLATFORM_CONFIG_FORM_PRIVATE
//
typedef struct {
  LIST_ENTRY                                  Link;
  UINT16                                      Id;           // Form ID.
  EFI_STRING_ID                               Title;        // String token of form title.
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE    *ParentFormset;
  HII_FORM                                    *HiiForm;      // Pointer to HII form data.
  LIST_ENTRY                                  StatementList; // Statement list that keep statement under this form.
  BOOLEAN                                     Suppressed;    // Form is suppressed
} REDFISH_PLATFORM_CONFIG_FORM_PRIVATE;

#define REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK(a)  BASE_CR (a, REDFISH_PLATFORM_CONFIG_FORM_PRIVATE, Link)

//
// Definition of REDFISH_PLATFORM_CONFIG_STATEMENT_DATA
//
typedef struct {
  UINT64    NumMinimum;
  UINT64    NumMaximum;
  UINT64    NumStep;
  UINT8     StrMinSize;
  UINT8     StrMaxSize;
} REDFISH_PLATFORM_CONFIG_STATEMENT_DATA;

//
// Definition of REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE
//
struct _REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE {
  LIST_ENTRY                                Link;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE      *ParentForm;
  HII_STATEMENT                             *HiiStatement;     // Pointer to HII statement data.
  EFI_QUESTION_ID                           QuestionId;        // Question ID of this statement.
  EFI_STRING_ID                             Description;       // String token of this question.
  CHAR16                                    *XuefiRedfishStr;  // x-UEFI-redfish string of this question.
  EFI_STRING_ID                             Help;              // String token of help message.
  UINT8                                     Flags;             // The statement flag.
  REDFISH_PLATFORM_CONFIG_STATEMENT_DATA    StatementData;     // The max/min for statement value.
  BOOLEAN                                   Suppressed;        // Statement is suppressed.
  BOOLEAN                                   GrayedOut;         // Statement is GrayedOut.
};

#define REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK(a)  BASE_CR (a, REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE, Link)

//
// Definition of REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF
//
typedef struct {
  LIST_ENTRY                                   Link;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE    *Statement;
} REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF;

#define REDFISH_PLATFORM_CONFIG_STATEMENT_REF_FROM_LINK(a)  BASE_CR (a, REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF, Link)

//
// Definition of REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST
//
typedef struct {
  LIST_ENTRY    StatementList;      // List of REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF
  UINTN         Count;
} REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST;

/**
  Release formset list and all the forms that belong to this formset.

  @param[in]      FormsetList   Pointer to formset list that needs to be
                                released.

  @retval         EFI_STATUS

**/
EFI_STATUS
ReleaseFormsetList (
  IN  LIST_ENTRY  *FormsetList
  );

/**
  Release formset list and all the forms that belong to this formset.

  @param[in]      FormsetList   Pointer to formset list that needs to be
                                released.

  @retval         EFI_STATUS

**/
EFI_STATUS
LoadFormsetList (
  IN   EFI_HII_HANDLE  *HiiHandle,
  OUT  LIST_ENTRY      *FormsetList
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Same as HiiGetRedfishString() but always reads through
  EFI_HII_STRING_PROTOCOL, bypassing the per-formset string cache.  Use this
  for strings that are question *values* (string questions, EFI_IFR_TEXT
  TextTwo), which platform drivers may rewrite at runtime with HiiSetString()
  without any HII database notification.  The cache is only safe for
  metadata (prompts, help, option labels, form titles).

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Release every per-language string cache attached to the formset.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.

**/
VOID
ReleaseHiiStringCacheList (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  );

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
  );

/**
  Release the configure-language index of the formset.

  @param[in]  FormsetPrivate  Pointer to HII form-set private instance.

**/
VOID
ReleaseConfigureLangIndex (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate
  );

/**
  Look up a configure language in the index of one formset and return the
  first statement in load order that carries it and is visible under the
  current REDFISH_PLATFORM_CONFIG_ALLOW_SUPPRESSED policy.

  @param[in]  FormsetPrivate  Formset whose index to search.
  @param[in]  ConfigureLang   Configure language to find.

  @retval Statement pointer, or NULL when no visible statement matches.

**/
REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE *
SearchConfigureLangIndex (
  IN  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *FormsetPrivate,
  IN  EFI_STRING                                ConfigureLang
  );

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
  );

/**
  Release all resource in statement list.

  @param[in]  StatementList   Statement list to be released.

  @retval EFI_SUCCESS             All resource are released.
  @retval EFI_INVALID_PARAMETER   StatementList is NULL.

**/
EFI_STATUS
ReleaseStatementList (
  IN  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST  *StatementList
  );

/**
  Return the HII string length. We don't check word alignment
  of the input string as the same as the checking in StrLen
  function. Because the HII string in the database is compact
  at the byte alignment.

  @param[in]  String  Input UCS format string.

  @retval Length of

**/
UINTN
EFIAPI
HiiStrLen (
  IN  CONST CHAR16  *String
  );

/**
  Return the HII string size. We don't check word alignment
  of the input string as the same as the checking in StrLen
  function. Because the HII string in the database is compact
  at the byte alignment.

  @param[in]  String  Input UCS format string.

  @retval Size of the string.

**/
UINTN
EFIAPI
HiiStrSize (
  IN      CONST CHAR16  *String
  );

/**
  Check if the debug property is enabled or not.

  @param[in]  DebugType  Debug enablement type

  @retval TRUE, the debug property is enabled.
          FALSE, the debug property is not enabled.

**/
BOOLEAN
RedfishPlatformConfigDebugProp (
  IN UINT64  DebugProp
  );

/**
  Check if the Platform Configure feature is enabled or not.

  @param[in]  FeatureType  Redfish platform config feature enablement

  @retval TRUE, the feature is enabled.
          FALSE, the feature is not enabled.

**/
BOOLEAN
RedfishPlatformConfigFeatureProp (
  IN UINT64  FeatureProp
  );
