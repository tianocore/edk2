/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include "InternalHiiLib.h"

/**
  This function create a new string in String Package or updates an existing
  string in a String Package.  If StringId is 0, then a new string is added to
  a String Package.  If StringId is not zero, then a string in String Package is
  updated.  If SupportedLanguages is NULL, then the string is added or updated
  for all the languages that the String Package supports.  If SupportedLanguages
  is not NULL, then the string is added or updated for the set of languages
  specified by SupportedLanguages.

  If HiiHandle is NULL, then ASSERT().
  If String is NULL, then ASSERT().

  @param[in]  HiiHandle           A handle that was previously registered in the
                                  HII Database.
  @param[in]  StringId            If zero, then a new string is created in the
                                  String Package associated with HiiHandle.  If
                                  non-zero, then the string specified by StringId
                                  is updated in the String Package  associated
                                  with HiiHandle.
  @param[in]  String              A pointer to the Null-terminated Unicode string
                                  to add or update in the String Package associated
                                  with HiiHandle.
  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string of
                                  language codes.  If this parameter is NULL, then
                                  String is added or updated in the String Package
                                  associated with HiiHandle for all the languages
                                  that the String Package supports.  If this
                                  parameter is not NULL, then then String is added
                                  or updated in the String Package associated with
                                  HiiHandle for the set oflanguages specified by
                                  SupportedLanguages.  The format of
                                  SupportedLanguages must follow the language
                                  format assumed the HII Database.

  @retval 0      The string could not be added or updated in the String Package.
  @retval Other  The EFI_STRING_ID of the newly added or updated string.

**/
EFI_STRING_ID
EFIAPI
HiiSetString (
  IN EFI_HII_HANDLE    HiiHandle,
  IN EFI_STRING_ID     StringId,            OPTIONAL
  IN CONST EFI_STRING  String,
  IN CONST CHAR8       *SupportedLanguages  OPTIONAL
  )
{
  EFI_STATUS     Status;
  CHAR8          *AllocatedLanguages;
  CHAR8          *Supported;
  CHAR8          *Language;

  ASSERT (HiiHandle != NULL);
  ASSERT (String != NULL);

  if (SupportedLanguages == NULL) {
    //
    // Retrieve the languages that the package specified by HiiHandle supports
    //
    AllocatedLanguages = HiiGetSupportedLanguages (HiiHandle);
  } else {
    //
    // Allocate a copy of the SupportLanguages string that passed in
    //
    AllocatedLanguages = AllocateCopyPool (AsciiStrSize (SupportedLanguages), SupportedLanguages);
  }

  //
  // If there are not enough resources for the supported languages string, then return a StringId of 0
  //
  if (AllocatedLanguages == NULL) {
    return (EFI_STRING_ID)(0);
  }

  Status = EFI_INVALID_PARAMETER;
  //
  // Loop through each language that the string supports
  //
  for (Supported = AllocatedLanguages; *Supported != '\0'; ) {
    //
    // Cache a pointer to the beginning of the current language in the list of languages
    //
    Language = Supported;

    //
    // Search for the next language separator and replace it with a Null-terminator
    //
    for (; *Supported != 0 && *Supported != ';'; Supported++);
    if (*Supported != 0) {
      *(Supported++) = '\0';
    }

    if ((SupportedLanguages == NULL) && AsciiStrnCmp (Language, UEFI_CONFIG_LANG, AsciiStrLen (UEFI_CONFIG_LANG)) == 0) {
      //
      // Skip string package used for keyword protocol.
      //
      continue;
    }

    //
    // If StringId is 0, then call NewString().  Otherwise, call SetString()
    //
    if (StringId == (EFI_STRING_ID)(0)) {
      Status = gHiiString->NewString (gHiiString, HiiHandle, &StringId, Language, NULL, String, NULL);
    } else {
      Status = gHiiString->SetString (gHiiString, HiiHandle, StringId, Language, String, NULL);
    }

    //
    // If there was an error, then break out of the loop and return a StringId of 0
    //
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  //
  // Free the buffer of supported languages
  //
  FreePool (AllocatedLanguages);

  if (EFI_ERROR (Status)) {
    return (EFI_STRING_ID)(0);
  } else {
    return StringId;
  }
}


/**
  Retrieves a string from a string package names by GUID in a specific language.
  If the language is not specified, then a string from a string package in the
  current platform  language is retrieved.  If the string can not be retrieved
  using the specified language or the current platform language, then the string
  is retrieved from the string package in the first language the string package
  supports.  The returned string is allocated using AllocatePool().  The caller
  is responsible for freeing the allocated buffer using FreePool().

  If PackageListGuid is NULL, then ASSERT().
  If StringId is 0, then ASSERT.

  @param[in]  PackageListGuid  The GUID of a package list that was previously
                               registered in the HII Database.
  @param[in]  StringId         The identifier of the string to retrieved from the
                               string package associated with PackageListGuid.
  @param[in]  Language         The language of the string to retrieve.  If this
                               parameter is NULL, then the current platform
                               language is used.  The format of Language must
                               follow the language format assumed the HII Database.

  @retval NULL   The package list specified by PackageListGuid is not present in the
                 HII Database.
  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
EFIAPI
HiiGetPackageString (
  IN CONST EFI_GUID  *PackageListGuid,
  IN EFI_STRING_ID   StringId,
  IN CONST CHAR8     *Language  OPTIONAL
  )
{
  EFI_HANDLE  *HiiHandleBuffer;
  EFI_HANDLE  HiiHandle;

  ASSERT (PackageListGuid != NULL);

  HiiHandleBuffer = HiiGetHiiHandles (PackageListGuid);
  if (HiiHandleBuffer == NULL) {
    return NULL;
  }

  HiiHandle = HiiHandleBuffer[0];
  FreePool (HiiHandleBuffer);

  return HiiGetString (HiiHandle, StringId, Language);
}

/**
  Retrieves a string from a string package in a specific language.  If the language
  is not specified, then a string from a string package in the current platform
  language is retrieved.  If the string can not be retrieved using the specified
  language or the current platform language, then the string is retrieved from
  the string package in the first language the string package supports.  The
  returned string is allocated using AllocatePool().  The caller is responsible
  for freeing the allocated buffer using FreePool().

  If HiiHandle is NULL, then ASSERT().
  If StringId is 0, then ASSET.

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.
  @param[in]  StringId   The identifier of the string to retrieved from the string
                         package associated with HiiHandle.
  @param[in]  Language   The language of the string to retrieve.  If this parameter
                         is NULL, then the current platform language is used.  The
                         format of Language must follow the language format assumed
                         the HII Database.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
EFIAPI
HiiGetString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId,
  IN CONST CHAR8     *Language  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       StringSize;
  CHAR16      TempString;
  EFI_STRING  String;
  CHAR8       *SupportedLanguages;
  CHAR8       *PlatformLanguage;
  CHAR8       *BestLanguage;

  ASSERT (HiiHandle != NULL);
  ASSERT (StringId != 0);

  //
  // Initialize all allocated buffers to NULL
  //
  SupportedLanguages = NULL;
  PlatformLanguage   = NULL;
  BestLanguage       = NULL;
  String             = NULL;

  //
  // Get the languages that the package specified by HiiHandle supports
  //
  SupportedLanguages = HiiGetSupportedLanguages (HiiHandle);
  if (SupportedLanguages == NULL) {
    goto Error;
  }

  //
  // Get the current platform language setting
  //
  GetEfiGlobalVariable2 (L"PlatformLang", (VOID**)&PlatformLanguage, NULL);

  //
  // If Languag is NULL, then set it to an empty string, so it will be
  // skipped by GetBestLanguage()
  //
  if (Language == NULL) {
    Language = "";
  }

  //
  // Get the best matching language from SupportedLanguages
  //
  BestLanguage = GetBestLanguage (
                   SupportedLanguages,
                   FALSE,                                             // RFC 4646 mode
                   Language,                                          // Highest priority
                   PlatformLanguage != NULL ? PlatformLanguage : "",  // Next highest priority
                   SupportedLanguages,                                // Lowest priority
                   NULL
                   );
  if (BestLanguage == NULL) {
    goto Error;
  }

  //
  // Retrieve the size of the string in the string package for the BestLanguage
  //
  StringSize = 0;
  Status = gHiiString->GetString (
                         gHiiString,
                         BestLanguage,
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
    goto Error;
  }

  //
  // Allocate a buffer for the return string
  //
  String = AllocateZeroPool (StringSize);
  if (String == NULL) {
    goto Error;
  }

  //
  // Retrieve the string from the string package
  //
  Status = gHiiString->GetString (
                         gHiiString,
                         BestLanguage,
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

Error:
  //
  // Free allocated buffers
  //
  if (SupportedLanguages != NULL) {
    FreePool (SupportedLanguages);
  }
  if (PlatformLanguage != NULL) {
    FreePool (PlatformLanguage);
  }
  if (BestLanguage != NULL) {
    FreePool (BestLanguage);
  }

  //
  // Return the Null-terminated Unicode string
  //
  return String;
}

