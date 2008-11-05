/** @file
  Unicode and ASCII string primatives.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "BaseLibInternals.h"

#define QUOTIENT_MAX_UINTN_DIVIDED_BY_10      ((UINTN) -1 / 10)
#define REMAINDER_MAX_UINTN_DIVIDED_BY_10    ((UINTN) -1 % 10)

#define QUOTIENT_MAX_UINTN_DIVIDED_BY_16      ((UINTN) -1 / 16)
#define REMAINDER_MAX_UINTN_DIVIDED_BY_16    ((UINTN) -1 % 16)

#define QUOTIENT_MAX_UINT64_DIVIDED_BY_10      ((UINT64) -1 / 10)
#define REMAINDER_MAX_UINT64_DIVIDED_BY_10    ((UINT64) -1 % 10)

#define QUOTIENT_MAX_UINT64_DIVIDED_BY_16      ((UINT64) -1 / 16)
#define REMAINDER_MAX_UINT64_DIVIDED_BY_16    ((UINT64) -1 % 16)

/**
  Copies one Null-terminated Unicode string to another Null-terminated Unicode
  string and returns the new Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.

  @return Destination pointing to the copied string.

**/
CHAR16 *
EFIAPI
StrCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  )
{
  CHAR16                            *ReturnValue;

  //
  // Destination cannot be NULL
  //
  ASSERT (Destination != NULL);
  ASSERT (((UINTN) Destination & BIT0) == 0);

  //
  // Destination and source cannot overlap
  //
  ASSERT ((UINTN)(Destination - Source) > StrLen (Source));
  ASSERT ((UINTN)(Source - Destination) > StrLen (Source));

  ReturnValue = Destination;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;
  return ReturnValue;
}

/**
  Copies one Null-terminated Unicode string with a maximum length to another
  Null-terminated Unicode string with a maximum length and returns the new
  Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. At most, Length Unicode
  characters are copied from Source to Destination. If Length is 0, then
  Destination is returned unmodified. If Length is greater that the number of
  Unicode characters in Source, then Destination is padded with Null Unicode
  characters. If Source and Destination overlap, then the results are
  undefined.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit bounadry, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.
  @param  Length      Maximum number of Unicode characters to copy.

  @return Destination pointing to the copied string.

**/
CHAR16 *
EFIAPI
StrnCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source,
  IN      UINTN                     Length
  )
{
  CHAR16                            *ReturnValue;

  if (Length == 0) {
    return Destination;
  }

  //
  // Destination cannot be NULL if Length is not zero
  //
  ASSERT (Destination != NULL);
  ASSERT (((UINTN) Destination & BIT0) == 0);

  //
  // Destination and source cannot overlap
  //
  ASSERT ((UINTN)(Destination - Source) > StrLen (Source));
  ASSERT ((UINTN)(Source - Destination) >= Length);

  ReturnValue = Destination;

  while ((*Source != L'\0') && (Length > 0)) {
    *(Destination++) = *(Source++);
    Length--;
  }

  ZeroMem (Destination, Length * sizeof (*Destination));
  return ReturnValue;
}

/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().

  @param  String  Pointer to a Null-terminated Unicode string.

  @return The length of String.

**/
UINTN
EFIAPI
StrLen (
  IN      CONST CHAR16              *String
  )
{
  UINTN                             Length;

  ASSERT (String != NULL);
  ASSERT (((UINTN) String & BIT0) == 0);

  for (Length = 0; *String != L'\0'; String++, Length++) {
    //
    // If PcdMaximumUnicodeStringLength is not zero,
    // length should not more than PcdMaximumUnicodeStringLength
    //
    if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
      ASSERT (Length < PcdGet32 (PcdMaximumUnicodeStringLength));
    }
  }
  return Length;
}

/**
  Returns the size of a Null-terminated Unicode string in bytes, including the
  Null terminator.

  This function returns the size, in bytes, of the Null-terminated Unicode
  string specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().

  @param  String  Pointer to a Null-terminated Unicode string.

  @return The size in bytes of String.

**/
UINTN
EFIAPI
StrSize (
  IN      CONST CHAR16              *String
  )
{
  return (StrLen (String) + 1) * sizeof (*String);
}

/**
  Compares two Null-terminated Unicode strings, and returns the difference
  between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. If FirstString is identical to
  SecondString, then 0 is returned. Otherwise, the value returned is the first
  mismatched Unicode character in SecondString subtracted from the first
  mismatched Unicode character in FirstString.

  If FirstString is NULL, then ASSERT().
  If FirstString is not aligned on a 16-bit boundary, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If SecondString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated Unicode string.
  @param  SecondString  Pointer to a Null-terminated Unicode string.

  @retval 0      FirstString is identical to SecondString.
  @return The first mismatched Unicode character in SecondString subtracted
          from the first mismatched Unicode character in FirstString.

**/
INTN
EFIAPI
StrCmp (
  IN      CONST CHAR16              *FirstString,
  IN      CONST CHAR16              *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }
  return *FirstString - *SecondString;
}

/**
  Compares two Null-terminated Unicode strings with maximum lengths, and
  returns the difference between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. At most, Length Unicode
  characters will be compared. If Length is 0, then 0 is returned. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched Unicode character in SecondString
  subtracted from the first mismatched Unicode character in FirstString.

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and FirstString is not aligned on a 16-bit bounadary, then ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If Length > 0 and SecondString is not aligned on a 16-bit bounadary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated Unicode string.
  @param  SecondString  Pointer to a Null-terminated Unicode string.
  @param  Length        Maximum number of Unicode characters to compare.

  @retval 0      FirstString is identical to SecondString.
  @return The value returned is the first mismatched Unicode character in SecondString
          subtracted from the first mismatched Unicode character in FirstString.

**/
INTN
EFIAPI
StrnCmp (
  IN      CONST CHAR16              *FirstString,
  IN      CONST CHAR16              *SecondString,
  IN      UINTN                     Length
  )
{
  if (Length == 0) {
    return 0;
  }

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (FirstString) != 0);
  ASSERT (StrSize (SecondString) != 0);

  while ((*FirstString != L'\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    Length--;
  }

  return *FirstString - *SecondString;
}

/**
  Concatenates one Null-terminated Unicode string to another Null-terminated
  Unicode string, and returns the concatenated Unicode string.

  This function concatenates two Null-terminated Unicode strings. The contents
  of Null-terminated Unicode string Source are concatenated to the end of
  Null-terminated Unicode string Destination. The Null-terminated concatenated
  Unicode String is returned. If Source and Destination overlap, then the
  results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.

  @return Destination pointing to the concatenated Unicode string.

**/
CHAR16 *
EFIAPI
StrCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  )
{
  StrCpy (Destination + StrLen (Destination), Source);

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (StrSize (Destination) != 0);
  return Destination;
}

