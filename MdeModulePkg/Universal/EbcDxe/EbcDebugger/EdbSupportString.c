/** @file

Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "Edb.h"

/**

  Convert hex string to uint.

  @param  Str  -  The string

**/
UINTN
EFIAPI
Xtoi (
  CHAR16  *Str
  )
{
  UINTN   RetVal;
  CHAR16  TempChar;
  UINTN   MaxVal;

  ASSERT (Str != NULL);

  MaxVal = (UINTN) -1 >> 4;
  //
  // skip preceeding white space
  //
  while (*Str != '\0' && *Str == ' ') {
    Str += 1;
  }
  //
  // skip preceeding zeros
  //
  while (*Str != '\0' && *Str == '0') {
    Str += 1;
  }
  //
  // skip preceeding white space
  //
  if (*Str != '\0' && (*Str == 'x' || *Str == 'X')) {
    Str += 1;
  }
  //
  // convert hex digits
  //
  RetVal = 0;
  TempChar = *(Str++);
  while (TempChar != '\0') {
    if (TempChar >= 'a' && TempChar <= 'f') {
      TempChar -= 'a' - 'A';
    }

    if ((TempChar >= '0' && TempChar <= '9') || (TempChar >= 'A' && TempChar <= 'F')) {
      if (RetVal > MaxVal) {
        return (UINTN) -1;
      }

      RetVal = (RetVal << 4) | (TempChar - (TempChar >= 'A' ? 'A' - 10 : '0'));
    } else {
      break;
    }

    TempChar = *(Str++);
  }

  return RetVal;
}

/**

  Convert hex string to uint.

  @param  Str  -  The string

**/
UINT64
EFIAPI
LXtoi (
  CHAR16  *Str
  )
{
  UINT64  RetVal;
  CHAR16  TempChar;
  UINT64  MaxVal;

  ASSERT (Str != NULL);

  MaxVal = RShiftU64 ((UINT64) -1, 4);
  //
  // skip preceeding white space
  //
  while (*Str != '\0' && *Str == ' ') {
    Str += 1;
  }
  //
  // skip preceeding zeros
  //
  while (*Str != '\0' && *Str == '0') {
    Str += 1;
  }
  //
  // skip preceeding white space
  //
  if (*Str != '\0' && (*Str == 'x' || *Str == 'X')) {
    Str += 1;
  }
  //
  // convert hex digits
  //
  RetVal = 0;
  TempChar = *(Str++);
  while (TempChar != '\0') {
    if (TempChar >= 'a' && TempChar <= 'f') {
      TempChar -= 'a' - 'A';
    }

    if ((TempChar >= '0' && TempChar <= '9') || (TempChar >= 'A' && TempChar <= 'F')) {
      if (RetVal > MaxVal) {
        return (UINT64) -1;
      }

      RetVal = LShiftU64 (RetVal, 4);
      RetVal = RetVal + (TempChar - (TempChar >= 'A' ? 'A' - 10 : '0'));
    } else {
      break;
    }

    TempChar = *(Str++);
  }

  return RetVal;
}

/**

  Convert hex string to uint.

  @param Str  -  The string

**/
UINTN
EFIAPI
Atoi (
  CHAR16  *Str
  )
{
  UINTN   RetVal;
  CHAR16  TempChar;
  UINTN   MaxVal;
  UINTN   ResteVal;

  ASSERT (Str != NULL);

  MaxVal = (UINTN) -1 / 10;
  ResteVal = (UINTN) -1 % 10;
  //
  // skip preceeding white space
  //
  while (*Str != '\0' && *Str == ' ') {
    Str += 1;
  }
  //
  // convert digits
  //
  RetVal = 0;
  TempChar = *(Str++);
  while (TempChar != '\0') {
    if (TempChar >= '0' && TempChar <= '9') {
      if (RetVal > MaxVal || (RetVal == MaxVal && TempChar - '0' > (INTN) ResteVal)) {
        return (UINTN) -1;
      }

      RetVal = (RetVal * 10) + TempChar - '0';
    } else {
      break;
    }

    TempChar = *(Str++);
  }

  return RetVal;
}

