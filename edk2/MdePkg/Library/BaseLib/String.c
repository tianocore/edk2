/** @file
  Unicode string primatives.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  String.c

**/

/**
  Copies one Null-terminated Unicode string to another Null-terminated Unicode
  string and returns the new Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.

  @return Destiantion

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

  //
  // Destination and source cannot overlap
  //
  ASSERT ((UINTN)(Destination - Source) > StrLen (Source));
  ASSERT ((UINTN)(Source - Destination) > StrLen (Source));

  ReturnValue = Destination;
  while (*Source) {
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

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.
  @param  Length      Maximum number of Unicode characters to copy.

  @return Destination

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

  //
  // Destination and source cannot overlap
  // Q: Does Source have to be NULL-terminated?
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
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

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

  for (Length = 0; *String != L'\0'; String++, Length++) {
    //
    // If PcdMaximumUnicodeStringLength is not zero,
    // length should not more than PcdMaximumUnicodeStringLength
    //
    if (FixedPcdGet32 (PcdMaximumUnicodeStringLength) != 0) {
      ASSERT (Length < FixedPcdGet32 (PcdMaximumUnicodeStringLength));
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
  If PcdMaximumUnicodeStringLength is not zero, and String contains more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  String  Pointer to a Null-terminated Unicode string.

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
  If SecondString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated Unicode string.
  @param  SecondString  Pointer to a Null-terminated Unicode string.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

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

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FirstString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and SecondString contains more
  than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated Unicode string.
  @param  SecondString  Pointer to a Null-terminated Unicode string.
  @param  Length        Maximum number of Unicode characters to compare.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

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
         (*FirstString != *SecondString) &&
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
  than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.

  @return Destination

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
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Destination contains more
  than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and Source contains more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and concatenating Destination
  and Source results in a Unicode string with more than
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated Unicode string.
  @param  Source      Pointer to a Null-terminated Unicode string.
  @param  Length      Maximum number of Unicode characters to concatenate from
                      Source.

  @return Destination

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
  Copies one Null-terminated ASCII string to another Null-terminated ASCII
  string and returns the new ASCII string.

  This function copies the contents of the ASCII string Source to the ASCII
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source and Destination overlap, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.

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
  while (*Source) {
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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.
  @param  Length      Maximum number of ASCII characters to copy.

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

  ReturnValue = Destination;

  while (*Source && Length > 0) {
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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

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
    if (FixedPcdGet32 (PcdMaximumAsciiStringLength) != 0) {
      ASSERT (Length < FixedPcdGet32 (PcdMaximumAsciiStringLength));
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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

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

STATIC
CHAR8
EFIAPI
AsciiToUpper (
  IN      CHAR8                     Chr
  )
{
  return (Chr >= 'a' && Chr <= 'z') ? Chr - ('a' - 'A') : Chr;
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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0   FirstString is identical to SecondString using case insensitive
              comparisons.
  @retval !=0 FirstString is not identical to SecondString using case
              insensitive comparisons.

**/
INTN
EFIAPI
AsciiStriCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  while ((*FirstString != '\0') &&
         (AsciiToUpper (*FirstString) == AsciiToUpper (*SecondString))) {
    FirstString++;
    SecondString++;
  }

  return AsciiToUpper (*FirstString) - AsciiToUpper (*SecondString);
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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and SecondString contains more
  than PcdMaximumAsciiStringLength ASCII characters, then ASSERT().

  @param  FirstString   Pointer to a Null-terminated ASCII string.
  @param  SecondString  Pointer to a Null-terminated ASCII string.

  @retval 0   FirstString is identical to SecondString.
  @retval !=0 FirstString is not identical to SecondString.

**/
INTN
EFIAPI
AsciiStrnCmp (
  IN      CONST CHAR8               *FirstString,
  IN      CONST CHAR8               *SecondString,
  IN      UINTN                     Length
  )
{
  //
  // ASSERT both strings are less long than PcdMaximumAsciiStringLength
  //
  ASSERT (AsciiStrSize (FirstString));
  ASSERT (AsciiStrSize (SecondString));

  while ((*FirstString != '\0') &&
         (*FirstString != *SecondString) &&
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
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.

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
  If PcdMaximumAsciiStringLength is not zero, and Destination contains more
  than PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and Source contains more than
  PcdMaximumAsciiStringLength ASCII characters, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and concatenating Destination and
  Source results in a ASCII string with more than PcdMaximumAsciiStringLength
  ASCII characters, then ASSERT().

  @param  Destination Pointer to a Null-terminated ASCII string.
  @param  Source      Pointer to a Null-terminated ASCII string.
  @param  Length      Maximum number of ASCII characters to concatenate from
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
  AsciiStrnCpy (Destination + AsciiStrLen (Destination), Source, Length);

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside StrLen().
  //
  ASSERT (AsciiStrSize (Destination) != 0);
  return Destination;
}