/**
  Concatenates one Null-terminated Unicode string with a maximum length to the
  end of another Null-terminated Unicode string, and returns the concatenated
  Unicode string.

  This function concatenates two Null-terminated Unicode strings. The contents
  of Null-terminated Unicode string Source are concatenated to the end of
  Null-terminated Unicode string Destination, and Destination is returned. At
  most, Length Unicode characters are concatenated from Source to the end of
  Destination, and Destination is always Null-terminated. If Length is 0, then
  Destination is returned unmodified. If Source and Destination overlap, then
  the results are undefined.

  If Destination is NULL, then ASSERT().
  If Length > 0 and Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Length > 0 and Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.
  @param  Length      Maximum number of Unicode characters to concatenate from
                      Source.

  @return Destination pointing to the concatenated Unicode string.

**/
CHAR16 *
EFIAPI
StrnCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source,
  IN      UINTN                     Length
  )
{
  StrnCpy (Destination + StrLen (Destination), Source, Length);

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (StrSize (Destination) != 0);
  return Destination;
}

/**
  Returns the first occurance of a Null-terminated Unicode sub-string 
  in a Null-terminated Unicode string.

  This function scans the contents of the Null-terminated Unicode string 
  specified by String and returns the first occurrence of SearchString.  
  If SearchString is not found in String, then NULL is returned.  If 
  the length of SearchString is zero, then String is 
  returned.
  
  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If SearchString is NULL, then ASSERT().
  If SearchString is not aligned on a 16-bit boundary, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and SearchString 
  or String contains more than PcdMaximumUnicodeStringLength Unicode 
  characters not including the Null-terminator, then ASSERT().

  @param  String        Pointer to a Null-terminated Unicode string.
  @param  SearchString  Pointer to a Null-terminated Unicode string to search for.

  @retval NULL          If the SearchString does not appear in String.
  @return Pointer to the matching sub-string.

**/
CHAR16 *
EFIAPI
StrStr (
  IN      CONST CHAR16                *String,
  IN      CONST CHAR16                *SearchString
  )
{
  CONST CHAR16 *FirstMatch;
  CONST CHAR16 *SearchStringTmp;

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);
  ASSERT (StrSize (SearchString) != 0);

  while (*String != '\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;
    
    while ((*String == *SearchStringTmp) 
            && (*SearchStringTmp != '\0') 
            && (*String != '\0')) {
      String++;
      SearchStringTmp++;
    } 
    
    if (*SearchStringTmp == '\0') {
      return (CHAR16 *) FirstMatch;
    }

    if (SearchStringTmp == SearchString) {
      //
      // If no character from SearchString match,
      // move the pointer to the String under search
      // by one character.
      //
      String++;
    }
  }

  return NULL;
}

/**
  Check if a Unicode character is a decimal character.

  This internal function checks if a Unicode character is a 
  decimal character. The valid decimal character is from
  L'0' to L'9'.

  @param  Char  The character to check against.

  @retval TRUE  If the Char is a decmial character.
  @retval FALSE If the Char is not a decmial character.

**/
BOOLEAN
EFIAPI
InternalIsDecimalDigitCharacter (
  IN      CHAR16                    Char
  )
{
  return (BOOLEAN) (Char >= L'0' && Char <= L'9');
}

/**
  Convert a Unicode character to upper case only if 
  it maps to a valid small-case ASCII character.

  This internal function only deal with Unicode character
  which maps to a valid small-case ASCII character, i.e.
  L'a' to L'z'. For other Unicode character, the input character
  is returned directly.

  @param  Char  The character to convert.

  @retval LowerCharacter   If the Char is with range L'a' to L'z'.
  @retval Unchanged        Otherwise.

**/
CHAR16
EFIAPI
InternalCharToUpper (
  IN      CHAR16                    Char
  )
{
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}

/**
  Convert a Unicode character to numerical value.

  This internal function only deal with Unicode character
  which maps to a valid hexadecimal ASII character, i.e.
  L'0' to L'9', L'a' to L'f' or L'A' to L'F'. For other 
  Unicode character, the value returned does not make sense.

  @param  Char  The character to convert.

  @return The numerical value converted.

**/
UINTN
EFIAPI
InternalHexCharToUintn (
  IN      CHAR16                    Char
  )
{
  if (InternalIsDecimalDigitCharacter (Char)) {
    return Char - L'0';
  }

  return (UINTN) (10 + InternalCharToUpper (Char) - L'A');
}

/**
  Check if a Unicode character is a hexadecimal character.

  This internal function checks if a Unicode character is a 
  decimal character.  The valid hexadecimal character is 
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.


  @param  Char  The character to check against.

  @retval TRUE  If the Char is a hexadecmial character.
  @retval FALSE If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
InternalIsHexaDecimalDigitCharacter (
  IN      CHAR16                    Char
  )
{

  return (BOOLEAN) (InternalIsDecimalDigitCharacter (Char) ||
    (Char >= L'A' && Char <= L'F') ||
    (Char >= L'a' && Char <= L'f'));
}

/**
  Convert a Null-terminated Unicode decimal string to a value of 
  type UINTN.

  This function returns a value of type UINTN by interpreting the contents 
  of the Unicode string specified by String as a decimal number. The format 
  of the input Unicode string String is:
  
                  [spaces] [decimal digits].
                  
  The valid decimal digit character is in the range [0-9]. The 
  function will ignore the pad space, which includes spaces or 
  tab characters, before [decimal digits]. The running zero in the 
  beginning of [decimal digits] will be ignored. Then, the function 
  stops at the first character that is a not a valid decimal character 
  or a Null-terminator, whichever one comes first. 
  
  If String is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().  
  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits, 
  then 0 is returned.
  If the number represented by String overflows according 
  to the range defined by UINTN, then ASSERT().
  
  If PcdMaximumUnicodeStringLength is not zero, and String contains 
  more than PcdMaximumUnicodeStringLength Unicode characters not including 
  the Null-terminator, then ASSERT().

  @param  String                Pointer to a Null-terminated Unicode string.

  @return The value of type UINTN converted.

**/
UINTN
EFIAPI
StrDecimalToUintn (
  IN      CONST CHAR16                *String
  )
{
  UINTN     Result;
  
  //
  // ASSERT String is less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  Result = 0;

  while (InternalIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINTN_DIVIDED_BY_10) ||
      ((Result == QUOTIENT_MAX_UINTN_DIVIDED_BY_10) &&
      (*String - L'0') <= REMAINDER_MAX_UINTN_DIVIDED_BY_10)
      );

    Result = Result * 10 + (*String - L'0');
    String++;
  }
  
  return Result;
}


