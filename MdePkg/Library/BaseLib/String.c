/** @file
  Unicode and ASCII string primitives.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BaseLibInternals.h"

#ifndef DISABLE_NEW_DEPRECATED_INTERFACES

/**
  [ATTENTION] This function will be deprecated for security reason.

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
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.

  @return Destination.

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
  [ATTENTION] This function will be deprecated for security reason.

  Copies up to a specified length from one Null-terminated Unicode string  to
  another Null-terminated Unicode string and returns the new Unicode string.

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
  If Length > 0 and Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.
  @param  Length      The maximum number of Unicode characters to copy.

  @return Destination.

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

  if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumUnicodeStringLength));
  }

  ReturnValue = Destination;

  while ((*Source != L'\0') && (Length > 0)) {
    *(Destination++) = *(Source++);
    Length--;
  }

  ZeroMem (Destination, Length * sizeof (*Destination));
  return ReturnValue;
}
#endif

/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  String  A pointer to a Null-terminated Unicode string.

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

  This function returns the size, in bytes, of the Null-terminated Unicode string
  specified by String.

  If String is NULL, then ASSERT().
  If String is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  String  A pointer to a Null-terminated Unicode string.

  @return The size of String.

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
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.

  @retval 0      FirstString is identical to SecondString.
  @return others FirstString is not identical to SecondString.

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
  Compares up to a specified length the contents of two Null-terminated Unicode strings,
  and returns the difference between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. At most, Length Unicode
  characters will be compared. If Length is 0, then 0 is returned. If
  FirstString is identical to SecondString, then 0 is returned. Otherwise, the
  value returned is the first mismatched Unicode character in SecondString
  subtracted from the first mismatched Unicode character in FirstString.

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and FirstString is not aligned on a 16-bit boundary, then ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If Length > 0 and SecondString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.
  @param  Length        The maximum number of Unicode characters to compare.

  @retval 0      FirstString is identical to SecondString.
  @return others FirstString is not identical to SecondString.

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

  if (PcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumUnicodeStringLength));
  }

  while ((*FirstString != L'\0') &&
         (*SecondString != L'\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    Length--;
  }

  return *FirstString - *SecondString;
}

#ifndef DISABLE_NEW_DEPRECATED_INTERFACES

/**
  [ATTENTION] This function will be deprecated for security reason.

  Concatenates one Null-terminated Unicode string to another Null-terminated
  Unicode string, and returns the concatenated Unicode string.

  This function concatenates two Null-terminated Unicode strings. The contents
  of Null-terminated Unicode string Source are concatenated to the end of
  Null-terminated Unicode string Destination. The Null-terminated concatenated
  Unicode String is returned. If Source and Destination overlap, then the
  results are undefined.

  If Destination is NULL, then ASSERT().
  If Destination is not aligned on a 16-bit boundary, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.

  @return Destination.

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
  [ATTENTION] This function will be deprecated for security reason.

  Concatenates up to a specified length one Null-terminated Unicode to the end
  of another Null-terminated Unicode string, and returns the concatenated
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
  If PcdMaximumUnicodeStringLength is not zero, and Length is greater than
  PcdMaximumUnicodeStringLength, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than PcdMaximumUnicodeStringLength
  Unicode characters, not including the Null-terminator, then ASSERT().

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.
  @param  Length      The maximum number of Unicode characters to concatenate from
                      Source.

  @return Destination.

**/
CHAR16 *
EFIAPI
StrnCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source,
  IN      UINTN                     Length
  )
{
  UINTN   DestinationLen;

  DestinationLen = StrLen (Destination);
  StrnCpy (Destination + DestinationLen, Source, Length);
  Destination[DestinationLen + Length] = L'\0';

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (StrSize (Destination) != 0);
  return Destination;
}
#endif

