/** @file

 Common library.

 Copyright (c) 2011-2019, Intel Corporation. All rights reserved.<BR>
 SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "Common.h"

#define WARNING_STATUS_NUMBER         4
#define ERROR_STATUS_NUMBER           24

CONST CHAR8 mHexStr[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

CONST CHAR8 *mStatusString[] = {
  "Success",                      //  RETURN_SUCCESS                = 0
  "Warning Unknown Glyph",        //  RETURN_WARN_UNKNOWN_GLYPH     = 1
  "Warning Delete Failure",       //  RETURN_WARN_DELETE_FAILURE    = 2
  "Warning Write Failure",        //  RETURN_WARN_WRITE_FAILURE     = 3
  "Warning Buffer Too Small",     //  RETURN_WARN_BUFFER_TOO_SMALL  = 4
  "Load Error",                   //  RETURN_LOAD_ERROR             = 1  | MAX_BIT
  "Invalid Parameter",            //  RETURN_INVALID_PARAMETER      = 2  | MAX_BIT
  "Unsupported",                  //  RETURN_UNSUPPORTED            = 3  | MAX_BIT
  "Bad Buffer Size",              //  RETURN_BAD_BUFFER_SIZE        = 4  | MAX_BIT
  "Buffer Too Small",             //  RETURN_BUFFER_TOO_SMALL,      = 5  | MAX_BIT
  "Not Ready",                    //  RETURN_NOT_READY              = 6  | MAX_BIT
  "Device Error",                 //  RETURN_DEVICE_ERROR           = 7  | MAX_BIT
  "Write Protected",              //  RETURN_WRITE_PROTECTED        = 8  | MAX_BIT
  "Out of Resources",             //  RETURN_OUT_OF_RESOURCES       = 9  | MAX_BIT
  "Volume Corrupt",               //  RETURN_VOLUME_CORRUPTED       = 10 | MAX_BIT
  "Volume Full",                  //  RETURN_VOLUME_FULL            = 11 | MAX_BIT
  "No Media",                     //  RETURN_NO_MEDIA               = 12 | MAX_BIT
  "Media changed",                //  RETURN_MEDIA_CHANGED          = 13 | MAX_BIT
  "Not Found",                    //  RETURN_NOT_FOUND              = 14 | MAX_BIT
  "Access Denied",                //  RETURN_ACCESS_DENIED          = 15 | MAX_BIT
  "No Response",                  //  RETURN_NO_RESPONSE            = 16 | MAX_BIT
  "No mapping",                   //  RETURN_NO_MAPPING             = 17 | MAX_BIT
  "Time out",                     //  RETURN_TIMEOUT                = 18 | MAX_BIT
  "Not started",                  //  RETURN_NOT_STARTED            = 19 | MAX_BIT
  "Already started",              //  RETURN_ALREADY_STARTED        = 20 | MAX_BIT
  "Aborted",                      //  RETURN_ABORTED                = 21 | MAX_BIT
  "ICMP Error",                   //  RETURN_ICMP_ERROR             = 22 | MAX_BIT
  "TFTP Error",                   //  RETURN_TFTP_ERROR             = 23 | MAX_BIT
  "Protocol Error"                //  RETURN_PROTOCOL_ERROR         = 24 | MAX_BIT
};

/**
  Copies one Null-terminated Unicode string to another Null-terminated Unicode
  string and returns the new Unicode string.

  This function copies the contents of the Unicode string Source to the Unicode
  string Destination, and returns Destination. If Source and Destination
  overlap, then the results are undefined.

  If Destination is NULL, then return NULL.
  If Destination is not aligned on a 16-bit boundary, then return NULL.

  @param  Destination A pointer to a Null-terminated Unicode string.
  @param  Source      A pointer to a Null-terminated Unicode string.

  @return Destination.

**/
CHAR16 *
StrCpy (
  OUT     CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  )
{
  CHAR16                            *ReturnValue;

  ReturnValue = NULL;

  if ((Destination == NULL) || ((UINTN) Destination % 2 != 0)) {
    return NULL;
  }

  ReturnValue = Destination;
  while (*Source != 0) {
    *(Destination++) = *(Source++);
  }
  *Destination = 0;
  return ReturnValue;
}