/**
  Convert a Null-terminated Unicode decimal string to a value of 
  type UINT64.

  This function returns a value of type UINT64 by interpreting the contents 
  of the Unicode string specified by String as a decimal number. The format 
  of the input Unicode string String is:
  
                  [spaces] [decimal digits].
                  
  The valid decimal digit character is in the range [0-9]. The 
  function will ignore the pad space, which includes spaces or 
  tab characters, before [decimal digits]. The running zero in the 
  beginning of [decimal digits] will be ignored. Then, the function 
  stops at the first character that is a not a valid decimal character 
  or a Null-terminator, whichever one comes first. 
  
  If String is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().  
  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits, 
  then 0 is returned.
  If the number represented by String overflows according 
  to the range defined by UINT64, then ASSERT().
  
  If PcdMaximumUnicodeStringLength is not zero, and String contains 
  more than PcdMaximumUnicodeStringLength Unicode characters not including 
  the Null-terminator, then ASSERT().

  @param  String                Pointer to a Null-terminated Unicode string.

  @return The value of type UINT64 converted.

**/
UINT64
EFIAPI
StrDecimalToUint64 (
  IN      CONST CHAR16                *String
  )
{
  UINT64     Result;
  
  //
  // ASSERT String is less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  Result = 0;

  while (InternalIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINT64_DIVIDED_BY_10) || 
      ((Result == QUOTIENT_MAX_UINT64_DIVIDED_BY_10) && 
      (*String - L'0') <= REMAINDER_MAX_UINT64_DIVIDED_BY_10)
      );

    Result = MultU64x32 (Result, 10) + (*String - L'0');
    String++;
  }
  
  return Result;
}

/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type UINTN.

  This function returns a value of type UINTN by interpreting the contents 
  of the Unicode string specified by String as a hexadecimal number. 
  The format of the input Unicode string String is:
  
                  [spaces][zeros][x][hexadecimal digits]. 

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F]. 
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. 
  If "x" appears in the input string, it must be prefixed with at least one 0. 
  The function will ignore the pad space, which includes spaces or tab characters, 
  before [zeros], [x] or [hexadecimal digit]. The running zero before [x] or 
  [hexadecimal digit] will be ignored. Then, the decoding starts after [x] or the 
  first valid hexadecimal digit. Then, the function stops at the first character that is 
  a not a valid hexadecimal character or NULL, whichever one comes first.

  If String is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If String has only pad spaces, then zero is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits, 
  then zero is returned.
  If the number represented by String overflows according to the range defined by 
  UINTN, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and String contains more than 
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, 
  then ASSERT().

  @param  String                Pointer to a Null-terminated Unicode string.

  @return The value of type UINTN converted.

**/
UINTN
EFIAPI
StrHexToUintn (
  IN      CONST CHAR16                *String
  )
{
  UINTN     Result;

  //
  // ASSERT String is less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);
  
  //
  // Ignore the pad spaces (space or tab) 
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  if (InternalCharToUpper (*String) == L'X') {
    ASSERT (*(String - 1) == L'0');
    if (*(String - 1) != L'0') {
      return 0;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  Result = 0;
  
  while (InternalIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the Hex Number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINTN_DIVIDED_BY_16) ||
      ((Result == QUOTIENT_MAX_UINTN_DIVIDED_BY_16) && 
      (InternalHexCharToUintn (*String) <= REMAINDER_MAX_UINTN_DIVIDED_BY_16))
      );

    Result = (Result << 4) + InternalHexCharToUintn (*String);
    String++;
  }

  return Result;
}


/**
  Convert a Null-terminated Unicode hexadecimal string to a value of type UINT64.

  This function returns a value of type UINT64 by interpreting the contents 
  of the Unicode string specified by String as a hexadecimal number. 
  The format of the input Unicode string String is 
  
                  [spaces][zeros][x][hexadecimal digits]. 

  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F]. 
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. 
  If "x" appears in the input string, it must be prefixed with at least one 0. 
  The function will ignore the pad space, which includes spaces or tab characters, 
  before [zeros], [x] or [hexadecimal digit]. The running zero before [x] or 
  [hexadecimal digit] will be ignored. Then, the decoding starts after [x] or the 
  first valid hexadecimal digit. Then, the function stops at the first character that is 
  a not a valid hexadecimal character or NULL, whichever one comes first.

  If String is NULL, then ASSERT().
  If String is not aligned in a 16-bit boundary, then ASSERT().
  If String has only pad spaces, then zero is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits, 
  then zero is returned.
  If the number represented by String overflows according to the range defined by 
  UINT64, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and String contains more than 
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, 
  then ASSERT().

  @param  String                Pointer to a Null-terminated Unicode string.

  @return The value of type UINT64 converted.

**/
UINT64
EFIAPI
StrHexToUint64 (
  IN      CONST CHAR16                *String
  )
{
  UINT64    Result;

  //
  // ASSERT String is less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (String) != 0);
  
  //
  // Ignore the pad spaces (space or tab) 
  //
  while ((*String == L' ') || (*String == L'\t')) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == L'0') {
    String++;
  }

  if (InternalCharToUpper (*String) == L'X') {
    ASSERT (*(String - 1) == L'0');
    if (*(String - 1) != L'0') {
      return 0;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  Result = 0;
  
  while (InternalIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the Hex Number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINT64_DIVIDED_BY_16)|| 
      ((Result == QUOTIENT_MAX_UINT64_DIVIDED_BY_16) && 
      (InternalHexCharToUintn (*String) <= REMAINDER_MAX_UINT64_DIVIDED_BY_16))
      );

    Result = LShiftU64 (Result, 4);
    Result = Result + InternalHexCharToUintn (*String);
    String++;
  }

  return Result;
}

/**
  Check if a ASCII character is a decimal character.

  This internal function checks if a Unicode character is a 
  decimal character. The valid decimal character is from
  '0' to '9'.

  @param  Char  The character to check against.

  @retval TRUE  If the Char is a decmial character.
  @retval FALSE If the Char is not a decmial character.

**/
BOOLEAN
EFIAPI
InternalAsciiIsDecimalDigitCharacter (
  IN      CHAR8                     Char
  )
{
  return (BOOLEAN) (Char >= '0' && Char <= '9');
}

/**
  Check if a ASCII character is a hexadecimal character.

  This internal function checks if a ASCII character is a 
  decimal character.  The valid hexadecimal character is 
  L'0' to L'9', L'a' to L'f', or L'A' to L'F'.


  @param  Char  The character to check against.

  @retval TRUE  If the Char is a hexadecmial character.
  @retval FALSE If the Char is not a hexadecmial character.

**/
BOOLEAN
EFIAPI
InternalAsciiIsHexaDecimalDigitCharacter (
  IN      CHAR8                    Char
  )
{

  return (BOOLEAN) (InternalAsciiIsDecimalDigitCharacter (Char) ||
    (Char >= 'A' && Char <= 'F') ||
    (Char >= 'a' && Char <= 'f'));
}

