/** @file
  Driver to implement English version of Unicode Collation Protocol.

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "UnicodeCollationEng.h"

CHAR8 mEngUpperMap[MAP_TABLE_SIZE];
CHAR8 mEngLowerMap[MAP_TABLE_SIZE];
CHAR8 mEngInfoMap[MAP_TABLE_SIZE];

CHAR8 mOtherChars[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  '\\',
  '.',
  '_',
  '^',
  '$',
  '~',
  '!',
  '#',
  '%',
  '&',
  '-',
  '{',
  '}',
  '(',
  ')',
  '@',
  '`',
  '\'',
  '\0'
};

EFI_HANDLE  mHandle = NULL;

//
// EFI Unicode Collation Protocol supporting ISO 639-2 language code
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_COLLATION_PROTOCOL  UnicodeEng = {
  EngStriColl,
  EngMetaiMatch,
  EngStrLwr,
  EngStrUpr,
  EngFatToStr,
  EngStrToFat,
  "eng"
};

//
// EFI Unicode Collation2 Protocol supporting RFC 4646 language code
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_COLLATION_PROTOCOL  Unicode2Eng = {
  EngStriColl,
  EngMetaiMatch,
  EngStrLwr,
  EngStrUpr,
  EngFatToStr,
  EngStrToFat,
  "en"
};

/**
  The user Entry Point for English module.
 
  This function initializes unicode character mapping and then installs Unicode
  Collation & Unicode Collation 2 Protocols based on the feature flags.  

  @param  ImageHandle    The firmware allocated handle for the EFI image.  
  @param  SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS    The entry point is executed successfully.
  @retval other          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeUnicodeCollationEng (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       Index2;

  //
  // Initialize mapping tables for the supported languages
  //
  for (Index = 0; Index < MAP_TABLE_SIZE; Index++) {
    mEngUpperMap[Index] = (CHAR8) Index;
    mEngLowerMap[Index] = (CHAR8) Index;
    mEngInfoMap[Index]  = 0;

    if ((Index >= 'a' && Index <= 'z') || (Index >= 0xe0 && Index <= 0xf6) || (Index >= 0xf8 && Index <= 0xfe)) {

      Index2                = Index - 0x20;
      mEngUpperMap[Index]   = (CHAR8) Index2;
      mEngLowerMap[Index2]  = (CHAR8) Index;

      mEngInfoMap[Index] |= CHAR_FAT_VALID;
      mEngInfoMap[Index2] |= CHAR_FAT_VALID;
    }
  }

  for (Index = 0; mOtherChars[Index] != 0; Index++) {
    Index2 = mOtherChars[Index];
    mEngInfoMap[Index2] |= CHAR_FAT_VALID;
  }

  if (FeaturePcdGet (PcdUnicodeCollation2Support)) {
    if (FeaturePcdGet (PcdUnicodeCollationSupport)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mHandle,
                      &gEfiUnicodeCollationProtocolGuid,
                      &UnicodeEng,
                      &gEfiUnicodeCollation2ProtocolGuid,
                      &Unicode2Eng,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
    } else {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mHandle,
                      &gEfiUnicodeCollation2ProtocolGuid,
                      &Unicode2Eng,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
    }
  } else {
    if (FeaturePcdGet (PcdUnicodeCollationSupport)) {
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &mHandle,
                      &gEfiUnicodeCollationProtocolGuid,
                      &UnicodeEng,
                      NULL
                      );
      ASSERT_EFI_ERROR (Status);
    } else {
      //
      // This module must support to produce at least one of Unicode Collation Protocol
      // and Unicode Collation 2 Protocol.
      //
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
    }
  }

  return Status;
}


/**
  Performs a case-insensitive comparison of two Null-terminated strings.

  @param  This Protocol instance pointer.
  @param  Str1 A pointer to a Null-terminated string.
  @param  Str2 A pointer to a Null-terminated string.

  @retval 0   Str1 is equivalent to Str2
  @retval > 0 Str1 is lexically greater than Str2
  @retval < 0 Str1 is lexically less than Str2

**/
INTN
EFIAPI
EngStriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *Str1,
  IN CHAR16                           *Str2
  )
{
  while (*Str1 != 0) {
    if (TO_UPPER (*Str1) != TO_UPPER (*Str2)) {
      break;
    }

    Str1 += 1;
    Str2 += 1;
  }

  return TO_UPPER (*Str1) - TO_UPPER (*Str2);
}


/**
  Converts all the characters in a Null-terminated string to 
  lower case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
EngStrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
{
  while (*Str != 0) {
    *Str = TO_LOWER (*Str);
    Str += 1;
  }
}


/**
  Converts all the characters in a Null-terminated string to upper
  case characters.

  @param  This   Protocol instance pointer.
  @param  Str    A pointer to a Null-terminated string.

**/
VOID
EFIAPI
EngStrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
{
  while (*Str != 0) {
    *Str = TO_UPPER (*Str);
    Str += 1;
  }
}

