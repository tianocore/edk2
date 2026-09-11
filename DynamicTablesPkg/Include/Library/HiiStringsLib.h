/** @file
  Functions for dynamically building a HII string package.

  This file contains function prototypes that are needed
  for generating String package dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure

  @par Reference(s):
  - UEFI specification 2.11, section 33.3.6 String Package

**/

#pragma once

#include <Base.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/// A generic, opaque handle for HII string objects
typedef VOID *DYN_HII_STR_HANDLE;

/** Structure used for adding a string to a string
    package.
*/
typedef struct {
  /// The language code for which the
  /// string is to be added
  CHAR8         *LanguageCode;

  /// The actual string to be added to
  /// the string package
  EFI_STRING    String;
} DYN_HII_STR_ENTRY;

/** Add a string to the string list.

  Add the string(s) passed to the function to a list of strings that would
  be added to form the string package. Each index of StrTable is a pair of
  language-code and the corresponding string. If there are multiple
  languages that are supported, strings for those languages can be added.
  Alternatively, it is also possible to not add a string for a supported
  language, in which case the string is left blank.

  @param [in]   StrPkgHandle      Handle to the top-level string package
                                  information structure.
  @param [in]   StrTable          An array of entries of languagecode and
                                  string pair to be added.
  @param [in]   StrCount          Number of entries of StrTable.
  @param [out]  StringId          String Identifier assigned for this set
                                  of strings.

  @retval  EFI_SUCCESS            The string was added to the list successfully.
  @retval  EFI_INVALID_PARAMETER  The StrList or String is NULL.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddString (
  IN  CONST DYN_HII_STR_HANDLE  StrPkgHandle,
  IN  CONST DYN_HII_STR_ENTRY   *StrTable,
  IN        UINT32              StrCount,
  OUT       EFI_STRING_ID       *StringId
  );

/** Generate string package.

  Generate the string package from the input parameter. The string package is
  subsequently added to the HII database through the HiiAddPackages() call.

  The serialized string package buffer is passed back to the caller in
  StrPkgData. It is the caller's responsibility to free up the string package
  buffer through a call to FreePool().

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