/**
  Convert a Null-terminated Unicode string to a Null-terminated 
  ASCII string and returns the ASCII string.
  
  This function converts the content of the Unicode string Source 
  to the ASCII string Destination by copying the lower 8 bits of 
  each Unicode character. It returns Destination. The function terminates 
  the ASCII string Destination  by appending a Null-terminator character 
  at the end. The caller is responsible to make sure Destination points 
  to a buffer with size equal or greater than (StrLen (Source) + 1) in bytes.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().

  If any Unicode characters in Source contain non-zero value in 
  the upper 8 bits, then ASSERT().
  
  If PcdMaximumUnicodeStringLength is not zero, and Source contains 
  more than PcdMaximumUnicodeStringLength Unicode characters not including 
  the Null-terminator, then ASSERT().
  
  If PcdMaximumAsciiStringLength is not zero, and Source contains more 
  than PcdMaximumAsciiStringLength Unicode characters not including the 
  Null-terminator, then ASSERT().

  @param  Source        Pointer to a Null-terminated Unicode string.
  @param  Destination   Pointer to a Null-terminated ASCII string.

  @return Destination pointing to the converted ASCII string.

**/
CHAR8 *
EFIAPI
UnicodeStrToAsciiStr (
  IN      CONST CHAR16                *Source,
  OUT     CHAR8                       *Destination
  )
{
  CHAR8                               *ReturnValue;

  ASSERT (Destination != NULL);

  //
  // ASSERT if Source is long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside StrLen().
  //
  ASSERT (StrSize (Source) != 0);

  //
  // Source and Destination should not overlap
  //
  ASSERT ((UINTN) ((CHAR16 *) Destination -  Source) > StrLen (Source));
  ASSERT ((UINTN) ((CHAR8 *) Source - Destination) > StrLen (Source));


  ReturnValue = Destination;
  while (*Source != '\0') {
    //
    // If any Unicode characters in Source contain 
    // non-zero value in the upper 8 bits, then ASSERT().
    //
    ASSERT (*Source < 0x100);
    *(Destination++) = (CHAR8) *(Source++);
  }

  *Destination = '\0';

  //
  // ASSERT Original Destination is less long than PcdMaximumAsciiStringLength.
  // Length tests are performed inside AsciiStrLen().
  //
  ASSERT (AsciiStrSize (ReturnValue) != 0);

  return ReturnValue;
}


/**
  Copies one Null-terminated ASCII string to another Null-terminated ASCII
  string and returns the new ASCII string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.

  @return Destination pointing to the copied string.

**/
CHAR8 *
EFIAPI
AsciiStrCpy (
  OUT     CHAR8                     *Destination,
  IN      CONST CHAR8               *Source
  )
{
  CHAR8                             *ReturnValue;

  //
  // Destination cannot be NULL
  //
  ASSERT (Destination != NULL);

  //
  // Destination and source cannot overlap
  //
  ASSERT ((UINTN)(Destination - Source) > AsciiStrLen (Source));
  ASSERT ((UINTN)(Source - Destination) > AsciiStrLen (Source));

  ReturnValue = Destination;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;
  return ReturnValue;
}

/**
  Copies one Null-terminated ASCII string with a maximum length to another
  Null-terminated ASCII string with a maximum length and returns the new ASCII
  string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. At most, Length ASCII characters
  are copied from Source to Destination. If Length is 0, then Destination is
  returned unmodified. If Length is greater that the number of ASCII characters
  in Source, then Destination is padded with Null ASCII characters. If Source
  and Destination overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.
  @param  Length      Maximum number of ASCII characters to copy.

  @return Destination pointing to the copied string.

**/
CHAR8 *
EFIAPI
AsciiStrnCpy (
  OUT     CHAR8                     *Destination,
  IN      CONST CHAR8               *Source,
  IN      UINTN                     Length
  )
{
  CHAR8                             *ReturnValue;

  if (Length == 0) {
    return Destination;
  }

  //
  // Destination cannot be NULL
  //
  ASSERT (Destination != NULL);

  //
  // Destination and source cannot overlap
  //
  ASSERT ((UINTN)(Destination - Source) > AsciiStrLen (Source));
  ASSERT ((UINTN)(Source - Destination) >= Length);

  ReturnValue = Destination;

  while (*Source != 0 && Length > 0) {
    *(Destination++) = *(Source++);
    Length--;
  }

  ZeroMem (Destination, Length * sizeof (*Destination));
  return ReturnValue;
}

/**
  Returns the length of a Null-terminated ASCII string.

  This function returns the number of ASCII characters in the Null-terminated
  ASCII string specified by String.

  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String  Pointer to a Null-terminated ASCII string.

  @return The length of String.

**/
UINTN
EFIAPI
AsciiStrLen (
  IN      CONST CHAR8               *String
  )
{
  UINTN                             Length;

  ASSERT (String != NULL);

  for (Length = 0; *String != '\0'; String++, Length++) {
    //
    // If PcdMaximumUnicodeStringLength is not zero,
    // length should not more than PcdMaximumUnicodeStringLength
    //
    if (PcdGet32 (PcdMaximumAsciiStringLength) != 0) {
      ASSERT (Length < PcdGet32 (PcdMaximumAsciiStringLength));
    }
  }
  return Length;
}

/**
  Returns the size of a Null-terminated ASCII string in bytes, including the
  Null terminator.

  This function returns the size, in bytes, of the Null-terminated ASCII string
  specified by String.

  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String  Pointer to a Null-terminated ASCII string.

  @return The size of String.

**/
UINTN
EFIAPI
AsciiStrSize (
  IN      CONST CHAR8               *String
  )
{
  return (AsciiStrLen (String) + 1) * sizeof (*String);
}

