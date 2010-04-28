/*++

Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UnicodeCollation2.h

Abstract:

  Unicode Collation2 protocol that follows the UEFI 2.0 specification.

--*/

#ifndef _UNICODE_COLLATION2_H_
#define _UNICODE_COLLATION2_H_

#define EFI_UNICODE_COLLATION2_PROTOCOL_GUID \
  { \
    0xa4c751fc, 0x23ae, 0x4c3e, {0x92, 0xe9, 0x49, 0x64, 0xcf, 0x63, 0xf3, 0x49} \
  }

EFI_FORWARD_DECLARATION (EFI_UNICODE_COLLATION2_PROTOCOL);

//
// Protocol data structures and defines
//
#define EFI_UNICODE_BYTE_ORDER_MARK          ((CHAR16)(0xfeff))
#define EFI_UNICODE_BYTE_ORDER_MARK_REVERSE  ((CHAR16)(0xfffe))

//
// Protocol member functions
//
typedef
INTN
(EFIAPI *EFI_UNICODE_COLLATION2_STRICOLL) (
  IN EFI_UNICODE_COLLATION2_PROTOCOL        * This,
  IN CHAR16                                 *Str1,
  IN CHAR16                                 *Str2
  )
/*++

  Routine Description:
    Performs a case-insensitive comparison of two Null-terminated Unicode
    strings.

  Arguments:
    This - Protocol instance pointer.
    Str1 - A pointer to a Null-terminated Unicode string.
    Str2 - A pointer to a Null-terminated Unicode string.

  Returns:
    0   - Str1 is equivalent to Str2
    > 0 - Str1 is lexically greater than Str2
    < 0 - Str1 is lexically less than Str2

--*/
;

typedef
BOOLEAN
(EFIAPI *EFI_UNICODE_COLLATION2_METAIMATCH) (
  IN EFI_UNICODE_COLLATION2_PROTOCOL        * This,
  IN CHAR16                                 *String,
  IN CHAR16                                 *Pattern
  )
/*++

  Routine Description:
    Performs a case-insensitive comparison of a Null-terminated Unicode
    pattern string and a Null-terminated Unicode string.

  Arguments:
    This - Protocol instance pointer.
    String  - A pointer to a Null-terminated Unicode string.
    Pattern - A pointer to a Null-terminated Unicode pattern string.

  Returns:
    TRUE -  Pattern was found in String.
    FALSE - Pattern was not found in String.

--*/
;

typedef
VOID
(EFIAPI *EFI_UNICODE_COLLATION2_STRLWR) (
  IN EFI_UNICODE_COLLATION2_PROTOCOL        * This,
  IN OUT CHAR16                             *Str
  )
/*++

  Routine Description:
    Converts all the Unicode characters in a Null-terminated Unicode string to
    lower case Unicode characters.

  Arguments:
    This - Protocol instance pointer.
    String  - A pointer to a Null-terminated Unicode string.

  Returns:
    NONE

--*/
;

typedef
VOID
(EFIAPI *EFI_UNICODE_COLLATION2_STRUPR) (
  IN EFI_UNICODE_COLLATION2_PROTOCOL        * This,
  IN OUT CHAR16                             *Str
  )
/*++

  Routine Description:
   Converts all the Unicode characters in a Null-terminated Unicode string to upper
   case Unicode characters.

  Arguments:
    This   - Protocol instance pointer.
    String - A pointer to a Null-terminated Unicode string.

  Returns:
    NONE

--*/
;

typedef
VOID
(EFIAPI *EFI_UNICODE_COLLATION2_FATTOSTR) (
  IN EFI_UNICODE_COLLATION2_PROTOCOL        * This,
  IN UINTN                                  FatSize,
  IN CHAR8                                  *Fat,
  OUT CHAR16                                *String
  )
/*++

  Routine Description:
   Converts an 8.3 FAT file name in an OEM character set to a Null-terminated
   Unicode string.

  Arguments:
    This    - Protocol instance pointer.
    FatSize - The size of the string Fat in bytes.
    Fat     - A pointer to a Null-terminated string that contains an 8.3 file
               name using an OEM character set.
    String  - A pointer to a Null-terminated Unicode string. The string must
               be preallocated to hold FatSize Unicode characters.
  Returns:
    NONE

--*/
;

typedef
BOOLEAN
(EFIAPI *EFI_UNICODE_COLLATION2_STRTOFAT) (
  IN EFI_UNICODE_COLLATION2_PROTOCOL        * This,
  IN CHAR16                                 *String,
  IN UINTN                                  FatSize,
  OUT CHAR8                                 *Fat
  )
/*++

  Routine Description:
    Converts a Null-terminated Unicode string to legal characters in a FAT
    filename using an OEM character set.

  Arguments:
    This    - Protocol instance pointer.
    String  - A pointer to a Null-terminated Unicode string. The string must
               be preallocated to hold FatSize Unicode characters.
    FatSize - The size of the string Fat in bytes.
    Fat     - A pointer to a Null-terminated string that contains an 8.3 file
               name using an OEM character set.
  Returns:
    TRUE  - Fat is a Long File Name
    FALSE - Fat is an 8.3 file name

--*/
;

struct _EFI_UNICODE_COLLATION2_PROTOCOL {
  //
  // general
  //
  EFI_UNICODE_COLLATION2_STRICOLL    StriColl;
  EFI_UNICODE_COLLATION2_METAIMATCH  MetaiMatch;
  EFI_UNICODE_COLLATION2_STRLWR      StrLwr;
  EFI_UNICODE_COLLATION2_STRUPR      StrUpr;

  //
  // for supporting fat volumes
  //
  EFI_UNICODE_COLLATION2_FATTOSTR    FatToStr;
  EFI_UNICODE_COLLATION2_STRTOFAT    StrToFat;

  CHAR8                             *SupportedLanguages;
};

extern EFI_GUID gEfiUnicodeCollation2ProtocolGuid;

#endif
