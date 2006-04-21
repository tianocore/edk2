/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  UnicodeCollationEng.c

Abstract:
  
  Unicode Collation Protocol (English)

Revision History

--*/

#include "UnicodeCollationEng.h"

CHAR8 mEngUpperMap[0x100];
CHAR8 mEngLowerMap[0x100];
CHAR8 mEngInfoMap[0x100];

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

EFI_UNICODE_COLLATION_PROTOCOL  UnicodeEng = {
  EngStriColl,
  EngMetaiMatch,
  EngStrLwr,
  EngStrUpr,
  EngFatToStr,
  EngStrToFat,
  "eng"
};

//
//
//
EFI_STATUS
InitializeUnicodeCollationEng (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
/*++

Routine Description:

  Initializes the Unicode Collation Driver

Arguments:

  ImageHandle -

  SystemTable -
  
Returns:

  EFI_SUCCESS
  EFI_OUT_OF_RESOURCES

--*/
{
  UINTN       Index;
  UINTN       Index2;

  //
  // Initialize mapping tables for the supported languages
  //
  for (Index = 0; Index < 0x100; Index++) {
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

  for (Index = 0; mOtherChars[Index]; Index++) {
    Index2 = mOtherChars[Index];
    mEngInfoMap[Index2] |= CHAR_FAT_VALID;
  }
  //
  // Create a handle for the device
  //
  return gBS->InstallProtocolInterface (
                &mHandle,
                &gEfiUnicodeCollationProtocolGuid,
                EFI_NATIVE_INTERFACE,
                &UnicodeEng
                );
}

INTN
EFIAPI
EngStriColl (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *s1,
  IN CHAR16                           *s2
  )
/*++

Routine Description:

  Performs a case-insensitive comparison of two Null-terminated Unicode strings.
  
Arguments:

  This
  s1
  s2
  
Returns:

--*/
{
  while (*s1) {
    if (ToUpper (*s1) != ToUpper (*s2)) {
      break;
    }

    s1 += 1;
    s2 += 1;
  }

  return ToUpper (*s1) - ToUpper (*s2);
}

VOID
EFIAPI
EngStrLwr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
/*++

Routine Description:

  Converts all the Unicode characters in a Null-terminated Unicode string
  to lower case Unicode characters.

Arguments:

  This - A pointer to the EFI_UNICODE_COLLATION_PROTOCOL instance.
  Str1  - A pointer to a Null-terminated Unicode string.
  Str2  - A pointer to a Null-terminated Unicode string.

Returns:

  0   - s1 is equivalent to s2.
  > 0 - s1 is lexically greater than s2.
  < 0 - s1 is lexically less than s2.

--*/
{
  while (*Str) {
    *Str = ToLower (*Str);
    Str += 1;
  }
}

VOID
EFIAPI
EngStrUpr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN OUT CHAR16                       *Str
  )
/*++

Routine Description:

  Converts all the Unicode characters in a Null-terminated 
  Unicode string to upper case Unicode characters.

Arguments:
  This
  Str

Returns:
  None
  
--*/
{
  while (*Str) {
    *Str = ToUpper (*Str);
    Str += 1;
  }
}

BOOLEAN
EFIAPI
EngMetaiMatch (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  )
/*++

Routine Description:

  Performs a case-insensitive comparison between a Null-terminated
  Unicode pattern string and a Null-terminated Unicode string. 

  The pattern string can use the '?' wildcard to match any character, 
  and the '*' wildcard to match any sub-string.

Arguments:

  This     - A pointer to the EFI_UNICODE_COLLATION_PROTOCOL instance.
  String   - A pointer to a Null-terminated Unicode string.
  Pattern  - A pointer to a Null-terminated Unicode pattern string.

Returns:

  TRUE  - Pattern was found in String.
  FALSE - Pattern was not found in String.

--*/
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
      if (*String) {
        return FALSE;
      } else {
        return TRUE;
      }

    case '*':
      //
      // Match zero or more chars
      //
      while (*String) {
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
      if (!*String) {
        return FALSE;
      }

      String += 1;
      break;

    case '[':
      //
      // Match char set
      //
      CharC = *String;
      if (!CharC) {
        //
        // syntax problem
        //
        return FALSE;
      }

      Index3  = 0;
      CharP   = *Pattern++;
      while (CharP) {
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

          if (ToUpper (CharC) >= ToUpper (Index3) && ToUpper (CharC) <= ToUpper (CharP)) {
            //
            // if in range, it's a match
            //
            break;
          }
        }

        Index3 = CharP;
        if (ToUpper (CharC) == ToUpper (CharP)) {
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
      while (CharP && CharP != ']') {
        CharP = *Pattern;
        Pattern += 1;
      }

      String += 1;
      break;

    default:
      CharC = *String;
      if (ToUpper (CharC) != ToUpper (CharP)) {
        return FALSE;
      }

      String += 1;
      break;
    }
  }
}

VOID
EFIAPI
EngFatToStr (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN UINTN                            FatSize,
  IN CHAR8                            *Fat,
  OUT CHAR16                          *String
  )
/*++

Routine Description:

  Converts an 8.3 FAT file name using an OEM character set 
  to a Null-terminated Unicode string.

  BUGBUG: Function has to expand DBCS FAT chars, currently not.

Arguments:
  This
  FatSize
  Fat
  String
  
Returns:

--*/
{
  //
  // No DBCS issues, just expand and add null terminate to end of string
  //
  while (*Fat && FatSize) {
    *String = *Fat;
    String += 1;
    Fat += 1;
    FatSize -= 1;
  }

  *String = 0;
}

BOOLEAN
EFIAPI
EngStrToFat (
  IN EFI_UNICODE_COLLATION_PROTOCOL   *This,
  IN CHAR16                           *String,
  IN UINTN                            FatSize,
  OUT CHAR8                           *Fat
  )
/*++

Routine Description:

  Converts a Null-terminated Unicode string to legal characters 
  in a FAT filename using an OEM character set.

  Functions has to crunch string to a fat string. Replacing
  any chars that can't be represented in the fat name.

Arguments:
  This
  String
  FatSize
  Fat

Returns:
  TRUE
  FALSE
--*/
{
  BOOLEAN SpecialCharExist;

  SpecialCharExist = FALSE;
  while (*String && FatSize) {
    //
    // Skip '.' or ' ' when making a fat name
    //
    if (*String != '.' && *String != ' ') {
      //
      // If this is a valid fat char, move it.
      // Otherwise, move a '_' and flag the fact that the name needs an Lfn
      //
      if (*String < 0x100 && (mEngInfoMap[*String] & CHAR_FAT_VALID)) {
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