/**
  Compares two Null-terminated ASCII strings, and returns the difference
  between the first mismatched ASCII characters.

  This function compares the Null-terminated ASCII string FirstString to the
  Null-terminated ASCII string SecondString. If FirstString is identical to
  SecondString, then 0 is returned. Otherwise, the value returned is the first
  mismatched ASCII character in SecondString subtracted from the first
  mismatched ASCII character in FirstString.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0      FirstString is identical to SecondString.
  @return The first mismatched ASCII character in SecondString subtracted
          from the first mismatched ASCII character in FirstString.

**/
INTN
EFIAPI
AsciiStrCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  while ((*FirstString != '\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }

  return *FirstString - *SecondString;
}

/**
  Converts a lowercase Ascii character to upper one.

  If Chr is lowercase Ascii character, then converts it to upper one.

  If Value >= 0xA0, then ASSERT().
  If (Value & 0x0F) >= 0x0A, then ASSERT().

  @param  Chr   one Ascii character

  @return The uppercase value of Ascii character 

**/
CHAR8
EFIAPI
AsciiToUpper (
  IN      CHAR8                     Chr
  )
{
  return (UINT8) ((Chr >= 'a' && Chr <= 'z') ? Chr - ('a' - 'A') : Chr);
}

/**
  Convert a ASCII character to numerical value.

  This internal function only deal with Unicode character
  which maps to a valid hexadecimal ASII character, i.e.
  '0' to '9', 'a' to 'f' or 'A' to 'F'. For other 
  ASCII character, the value returned does not make sense.

  @param  Char  The character to convert.

  @return The numerical value converted.

**/
UINTN
EFIAPI
InternalAsciiHexCharToUintn (
  IN      CHAR8                    Char
  )
{
  if (InternalIsDecimalDigitCharacter (Char)) {
    return Char - '0';
  }

  return (UINTN) (10 + AsciiToUpper (Char) - 'A');
}


/**
  Performs a case insensitive comparison of two Null-terminated ASCII strings,
  and returns the difference between the first mismatched ASCII characters.

  This function performs a case insensitive comparison of the Null-terminated
  ASCII string FirstString to the Null-terminated ASCII string SecondString. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched lower case ASCII character in
  SecondString subtracted from the first mismatched lower case ASCII character
  in FirstString.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0      FirstString is identical to SecondString using case insensitive
                 comparisons.
  @return The first mismatched lower case ASCII character in SecondString subtracted
          from the first mismatched lower case ASCII character in FirstString.

**/
INTN
EFIAPI
AsciiStriCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString
  )
{
  CHAR8  UpperFirstString;
  CHAR8  UpperSecondString;

  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  UpperFirstString  = AsciiToUpper (*FirstString);
  UpperSecondString = AsciiToUpper (*SecondString);
  while ((*FirstString != '\0') && (UpperFirstString == UpperSecondString)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = AsciiToUpper (*FirstString);
    UpperSecondString = AsciiToUpper (*SecondString);
  }

  return UpperFirstString - UpperSecondString;
}

/**
  Compares two Null-terminated ASCII strings with maximum lengths, and returns
  the difference between the first mismatched ASCII characters.

  This function compares the Null-terminated ASCII string FirstString to the
  Null-terminated ASCII  string SecondString. At most, Length ASCII characters
  will be compared. If Length is 0, then 0 is returned. If FirstString is
  identical to SecondString, then 0 is returned. Otherwise, the value returned
  is the first mismatched ASCII character in SecondString subtracted from the
  first mismatched ASCII character in FirstString.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.
  @param  Length        Maximum number of ASCII characters to compare.
                        
  @retval 0      FirstString is identical to SecondString.
  @return The first mismatched ASCII character in SecondString subtracted from the
          first mismatched ASCII character in FirstString.

**/
INTN
EFIAPI
AsciiStrnCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString,
  IN      UINTN                     Length
  )
{
  if (Length == 0) {
    return 0;
  }

  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  while ((*FirstString != '\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    Length--;
  }
  return *FirstString - *SecondString;
}

/**
  Concatenates one Null-terminated ASCII string to another Null-terminated
  ASCII string, and returns the concatenated ASCII string.

  This function concatenates two Null-terminated ASCII strings. The contents of
  Null-terminated ASCII string Source are concatenated to the end of Null-
  terminated ASCII string Destination. The Null-terminated concatenated ASCII
  String is returned.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Destination contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.

  @return Destination pointing to the concatenated ASCII string.

**/
CHAR8 *
EFIAPI
AsciiStrCat (
  IN OUT CHAR8    *Destination,
  IN CONST CHAR8  *Source
  )
{
  AsciiStrCpy (Destination + AsciiStrLen (Destination), Source);

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (AsciiStrSize (Destination) != 0);
  return Destination;
}

/**
  Concatenates one Null-terminated ASCII string with a maximum length to the
  end of another Null-terminated ASCII string, and returns the concatenated
  ASCII string.

  This function concatenates two Null-terminated ASCII strings. The contents
  of Null-terminated ASCII string Source are concatenated to the end of Null-
  terminated ASCII string Destination, and Destination is returned. At most,
  Length ASCII characters are concatenated from Source to the end of
  Destination, and Destination is always Null-terminated. If Length is 0, then
  Destination is returned unmodified. If Source and Destination overlap, then
  the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Destination contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters not including the Null-terminator, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.
  @param  Length      Maximum number of ASCII characters to concatenate from
                      Source.

  @return Destination pointing to the concatenated ASCII string.

**/
CHAR8 *
EFIAPI
AsciiStrnCat (
  IN OUT  CHAR8                     *Destination,
  IN      CONST CHAR8               *Source,
  IN      UINTN                     Length
  )
{
  AsciiStrnCpy (Destination + AsciiStrLen (Destination), Source, Length);

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (AsciiStrSize (Destination) != 0);
  return Destination;
}

/**
  Returns the first occurance of a Null-terminated ASCII sub-string 
  in a Null-terminated ASCII string.

  This function scans the contents of the ASCII string specified by String 
  and returns the first occurrence of SearchString. If SearchString is not 
  found in String, then NULL is returned. If the length of SearchString is zero, 
  then String is returned.
  
  If String is NULL, then ASSERT().
  If SearchString is NULL, then ASSERT().

  If PcdMaximumAsciiStringLength is not zero, and SearchString or 
  String contains more than PcdMaximumAsciiStringLength Unicode characters 
  not including the Null-terminator, then ASSERT().

  @param  String          Pointer to a Null-terminated ASCII string.
  @param  SearchString    Pointer to a Null-terminated ASCII string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @return Pointer to the matching sub-string.

**/
CHAR8 *
EFIAPI
AsciiStrStr (
  IN      CONST CHAR8             *String,
  IN      CONST CHAR8             *SearchString
  )
{
  CONST CHAR8 *FirstMatch;
  CONST CHAR8 *SearchStringTmp;

  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);
  ASSERT (AsciiStrSize (SearchString) != 0);

  while (*String != '\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;
    
    while ((*String == *SearchStringTmp) 
            && (*SearchStringTmp != '\0') 
            && (*String != '\0')) {
      String++;
      SearchStringTmp++;
    } 
    
    if (*SearchStringTmp == '\0') {
      return (CHAR8 *) FirstMatch;
    }

    if (SearchStringTmp == SearchString) {
      //
      // If no character from SearchString match,
      // move the pointer to the String under search
      // by one character.
      //
      String++;
    }

  }

  return NULL;
}

