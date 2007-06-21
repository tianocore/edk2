/** @file
  Unicode Collation protocol that follows the EFI 1.0 specification.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  UnicodeCollation.h

**/

#ifndef __UNICODE_COLLATION_H__
#define __UNICODE_COLLATION_H__

#define EFI_UNICODE_COLLATION_PROTOCOL_GUID \
  { \
    0x1d85cd7f, 0xf43d, 0x11d2, {0x9a, 0xc, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

typedef struct _EFI_UNICODE_COLLATION_PROTOCOL  EFI_UNICODE_COLLATION_PROTOCOL;


//
// Protocol GUID name defined in EFI1.1.
// 
#define UNICODE_COLLATION_PROTOCOL              EFI_UNICODE_COLLATION_PROTOCOL_GUID

//
// Protocol defined in EFI1.1.
// 
typedef EFI_UNICODE_COLLATION_PROTOCOL          UNICODE_COLLATION_INTERFACE;

//
// Protocol data structures and defines
//
#define EFI_UNICODE_BYTE_ORDER_MARK (CHAR16) (0xfeff)

//
// Protocol member functions
//
/**
  Performs a case-insensitive comparison of two Null-terminated Unicode 
  strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated Unicode string.
  @param  Str2 A pointer to a Null-terminated Unicode string.

  @retval 0   Str1 is equivalent to Str2
  @retval >_0 Str1 is lexically greater than Str2
  @retval <_0 Str1 is lexically less than Str2

**/
typedef
INTN
(EFIAPI *EFI_UNICODE_COLLATION_STRICOLL) (
  IN EFI_UNICODE_COLLATION_PROTOCOL         *This,
  IN CHAR16                                 *Str1,
  IN CHAR16                                 *Str2
  )
;

/**
  Performs a case-insensitive comparison of a Null-terminated Unicode 
  pattern string and a Null-terminated Unicode string.

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated Unicode string.
  @param  Pattern A pointer to a Null-terminated Unicode pattern string.

  @retval TRUE    Pattern was found in String.
  @retval FALSE   Pattern was not found in String.

**/
typedef
BOOLEAN
(EFIAPI *EFI_UNICODE_COLLATION_METAIMATCH) (
  IN EFI_UNICODE_COLLATION_PROTOCOL         *This,
  IN CHAR16                                 *String,
  IN CHAR16                                 *Pattern
  )
;

/**
  Converts all the Unicode characters in a Null-terminated Unicode string to 
  lower case Unicode characters.

  @param  This   Protocol instance pointer.
  @param  String A pointer to a Null-terminated Unicode string.

  NONE

**/
typedef
VOID
(EFIAPI *EFI_UNICODE_COLLATION_STRLWR) (
  IN EFI_UNICODE_COLLATION_PROTOCOL         *This,
  IN OUT CHAR16                             *Str
  )
;

/**
  Converts all the Unicode characters in a Null-terminated Unicode string to upper
  case Unicode characters.

  @param  This   Protocol instance pointer.
  @param  String A pointer to a Null-terminated Unicode string.

  NONE

**/
typedef
VOID
(EFIAPI *EFI_UNICODE_COLLATION_STRUPR) (
  IN EFI_UNICODE_COLLATION_PROTOCOL         *This,
  IN OUT CHAR16                             *Str
  )
;

/**
  Converts an 8.3 FAT file name in an OEM character set to a Null-terminated 
  Unicode string.

  @param  This    Protocol instance pointer.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.
  @param  String  A pointer to a Null-terminated Unicode string. The string must
                  be preallocated to hold FatSize Unicode characters.

  NONE

**/
typedef
VOID
(EFIAPI *EFI_UNICODE_COLLATION_FATTOSTR) (
  IN EFI_UNICODE_COLLATION_PROTOCOL         *This,
  IN UINTN                                  FatSize,
  IN CHAR8                                  *Fat,
  OUT CHAR16                                *String
  )
;

/**
  Converts a Null-terminated Unicode string to legal characters in a FAT 
  filename using an OEM character set. 

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated Unicode string. The string must
                  be preallocated to hold FatSize Unicode characters.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.

  @retval TRUE    Fat is a Long File Name
  @retval FALSE   Fat is an 8.3 file name

**/
typedef
BOOLEAN
(EFIAPI *EFI_UNICODE_COLLATION_STRTOFAT) (
  IN EFI_UNICODE_COLLATION_PROTOCOL         *This,
  IN CHAR16                                 *String,
  IN UINTN                                  FatSize,
  OUT CHAR8                                 *Fat
  )
;

struct _EFI_UNICODE_COLLATION_PROTOCOL {
  //
  // general
  //
  EFI_UNICODE_COLLATION_STRICOLL    StriColl;
  EFI_UNICODE_COLLATION_METAIMATCH  MetaiMatch;
  EFI_UNICODE_COLLATION_STRLWR      StrLwr;
  EFI_UNICODE_COLLATION_STRUPR      StrUpr;

  //
  // for supporting fat volumes
  //
  EFI_UNICODE_COLLATION_FATTOSTR    FatToStr;
  EFI_UNICODE_COLLATION_STRTOFAT    StrToFat;

  CHAR8                             *SupportedLanguages;
};

extern EFI_GUID gEfiUnicodeCollationProtocolGuid;

#endif