/**

  Convert hex string to uint.

  @param  Str  -  The string

**/
UINTN
EFIAPI
AsciiXtoi (
  CHAR8  *Str
  )
{
  UINTN   RetVal;
  CHAR8   TempChar;
  UINTN   MaxVal;

  ASSERT (Str != NULL);

  MaxVal = (UINTN) -1 >> 4;
  //
  // skip preceeding white space
  //
  while (*Str != '\0' && *Str == ' ') {
    Str += 1;
  }
  //
  // skip preceeding zeros
  //
  while (*Str != '\0' && *Str == '0') {
    Str += 1;
  }
  //
  // skip preceeding white space
  //
  if (*Str != '\0' && (*Str == 'x' || *Str == 'X')) {
    Str += 1;
  }
  //
  // convert hex digits
  //
  RetVal = 0;
  TempChar = *(Str++);
  while (TempChar != '\0') {
    if (TempChar >= 'a' && TempChar <= 'f') {
      TempChar -= 'a' - 'A';
    }

    if ((TempChar >= '0' && TempChar <= '9') || (TempChar >= 'A' && TempChar <= 'F')) {
      if (RetVal > MaxVal) {
        return (UINTN) -1;
      }

      RetVal = (RetVal << 4) | (TempChar - (TempChar >= 'A' ? 'A' - 10 : '0'));
    } else {
      break;
    }

    TempChar = *(Str++);
  }

  return RetVal;
}

/**

  Convert hex string to uint.

  @param Str  -  The string

**/
UINTN
EFIAPI
AsciiAtoi (
  CHAR8  *Str
  )
{
  UINTN   RetVal;
  CHAR8   TempChar;
  UINTN   MaxVal;
  UINTN   ResteVal;

  ASSERT (Str != NULL);

  MaxVal = (UINTN) -1 / 10;
  ResteVal = (UINTN) -1 % 10;
  //
  // skip preceeding white space
  //
  while (*Str != '\0' && *Str == ' ') {
    Str += 1;
  }
  //
  // convert digits
  //
  RetVal = 0;
  TempChar = *(Str++);
  while (TempChar != '\0') {
    if (TempChar >= '0' && TempChar <= '9') {
      if (RetVal > MaxVal || (RetVal == MaxVal && TempChar - '0' > (INTN) ResteVal)) {
        return (UINTN) -1;
      }

      RetVal = (RetVal * 10) + TempChar - '0';
    } else {
      break;
    }

    TempChar = *(Str++);
  }

  return RetVal;
}

/**

  Convert the character to upper case.

  @param  Chr    the character to be converted.

**/
STATIC
CHAR16
UnicodeToUpper (
  IN      CHAR16                    Chr
  )
{
  return (Chr >= L'a' && Chr <= L'z') ? Chr - (L'a' - L'A') : Chr;
}

/**

  Convert the character to upper case.

  @param  Chr    the character to be converted.

**/
STATIC
CHAR8
AsciiToUpper (
  IN      CHAR8                     Chr
  )
{
  return (Chr >= 'a' && Chr <= 'z') ? Chr - ('a' - 'A') : Chr;
}

/**
  Compare the Unicode and Ascii string pointed by String to the string pointed by String2.

  @param String - Unicode String to process

  @param String2 - Ascii string to process

  @return Return a positive integer if String is lexicall greater than String2; Zero if
  the two strings are identical; and a negative interger if String is lexically
  less than String2.

**/
INTN
EFIAPI
StrCmpUnicodeAndAscii (
  IN CHAR16   *String,
  IN CHAR8    *String2
  )
{
  while (*String != '\0') {
    if (*String != (CHAR16)*String2) {
      break;
    }

    String += 1;
    String2 += 1;
  }

  return (*String - (CHAR16)*String2);
}

/**

  Compare the Unicode string pointed by String to the string pointed by String2.

  @param  String - Unicode String to process
  @param  String2 - Unicode string to process

  @return Return a positive integer if String is lexically greater than String2; Zero if
  the two strings are identical; and a negative integer if String is lexically
  less than String2.

**/
INTN
EFIAPI
StriCmp (
  IN CHAR16   *String,
  IN CHAR16   *String2
  )
{
  while ((*String != L'\0') &&
         (UnicodeToUpper (*String) == UnicodeToUpper (*String2))) {
    String++;
    String2++;
  }

  return UnicodeToUpper (*String) - UnicodeToUpper (*String2);
}

/**

  Compare the Unicode and Ascii string pointed by String to the string pointed by String2.

  @param  String - Unicode String to process
  @param  String2 - Ascii string to process

  @return Return a positive integer if String is lexically greater than String2; Zero if
  the two strings are identical; and a negative integer if String is lexically
  less than String2.

**/
INTN
EFIAPI
StriCmpUnicodeAndAscii (
  IN CHAR16   *String,
  IN CHAR8    *String2
  )
{
  while ((*String != L'\0') &&
         (UnicodeToUpper (*String) == (CHAR16)AsciiToUpper (*String2))) {
    String++;
    String2++;
  }

  return UnicodeToUpper (*String) - (CHAR16)AsciiToUpper (*String2);
}

