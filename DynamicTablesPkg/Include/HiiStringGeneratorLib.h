/** @file
  Functions and Data structures for building a HII string package.

  This file contains data structures and function prototypes that
  are needed for generating String package dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <Base.h>
#include <Uefi.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/** A structure for a string to be
    added to the string package.
*/
typedef struct {
  /// A link to another string
  LIST_ENTRY       Link;

  /// String ID of the corresponding
  /// string
  EFI_STRING_ID    StringId;

  /// String to be added to the string
  /// package
  CHAR16           *String;

  /// Combined length of the string and
  /// the EFI_HII_STRING_BLOCK structure
  UINT32           Length;
} DYN_HII_STR_INFO;

/** A structure for keeping information
    specific to a string package.
*/
typedef struct {
  /// List of strings in the form of
  /// DYN_HII_STR_INFO that are associated
  /// with this string package
  LIST_ENTRY       StringList;

  /// Language string ID for this string
  /// package
  EFI_STRING_ID    LanguageId;

  /// Language code string associated with
  /// the string package
  CHAR8            *LanguageCode;

  /// String package buffer
  UINT8            *StrPkgBuf;
} DYN_HII_STR_PKG_INFO;

/** Add a string to the string list.

  Add the string passed to the function to a list of strings that would be
  added to form the string package.

  @param [in]  StrList          String List.
  @param [in]  StringId         String Identifier that gets used in the forms.
  @param [in]  String           The string that is to be added.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrList or String is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddString (
  IN LIST_ENTRY     *StrList,
  IN EFI_STRING_ID  StringId,
  IN CHAR16         *String
  );

/** Generate string package.

  Generate the string package from the input parameter. The string package is
  subsequently added to the HII database through the HiiAddPackages() call.

  @param [in]  StrPkgInfo       The structure that contains all information
                                needed to generate the string package.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_ALREADY_STARTED    If StrPkgBuf is not NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateStringPackage (
  IN  DYN_HII_STR_PKG_INFO  *StrPkgInfo
  );

/** Free all the strings in a string list.

  Iterate through the string list, and free up memory allocated for the
  string and the DYN_HII_STR_INFO structure that contains information about
  that string.

  @param [in]  StringList       The list of strings to be freed.

**/
VOID
EFIAPI
DynHiiFreeStrings (
  IN LIST_ENTRY  *StringList
  );

/** Free up the string package buffer.

  Free up the string package buffer along with any other allocations that
  were done for the string package information structure.

  @param [in]  StrPkgInfo       Pointer to the string package structure.

**/
VOID
EFIAPI
DynHiiFreeStringPackage (
  IN DYN_HII_STR_PKG_INFO  *StrPkgInfo
  );

/** Initialize the string package info structure.

  Initialize the DYN_HII_STR_PKG_INFO structure by allocating memory for
  certain members of the structure and then initializing them with data
  that were passed to the function.

  @param [in]  LanguageCode     LanguageCode string to be added.
  @param [in]  LangStrId        String ID of the Language string that has
                                been added to the string list.
  @param [out] StrPkgInfo       Pointer to the DYN_HII_STR_PKG_INFO structure
                                allocated by this function.

  @retval  EFI_SUCCESS            The structure was initialized successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiInitStringPkgInfo (
  IN  CONST   CHAR8                 *LanguageCode,
  IN  CONST   EFI_STRING_ID         LangStrId,
  OUT         DYN_HII_STR_PKG_INFO  **StrPkgInfo
  );