/**
  Returns the first occurrence of a Null-terminated Unicode sub-string
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
  characters, not including the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.
  @param  SearchString    A pointer to a Null-terminated Unicode string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @return others          If there is a match.

**/
CHAR16 *
EFIAPI
StrStr (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
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

  if (*SearchString == L'\0') {
    return (CHAR16 *) String;
  }

  while (*String != L'\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;

    while ((*String == *SearchStringTmp)
            && (*String != L'\0')) {
      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == L'\0') {
      return (CHAR16 *) FirstMatch;
    }

    if (*String == L'\0') {
      return NULL;
    }

    String = FirstMatch + 1;
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
CharToUpper (
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

  return (10 + CharToUpper (Char) - L'A');
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
  to the range defined by UINTN, then MAX_UINTN is returned.

  If PcdMaximumUnicodeStringLength is not zero, and String contains
  more than PcdMaximumUnicodeStringLength Unicode characters, not including
  the Null-terminator, then ASSERT().

  @param  String      A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINTN
EFIAPI
StrDecimalToUintn (
  IN      CONST CHAR16              *String
  )
{
  UINTN     Result;

  StrDecimalToUintnS (String, (CHAR16 **) NULL, &Result);
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
  to the range defined by UINT64, then MAX_UINT64 is returned.

  If PcdMaximumUnicodeStringLength is not zero, and String contains
  more than PcdMaximumUnicodeStringLength Unicode characters, not including
  the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINT64
EFIAPI
StrDecimalToUint64 (
  IN      CONST CHAR16              *String
  )
{
  UINT64     Result;

  StrDecimalToUint64S (String, (CHAR16 **) NULL, &Result);
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
  UINTN, then MAX_UINTN is returned.

  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINTN
EFIAPI
StrHexToUintn (
  IN      CONST CHAR16              *String
  )
{
  UINTN     Result;

  StrHexToUintnS (String, (CHAR16 **) NULL, &Result);
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
  UINT64, then MAX_UINT64 is returned.

  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINT64
EFIAPI
StrHexToUint64 (
  IN      CONST CHAR16             *String
  )
{
  UINT64    Result;

  StrHexToUint64S (String, (CHAR16 **) NULL, &Result);
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

#ifndef DISABLE_NEW_DEPRECATED_INTERFACES

/**
  [ATTENTION] This function is deprecated for security reason.

  Convert a Null-terminated Unicode string to a Null-terminated
  ASCII string and returns the ASCII string.

  This function converts the content of the Unicode string Source
  to the ASCII string Destination by copying the lower 8 bits of
  each Unicode character. It returns Destination.

  The caller is responsible to make sure Destination points to a buffer with size
  equal or greater than ((StrLen (Source) + 1) * sizeof (CHAR8)) in bytes.

  If any Unicode characters in Source contain non-zero value in
  the upper 8 bits, then ASSERT().

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().

  If PcdMaximumUnicodeStringLength is not zero, and Source contains
  more than PcdMaximumUnicodeStringLength Unicode characters, not including
  the Null-terminator, then ASSERT().

  If PcdMaximumAsciiStringLength is not zero, and Source contains more
  than PcdMaximumAsciiStringLength Unicode characters, not including the
  Null-terminator, then ASSERT().

  @param  Source        A pointer to a Null-terminated Unicode string.
  @param  Destination   A pointer to a Null-terminated ASCII string.

  @return Destination.

**/
CHAR8 *
EFIAPI
UnicodeStrToAsciiStr (
  IN      CONST CHAR16              *Source,
  OUT     CHAR8                     *Destination
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
  ASSERT ((UINTN) (Destination - (CHAR8 *) Source) >= StrSize (Source));
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
  [ATTENTION] This function will be deprecated for security reason.

  Copies one Null-terminated ASCII string to another Null-terminated ASCII
  string and returns the new ASCII string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().

  @param  Destination A pointer to a Null-terminated ASCII string.
  @param  Source      A pointer to a Null-terminated ASCII string.

  @return Destination

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
  [ATTENTION] This function will be deprecated for security reason.

  Copies up to a specified length one Null-terminated ASCII string to another
  Null-terminated ASCII string and returns the new ASCII string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. At most, Length ASCII characters
  are copied from Source to Destination. If Length is 0, then Destination is
  returned unmodified. If Length is greater that the number of ASCII characters
  in Source, then Destination is padded with Null ASCII characters. If Source
  and Destination overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Length is greater than
  PcdMaximumAsciiStringLength, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().

  @param  Destination A pointer to a Null-terminated ASCII string.
  @param  Source      A pointer to a Null-terminated ASCII string.
  @param  Length      The maximum number of ASCII characters to copy.

  @return Destination

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

  if (PcdGet32 (PcdMaximumAsciiStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumAsciiStringLength));
  }

  ReturnValue = Destination;

  while (*Source != 0 && Length > 0) {
    *(Destination++) = *(Source++);
    Length--;
  }

  ZeroMem (Destination, Length * sizeof (*Destination));
  return ReturnValue;
}
#endif

/**
  Returns the length of a Null-terminated ASCII string.

  This function returns the number of ASCII characters in the Null-terminated
  ASCII string specified by String.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and String contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().

  @param  String  A pointer to a Null-terminated ASCII string.

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
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().

  @param  String  A pointer to a Null-terminated ASCII string.

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
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters, not including the
  Null-terminator, then ASSERT().

  @param  FirstString   A pointer to a Null-terminated ASCII string.
  @param  SecondString  A pointer to a Null-terminated ASCII string.

  @retval ==0      FirstString is identical to SecondString.
  @retval !=0      FirstString is not identical to SecondString.

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
AsciiCharToUpper (
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

  return (10 + AsciiCharToUpper (Char) - 'A');
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
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters, not including the
  Null-terminator, then ASSERT().

  @param  FirstString   A pointer to a Null-terminated ASCII string.
  @param  SecondString  A pointer to a Null-terminated ASCII string.

  @retval ==0    FirstString is identical to SecondString using case insensitive
                 comparisons.
  @retval !=0    FirstString is not identical to SecondString using case
                 insensitive comparisons.

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

  UpperFirstString  = AsciiCharToUpper (*FirstString);
  UpperSecondString = AsciiCharToUpper (*SecondString);
  while ((*FirstString != '\0') && (*SecondString != '\0') && (UpperFirstString == UpperSecondString)) {
    FirstString++;
    SecondString++;
    UpperFirstString  = AsciiCharToUpper (*FirstString);
    UpperSecondString = AsciiCharToUpper (*SecondString);
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

  If Length > 0 and FirstString is NULL, then ASSERT().
  If Length > 0 and SecondString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Length is greater than
  PcdMaximumAsciiStringLength, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and FirstString contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and SecondString contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().

  @param  FirstString   A pointer to a Null-terminated ASCII string.
  @param  SecondString  A pointer to a Null-terminated ASCII string.
  @param  Length        The maximum number of ASCII characters for compare.

  @retval ==0       FirstString is identical to SecondString.
  @retval !=0       FirstString is not identical to SecondString.

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

  if (PcdGet32 (PcdMaximumAsciiStringLength) != 0) {
    ASSERT (Length <= PcdGet32 (PcdMaximumAsciiStringLength));
  }

  while ((*FirstString != '\0') &&
         (*SecondString != '\0') &&
         (*FirstString == *SecondString) &&
         (Length > 1)) {
    FirstString++;
    SecondString++;
    Length--;
  }
  return *FirstString - *SecondString;
}

#ifndef DISABLE_NEW_DEPRECATED_INTERFACES

/**
  [ATTENTION] This function will be deprecated for security reason.

  Concatenates one Null-terminated ASCII string to another Null-terminated
  ASCII string, and returns the concatenated ASCII string.

  This function concatenates two Null-terminated ASCII strings. The contents of
  Null-terminated ASCII string Source are concatenated to the end of Null-
  terminated ASCII string Destination. The Null-terminated concatenated ASCII
  String is returned.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Destination contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters, then ASSERT().

  @param  Destination A pointer to a Null-terminated ASCII string.
  @param  Source      A pointer to a Null-terminated ASCII string.

  @return Destination

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
  [ATTENTION] This function will be deprecated for security reason.

  Concatenates up to a specified length one Null-terminated ASCII string to
  the end of another Null-terminated ASCII string, and returns the
  concatenated ASCII string.

  This function concatenates two Null-terminated ASCII strings. The contents
  of Null-terminated ASCII string Source are concatenated to the end of Null-
  terminated ASCII string Destination, and Destination is returned. At most,
  Length ASCII characters are concatenated from Source to the end of
  Destination, and Destination is always Null-terminated. If Length is 0, then
  Destination is returned unmodified. If Source and Destination overlap, then
  the results are undefined.

  If Length > 0 and Destination is NULL, then ASSERT().
  If Length > 0 and Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Length is greater than
  PcdMaximumAsciiStringLength, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Destination contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, not including the Null-terminator,
  then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters, not including the Null-terminator, then ASSERT().

  @param  Destination A pointer to a Null-terminated ASCII string.
  @param  Source      A pointer to a Null-terminated ASCII string.
  @param  Length      The maximum number of ASCII characters to concatenate from
                      Source.

  @return Destination

**/
CHAR8 *
EFIAPI
AsciiStrnCat (
  IN OUT  CHAR8                     *Destination,
  IN      CONST CHAR8               *Source,
  IN      UINTN                     Length
  )
{
  UINTN   DestinationLen;

  DestinationLen = AsciiStrLen (Destination);
  AsciiStrnCpy (Destination + DestinationLen, Source, Length);
  Destination[DestinationLen + Length] = '\0';

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (AsciiStrSize (Destination) != 0);
  return Destination;
}
#endif

/**
  Returns the first occurrence of a Null-terminated ASCII sub-string
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

  @param  String          A pointer to a Null-terminated ASCII string.
  @param  SearchString    A pointer to a Null-terminated ASCII string to search for.

  @retval NULL            If the SearchString does not appear in String.
  @retval others          If there is a match return the first occurrence of SearchingString.
                          If the length of SearchString is zero,return String.

**/
CHAR8 *
EFIAPI
AsciiStrStr (
  IN      CONST CHAR8               *String,
  IN      CONST CHAR8               *SearchString
  )
{
  CONST CHAR8 *FirstMatch;
  CONST CHAR8 *SearchStringTmp;

  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (String) != 0);
  ASSERT (AsciiStrSize (SearchString) != 0);

  if (*SearchString == '\0') {
    return (CHAR8 *) String;
  }

  while (*String != '\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;

    while ((*String == *SearchStringTmp)
            && (*String != '\0')) {
      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == '\0') {
      return (CHAR8 *) FirstMatch;
    }

    if (*String == '\0') {
      return NULL;
    }

    String = FirstMatch + 1;
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
  UINTN, then MAX_UINTN is returned.
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String          A pointer to a Null-terminated ASCII string.

  @retval Value translated from String.

**/
UINTN
EFIAPI
AsciiStrDecimalToUintn (
  IN      CONST CHAR8               *String
  )
{
  UINTN     Result;

  AsciiStrDecimalToUintnS (String, (CHAR8 **) NULL, &Result);
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
  UINT64, then MAX_UINT64 is returned.
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and String contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator,
  then ASSERT().

  @param  String          A pointer to a Null-terminated ASCII string.

  @retval Value translated from String.

**/
UINT64
EFIAPI
AsciiStrDecimalToUint64 (
  IN      CONST CHAR8               *String
  )
{
  UINT64     Result;

  AsciiStrDecimalToUint64S (String, (CHAR8 **) NULL, &Result);
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
  then MAX_UINTN is returned.
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero,
  and String contains more than PcdMaximumAsciiStringLength ASCII characters not including
  the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated ASCII string.

  @retval Value translated from String.

**/
UINTN
EFIAPI
AsciiStrHexToUintn (
  IN      CONST CHAR8               *String
  )
{
  UINTN     Result;

  AsciiStrHexToUintnS (String, (CHAR8 **) NULL, &Result);
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
  then MAX_UINT64 is returned.
  If String is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero,
  and String contains more than PcdMaximumAsciiStringLength ASCII characters not including
  the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated ASCII string.

  @retval Value translated from String.

**/
UINT64
EFIAPI
AsciiStrHexToUint64 (
  IN      CONST CHAR8                *String
  )
{
  UINT64    Result;

  AsciiStrHexToUint64S (String, (CHAR8 **) NULL, &Result);
  return Result;
}

#ifndef DISABLE_NEW_DEPRECATED_INTERFACES

/**
  [ATTENTION] This function is deprecated for security reason.

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

  @param  Source        A pointer to a Null-terminated ASCII string.
  @param  Destination   A pointer to a Null-terminated Unicode string.

  @return Destination.

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
  ASSERT ((UINTN) (Source - (CHAR8 *) Destination) >= (AsciiStrSize (Source) * sizeof (CHAR16)));


  ReturnValue = Destination;
  while (*Source != '\0') {
    *(Destination++) = (CHAR16)(UINT8) *(Source++);
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

#endif

//
// The basis for Base64 encoding is RFC 4686 https://tools.ietf.org/html/rfc4648
//
// RFC 4686 has a number of MAY and SHOULD cases.  This implementation chooses
// the more restrictive versions for security concerns (see RFC 4686 section 3.3).
//
// A invalid character, if encountered during the decode operation, causes the data
// to be rejected. In addition, the '=' padding character is only allowed at the end
// of the Base64 encoded string.
//
#define BAD_V  99

STATIC CHAR8 EncodingTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                "abcdefghijklmnopqrstuvwxyz"
                                "0123456789+/";

STATIC UINT8 DecodingTable[] = {
  //
  // Valid characters ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/
  // Also, set '=' as a zero for decoding
  // 0  ,            1,           2,           3,            4,           5,            6,           7,           8,            9,           a,            b,            c,           d,            e,            f
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //   0
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  10
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,     62,  BAD_V,  BAD_V,  BAD_V,     63,   //  20
     52,     53,     54,     55,     56,     57,     58,     59,     60,     61,  BAD_V,  BAD_V,  BAD_V,      0,  BAD_V,  BAD_V,   //  30
  BAD_V,      0,      1,      2,      3,      4,      5,      6,      7,      8,      9,     10,     11,     12,     13,     14,   //  40
     15,     16,     17,     18,     19,     20,     21,     22,     23,     24,     25,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  50
  BAD_V,     26,     27,     28,     29,     30,     31,     32,     33,     34,     35,     36,     37,     38,     39,     40,   //  60
     41,     42,     43,     44,     45,     46,     47,     48,     49,     50,     51,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  70
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  80
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  90
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  a0
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  b0
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  c0
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  d0
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,   //  d0
  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V,  BAD_V    //  f0
};

/**
  Convert binary data to a Base64 encoded ascii string based on RFC4648.

  Produce a Null-terminated Ascii string in the output buffer specified by Destination and DestinationSize.
  The Ascii string is produced by converting the data string specified by Source and SourceLength.

  @param Source            Input UINT8 data
  @param SourceLength      Number of UINT8 bytes of data
  @param Destination       Pointer to output string buffer
  @param DestinationSize   Size of ascii buffer. Set to 0 to get the size needed.
                           Caller is responsible for passing in buffer of DestinationSize

  @retval RETURN_SUCCESS             When ascii buffer is filled in.
  @retval RETURN_INVALID_PARAMETER   If Source is NULL or DestinationSize is NULL.
  @retval RETURN_INVALID_PARAMETER   If SourceLength or DestinationSize is bigger than (MAX_ADDRESS - (UINTN)Destination).
  @retval RETURN_BUFFER_TOO_SMALL    If SourceLength is 0 and DestinationSize is <1.
  @retval RETURN_BUFFER_TOO_SMALL    If Destination is NULL or DestinationSize is smaller than required buffersize.

**/
RETURN_STATUS
EFIAPI
Base64Encode (
  IN  CONST UINT8  *Source,
  IN        UINTN   SourceLength,
  OUT       CHAR8  *Destination   OPTIONAL,
  IN OUT    UINTN  *DestinationSize
  )
{

  UINTN          RequiredSize;
  UINTN          Left;

  //
  // Check pointers, and SourceLength is valid
  //
  if ((Source == NULL) || (DestinationSize == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Allow for RFC 4648 test vector 1
  //
  if (SourceLength == 0) {
    if (*DestinationSize < 1) {
      *DestinationSize = 1;
      return RETURN_BUFFER_TOO_SMALL;
    }
    *DestinationSize = 1;
    *Destination = '\0';
    return RETURN_SUCCESS;
  }

  //
  // Check if SourceLength or  DestinationSize is valid
  //
  if ((SourceLength >= (MAX_ADDRESS - (UINTN)Source)) || (*DestinationSize >= (MAX_ADDRESS - (UINTN)Destination))){
    return RETURN_INVALID_PARAMETER;
  }

  //
  // 4 ascii per 3 bytes + NULL
  //
  RequiredSize = ((SourceLength + 2) / 3) * 4 + 1;
  if ((Destination == NULL) || *DestinationSize < RequiredSize) {
    *DestinationSize = RequiredSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  Left = SourceLength;

  //
  // Encode 24 bits (three bytes) into 4 ascii characters
  //
  while (Left >= 3) {

    *Destination++ = EncodingTable[( Source[0] & 0xfc) >> 2 ];
    *Destination++ = EncodingTable[((Source[0] & 0x03) << 4) + ((Source[1] & 0xf0) >> 4)];
    *Destination++ = EncodingTable[((Source[1] & 0x0f) << 2) + ((Source[2] & 0xc0) >> 6)];
    *Destination++ = EncodingTable[( Source[2] & 0x3f)];
    Left -= 3;
    Source += 3;
  }

  //
  // Handle the remainder, and add padding '=' characters as necessary.
  //
  switch (Left) {
    case 0:

      //
      // No bytes Left, done.
      //
      break;
    case 1:

      //
      // One more data byte, two pad characters
      //
      *Destination++ = EncodingTable[( Source[0] & 0xfc) >> 2];
      *Destination++ = EncodingTable[((Source[0] & 0x03) << 4)];
      *Destination++ = '=';
      *Destination++ = '=';
      break;
    case 2:

      //
      // Two more data bytes, and one pad character
      //
      *Destination++ = EncodingTable[( Source[0] & 0xfc) >> 2];
      *Destination++ = EncodingTable[((Source[0] & 0x03) << 4) + ((Source[1] & 0xf0) >> 4)];
      *Destination++ = EncodingTable[((Source[1] & 0x0f) << 2)];
      *Destination++ = '=';
      break;
    }
  //
  // Add terminating NULL
  //
  *Destination = '\0';
  return RETURN_SUCCESS;
}

/**
  Convert Base64 ascii string to binary data based on RFC4648.

  Produce Null-terminated binary data in the output buffer specified by Destination and DestinationSize.
  The binary data is produced by converting the Base64 ascii string specified by Source and SourceLength.

  @param Source            Input ASCII characters
  @param SourceLength      Number of ASCII characters
  @param Destination       Pointer to output buffer
  @param DestinationSize   Caller is responsible for passing in buffer of at least DestinationSize.
                           Set 0 to get the size needed. Set to bytes stored on return.

  @retval RETURN_SUCCESS             When binary buffer is filled in.
  @retval RETURN_INVALID_PARAMETER   If Source is NULL or DestinationSize is NULL.
  @retval RETURN_INVALID_PARAMETER   If SourceLength or DestinationSize is bigger than (MAX_ADDRESS -(UINTN)Destination ).
  @retval RETURN_INVALID_PARAMETER   If there is any invalid character in input stream.
  @retval RETURN_BUFFER_TOO_SMALL    If buffer length is smaller than required buffer size.
 **/
RETURN_STATUS
EFIAPI
Base64Decode (
  IN  CONST CHAR8  *Source,
  IN        UINTN   SourceLength,
  OUT       UINT8  *Destination   OPTIONAL,
  IN OUT    UINTN  *DestinationSize
  )
{

  UINT32   Value;
  CHAR8    Chr;
  INTN     BufferSize;
  UINTN    SourceIndex;
  UINTN    DestinationIndex;
  UINTN    Index;
  UINTN    ActualSourceLength;

  //
  // Check pointers are not NULL
  //
  if ((Source == NULL) || (DestinationSize == NULL)) {
    return RETURN_INVALID_PARAMETER;
  }

  //
  // Check if SourceLength or  DestinationSize is valid
  //
  if ((SourceLength >= (MAX_ADDRESS - (UINTN)Source)) || (*DestinationSize >= (MAX_ADDRESS - (UINTN)Destination))){
    return RETURN_INVALID_PARAMETER;
  }

  ActualSourceLength = 0;
  BufferSize = 0;

  //
  // Determine the actual number of valid characters in the string.
  // All invalid characters except selected white space characters,
  // will cause the Base64 string to be rejected. White space to allow
  // properly formatted XML will be ignored.
  //
  // See section 3.3 of RFC 4648.
  //
  for (SourceIndex = 0; SourceIndex < SourceLength; SourceIndex++) {

    //
    // '=' is part of the quantum
    //
    if (Source[SourceIndex] == '=') {
      ActualSourceLength++;
      BufferSize--;

      //
      // Only two '=' characters can be valid.
      //
      if (BufferSize < -2) {
        return RETURN_INVALID_PARAMETER;
      }
    }
    else {
      Chr = Source[SourceIndex];
      if (BAD_V != DecodingTable[(UINT8) Chr]) {

        //
        // The '=' characters are only valid at the end, so any
        // valid character after an '=', will be flagged as an error.
        //
        if (BufferSize < 0) {
          return RETURN_INVALID_PARAMETER;
        }
          ActualSourceLength++;
      }
        else {

        //
        // The reset of the decoder will ignore all invalid characters allowed here.
        // Ignoring selected white space is useful.  In this case, the decoder will
        // ignore ' ', '\t', '\n', and '\r'.
        //
        if ((Chr != ' ') &&(Chr != '\t') &&(Chr != '\n') &&(Chr != '\r')) {
          return RETURN_INVALID_PARAMETER;
        }
      }
    }
  }

  //
  // The Base64 character string must be a multiple of 4 character quantums.
  //
  if (ActualSourceLength % 4 != 0) {
    return RETURN_INVALID_PARAMETER;
  }

  BufferSize += ActualSourceLength / 4 * 3;
    if (BufferSize < 0) {
      return RETURN_INVALID_PARAMETER;
  }

  //
  // BufferSize is >= 0
  //
  if ((Destination == NULL) || (*DestinationSize < (UINTN) BufferSize)) {
    *DestinationSize = BufferSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  //
  // If no decodable characters, return a size of zero. RFC 4686 test vector 1.
  //
  if (ActualSourceLength == 0) {
    *DestinationSize = 0;
    return RETURN_SUCCESS;
  }

  //
  // Input data is verified to be a multiple of 4 valid charcters.  Process four
  // characters at a time. Uncounted (ie. invalid)  characters will be ignored.
  //
  for (SourceIndex = 0, DestinationIndex = 0; (SourceIndex < SourceLength) && (DestinationIndex < *DestinationSize); ) {
    Value = 0;

    //
    // Get 24 bits of data from 4 input characters, each character representing 6 bits
    //
    for (Index = 0; Index < 4; Index++) {
      do {
      Chr = DecodingTable[(UINT8) Source[SourceIndex++]];
      } while (Chr == BAD_V);
      Value <<= 6;
      Value |= (UINT32)Chr;
    }

    //
    // Store 3 bytes of binary data (24 bits)
    //
    *Destination++ = (UINT8) (Value >> 16);
    DestinationIndex++;

    //
    // Due to the '=' special cases for the two bytes at the end,
    // we have to check the length and not store the padding data
    //
    if (DestinationIndex++ < *DestinationSize) {
      *Destination++ = (UINT8) (Value >>  8);
    }
    if (DestinationIndex++ < *DestinationSize) {
      *Destination++ = (UINT8) Value;
    }
  }

  return RETURN_SUCCESS;
}

/**
  Converts an 8-bit value to an 8-bit BCD value.

  Converts the 8-bit value specified by Value to BCD. The BCD value is
  returned.

  If Value >= 100, then ASSERT().

  @param  Value The 8-bit value to convert to BCD. Range 0..99.

  @return The BCD value.

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

  @return The 8-bit value is returned.

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