/**
  Returns the length of a Null-terminated Unicode string.

  This function returns the number of Unicode characters in the Null-terminated
  Unicode string specified by String.

  If String is NULL, then return 0.

  @param  String  A pointer to a Null-terminated Unicode string.

  @return The length of String.

**/
UINTN
FceStrLen (
  IN      CONST CHAR16              *String
  )
{
  UINTN           Length;

  if (String == NULL) {
    return 0;
  }
  for (Length = 0; *String != L'\0'; String++, Length++) {
    ;
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
FceStrSize (
  IN      CONST CHAR16              *String
  )
{
  return (FceStrLen (String) + 1) * sizeof (*String);
}

/**
  Compares two Null-terminated Unicode strings, and returns the difference
  between the first mismatched Unicode characters.

  This function compares the Null-terminated Unicode string FirstString to the
  Null-terminated Unicode string SecondString. If FirstString is identical to
  SecondString, then 0 is returned. Otherwise, the value returned is the first
  mismatched Unicode character in SecondString subtracted from the first
  mismatched Unicode character in FirstString.

  @param  FirstString   A pointer to a Null-terminated Unicode string.
  @param  SecondString  A pointer to a Null-terminated Unicode string.

  @retval 0      FirstString is identical to SecondString.
  @return others FirstString is not identical to SecondString.

**/
INTN
FceStrCmp (
  IN      CONST CHAR16              *FirstString,
  IN      CONST CHAR16              *SecondString
  )
{
  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
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
StrCat (
  IN OUT  CHAR16                    *Destination,
  IN      CONST CHAR16              *Source
  )
{
  StrCpy (Destination + FceStrLen (Destination), Source);

  //
  // Size of the resulting string should never be zero.
  // PcdMaximumUnicodeStringLength is tested inside FceStrLen().
  //
  ASSERT (FceStrSize (Destination) != 0);
  return Destination;
}

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
StrStr (
  IN      CONST CHAR16              *String,
  IN      CONST CHAR16              *SearchString
  )
{
  CONST CHAR16 *FirstMatch;
  CONST CHAR16 *SearchStringTmp;

  //
  // ASSERT both strings are less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside FceStrLen().
  //
  ASSERT (FceStrSize (String) != 0);
  ASSERT (FceStrSize (SearchString) != 0);

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
  Convert one Null-terminated ASCII string to a Null-terminated
  Unicode string and returns the Unicode string.

  This function converts the contents of the ASCII string Source to the Unicode
  string Destination, and returns Destination.  The function terminates the
  Unicode string Destination by appending a Null-terminator character at the end.
  The caller is responsible to make sure Destination points to a buffer with size
  equal or greater than ((AsciiStrLen (Source) + 1) * sizeof (CHAR16)) in bytes.

  @param  Source        A pointer to a Null-terminated ASCII string.
  @param  Destination   A pointer to a Null-terminated Unicode string.

  @return Destination.
  @return NULL          If Destination or Source is NULL, return NULL.

**/
CHAR16 *
AsciiStrToUnicodeStr (
  IN      CONST CHAR8               *Source,
  OUT     CHAR16                    *Destination
  )
{
  CHAR16                            *ReturnValue;

  ReturnValue = NULL;

  if ((Destination == NULL) || (Source == NULL) || (strlen (Source) == 0)) {
    return NULL;
  }
  ReturnValue = Destination;
  while (*Source != '\0') {
    *(Destination++) = (CHAR16) *(Source++);
  }
  //
  // End the Destination with a NULL.
  //
  *Destination = '\0';

  return ReturnValue;
}

/**
  Internal function that convert a number to a string in Buffer.

  Print worker function that converts a decimal or hexadecimal number to an ASCII string in Buffer.

  @param  Buffer    Location to place the ASCII string of Value.
  @param  Value     The value to convert to a Decimal or Hexadecimal string in Buffer.
  @param  Radix     Radix of the value

  @return A pointer to the end of buffer filled with ASCII string.

**/
CHAR8 *
BasePrintLibValueToString (
  IN OUT CHAR8  *Buffer,
  IN INT64      Value,
  IN UINTN      Radix
  )
{
  UINT32  Remainder;

  //
  // Loop to convert one digit at a time in reverse order
  //
  *Buffer = 0;
  do {
    Value = (INT64)DivU64x32Remainder ((UINT64)Value, (UINT32)Radix, &Remainder);
    *(++Buffer) = mHexStr[Remainder];
  } while (Value != 0);

  //
  // Return pointer of the end of filled buffer.
  //
  return Buffer;
}

/**
  Reads a 16-bit value from memory that may be unaligned.

  This function returns the 16-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 16-bit value that may be unaligned.

  @return The 16-bit value read from Buffer.

**/
UINT16
FceReadUnaligned16 (
  IN CONST UINT16              *Buffer
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer;
}

/**
  Reads a 32-bit value from memory that may be unaligned.

  This function returns the 32-bit value pointed to by Buffer. The function
  guarantees that the read operation does not produce an alignment fault.

  If the Buffer is NULL, then ASSERT().

  @param  Buffer  A pointer to a 32-bit value that may be unaligned.

  @return The 32-bit value read from Buffer.

**/
UINT32
ReadUnaligned32 (
  IN CONST UINT32              *Buffer
  )
{
  ASSERT (Buffer != NULL);

  return *Buffer;
}

/**
  Internal function that places the character into the Buffer.

  Internal function that places ASCII or Unicode character into the Buffer.

  @param  Buffer      The buffer to place the Unicode or ASCII string.
  @param  EndBuffer   The end of the input Buffer. No characters will be
                      placed after that.
  @param  Length      The count of character to be placed into Buffer.
                      (Negative value indicates no buffer fill.)
  @param  Character   The character to be placed into Buffer.
  @param  Increment   The character increment in Buffer.

  @return Buffer.

**/
CHAR8 *
BasePrintLibFillBuffer (
  OUT CHAR8   *Buffer,
  IN  CHAR8   *EndBuffer,
  IN  INTN    Length,
  IN  UINTN   Character,
  IN  INTN    Increment
  )
{
  INTN  Index;

  for (Index = 0; Index < Length && Buffer < EndBuffer; Index++) {
    *Buffer = (CHAR8) Character;
    if (Increment != 1) {
      *(Buffer + 1) = (CHAR8)(Character >> 8);
    }
    Buffer += Increment;
  }

  return Buffer;
}

/**
  Worker function that produces a Null-terminated string in an output buffer
  based on a Null-terminated format string and a VA_LIST argument list.

  VSPrint function to process format and place the results in Buffer. Since a
  VA_LIST is used this routine allows the nesting of Vararg routines. Thus
  this is the main print working routine.

  If COUNT_ONLY_NO_PRINT is set in Flags, Buffer will not be modified at all.

  @param[out] Buffer          The character buffer to print the results of the
                              parsing of Format into.
  @param[in]  BufferSize      The maximum number of characters to put into
                              buffer.
  @param[in]  Flags           Initial flags value.
                              Can only have FORMAT_UNICODE, OUTPUT_UNICODE,
                              and COUNT_ONLY_NO_PRINT set.
  @param[in]  Format          A Null-terminated format string.
  @param[in]  VaListMarker    VA_LIST style variable argument list consumed by
                              processing Format.
  @param[in]  BaseListMarker  BASE_LIST style variable argument list consumed
                              by processing Format.

  @return The number of characters printed not including the Null-terminator.
          If COUNT_ONLY_NO_PRINT was set returns the same, but without any
          modification to Buffer.

**/
UINTN
BasePrintLibSPrintMarker (
  OUT CHAR8        *Buffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker,   OPTIONAL
  IN  BASE_LIST    BaseListMarker  OPTIONAL
  )
{
  CHAR8             *OriginalBuffer;
  CHAR8             *EndBuffer;
  CHAR8             ValueBuffer[MAXIMUM_VALUE_CHARACTERS];
  UINT32            BytesPerOutputCharacter;
  UINTN             BytesPerFormatCharacter;
  UINTN             FormatMask;
  UINTN             FormatCharacter;
  UINTN             Width;
  UINTN             Precision;
  INT64             Value;
  CONST CHAR8       *ArgumentString;
  UINTN             Character;
  EFI_GUID          *TmpGuid;
  TIME              *TmpTime;
  UINTN             Count;
  UINTN             ArgumentMask;
  INTN              BytesPerArgumentCharacter;
  UINTN             ArgumentCharacter;
  BOOLEAN           Done;
  UINTN             Index;
  CHAR8             Prefix;
  BOOLEAN           ZeroPad;
  BOOLEAN           Comma;
  UINTN             Digits;
  UINTN             Radix;
  RETURN_STATUS     Status;
  UINT32            GuidData1;
  UINT16            GuidData2;
  UINT16            GuidData3;
  UINTN             LengthToReturn;

  //
  // If you change this code be sure to match the 2 versions of this function.
  // Nearly identical logic is found in the BasePrintLib and
  // DxePrintLibPrint2Protocol (both PrintLib instances).
  //

  if ((Flags & COUNT_ONLY_NO_PRINT) != 0) {
    if (BufferSize == 0) {
      Buffer = NULL;
    }
  } else {
    //
    // We can run without a Buffer for counting only.
    //
    if (BufferSize == 0) {
      return 0;
    }
    ASSERT (Buffer != NULL);
  }

  if ((Flags & OUTPUT_UNICODE) != 0) {
    BytesPerOutputCharacter = 2;
  } else {
    BytesPerOutputCharacter = 1;
  }

  LengthToReturn = 0;

  //
  // Reserve space for the Null terminator.
  //
  BufferSize--;
  OriginalBuffer = Buffer;

  //
  // Set the tag for the end of the input Buffer.
  //
  EndBuffer      = Buffer + BufferSize * BytesPerOutputCharacter;

  if ((Flags & FORMAT_UNICODE) != 0) {
    //
    // Make sure format string cannot contain more than PcdMaximumUnicodeStringLength
    // Unicode characters if PcdMaximumUnicodeStringLength is not zero.
    //
    ASSERT (FceStrSize ((CHAR16 *) Format) != 0);
    BytesPerFormatCharacter = 2;
    FormatMask = 0xffff;
  } else {
    //
    // Make sure format string cannot contain more than PcdMaximumAsciiStringLength
    // Ascii characters if PcdMaximumAsciiStringLength is not zero.
    //
    ASSERT (strlen (Format) + 1 != 0);
    BytesPerFormatCharacter = 1;
    FormatMask = 0xff;
  }

  //
  // Get the first character from the format string
  //
  FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;

  //
  // Loop until the end of the format string is reached or the output buffer is full
  //
  while (FormatCharacter != 0 && Buffer < EndBuffer) {
    //
    // Clear all the flag bits except those that may have been passed in
    //
    Flags &= (OUTPUT_UNICODE | FORMAT_UNICODE | COUNT_ONLY_NO_PRINT);

    //
    // Set the default width to zero, and the default precision to 1
    //
    Width     = 0;
    Precision = 1;
    Prefix    = 0;
    Comma     = FALSE;
    ZeroPad   = FALSE;
    Count     = 0;
    Digits    = 0;

    switch (FormatCharacter) {
    case '%':
      //
      // Parse Flags and Width
      //
      for (Done = FALSE; !Done; ) {
        Format += BytesPerFormatCharacter;
        FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
        switch (FormatCharacter) {
        case '.':
          Flags |= PRECISION;
          break;
        case '-':
          Flags |= LEFT_JUSTIFY;
          break;
        case '+':
          Flags |= PREFIX_SIGN;
          break;
        case ' ':
          Flags |= PREFIX_BLANK;
          break;
        case ',':
          Flags |= COMMA_TYPE;
          break;
        case 'L':
        case 'l':
          Flags |= LONG_TYPE;
          break;
        case '*':
          if ((Flags & PRECISION) == 0) {
            Flags |= PAD_TO_WIDTH;
            if (BaseListMarker == NULL) {
              Width = VA_ARG (VaListMarker, UINTN);
            } else {
              Width = BASE_ARG (BaseListMarker, UINTN);
            }
          } else {
            if (BaseListMarker == NULL) {
              Precision = VA_ARG (VaListMarker, UINTN);
            } else {
              Precision = BASE_ARG (BaseListMarker, UINTN);
            }
          }
          break;
        case '0':
          if ((Flags & PRECISION) == 0) {
            Flags |= PREFIX_ZERO;
          }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          for (Count = 0; ((FormatCharacter >= '0') &&  (FormatCharacter <= '9')); ){
            Count = (Count * 10) + FormatCharacter - '0';
            Format += BytesPerFormatCharacter;
            FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
          }
          Format -= BytesPerFormatCharacter;
          if ((Flags & PRECISION) == 0) {
            Flags |= PAD_TO_WIDTH;
            Width = Count;
          } else {
            Precision = Count;
          }
          break;

        case '\0':
          //
          // Make no output if Format string terminates unexpectedly when
          // looking up for flag, width, precision and type.
          //
          Format   -= BytesPerFormatCharacter;
          Precision = 0;
          //
          // break skipped on purpose.
          //
        default:
          Done = TRUE;
          break;
        }
      }

      //
      // Handle each argument type
      //
      switch (FormatCharacter) {
      case 'p':
        //
        // Flag space, +, 0, L & l are invalid for type p.
        //
        Flags &= ~(PREFIX_BLANK | PREFIX_SIGN | PREFIX_ZERO | LONG_TYPE);
        if (sizeof (VOID *) > 4) {
          Flags |= LONG_TYPE;
        }
      case 'X':
        Flags |= PREFIX_ZERO;
        //
        // break skipped on purpose
        //
      case 'x':
        Flags |= RADIX_HEX;
        //
        // break skipped on purpose
        //
      case 'd':
        if ((Flags & LONG_TYPE) == 0) {
          //
          // 'd','x', and 'X' that are not preceded by 'l' or 'L' are assumed to be type "int".
          // This assumption is made so the format string definition is compatible with the ANSI C
          // Specification for formatted strings.  It is recommended that the Base Types be used
          // everywhere, but in this one case, compliance with ANSI C is more important, and
          // provides an implementation that is compatible with that largest possible set of CPU
          // architectures.  This is why the type "int" is used in this one case.
          //
          if (BaseListMarker == NULL) {
            Value = VA_ARG (VaListMarker, int);
          } else {
            Value = BASE_ARG (BaseListMarker, int);
          }
        } else {
          if (BaseListMarker == NULL) {
            Value = VA_ARG (VaListMarker, INT64);
          } else {
            Value = BASE_ARG (BaseListMarker, INT64);
          }
        }
        if ((Flags & PREFIX_BLANK) != 0) {
          Prefix = ' ';
        }
        if ((Flags & PREFIX_SIGN) != 0) {
          Prefix = '+';
        }
        if ((Flags & COMMA_TYPE) != 0) {
          Comma = TRUE;
        }
        if ((Flags & RADIX_HEX) == 0) {
          Radix = 10;
          if (Comma) {
            Flags &= (~PREFIX_ZERO);
            Precision = 1;
          }
          if (Value < 0) {
            Flags |= PREFIX_SIGN;
            Prefix = '-';
            Value = -Value;
          }
        } else {
          Radix = 16;
          Comma = FALSE;
          if ((Flags & LONG_TYPE) == 0 && Value < 0) {
            //
            // 'd','x', and 'X' that are not preceded by 'l' or 'L' are assumed to be type "int".
            // This assumption is made so the format string definition is compatible with the ANSI C
            // Specification for formatted strings.  It is recommended that the Base Types be used
            // everywhere, but in this one case, compliance with ANSI C is more important, and
            // provides an implementation that is compatible with that largest possible set of CPU
            // architectures.  This is why the type "unsigned int" is used in this one case.
            //
            Value = (unsigned int)Value;
          }
        }
        //
        // Convert Value to a reversed string
        //
        Count = BasePrintLibValueToString (ValueBuffer, Value, Radix) - ValueBuffer;
        if (Value == 0 && Precision == 0) {
          Count = 0;
        }
        ArgumentString = (CHAR8 *)ValueBuffer + Count;

        Digits = Count % 3;
        if (Digits != 0) {
          Digits = 3 - Digits;
        }
        if (Comma && Count != 0) {
          Count += ((Count - 1) / 3);
        }
        if (Prefix != 0) {
          Count++;
          Precision++;
        }
        Flags |= ARGUMENT_REVERSED;
        ZeroPad = TRUE;
        if ((Flags & PREFIX_ZERO) != 0) {
          if ((Flags & LEFT_JUSTIFY) == 0) {
            if ((Flags & PAD_TO_WIDTH) != 0) {
              if ((Flags & PRECISION) == 0) {
                Precision = Width;
              }
            }
          }
        }
        break;

      case 's':
      case 'S':
        Flags |= ARGUMENT_UNICODE;
        //
        // break skipped on purpose
        //
      case 'a':
        if (BaseListMarker == NULL) {
          ArgumentString = VA_ARG (VaListMarker, CHAR8 *);
        } else {
          ArgumentString = BASE_ARG (BaseListMarker, CHAR8 *);
        }
        if (ArgumentString == NULL) {
          Flags &= (~ARGUMENT_UNICODE);
          ArgumentString = "<null string>";
        }
        //
        // Set the default precision for string to be zero if not specified.
        //
        if ((Flags & PRECISION) == 0) {
          Precision = 0;
        }
        break;

      case 'c':
        if (BaseListMarker == NULL) {
          Character = VA_ARG (VaListMarker, UINTN) & 0xffff;
        } else {
          Character = BASE_ARG (BaseListMarker, UINTN) & 0xffff;
        }
        ArgumentString = (CHAR8 *)&Character;
        Flags |= ARGUMENT_UNICODE;
        break;

      case 'g':
        if (BaseListMarker == NULL) {
          TmpGuid = VA_ARG (VaListMarker, EFI_GUID *);
        } else {
          TmpGuid = BASE_ARG (BaseListMarker, EFI_GUID *);
        }
        if (TmpGuid == NULL) {
          ArgumentString = "<null guid>";
        } else {
          GuidData1 = ReadUnaligned32 (&(TmpGuid->Data1));
          GuidData2 = FceReadUnaligned16 (&(TmpGuid->Data2));
          GuidData3 = FceReadUnaligned16 (&(TmpGuid->Data3));
          BasePrintLibSPrint (
            ValueBuffer,
            MAXIMUM_VALUE_CHARACTERS,
            0,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            GuidData1,
            GuidData2,
            GuidData3,
            TmpGuid->Data4[0],
            TmpGuid->Data4[1],
            TmpGuid->Data4[2],
            TmpGuid->Data4[3],
            TmpGuid->Data4[4],
            TmpGuid->Data4[5],
            TmpGuid->Data4[6],
            TmpGuid->Data4[7]
            );
          ArgumentString = ValueBuffer;
        }
        break;

      case 't':
        if (BaseListMarker == NULL) {
          TmpTime = VA_ARG (VaListMarker, TIME *);
        } else {
          TmpTime = BASE_ARG (BaseListMarker, TIME *);
        }
        if (TmpTime == NULL) {
          ArgumentString = "<null time>";
        } else {
          BasePrintLibSPrint (
            ValueBuffer,
            MAXIMUM_VALUE_CHARACTERS,
            0,
            "%02d/%02d/%04d  %02d:%02d",
            TmpTime->Month,
            TmpTime->Day,
            TmpTime->Year,
            TmpTime->Hour,
            TmpTime->Minute
            );
          ArgumentString = ValueBuffer;
        }
        break;

      case 'r':
        if (BaseListMarker == NULL) {
          Status = VA_ARG (VaListMarker, RETURN_STATUS);
        } else {
          Status = BASE_ARG (BaseListMarker, RETURN_STATUS);
        }
        ArgumentString = ValueBuffer;
        if (RETURN_ERROR (Status)) {
          //
          // Clear error bit
          //
          Index = Status & ~MAX_BIT;
          if (Index > 0 && Index <= ERROR_STATUS_NUMBER) {
            ArgumentString = mStatusString [Index + WARNING_STATUS_NUMBER];
          }
        } else {
          Index = Status;
          if (Index <= WARNING_STATUS_NUMBER) {
            ArgumentString = mStatusString [Index];
          }
        }
        if (ArgumentString == ValueBuffer) {
          BasePrintLibSPrint ((CHAR8 *) ValueBuffer, MAXIMUM_VALUE_CHARACTERS, 0, "%08X", Status);
        }
        break;

      case '\r':
        Format += BytesPerFormatCharacter;
        FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
        if (FormatCharacter == '\n') {
          //
          // Translate '\r\n' to '\r\n'
          //
          ArgumentString = "\r\n";
        } else {
          //
          // Translate '\r' to '\r'
          //
          ArgumentString = "\r";
          Format   -= BytesPerFormatCharacter;
        }
        break;

      case '\n':
        //
        // Translate '\n' to '\r\n' and '\n\r' to '\r\n'
        //
        ArgumentString = "\r\n";
        Format += BytesPerFormatCharacter;
        FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
        if (FormatCharacter != '\r') {
          Format   -= BytesPerFormatCharacter;
        }
        break;

      case '%':
      default:
        //
        // if the type is '%' or unknown, then print it to the screen
        //
        ArgumentString = (CHAR8 *)&FormatCharacter;
        Flags |= ARGUMENT_UNICODE;
        break;
      }
      break;

    case '\r':
      Format += BytesPerFormatCharacter;
      FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
      if (FormatCharacter == '\n') {
        //
        // Translate '\r\n' to '\r\n'
        //
        ArgumentString = "\r\n";
      } else {
        //
        // Translate '\r' to '\r'
        //
        ArgumentString = "\r";
        Format   -= BytesPerFormatCharacter;
      }
      break;

    case '\n':
      //
      // Translate '\n' to '\r\n' and '\n\r' to '\r\n'
      //
      ArgumentString = "\r\n";
      Format += BytesPerFormatCharacter;
      FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
      if (FormatCharacter != '\r') {
        Format   -= BytesPerFormatCharacter;
      }
      break;

    default:
      ArgumentString = (CHAR8 *)&FormatCharacter;
      Flags |= ARGUMENT_UNICODE;
      break;
    }

    //
    // Retrieve the ArgumentString attriubutes
    //
    if ((Flags & ARGUMENT_UNICODE) != 0) {
      ArgumentMask = 0xffff;
      BytesPerArgumentCharacter = 2;
    } else {
      ArgumentMask = 0xff;
      BytesPerArgumentCharacter = 1;
    }
    if ((Flags & ARGUMENT_REVERSED) != 0) {
      BytesPerArgumentCharacter = -BytesPerArgumentCharacter;
    } else {
      //
      // Compute the number of characters in ArgumentString and store it in Count
      // ArgumentString is either null-terminated, or it contains Precision characters
      //
      for (Count = 0; Count < Precision || ((Flags & PRECISION) == 0); Count++) {
        ArgumentCharacter = ((ArgumentString[Count * BytesPerArgumentCharacter] & 0xff) | ((ArgumentString[Count * BytesPerArgumentCharacter + 1]) << 8)) & ArgumentMask;
        if (ArgumentCharacter == 0) {
          break;
        }
      }
    }

    if (Precision < Count) {
      Precision = Count;
    }

    //
    // Pad before the string
    //
    if ((Flags & (PAD_TO_WIDTH | LEFT_JUSTIFY)) == (PAD_TO_WIDTH)) {
      LengthToReturn += ((Width - Precision) * BytesPerOutputCharacter);
      if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
        Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, Width - Precision, ' ', BytesPerOutputCharacter);
      }
    }

    if (ZeroPad) {
      if (Prefix != 0) {
        LengthToReturn += (1 * BytesPerOutputCharacter);
        if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
          Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, 1, Prefix, BytesPerOutputCharacter);
        }
      }
      LengthToReturn += ((Precision - Count) * BytesPerOutputCharacter);
      if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
        Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, Precision - Count, '0', BytesPerOutputCharacter);
      }
    } else {
      LengthToReturn += ((Precision - Count) * BytesPerOutputCharacter);
      if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
        Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, Precision - Count, ' ', BytesPerOutputCharacter);
      }
      if (Prefix != 0) {
        LengthToReturn += (1 * BytesPerOutputCharacter);
        if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
          Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, 1, Prefix, BytesPerOutputCharacter);
        }
      }
    }

    //
    // Output the Prefix character if it is present
    //
    Index = 0;
    if (Prefix != 0) {
      Index++;
    }

    //
    // Copy the string into the output buffer performing the required type conversions
    //
    while (Index < Count) {
      ArgumentCharacter = ((*ArgumentString & 0xff) | (*(ArgumentString + 1) << 8)) & ArgumentMask;

      LengthToReturn += (1 * BytesPerOutputCharacter);
      if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
        Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, 1, ArgumentCharacter, BytesPerOutputCharacter);
      }
      ArgumentString    += BytesPerArgumentCharacter;
      Index++;
      if (Comma) {
        Digits++;
        if (Digits == 3) {
          Digits = 0;
          Index++;
          if (Index < Count) {
            LengthToReturn += (1 * BytesPerOutputCharacter);
            if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
              Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, 1, ',', BytesPerOutputCharacter);
            }
          }
        }
      }
    }

    //
    // Pad after the string
    //
    if ((Flags & (PAD_TO_WIDTH | LEFT_JUSTIFY)) == (PAD_TO_WIDTH | LEFT_JUSTIFY)) {
      LengthToReturn += ((Width - Precision) * BytesPerOutputCharacter);
      if ((Flags & COUNT_ONLY_NO_PRINT) == 0 && Buffer != NULL) {
        Buffer = BasePrintLibFillBuffer (Buffer, EndBuffer, Width - Precision, ' ', BytesPerOutputCharacter);
      }
    }

    //
    // Get the next character from the format string
    //
    Format += BytesPerFormatCharacter;

    //
    // Get the next character from the format string
    //
    FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;
  }

  if ((Flags & COUNT_ONLY_NO_PRINT) != 0) {
    return (LengthToReturn / BytesPerOutputCharacter);
  }

  ASSERT (Buffer != NULL);
  //
  // Null terminate the Unicode or ASCII string
  //
  BasePrintLibFillBuffer (Buffer, EndBuffer + BytesPerOutputCharacter, 1, 0, BytesPerOutputCharacter);
  //
  // Make sure output buffer cannot contain more than PcdMaximumUnicodeStringLength
  // Unicode characters if PcdMaximumUnicodeStringLength is not zero.
  //
  ASSERT ((((Flags & OUTPUT_UNICODE) == 0)) || (FceStrSize ((CHAR16 *) OriginalBuffer) != 0));
  //
  // Make sure output buffer cannot contain more than PcdMaximumAsciiStringLength
  // ASCII characters if PcdMaximumAsciiStringLength is not zero.
  //
  ASSERT ((((Flags & OUTPUT_UNICODE) != 0)) || ((strlen (OriginalBuffer) + 1) != 0));

  return ((Buffer - OriginalBuffer) / BytesPerOutputCharacter);
}

