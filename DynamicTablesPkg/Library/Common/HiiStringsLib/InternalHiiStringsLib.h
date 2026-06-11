/** @file
  Data structures for dynamically building a HII string package.

  This file contains data structures that are needed for
  generating string package dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <Base.h>
#include <Library/HiiStringsLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/// Signature for DYN_HII_STR_PKG
#define DYN_HII_STR_PKG_SIGNATURE  SIGNATURE_32 ('D', 'H', 'S', 'P')

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
  /// A link to another language package
  LIST_ENTRY                    Link;

  /// List of strings in the form of
  /// DYN_HII_STR_INFO that are associated
  /// with this string package
  LIST_ENTRY                    StringList;

  /// The package header for this string
  /// package
  EFI_HII_STRING_PACKAGE_HDR    *StrPkgHdr;

  /// Language code string associated with
  /// the string package. The StringId for
  /// the language string will always be 1
  CHAR8                         *LanguageCode;

  /// Max string ID for this package
  EFI_STRING_ID                 MaxStringId;
} DYN_HII_STR_PKG_INSTANCE;

typedef struct {
  /// Signature for the structure
  UINT32        Signature;

  /// A list of string packages, corresponding to
  /// the DYN_HII_STR_PKG_INSTANCE structure.
  LIST_ENTRY    StrPkgList;

  /// Number of languages that are part of the
  /// string package array.
  UINT32        LanguageCount;
} DYN_HII_STR_PKG;