/**

  Verify if the string is end with the sub string.

  @param  Str - The string where to search the sub string
  @param  SubStr - The substring.

**/
BOOLEAN
EFIAPI
StrEndWith (
  IN CHAR16                       *Str,
  IN CHAR16                       *SubStr
  )
{
  CHAR16  *Temp;

  if ((Str == NULL) || (SubStr == NULL) || (StrLen(Str) < StrLen(SubStr))) {
    return FALSE;
  }

  Temp = Str + StrLen(Str) - StrLen(SubStr);

  //
  // Compare
  //
  if (StriCmp (Temp, SubStr) == 0) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Duplicate a string.

  @param  Src  The string to be duplicated.

**/
CHAR16 *
EFIAPI
StrDuplicate (
  IN CHAR16   *Src
  )
{
  CHAR16      *Dest;
  UINTN       Size;

  Size = (StrLen(Src) + 1) * sizeof(CHAR16);
  Dest = AllocateZeroPool (Size);
  if (Dest != NULL) {
    CopyMem (Dest, Src, Size);
  }
  return Dest;
}


CHAR16  *mLineBuffer          = NULL;
CHAR16  *mFieldBuffer         = NULL;

/**

  Find the first substring.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
UINTN
EFIAPI
StrSpn (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  )
{
  UINTN   Count;
  CHAR16  *Str1;
  CHAR16  *Str2;

  Count = 0;

  for (Str1 = String; *Str1 != L'\0'; Str1 ++) {
    for (Str2 = CharSet; *Str2 != L'\0'; Str2 ++) {
      if (*Str1 == *Str2) {
        break;
      }
    }

    if (*Str2 == L'\0') {
      return Count;
    }

    Count ++;
  }

  return Count;
}

/**

  Searches a string for the first occurrence of a character contained in a
  specified buffer.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrBrk (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  )
{
  CHAR16  *Str1;
  CHAR16  *Str2;

  for (Str1 = String; *Str1 != L'\0'; Str1 ++) {
    for (Str2 = CharSet; *Str2 != L'\0'; Str2 ++) {
      if (*Str1 == *Str2) {
        return (CHAR16 *) Str1;
      }
    }
  }

  return NULL;
}

/**

  Find the next token after one or more specified characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrTokenLine (
  IN CHAR16                       *String OPTIONAL,
  IN CHAR16                       *CharSet
  )
{
  CHAR16  *Begin;
  CHAR16  *End;

  Begin = (String == NULL) ? mLineBuffer : String;
  if (Begin == NULL) {
    return NULL;
  }

  Begin += StrSpn (Begin, CharSet);
  if (*Begin == L'\0') {
    mLineBuffer = NULL;
    return NULL;
  }

  End = StrBrk (Begin, CharSet);
  if ((End != NULL) && (*End != L'\0')) {
    *End = L'\0';
    End ++;
  }

  mLineBuffer = End;
  return Begin;
}

/**

  Find the next token after one specificed characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrTokenField (
  IN CHAR16                       *String OPTIONAL,
  IN CHAR16                       *CharSet
  )
{
  CHAR16  *Begin;
  CHAR16  *End;


  Begin = (String == NULL) ? mFieldBuffer : String;
  if (Begin == NULL) {
    return NULL;
  }

  if (*Begin == L'\0') {
    mFieldBuffer = NULL;
    return NULL;
  }

  End = StrBrk (Begin, CharSet);
  if ((End != NULL) && (*End != L'\0')) {
    *End = L'\0';
    End ++;
  }

  mFieldBuffer = End;
  return Begin;
}

/**

  Find the next token after one or more specified characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNewTokenLine (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  )
{
  return StrTokenLine (String, CharSet);
}

/**

  Find the next token after one or more specified characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNextTokenLine (
  IN CHAR16                       *CharSet
  )
{
  return StrTokenLine (NULL, CharSet);
}

/**

  Find the next token after one specificed characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNewTokenField (
  IN CHAR16                       *String,
  IN CHAR16                       *CharSet
  )
{
  return StrTokenField (String, CharSet);
}

/**

  Find the next token after one specificed characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR16 *
EFIAPI
StrGetNextTokenField (
  IN CHAR16                       *CharSet
  )
{
  return StrTokenField (NULL, CharSet);
}

/**

  Patch a character to the end of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForStrTokenAfter (
  IN CHAR16    *Buffer,
  IN CHAR16    Patch
  )
{
  CHAR16 *Str;

  if (Buffer == NULL) {
    return ;
  }

  Str = Buffer;
  while (*Str != 0) {
    Str ++;
  }
  *Str = Patch;

  while (*(Str ++) != '\0') {
    if (*Str == 0) {
      *Str = Patch;
    } else {
      break;
    }
  }

  return ;
}

/**
  Patch a character at the beginning of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForStrTokenBefore (
  IN CHAR16    *Buffer,
  IN CHAR16    Patch
  )
{
  CHAR16 *Str;

  if (Buffer == NULL) {
    return ;
  }

  Str = Buffer;
  while (*(Str --) != '\0') {
    if ((*Str == 0) || (*Str == Patch)) {
      *Str = Patch;
    } else {
      break;
    }
  }

  return ;
}

CHAR8  *mAsciiLineBuffer          = NULL;
CHAR8  *mAsciiFieldBuffer         = NULL;

/**

  Find the first substring.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
UINTN
EFIAPI
AsciiStrSpn (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  )
{
  UINTN   Count;
  CHAR8  *Str1;
  CHAR8  *Str2;

  Count = 0;

  for (Str1 = String; *Str1 != '\0'; Str1 ++) {
    for (Str2 = CharSet; *Str2 != '\0'; Str2 ++) {
      if (*Str1 == *Str2) {
        break;
      }
    }

    if (*Str2 == '\0') {
      return Count;
    }

    Count ++;
  }

  return Count;
}

/**
  Searches a string for the first occurrence of a character contained in a
  specified buffer.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrBrk (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  )
{
  CHAR8  *Str1;
  CHAR8  *Str2;

  for (Str1 = String; *Str1 != '\0'; Str1 ++) {
    for (Str2 = CharSet; *Str2 != '\0'; Str2 ++) {
      if (*Str1 == *Str2) {
        return (CHAR8 *) Str1;
      }
    }
  }

  return NULL;
}

/**

  Find the next token after one or more specified characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrTokenLine (
  IN CHAR8                       *String OPTIONAL,
  IN CHAR8                       *CharSet
  )
{
  CHAR8  *Begin;
  CHAR8  *End;

  Begin = (String == NULL) ? mAsciiLineBuffer : String;
  if (Begin == NULL) {
    return NULL;
  }

  Begin += AsciiStrSpn (Begin, CharSet);
  if (*Begin == '\0') {
    mAsciiLineBuffer = NULL;
    return NULL;
  }

  End = AsciiStrBrk (Begin, CharSet);
  if ((End != NULL) && (*End != '\0')) {
    *End = '\0';
    End ++;
  }

  mAsciiLineBuffer = End;
  return Begin;
}

/**

  Find the next token after one specificed characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrTokenField (
  IN CHAR8                       *String OPTIONAL,
  IN CHAR8                       *CharSet
  )
{
  CHAR8  *Begin;
  CHAR8  *End;


  Begin = (String == NULL) ? mAsciiFieldBuffer : String;
  if (Begin == NULL) {
    return NULL;
  }

  if (*Begin == '\0') {
    mAsciiFieldBuffer = NULL;
    return NULL;
  }

  End = AsciiStrBrk (Begin, CharSet);
  if ((End != NULL) && (*End != '\0')) {
    *End = '\0';
    End ++;
  }

  mAsciiFieldBuffer = End;
  return Begin;
}

/**

  Find the next token after one or more specified characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNewTokenLine (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  )
{
  return AsciiStrTokenLine (String, CharSet);
}

/**

  Find the next token after one or more specified characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNextTokenLine (
  IN CHAR8                       *CharSet
  )
{
  return AsciiStrTokenLine (NULL, CharSet);
}

/**

  Find the next token after one specificed characters.

  @param  String    Point to the string where to find the substring.
  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNewTokenField (
  IN CHAR8                       *String,
  IN CHAR8                       *CharSet
  )
{
  return AsciiStrTokenField (String, CharSet);
}

/**

  Find the next token after one specificed characters.

  @param  CharSet   Point to the string to be found.

**/
CHAR8 *
EFIAPI
AsciiStrGetNextTokenField (
  IN CHAR8                       *CharSet
  )
{
  return AsciiStrTokenField (NULL, CharSet);
}

/**

  Patch a character to the end of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForAsciiStrTokenAfter (
  IN CHAR8    *Buffer,
  IN CHAR8    Patch
  )
{
  CHAR8 *Str;

  if (Buffer == NULL) {
    return ;
  }

  Str = Buffer;
  while (*Str != 0) {
    Str ++;
  }
  *Str = Patch;

  while (*(Str ++) != '\0') {
    if (*Str == 0) {
      *Str = Patch;
    } else {
      break;
    }
  }

  return ;
}

/**
  Patch a character at the beginning of a string.

  @param  Buffer   The string to be patched.
  @param  Patch    The patch character.

**/
VOID
EFIAPI
PatchForAsciiStrTokenBefore (
  IN CHAR8    *Buffer,
  IN CHAR8    Patch
  )
{
  CHAR8 *Str;

  if (Buffer == NULL) {
    return ;
  }

  Str = Buffer;
  while (*(Str --) != '\0') {
    if ((*Str == 0) || (*Str == Patch)) {
      *Str = Patch;
    } else {
      break;
    }
  }

  return ;
}