/**
  Convert a Null-terminated ASCII decimal string to a value of type 
  UINTN.

  This function returns a value of type UINTN by interpreting the contents 
  of the ASCII string String as a decimal number. The format of the input 
  ASCII string String is:
  
                    [spaces] [decimal digits].
  
  The valid decimal digit character is in the range [0-9]. The function will 
  ignore the pad space, which includes spaces or tab characters, before the digits. 
  The running zero in the beginning of [decimal digits] will be ignored. Then, the 
  function stops at the first character that is a not a valid decimal character or 
  Null-terminator, whichever on comes first.
  
  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits, then 0 is returned.
  If the number represented by String overflows according to the range defined by 
  UINTN, then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than 
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, 
  then ASSERT().

  @param  String                Pointer to a Null-terminated ASCII string.

  @return The value of type UINTN converted.

**/
UINTN
EFIAPI
AsciiStrDecimalToUintn (
  IN      CONST CHAR8               *String
  )
{
  UINTN     Result;
  
  //
  // ASSERT Strings is less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == ' ') || (*String == '\t' )) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  Result = 0;

  while (InternalAsciiIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINTN_DIVIDED_BY_10) ||
      ((Result == QUOTIENT_MAX_UINTN_DIVIDED_BY_10) && 
      (*String - '0') <= REMAINDER_MAX_UINTN_DIVIDED_BY_10)
      );

    Result = Result * 10 + (*String - '0');
    String++;
  }
  
  return Result;
}


/**
  Convert a Null-terminated ASCII decimal string to a value of type 
  UINT64.

  This function returns a value of type UINT64 by interpreting the contents 
  of the ASCII string String as a decimal number. The format of the input 
  ASCII string String is:
  
                    [spaces] [decimal digits].
  
  The valid decimal digit character is in the range [0-9]. The function will 
  ignore the pad space, which includes spaces or tab characters, before the digits. 
  The running zero in the beginning of [decimal digits] will be ignored. Then, the 
  function stops at the first character that is a not a valid decimal character or 
  Null-terminator, whichever on comes first.
  
  If String has only pad spaces, then 0 is returned.
  If String has no pad spaces or valid decimal digits, then 0 is returned.
  If the number represented by String overflows according to the range defined by 
  UINT64, then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than 
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, 
  then ASSERT().

  @param  String                Pointer to a Null-terminated ASCII string.

  @return The value of type UINT64 converted.

**/
UINT64
EFIAPI
AsciiStrDecimalToUint64 (
  IN      CONST CHAR8             *String
  )
{
  UINT64     Result;
  
  //
  // ASSERT Strings is less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);

  //
  // Ignore the pad spaces (space or tab)
  //
  while ((*String == ' ') || (*String == '\t' )) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  Result = 0;

  while (InternalAsciiIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINT64_DIVIDED_BY_10) || 
      ((Result == QUOTIENT_MAX_UINT64_DIVIDED_BY_10) && 
      (*String - '0') <= REMAINDER_MAX_UINT64_DIVIDED_BY_10)
      );

    Result = MultU64x32 (Result, 10) + (*String - '0');
    String++;
  }
  
  return Result;
}