/**
  Performs a case-insensitive comparison of a Null-terminated
  pattern string and a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string.
  @param  Pattern A pointer to a Null-terminated pattern string.

  @retval TRUE    Pattern was found in String.
  @retval FALSE   Pattern was not found in String.

**/
BOOLEAN
EFIAPI
EngMetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  )
{
  CHAR16  CharC;
  CHAR16  CharP;
  CHAR16  Index3;

  for (;;) {
    CharP = *Pattern;
    Pattern += 1;

    switch (CharP) {
    case 0:
      //
      // End of pattern.  If end of string, TRUE match
      //
      if (*String != 0) {
        return FALSE;
      } else {
        return TRUE;
      }

    case '*':
      //
      // Match zero or more chars
      //
      while (*String != 0) {
        if (EngMetaiMatch (This, String, Pattern)) {
          return TRUE;
        }

        String += 1;
      }

      return EngMetaiMatch (This, String, Pattern);

    case '?':
      //
      // Match any one char
      //
      if (*String == 0) {
        return FALSE;
      }

      String += 1;
      break;

    case '[':
      //
      // Match char set
      //
      CharC = *String;
      if (CharC == 0) {
        //
        // syntax problem
        //
        return FALSE;
      }

      Index3  = 0;
      CharP   = *Pattern++;
      while (CharP != 0) {
        if (CharP == ']') {
          return FALSE;
        }

        if (CharP == '-') {
          //
          // if range of chars, get high range
          //
          CharP = *Pattern;
          if (CharP == 0 || CharP == ']') {
            //
            // syntax problem
            //
            return FALSE;
          }

          if (TO_UPPER (CharC) >= TO_UPPER (Index3) && TO_UPPER (CharC) <= TO_UPPER (CharP)) {
            //
            // if in range, it's a match
            //
            break;
          }
        }

        Index3 = CharP;
        if (TO_UPPER (CharC) == TO_UPPER (CharP)) {
          //
          // if char matches
          //
          break;
        }

        CharP = *Pattern++;
      }
      //
      // skip to end of match char set
      //
      while ((CharP != 0) && (CharP != ']')) {
        CharP = *Pattern;
        Pattern += 1;
      }

      String += 1;
      break;

    default:
      CharC = *String;
      if (TO_UPPER (CharC) != TO_UPPER (CharP)) {
        return FALSE;
      }

      String += 1;
      break;
    }
  }
}


/**
  Converts an 8.3 FAT file name in an OEM character set to a Null-terminated string.

  @param  This    Protocol instance pointer.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an 8-bit OEM character set.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.

**/
VOID
EFIAPI
EngFatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN UINTN                            FatSize,
  IN CHAR8                            *Fat,
  OUT CHAR16                          *String
  )
{
  //
  // No DBCS issues, just expand and add null terminate to end of string
  //
  while ((*Fat != 0) && (FatSize != 0)) {
    *String = *Fat;
    String += 1;
    Fat += 1;
    FatSize -= 1;
  }

  *String = 0;
}


/**
  Converts a Null-terminated string to legal characters in a FAT 
  filename using an OEM character set. 

  @param  This    Protocol instance pointer.
  @param  String  A pointer to a Null-terminated string. The string must
                  be preallocated to hold FatSize characters.
  @param  FatSize The size of the string Fat in bytes.
  @param  Fat     A pointer to a Null-terminated string that contains an 8.3 file
                  name using an OEM character set.

  @retval TRUE    Fat is a Long File Name
  @retval FALSE   Fat is an 8.3 file name

**/
BOOLEAN
EFIAPI
EngStrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN UINTN                            FatSize,
  OUT CHAR8                           *Fat
  )
{
  BOOLEAN SpecialCharExist;

  SpecialCharExist = FALSE;
  while ((*String != 0) && (FatSize != 0)) {
    //
    // Skip '.' or ' ' when making a fat name
    //
    if (*String != '.' && *String != ' ') {
      //
      // If this is a valid fat char, move it.
      // Otherwise, move a '_' and flag the fact that the name needs a long file name.
      //
      if (*String < MAP_TABLE_SIZE && ((mEngInfoMap[*String] & CHAR_FAT_VALID) != 0)) {
        *Fat = mEngUpperMap[*String];
      } else {
        *Fat              = '_';
        SpecialCharExist  = TRUE;
      }

      Fat += 1;
      FatSize -= 1;
    }

    String += 1;
  }
  //
  // Do not terminate that fat string
  //
  return SpecialCharExist;
}
