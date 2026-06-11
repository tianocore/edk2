/** @file
  Functions for dynamically building a HII string package.

  This file contains function prototypes that are needed
  for generating String package dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <Base.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/// A generic, opaque handle for HII string objects
typedef VOID *DYN_HII_STR_HANDLE;

/** Add a string to the string list.

  Add the string passed to the function to a list of strings that would be
  added to form the string package.

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.
  @param [in]   Language          Language for which string is to be added.
  @param [in]   String            The string that is to be added.
  @param [out]  StringId          String Identifier assigned for this string.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrList or String is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddString (
  IN  CONST DYN_HII_STR_HANDLE  StrPkgHandle,
  IN  CONST CHAR8               *Language,
  IN  CONST EFI_STRING          String,
  OUT       EFI_STRING_ID       *StringId
  );

/** Generate string package.

  Generate the string package from the input parameter. The string package is
  subsequently added to the HII database through the HiiAddPackages() call.

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.
  @param [out]  StrPkgData        The generated array of string package(s).

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_ALREADY_STARTED    If StrPkgBuf is not NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateStringPackage (
  IN   DYN_HII_STR_HANDLE  StrPkgHandle,
  OUT  UINT8               **StrPkgData
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

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.

**/
VOID
EFIAPI
DynHiiFreeStringPackage (
  IN   DYN_HII_STR_HANDLE  StrPkgHandle
  );

/** Initialize the string package structure.

  Initialize the DYN_HII_STR_PKG structure by allocating memory for
  certain members of the structure and then initializing them with data
  that were passed to the function.

  @param [in]  StrPkgArr        Existing string package array which might
                                consist of multiple language string arrays
  @param [out] StrPkgHandle     Pointer to the DYN_HII_STR_HANDLE to hold
                                the string package structure.

  @retval  EFI_SUCCESS            The structure was initialized successfully.
  @retval  EFI_INVALID_PARAMETER  The StrPkgInfo or LanguageCode is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiInitStringPkg (
  IN  CONST   UINT8               *StrPkgArr,
  OUT         DYN_HII_STR_HANDLE  *StrPkgHandle
  );