/**
  Convert a Null-terminated ASCII hexadecimal string to a value of type UINTN.

  This function returns a value of type UINTN by interpreting the contents of 
  the ASCII string String as a hexadecimal number. The format of the input ASCII 
  string String is:
  
                  [spaces][zeros][x][hexadecimal digits].
                  
  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F]. 
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. If "x" 
  appears in the input string, it must be prefixed with at least one 0. The function 
  will ignore the pad space, which includes spaces or tab characters, before [zeros], 
  [x] or [hexadecimal digits]. The running zero before [x] or [hexadecimal digits] 
  will be ignored. Then, the decoding starts after [x] or the first valid hexadecimal 
  digit. Then, the function stops at the first character that is a not a valid 
  hexadecimal character or Null-terminator, whichever on comes first.
  
  If String has only pad spaces, then 0 is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits, then
  0 is returned.

  If the number represented by String overflows according to the range defined by UINTN, 
  then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, 
  and String contains more than PcdMaximumAsciiStringLength ASCII characters not including 
  the Null-terminator, then ASSERT().

  @param  String                Pointer to a Null-terminated ASCII string.

  @return The value of type UINTN converted.

**/
UINTN
EFIAPI
AsciiStrHexToUintn (
  IN      CONST CHAR8             *String
  )
{
  UINTN     Result;

  //
  // ASSERT Strings is less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);
  
  //
  // Ignore the pad spaces (space or tab) 
  //
  while ((*String == ' ') || (*String == '\t' )) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  if (AsciiToUpper (*String) == 'X') {
    ASSERT (*(String - 1) == '0');
    if (*(String - 1) != '0') {
      return 0;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  Result = 0;
  
  while (InternalAsciiIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the Hex Number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
     ASSERT ((Result < QUOTIENT_MAX_UINTN_DIVIDED_BY_16) ||
       ((Result == QUOTIENT_MAX_UINTN_DIVIDED_BY_16) && 
       (InternalAsciiHexCharToUintn (*String) <= REMAINDER_MAX_UINTN_DIVIDED_BY_16))
       );

    Result = (Result << 4) + InternalAsciiHexCharToUintn (*String);
    String++;
  }

  return Result;
}


/**
  Convert a Null-terminated ASCII hexadecimal string to a value of type UINT64.

  This function returns a value of type UINT64 by interpreting the contents of 
  the ASCII string String as a hexadecimal number. The format of the input ASCII 
  string String is:
  
                  [spaces][zeros][x][hexadecimal digits].
                  
  The valid hexadecimal digit character is in the range [0-9], [a-f] and [A-F]. 
  The prefix "0x" is optional. Both "x" and "X" is allowed in "0x" prefix. If "x" 
  appears in the input string, it must be prefixed with at least one 0. The function 
  will ignore the pad space, which includes spaces or tab characters, before [zeros], 
  [x] or [hexadecimal digits]. The running zero before [x] or [hexadecimal digits] 
  will be ignored. Then, the decoding starts after [x] or the first valid hexadecimal 
  digit. Then, the function stops at the first character that is a not a valid 
  hexadecimal character or Null-terminator, whichever on comes first.
  
  If String has only pad spaces, then 0 is returned.
  If String has no leading pad spaces, leading zeros or valid hexadecimal digits, then
  0 is returned.

  If the number represented by String overflows according to the range defined by UINT64, 
  then ASSERT().
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, 
  and String contains more than PcdMaximumAsciiStringLength ASCII characters not including 
  the Null-terminator, then ASSERT().

  @param  String                Pointer to a Null-terminated ASCII string.

  @return The value of type UINT64 converted.

**/
UINT64
EFIAPI
AsciiStrHexToUint64 (
  IN      CONST CHAR8             *String
  )
{
  UINT64    Result;

  //
  // ASSERT Strings is less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);
  
  //
  // Ignore the pad spaces (space or tab) and leading Zeros
  //
  //
  // Ignore the pad spaces (space or tab) 
  //
  while ((*String == ' ') || (*String == '\t' )) {
    String++;
  }

  //
  // Ignore leading Zeros after the spaces
  //
  while (*String == '0') {
    String++;
  }

  if (AsciiToUpper (*String) == 'X') {
    ASSERT (*(String - 1) == '0');
    if (*(String - 1) != '0') {
      return 0;
    }
    //
    // Skip the 'X'
    //
    String++;
  }

  Result = 0;
  
  while (InternalAsciiIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the Hex Number represented by String overflows according 
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT ((Result < QUOTIENT_MAX_UINT64_DIVIDED_BY_16) ||
      ((Result == QUOTIENT_MAX_UINT64_DIVIDED_BY_16) && 
      (InternalAsciiHexCharToUintn (*String) <= REMAINDER_MAX_UINT64_DIVIDED_BY_16))
      );

    Result = LShiftU64 (Result, 4);
    Result = Result + InternalAsciiHexCharToUintn (*String);
    String++;
  }

  return Result;
}


/**
  Convert one Null-terminated ASCII string to a Null-terminated 
  Unicode string and returns the Unicode string.

  This function converts the contents of the ASCII string Source to the Unicode 
  string Destination, and returns Destination.  The function terminates the 
  Unicode string Destination by appending a Null-terminator character at the end. 
  The caller is responsible to make sure Destination points to a buffer with size 
  equal or greater than ((AsciiStrLen (Source) + 1) * sizeof (CHAR16)) in bytes.
  
  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than 
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, 
  then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than 
  PcdMaximumUnicodeStringLength ASCII characters not including the 
  Null-terminator, then ASSERT().

  @param  Source        Pointer to a Null-terminated ASCII string.
  @param  Destination   Pointer to a Null-terminated Unicode string.

  @return Destination pointing to the converted Unicode string.

**/
CHAR16 *
EFIAPI
AsciiStrToUnicodeStr (
  IN      CONST CHAR8               *Source,
  OUT     CHAR16                    *Destination
  )
{
  CHAR16                            *ReturnValue;

  ASSERT (Destination != NULL);

  //
  // ASSERT Source is less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (Source) != 0);

  //
  // Source and Destination should not overlap
  //
  ASSERT ((UINTN) ((CHAR8 *) Destination - Source) > AsciiStrLen (Source));
  ASSERT ((UINTN) (Source - (CHAR8 *) Destination) > (AsciiStrLen (Source) * sizeof (CHAR16)));

 
  ReturnValue = Destination;
  while (*Source != '\0') {
    *(Destination++) = (CHAR16) *(Source++);
  }
  //
  // End the Destination with a NULL.
  //
  *Destination = '\0';

  //
  // ASSERT Original Destination is less long than PcdMaximumUnicodeStringLength
  //
  ASSERT (StrSize (ReturnValue) != 0);

  return ReturnValue;
}

/**
  Converts an 8-bit value to an 8-bit BCD value.

  Converts the 8-bit value specified by Value to BCD. The BCD value is
  returned.

  If Value >= 100, then ASSERT().

  @param  Value The 8-bit value to convert to BCD. Range 0..99.

  @return The BCD value converted.

**/
UINT8
EFIAPI
DecimalToBcd8 (
  IN      UINT8                     Value
  )
{
  ASSERT (Value < 100);
  return (UINT8) (((Value / 10) << 4) | (Value % 10));
}

/**
  Converts an 8-bit BCD value to an 8-bit value.

  Converts the 8-bit BCD value specified by Value to an 8-bit value. The 8-bit
  value is returned.

  If Value >= 0xA0, then ASSERT().
  If (Value & 0x0F) >= 0x0A, then ASSERT().

  @param  Value The 8-bit BCD value to convert to an 8-bit value.

  @return The 8-bit decimal value converted.

**/
UINT8
EFIAPI
BcdToDecimal8 (
  IN      UINT8                     Value
  )
{
  ASSERT (Value < 0xa0);
  ASSERT ((Value & 0xf) < 0xa);
  return (UINT8) ((Value >> 4) * 10 + (Value & 0xf));
}


/**
  Convert a nibble in the low 4 bits of a byte to a Unicode hexadecimal character.

  This function converts a nibble in the low 4 bits of a byte to a Unicode hexadecimal 
  character  For example, the nibble  0x01 and 0x0A will converted to L'1' and L'A' 
  respectively.

  The upper nibble in the input byte will be masked off.

  @param Nibble     The nibble which is in the low 4 bits of the input byte.

  @return   The Unicode hexadecimal character.
  
**/
CHAR16
EFIAPI
NibbleToHexChar (
  IN UINT8      Nibble
  )
{
  Nibble &= 0x0F;
  if (Nibble <= 0x9) {
    return (CHAR16)(Nibble + L'0');
  }

  return (CHAR16)(Nibble - 0xA + L'A');
}

/** 
  Convert binary buffer to a Unicode String in a specified sequence. 

  This function converts bytes in the memory block pointed by Buffer to a Unicode String Str. 
  Each byte will be represented by two Unicode characters. For example, byte 0xA1 will 
  be converted into two Unicode character L'A' and L'1'. In the output String, the Unicode Character 
  for the Most Significant Nibble will be put before the Unicode Character for the Least Significant
  Nibble. The output string for the buffer containing a single byte 0xA1 will be L"A1". 
  For a buffer with multiple bytes, the Unicode character produced by the first byte will be put into the 
  the last character in the output string. The one next to first byte will be put into the
  character before the last character. This rules applies to the rest of the bytes. The Unicode
  character by the last byte will be put into the first character in the output string. For example,
  the input buffer for a 64-bits unsigned integrer 0x12345678abcdef1234 will be converted to
  a Unicode string equal to L"12345678abcdef1234".

  @param String                 Pointer to the buffer allocated for the convertion.
  @param StringLen              On input: Pointer to length in bytes of buffer to hold the Unicode string.
                                On output:If return EFI_SUCCESS, pointer to length of Unicode string converted.
                                          If return EFI_BUFFER_TOO_SMALL, pointer to length of string buffer desired.
  @param Buffer                 The pointer to a input buffer.
  @param BufferSizeInBytes      Lenth in bytes of the input buffer.
  
  @retval EFI_SUCCESS           The convertion is successfull. All bytes in Buffer has been convert to the corresponding
                                Unicode character and placed into the right place in String.
  @retval EFI_BUFFER_TOO_SMALL  StringSizeInBytes is smaller than 2 * N + 1the number of bytes required to
                                complete the convertion. 
**/
RETURN_STATUS
EFIAPI
BufToHexString (
  IN OUT       CHAR16               *String,
  IN OUT       UINTN                *StringLen,
  IN     CONST UINT8                *Buffer,
  IN           UINTN                BufferSizeInBytes
  )
{
  UINTN       Idx;
  UINT8       Byte;
  UINTN       StrLen;

  //
  // Make sure string is either passed or allocate enough.
  // It takes 2 Unicode characters (4 bytes) to represent 1 byte of the binary buffer.
  // Plus the Unicode termination character.
  //
  StrLen = BufferSizeInBytes * 2;
  if (StrLen > ((*StringLen) - 1)) {
    *StringLen = StrLen + 1;
    return RETURN_BUFFER_TOO_SMALL;
  }

  *StringLen = StrLen + 1;
  //
  // Ends the string.
  //
  String[StrLen] = L'\0'; 

  for (Idx = 0; Idx < BufferSizeInBytes; Idx++) {

    Byte = Buffer[Idx];
    String[StrLen - 1 - Idx * 2] = NibbleToHexChar (Byte);
    String[StrLen - 2 - Idx * 2] = NibbleToHexChar ((UINT8)(Byte >> 4));
  }

  return RETURN_SUCCESS;
}


/**
  Convert a Unicode string consisting of hexadecimal characters to a output byte buffer.

  This function converts a Unicode string consisting of characters in the range of Hexadecimal
  character (L'0' to L'9', L'A' to L'F' and L'a' to L'f') to a output byte buffer. The function will stop
  at the first non-hexadecimal character or the NULL character. The convertion process can be
  simply viewed as the reverse operations defined by BufToHexString. Two Unicode characters will be 
  converted into one byte. The first Unicode character represents the Most Significant Nibble and the
  second Unicode character represents the Least Significant Nibble in the output byte. 
  The first pair of Unicode characters represents the last byte in the output buffer. The second pair of Unicode 
  characters represent the  the byte preceding the last byte. This rule applies to the rest pairs of bytes. 
  The last pair represent the first byte in the output buffer. 

  For example, a Unciode String L"12345678" will be converted into a buffer wil the following bytes 
  (first byte is the byte in the lowest memory address): "0x78, 0x56, 0x34, 0x12".

  If String has N valid hexadecimal characters for conversion,  the caller must make sure Buffer is at least 
  N/2 (if N is even) or (N+1)/2 (if N if odd) bytes. 

  @param Buffer                      The output buffer allocated by the caller.
  @param BufferSizeInBytes           On input, the size in bytes of Buffer. On output, it is updated to 
                                     contain the size of the Buffer which is actually used for the converstion.
                                     For Unicode string with 2*N hexadecimal characters (not including the 
                                     tailing NULL character), N bytes of Buffer will be used for the output.
  @param String                      The input hexadecimal string.
  @param ConvertedStrLen             The number of hexadecimal characters used to produce content in output
                                     buffer Buffer.

  @retval  RETURN_BUFFER_TOO_SMALL   The input BufferSizeInBytes is too small to hold the output. BufferSizeInBytes
                                     will be updated to the size required for the converstion.
  @retval  RETURN_SUCCESS            The convertion is successful or the first Unicode character from String
                                     is hexadecimal. If ConvertedStrLen is not NULL, it is updated
                                     to the number of hexadecimal character used for the converstion.
**/
RETURN_STATUS
EFIAPI
HexStringToBuf (
  OUT          UINT8                    *Buffer,   
  IN OUT       UINTN                    *BufferSizeInBytes,
  IN     CONST CHAR16                   *String,
  OUT          UINTN                    *ConvertedStrLen  OPTIONAL
  )
{
  UINTN       HexCnt;
  UINTN       Idx;
  UINTN       BufferLength;
  UINT8       Digit;
  UINT8       Byte;

  //
  // Find out how many hex characters the string has.
  //
  for (Idx = 0, HexCnt = 0; IsHexDigit (&Digit, String[Idx]); Idx++, HexCnt++);

  if (HexCnt == 0) {
    *ConvertedStrLen = 0;
    return RETURN_SUCCESS;
  }
  //
  // Two Unicode characters make up 1 buffer byte. Round up.
  //
  BufferLength = (HexCnt + 1) / 2; 

  //
  // Test if  buffer is passed enough.
  //
  if (BufferLength > (*BufferSizeInBytes)) {
    *BufferSizeInBytes = BufferLength;
    return RETURN_BUFFER_TOO_SMALL;
  }

  *BufferSizeInBytes = BufferLength;

  for (Idx = 0; Idx < HexCnt; Idx++) {

    IsHexDigit (&Digit, String[HexCnt - 1 - Idx]);

    //
    // For odd charaters, write the lower nibble for each buffer byte,
    // and for even characters, the upper nibble.
    //
    if ((Idx & 1) == 0) {
      Byte = Digit;
    } else {
      Byte = Buffer[Idx / 2];
      Byte &= 0x0F;
      Byte = (UINT8) (Byte | Digit << 4);
    }

    Buffer[Idx / 2] = Byte;
  }

  if (ConvertedStrLen != NULL) {
    *ConvertedStrLen = HexCnt;
  }

  return RETURN_SUCCESS;
}


/**
  Test if  a Unicode character is a hexadecimal digit. If true, the input
  Unicode character is converted to a byte. 

  This function tests if a Unicode character is a hexadecimal digit. If true, the input
  Unicode character is converted to a byte. For example, Unicode character
  L'A' will be converted to 0x0A. 

  If Digit is NULL, then ASSERT().
  
  @param  Digit       The output hexadecimal digit.
  @param  Char        The input Unicode character.

  @retval TRUE        Char is in the range of Hexadecimal number. Digit is updated
                      to the byte value of the number.
  @retval FALSE       Char is not in the range of Hexadecimal number. Digit is keep
                      intact.
  
**/
BOOLEAN
EFIAPI
IsHexDigit (
  OUT UINT8      *Digit,
  IN  CHAR16      Char
  )
{
  ASSERT (Digit != NULL);
  
  if ((Char >= L'0') && (Char <= L'9')) {
    *Digit = (UINT8) (Char - L'0');
    return TRUE;
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    *Digit = (UINT8) (Char - L'A' + 0x0A);
    return TRUE;
  }

  if ((Char >= L'a') && (Char <= L'f')) {
    *Digit = (UINT8) (Char - L'a' + 0x0A);
    return TRUE;
  }

  return FALSE;
}