/**
  Worker function that produces a Null-terminated string in an output buffer
  based on a Null-terminated format string and variable argument list.

  VSPrint function to process format and place the results in Buffer. Since a
  VA_LIST is used this routine allows the nesting of Vararg routines. Thus
  this is the main print working routine

  @param  StartOfBuffer The character buffer to print the results of the parsing
                        of Format into.
  @param  BufferSize    The maximum number of characters to put into buffer.
                        Zero means no limit.
  @param  Flags         Initial flags value.
                        Can only have FORMAT_UNICODE and OUTPUT_UNICODE set
  @param  FormatString  A Null-terminated format string.
  @param  ...           The variable argument list.

  @return The number of characters printed.

**/
UINTN
BasePrintLibSPrint (
  OUT CHAR8        *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *FormatString,
  ...
  )
{
  VA_LIST  Marker;

  VA_START (Marker, FormatString);
  return BasePrintLibSPrintMarker (StartOfBuffer, BufferSize, Flags, FormatString, Marker, NULL);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on
  a Null-terminated Unicode format string and a VA_LIST argument list

  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on the
  contents of the format string.
  The number of Unicode characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0 or 1, then no output buffer is produced and 0 is returned.

  If BufferSize > 1 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 1 and StartOfBuffer is not aligned on a 16-bit boundary, then ASSERT().
  If BufferSize > 1 and FormatString is NULL, then ASSERT().
  If BufferSize > 1 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.

  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
UnicodeVSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  )
{
  ASSERT_UNICODE_BUFFER (StartOfBuffer);
  ASSERT_UNICODE_BUFFER (FormatString);
  return BasePrintLibSPrintMarker ((CHAR8 *)StartOfBuffer, BufferSize >> 1, FORMAT_UNICODE | OUTPUT_UNICODE, (CHAR8 *)FormatString, Marker, NULL);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated
  Unicode format string and variable argument list.

  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the format string.
  The number of Unicode characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0 or 1, then no output buffer is produced and 0 is returned.

  If BufferSize > 1 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 1 and StartOfBuffer is not aligned on a 16-bit boundary, then ASSERT().
  If BufferSize > 1 and FormatString is NULL, then ASSERT().
  If BufferSize > 1 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    A Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the
                          format string specified by FormatString.

  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
UnicodeSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, FormatString);
  return UnicodeVSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Convert a Null-terminated Unicode string to a Null-terminated
  ASCII string and returns the ASCII string.

  This function converts the content of the Unicode string Source
  to the ASCII string Destination by copying the lower 8 bits of
  each Unicode character. It returns Destination. The function terminates
  the ASCII string Destination  by appending a Null-terminator character
  at the end. The caller is responsible to make sure Destination points
  to a buffer with size equal or greater than (FceStrLen (Source) + 1) in bytes.

  If Destination is NULL, then ASSERT().
  If Source is NULL, then ASSERT().
  If Source is not aligned on a 16-bit boundary, then ASSERT().
  If Source and Destination overlap, then ASSERT().

  If any Unicode characters in Source contain non-zero value in
  the upper 8 bits, then ASSERT().

  @param  Source        Pointer to a Null-terminated Unicode string.
  @param  Destination   Pointer to a Null-terminated ASCII string.

  @reture Destination

**/
CHAR8 *
UnicodeStrToAsciiStr (
  IN      CONST CHAR16             *Source,
  OUT           CHAR8              *Destination
  )
{
  CHAR8          *ReturnValue;

  ReturnValue = Destination;
  assert (Destination != NULL);
  assert (Source != NULL);
  assert (((UINTN) Source & 0x01) == 0);

  while (*Source != L'\0') {
    //
    // If any Unicode characters in Source contain
    // non-zero value in the upper 8 bits, then ASSERT().
    //
    assert (*Source < 0x100);
    *(ReturnValue++) = (CHAR8) *(Source++);
  }

  *ReturnValue = '\0';

  return Destination;
}

/**
  Allocate new memory and then copy the Unicode string Source to Destination.

  @param  Dest                   Location to copy string
  @param  Src                    String to copy

**/
VOID
NewStringCpy (
  IN OUT CHAR16       **Dest,
  IN CHAR16           *Src
  )
{
  if (*Dest != NULL) {
    FreePool (*Dest);
  }
  *Dest = FceAllocateCopyPool (FceStrSize (Src), Src);
  ASSERT (*Dest != NULL);
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
FceInternalIsDecimalDigitCharacter (
  IN      CHAR16                    Char
  )
{
  return (BOOLEAN) ((Char >= L'0') && (Char <= L'9'));
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
FceInternalCharToUpper (
  IN      CHAR16                    Char
  )
{
  if ((Char >= L'a') && (Char <= L'z')) {
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
FceInternalHexCharToUintn (
  IN      CHAR16                    Char
  )
{
  if (FceInternalIsDecimalDigitCharacter (Char)) {
    return Char - L'0';
  }

  return (UINTN) (10 + FceInternalCharToUpper (Char) - L'A');
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
FceInternalIsHexaDecimalDigitCharacter (
  IN      CHAR16                    Char
  )
{

  return (BOOLEAN) (FceInternalIsDecimalDigitCharacter (Char) ||
    ((Char >= L'A') && (Char <= L'F')) ||
    ((Char >= L'a') && (Char <= L'f')));
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
  more than PcdMaximumUnicodeStringLength Unicode characters, not including
  the Null-terminator, then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINT64
FceStrDecimalToUint64 (
  IN      CONST CHAR16              *String
  )
{
  UINT64     Result;

  //
  // ASSERT String is less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside FceStrLen().
  //
  ASSERT (FceStrSize (String) != 0);

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

  while (FceInternalIsDecimalDigitCharacter (*String)) {
    //
    // If the number represented by String overflows according
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT (Result <= DivU64x32 (((UINT64) ~0) - (*String - L'0') , 10));

    Result = MultU64x32 (Result, 10) + (*String - L'0');
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
  PcdMaximumUnicodeStringLength Unicode characters, not including the Null-terminator,
  then ASSERT().

  @param  String          A pointer to a Null-terminated Unicode string.

  @retval Value translated from String.

**/
UINT64
FceStrHexToUint64 (
  IN      CONST CHAR16             *String
  )
{
  UINT64    Result;

  //
  // ASSERT String is less long than PcdMaximumUnicodeStringLength.
  // Length tests are performed inside FceStrLen().
  //
  ASSERT (FceStrSize (String) != 0);

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

  if (FceInternalCharToUpper (*String) == L'X') {
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

  while (FceInternalIsHexaDecimalDigitCharacter (*String)) {
    //
    // If the Hex Number represented by String overflows according
    // to the range defined by UINTN, then ASSERT().
    //
    ASSERT (Result <= RShiftU64 (((UINT64) ~0) - FceInternalHexCharToUintn (*String) , 4));

    Result = LShiftU64 (Result, 4);
    Result = Result + FceInternalHexCharToUintn (*String);
    String++;
  }

  return Result;
}


CHAR16
ToUpper (
  CHAR16  a
  )
{
  if (('a' <= a) && (a <= 'z')) {
    return (CHAR16) (a - 0x20);
  } else {
    return a;
  }
}

CHAR16
ToLower (
  CHAR16  a
  )
{
  if (('A' <= a) && (a <= 'Z')) {
    return (CHAR16) (a + 0x20);
  } else {
    return a;
  }
}

/**
  Performs a case-insensitive comparison between a Null-terminated
  Unicode pattern string and a Null-terminated Unicode string.

  @param  String   - A pointer to a Null-terminated Unicode string.
  @param  Pattern  - A pointer to a Null-terminated Unicode pattern string.


  @retval TRUE     - Pattern was found in String.
  @retval FALSE    - Pattern was not found in String.

**/
BOOLEAN
MetaiMatch (
  IN CHAR16                           *String,
  IN CHAR16                           *Pattern
  )
{
  CHAR16  c;
  CHAR16  p;

  assert (String != NULL);
  assert (Pattern != NULL);

  for (;;) {
    p     = *Pattern;
    Pattern += 1;

    if (p == 0) {
      //
      // End of pattern.  If end of string, TRUE match
      //
      if (*String) {
        return FALSE;
      } else {
        return TRUE;
      }

    } else {

      c = *String;
      if (ToUpper (c) != ToUpper (p)) {
        return FALSE;
      }

      String += 1;

    }

  }

}
/**
  Multiplies a 64-bit unsigned integer by a 32-bit unsigned integer and
  generates a 64-bit unsigned result.

  This function multiplies the 64-bit unsigned value Multiplicand by the 32-bit
  unsigned value Multiplier and generates a 64-bit unsigned result. This 64-
  bit unsigned result is returned.

  @param  Multiplicand  A 64-bit unsigned value.
  @param  Multiplier    A 32-bit unsigned value.

  @return Multiplicand * Multiplier.

**/
UINT64
MultU64x32 (
  IN      UINT64                    Multiplicand,
  IN      UINT32                    Multiplier
  )
{
  return Multiplicand * Multiplier;
}

/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and generates
  a 64-bit unsigned result.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. This
  function returns the 64-bit unsigned quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.

  @return Dividend / Divisor

**/
UINT64
DivU64x32 (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor
  )
{
  ASSERT (Divisor != 0);
  return Dividend / Divisor;
}

/**
  Shifts a 64-bit integer left between 0 and 63 bits. The low bits are filled
  with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the left by Count bits. The
  low Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift left.
  @param  Count   The number of bits to shift left.

  @return Operand << Count.

**/
UINT64
LShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )
{
  ASSERT (Count < 64);
  return Operand << Count;
}

/**
  Shifts a 64-bit integer right between 0 and 63 bits. This high bits are
  filled with zeros. The shifted value is returned.

  This function shifts the 64-bit value Operand to the right by Count bits. The
  high Count bits are set to zero. The shifted value is returned.

  If Count is greater than 63, then ASSERT().

  @param  Operand The 64-bit operand to shift right.
  @param  Count   The number of bits to shift right.

  @return Operand >> Count.

**/
UINT64
RShiftU64 (
  IN      UINT64                    Operand,
  IN      UINTN                     Count
  )

{
  ASSERT (Count < 64);
  return Operand >> Count;
}


/**
  Divides a 64-bit unsigned integer by a 32-bit unsigned integer and generates
  a 64-bit unsigned result and an optional 32-bit unsigned remainder.

  This function divides the 64-bit unsigned value Dividend by the 32-bit
  unsigned value Divisor and generates a 64-bit unsigned quotient. If Remainder
  is not NULL, then the 32-bit unsigned remainder is returned in Remainder.
  This function returns the 64-bit unsigned quotient.

  If Divisor is 0, then ASSERT().

  @param  Dividend  A 64-bit unsigned value.
  @param  Divisor   A 32-bit unsigned value.
  @param  Remainder A pointer to a 32-bit unsigned value. This parameter is
                    optional and may be NULL.

  @return Dividend / Divisor

**/
UINT64
DivU64x32Remainder (
  IN      UINT64                    Dividend,
  IN      UINT32                    Divisor,
  OUT     UINT32                    *Remainder
  )
{
  ASSERT (Divisor != 0);

  if (Remainder != NULL) {
    *Remainder = (UINT32)(Dividend % Divisor);
  }
  return Dividend / Divisor;
}

/**
  Copies a buffer to an allocated buffer.

  Allocates the number bytes specified by AllocationSize, copies allocationSize bytes
  from Buffer to the newly allocated buffer, and returns a pointer to the allocated
  buffer.  If AllocationSize is 0, then a valid buffer of 0 size is returned.  If there
  is not enough memory remaining to satisfy the request, then NULL is returned.

  If Buffer is NULL, then ASSERT().

  @param  AllocationSize        The number of bytes to allocate and zero.
  @param  Buffer                The buffer to copy to the allocated buffer.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
FceAllocateCopyPool (
  IN UINTN       AllocationSize,
  IN CONST VOID  *Buffer
  )
{
  VOID  *Memory;

  Memory = NULL;

  if ((Buffer == NULL) || (AllocationSize == 0)) {
    return Memory;
  }

  Memory = calloc (AllocationSize, sizeof (CHAR8));
  if (Memory != NULL) {
     Memory = memcpy (Memory, Buffer, AllocationSize);
  }
  return Memory;
}

/**
  Initializes the head node of a doubly-linked list, and returns the pointer to
  the head node of the doubly-linked list.

  Initializes the forward and backward links of a new linked list. After
  initializing a linked list with this function, the other linked list
  functions may be used to add and remove nodes from the linked list. It is up
  to the caller of this function to allocate the memory for ListHead.

  If ListHead is NULL, then ASSERT().

  @param  ListHead  A pointer to the head node of a new doubly-linked list.

  @return ListHead

**/
LIST_ENTRY *
InitializeListHead (
  IN OUT  LIST_ENTRY                *ListHead
  )

{
  assert (ListHead != NULL);

  ListHead->ForwardLink = ListHead;
  ListHead->BackLink = ListHead;
  return ListHead;
}

/**
  Adds a node to the beginning of a doubly-linked list, and returns the pointer
  to the head node of the doubly-linked list.

  Adds the node Entry at the beginning of the doubly-linked list denoted by
  ListHead, and returns ListHead.

  If ListHead is NULL, then ASSERT().
  If Entry is NULL, then ASSERT().
  If ListHead was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly-linked list.
  @param  Entry     A pointer to a node that is to be inserted at the beginning
                    of a doubly-linked list.

  @return ListHead

**/
LIST_ENTRY *
InsertHeadList (
  IN OUT  LIST_ENTRY                *ListHead,
  IN OUT  LIST_ENTRY                *Entry
  )
{
  assert ((ListHead != NULL) && (Entry != NULL));

  Entry->ForwardLink = ListHead->ForwardLink;
  Entry->BackLink = ListHead;
  Entry->ForwardLink->BackLink = Entry;
  ListHead->ForwardLink = Entry;
  return ListHead;
}

/**
  Adds a node to the end of a doubly-linked list, and returns the pointer to
  the head node of the doubly-linked list.

  Adds the node Entry to the end of the doubly-linked list denoted by ListHead,
  and returns ListHead.

  If ListHead is NULL, then ASSERT().
  If Entry is NULL, then ASSERT().
  If ListHead was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and prior to insertion the number
  of nodes in ListHead, including the ListHead node, is greater than or
  equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly-linked list.
  @param  Entry     A pointer to a node that is to be added at the end of the
                    doubly-linked list.

  @return ListHead

**/
LIST_ENTRY *
InsertTailList (
  IN OUT  LIST_ENTRY                *ListHead,
  IN OUT  LIST_ENTRY                *Entry
  )
{
  assert ((ListHead != NULL) && (Entry != NULL));

  Entry->ForwardLink = ListHead;
  Entry->BackLink = ListHead->BackLink;
  Entry->BackLink->ForwardLink = Entry;
  ListHead->BackLink = Entry;
  return ListHead;
}

/**
  Retrieves the first node of a doubly-linked list.

  Returns the first node of a doubly-linked list.  List must have been
  initialized with INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead().
  If List is empty, then List is returned.

  If List is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.

  @return The first node of a doubly-linked list.
  @retval NULL  The list is empty.

**/
LIST_ENTRY *
GetFirstNode (
  IN      CONST LIST_ENTRY          *List
  )
{
  assert (List != NULL);

  return List->ForwardLink;
}

/**
  Retrieves the next node of a doubly-linked list.

  Returns the node of a doubly-linked list that follows Node.
  List must have been initialized with INTIALIZE_LIST_HEAD_VARIABLE()
  or InitializeListHead().  If List is empty, then List is returned.

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and List contains more than
  PcdMaximumLinkedListLenth nodes, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @return A pointer to the next node if one exists. Otherwise List is returned.

**/
LIST_ENTRY *
GetNextNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  assert ((List != NULL) && (Node != NULL));

  return Node->ForwardLink;
}

/**
  Retrieves the previous node of a doubly-linked list.

  Returns the node of a doubly-linked list that precedes Node.
  List must have been initialized with INTIALIZE_LIST_HEAD_VARIABLE()
  or InitializeListHead().  If List is empty, then List is returned.

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and List contains more than
  PcdMaximumLinkedListLenth nodes, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @return A pointer to the previous node if one exists. Otherwise List is returned.

**/
LIST_ENTRY *
GetPreviousNode (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  assert ((List != NULL) && (Node != NULL));

  return Node->BackLink;
}

/**
  Checks to see if a doubly-linked list is empty or not.

  Checks to see if the doubly-linked list is empty. If the linked list contains
  zero nodes, this function returns TRUE. Otherwise, it returns FALSE.

  If ListHead is NULL, then ASSERT().
  If ListHead was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().

  @param  ListHead  A pointer to the head node of a doubly-linked list.

  @retval TRUE  The linked list is empty.
  @retval FALSE The linked list is not empty.

**/
BOOLEAN
IsListEmpty (
  IN      CONST LIST_ENTRY          *ListHead
  )
{
  assert (ListHead != NULL);

  return (BOOLEAN)(ListHead->ForwardLink == ListHead);
}

/**
  Determines if a node in a doubly-linked list is the head node of a the same
  doubly-linked list.  This function is typically used to terminate a loop that
  traverses all the nodes in a doubly-linked list starting with the head node.

  Returns TRUE if Node is equal to List.  Returns FALSE if Node is one of the
  nodes in the doubly-linked list specified by List.  List must have been
  initialized with INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead().

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead(),
  then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List and Node is not
  equal to List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @retval TRUE  Node is the head of the doubly-linked list pointed by List.
  @retval FALSE Node is not the head of the doubly-linked list pointed by List.

**/
BOOLEAN
IsNull (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  assert ((List != NULL) && (Node != NULL));

  return (BOOLEAN)(Node == List);
}

/**
  Determines if a node the last node in a doubly-linked list.

  Returns TRUE if Node is the last node in the doubly-linked list specified by
  List. Otherwise, FALSE is returned. List must have been initialized with
  INTIALIZE_LIST_HEAD_VARIABLE() or InitializeListHead().

  If List is NULL, then ASSERT().
  If Node is NULL, then ASSERT().
  If List was not initialized with INTIALIZE_LIST_HEAD_VARIABLE() or
  InitializeListHead(), then ASSERT().
  If PcdMaximumLinkedListLenth is not zero, and the number of nodes
  in List, including the List node, is greater than or equal to
  PcdMaximumLinkedListLength, then ASSERT().
  If PcdVerifyNodeInList is TRUE and Node is not a node in List, then ASSERT().

  @param  List  A pointer to the head node of a doubly-linked list.
  @param  Node  A pointer to a node in the doubly-linked list.

  @retval TRUE  Node is the last node in the linked list.
  @retval FALSE Node is not the last node in the linked list.

**/
BOOLEAN
IsNodeAtEnd (
  IN      CONST LIST_ENTRY          *List,
  IN      CONST LIST_ENTRY          *Node
  )
{
  assert ((List != NULL) && (Node != NULL));

  return (BOOLEAN)(!IsNull (List, Node) && (List->BackLink == Node));
}

/**
  Removes a node from a doubly-linked list, and returns the node that follows
  the removed node.

  Removes the node Entry from a doubly-linked list. It is up to the caller of
  this function to release the memory used by this node if that is required. On
  exit, the node following Entry in the doubly-linked list is returned. If
  Entry is the only node in the linked list, then the head node of the linked
  list is returned.

  If Entry is NULL, then ASSERT().
  If Entry is the head node of an empty list, then ASSERT().
  If PcdMaximumLinkedListLength is not zero, and the number of nodes in the
  linked list containing Entry, including the Entry node, is greater than
  or equal to PcdMaximumLinkedListLength, then ASSERT().

  @param  Entry A pointer to a node in a linked list.

  @return Entry.

**/
LIST_ENTRY *
RemoveEntryList (
  IN      CONST LIST_ENTRY          *Entry
  )
{
  assert (!IsListEmpty (Entry));

  Entry->ForwardLink->BackLink = Entry->BackLink;
  Entry->BackLink->ForwardLink = Entry->ForwardLink;
  return Entry->ForwardLink;
}

