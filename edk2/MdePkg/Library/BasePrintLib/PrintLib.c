/** @file
  Print Library.

  Copyright (c) 2006, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PrintLib.c

**/

#include "PrintLibInternal.h"

typedef struct {
  RETURN_STATUS  Status;
  CHAR8          *String;
} STATUS_LOOKUP_TABLE_ENTRY;

static CONST STATUS_LOOKUP_TABLE_ENTRY  StatusString[] = {
  { RETURN_SUCCESS,               "Success" },
  { RETURN_LOAD_ERROR,            "Load Error" },
  { RETURN_INVALID_PARAMETER,     "Invalid Parameter" },
  { RETURN_UNSUPPORTED,           "Unsupported" },
  { RETURN_BAD_BUFFER_SIZE,       "Bad Buffer Size" },
  { RETURN_BUFFER_TOO_SMALL,      "Buffer Too Small" },
  { RETURN_NOT_READY,             "Not Ready" },
  { RETURN_DEVICE_ERROR,          "Device Error" },
  { RETURN_WRITE_PROTECTED,       "Write Protected" },
  { RETURN_OUT_OF_RESOURCES,      "Out of Resources" },
  { RETURN_VOLUME_CORRUPTED,      "Volume Corrupt" },
  { RETURN_VOLUME_FULL,           "Volume Full" },
  { RETURN_NO_MEDIA,              "No Media" },
  { RETURN_MEDIA_CHANGED,         "Media changed" },
  { RETURN_NOT_FOUND,             "Not Found" },
  { RETURN_ACCESS_DENIED,         "Access Denied" },
  { RETURN_NO_RESPONSE,           "No Response" },
  { RETURN_NO_MAPPING,            "No mapping" },
  { RETURN_TIMEOUT,               "Time out" },
  { RETURN_NOT_STARTED,           "Not started" },
  { RETURN_ALREADY_STARTED,       "Already started" },
  { RETURN_ABORTED,               "Aborted" },
  { RETURN_ICMP_ERROR,            "ICMP Error" },
  { RETURN_TFTP_ERROR,            "TFTP Error" },
  { RETURN_PROTOCOL_ERROR,        "Protocol Error" },
  { RETURN_WARN_UNKNOWN_GLYPH,    "Warning Unknown Glyph" },
  { RETURN_WARN_DELETE_FAILURE,   "Warning Delete Failure" },
  { RETURN_WARN_WRITE_FAILURE,    "Warning Write Failure" },
  { RETURN_WARN_BUFFER_TOO_SMALL, "Warning Buffer Too Small" },
  { 0,                              NULL                     }
};


/**
  Worker function that produces a Null-terminated string in an output buffer 
  based on a Null-terminated format string and a VA_LIST argument list.

  VSPrint function to process format and place the results in Buffer. Since a 
  VA_LIST is used this rountine allows the nesting of Vararg routines. Thus 
  this is the main print working routine

  @param  Buffer      Character buffer to print the results of the parsing
                      of Format into.
  @param  BufferSize  Maximum number of characters to put into buffer.
                      Zero means no limit.
  @param  Flags       Intial flags value.
                      Can only have FORMAT_UNICODE and OUTPUT_UNICODE set
  @param  Format      Null-terminated format string.
  @param  Marker      Vararg list consumed by processing Format.

  @return Number of characters printed.

**/
UINTN
BasePrintLibVSPrint (
  OUT CHAR8        *Buffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      Marker
  )
{
  CHAR8           *OriginalBuffer;
  CHAR8           ValueBuffer[MAXIMUM_VALUE_CHARACTERS];
  UINTN           BytesPerOutputCharacter;
  UINTN           BytesPerFormatCharacter;
  UINTN           FormatMask;
  UINTN           FormatCharacter;
  UINTN           Width;
  UINTN           Precision;
  INT64           Value;
  CHAR8           *ArgumentString;
  UINTN           Character;
  GUID            *TmpGuid;
  TIME            *TmpTime;
  UINTN           Count;
  UINTN           ArgumentMask;
  INTN            BytesPerArgumentCharacter;
  UINTN           ArgumentCharacter;
  BOOLEAN         Done;
  UINTN           Index;
  CHAR8           Prefix;
  BOOLEAN         ZeroPad;
  BOOLEAN         Comma;
  UINTN           Digits;
  UINTN           Radix;
  RETURN_STATUS   Status;

  ASSERT (Buffer != NULL);
  ASSERT (Format != NULL);

  OriginalBuffer = Buffer;

  if ((Flags & OUTPUT_UNICODE) != 0) {
    BytesPerOutputCharacter = 2;
  } else {
    BytesPerOutputCharacter = 1;
  }
  if ((Flags & FORMAT_UNICODE) != 0) {
    BytesPerFormatCharacter = 2;
    FormatMask = 0xffff;
  } else {
    BytesPerFormatCharacter = 1;
    FormatMask = 0xff;
  }

  //
  // Reserve space for the Null terminator.
  // If BufferSize is 0, this will set BufferSize to the max unsigned value
  //
  BufferSize--;

  //
  // Get the first character from the format string
  //
  FormatCharacter = (*Format | (*(Format + 1) << 8)) & FormatMask;

  //
  // Loop until the end of the format string is reached or the output buffer is full
  //
  while (FormatCharacter != 0 && BufferSize > 0) {
    //
    // Clear all the flag bits except those that may have been passed in
    //
    Flags &= (OUTPUT_UNICODE | FORMAT_UNICODE);

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
        FormatCharacter = (*Format | (*(Format + 1) << 8)) & FormatMask;
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
            Width = VA_ARG (Marker, UINTN);
          } else {
            Precision = VA_ARG (Marker, UINTN);
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
            FormatCharacter = (*Format | (*(Format + 1) << 8)) & FormatMask;
          }
          Format -= BytesPerFormatCharacter;
          if ((Flags & PRECISION) == 0) {
            Flags |= PAD_TO_WIDTH;
            Width = Count;
          } else {
            Precision = Count;
          }
          break;
        default:
          Done = TRUE;
          break;
        }
      } 

      //
      // Limit the maximum field width to the remaining characters in the output buffer
      //
      if (Width > BufferSize) {
        Width = BufferSize;
      }

      //
      // Handle each argument type
      //
      switch (FormatCharacter) {
      case 'p':
        if (sizeof (VOID *) > 4) {
          Flags |= LONG_TYPE;
        }
      case 'X':
        Flags |= PREFIX_ZERO;
        //
        // break skiped on purpose
        //
      case 'x':
        Flags |= RADIX_HEX;
        //
        // break skiped on purpose
        //
      case 'd':
        if ((Flags & LONG_TYPE) == 0) {
          Value = (VA_ARG (Marker, int));
        } else {
          Value = VA_ARG (Marker, INT64);
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
            Value = (unsigned int)Value;
          }
        }
        //
        // Convert Value to a reversed string
        //
        Count = BasePrintLibValueToString (ValueBuffer, Value, Radix);
        if (Value == 0 && Precision == 0) {
          Count = 0;
        }
        ArgumentString = (CHAR8 *)ValueBuffer + Count;
        Digits = 3 - (Count % 3);
        if (Comma && Count != 0) {
          Count += ((Count - 1) / 3);
        }
        if (Prefix != 0) {
          Count++;
        }
        Flags |= ARGUMENT_REVERSED;
        ZeroPad = TRUE;
        if ((Flags & PREFIX_ZERO) != 0) {
          if ((Flags & PAD_TO_WIDTH) != 0) {
            if ((Flags & PRECISION) == 0) {
              Precision = Width;
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
        ArgumentString = (CHAR8 *)VA_ARG (Marker, CHAR8 *);
        if (ArgumentString == NULL) {
          Flags &= (~ARGUMENT_UNICODE);
          ArgumentString = "<null string>";
        }
        break;

      case 'c':
        Character = VA_ARG (Marker, UINTN) & 0xffff;
        ArgumentString = (CHAR8 *)&Character;
        Flags |= ARGUMENT_UNICODE;
        break;

      case 'g':
        TmpGuid = VA_ARG (Marker, GUID *);
        if (TmpGuid == NULL) {
          ArgumentString = "<null guid>";
        } else {
          BasePrintLibSPrint (
            ValueBuffer,
            0, 
            0,
            "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            TmpGuid->Data1,
            TmpGuid->Data2,
            TmpGuid->Data3,
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
        TmpTime = VA_ARG (Marker, TIME *); 
        if (TmpTime == NULL) {
          ArgumentString = "<null time>";
        } else {
          BasePrintLibSPrint (
            ValueBuffer,
            0,
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
        Status = VA_ARG (Marker, RETURN_STATUS);
        ArgumentString = ValueBuffer;
        for (Index = 0; StatusString[Index].String != NULL; Index++) {
          if (Status == StatusString[Index].Status) {
            ArgumentString = StatusString[Index].String;
          }
        }
        if (ArgumentString == ValueBuffer) {
          BasePrintLibSPrint ((CHAR8 *) ValueBuffer, 0, 0, "%08X", Status);
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
    case '\n':
      ArgumentString = "\r\n";
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

    //
    // Limit the length of the string to append to the remaining characters in the output buffer
    //
    if (Count > BufferSize) {
      Count = BufferSize;
    }
    if (Precision < Count) {
      Precision = Count;
    }

    //
    // Pad before the string
    //
    if ((Flags & (PAD_TO_WIDTH | LEFT_JUSTIFY)) == (PAD_TO_WIDTH)) {
      Buffer = BasePrintLibFillBuffer (Buffer, Width - Precision, ' ', BytesPerOutputCharacter);
    }

    if (ZeroPad) {
      if (Prefix != 0) {
        Buffer = BasePrintLibFillBuffer (Buffer, 1, Prefix, BytesPerOutputCharacter);
      }
      Buffer = BasePrintLibFillBuffer (Buffer, Precision - Count, '0', BytesPerOutputCharacter);
    } else {
      Buffer = BasePrintLibFillBuffer (Buffer, Precision - Count, ' ', BytesPerOutputCharacter);
      if (Prefix != 0) {
        Buffer = BasePrintLibFillBuffer (Buffer, 1, Prefix, BytesPerOutputCharacter);
      }
    }

    //
    // Output the Prefix character if it is present
    //
    Index = 0;
    if (Prefix) {
      Index++;
    }

    //
    // Copy the string into the output buffer performing the required type conversions
    //
    while (Index < Count) {
      ArgumentCharacter = ((*ArgumentString & 0xff) | (*(ArgumentString + 1) << 8)) & ArgumentMask;

      Buffer = BasePrintLibFillBuffer (Buffer, 1, ArgumentCharacter, BytesPerOutputCharacter);
      ArgumentString    += BytesPerArgumentCharacter;
      Index++;
      if (Comma) {
        Digits++;
        if (Digits == 3) {
          Digits = 0;
          Index++;
          if (Index < Count) {
            Buffer = BasePrintLibFillBuffer (Buffer, 1, ',', BytesPerOutputCharacter);
          }
        }
      }
    }

    //
    // Pad after the string
    //
    if ((Flags & (PAD_TO_WIDTH | LEFT_JUSTIFY)) == (PAD_TO_WIDTH | LEFT_JUSTIFY)) {
      Buffer = BasePrintLibFillBuffer (Buffer, Width - Precision, ' ', BytesPerOutputCharacter);
    }

    //
    // Reduce the number of characters
    //
    BufferSize -= Count;

    //
    // Get the next character from the format string
    //
    Format += BytesPerFormatCharacter;

    //
    // Get the next character from the format string
    //
    FormatCharacter = (*Format | (*(Format + 1) << 8)) & FormatMask;
  }

  //
  // Null terminate the Unicode or ASCII string
  //
  Buffer = BasePrintLibFillBuffer (Buffer, 1, 0, BytesPerOutputCharacter);
   
  return ((Buffer - OriginalBuffer) / BytesPerOutputCharacter);
}

/**
  Worker function that produces a Null-terminated string in an output buffer 
  based on a Null-terminated format string and variable argument list.

  VSPrint function to process format and place the results in Buffer. Since a 
  VA_LIST is used this rountine allows the nesting of Vararg routines. Thus 
  this is the main print working routine

  @param  Buffer        Character buffer to print the results of the parsing
                        of Format into.
  @param  BufferSize    Maximum number of characters to put into buffer.
                        Zero means no limit.
  @param  Flags         Intial flags value.
                        Can only have FORMAT_UNICODE and OUTPUT_UNICODE set
  @param  FormatString  Null-terminated format string.

  @return Number of characters printed.

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
  return BasePrintLibVSPrint (StartOfBuffer, BufferSize, Flags, FormatString, Marker);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on 
  a Null-terminated Unicode format string and a VA_LIST argument list
  
  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.  
  The Unicode string is produced by parsing the format string specified by FormatString.  
  Arguments are pulled from the variable argument list specified by Marker based on the 
  contents of the format string.  
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return return Length of the produced output buffer.

**/
UINTN
EFIAPI
UnicodeVSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  )
{
  return BasePrintLibVSPrint ((CHAR8 *)StartOfBuffer, BufferSize >> 1, FORMAT_UNICODE | OUTPUT_UNICODE, (CHAR8 *)FormatString, Marker);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated 
  Unicode format string and variable argument list.
  
  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the format string.
  The length of the produced output buffer is returned.  
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
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
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated
  ASCII format string and a VA_LIST argument list
  
  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on the 
  contents of the format string.
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
UnicodeVSPrintAsciiFormat (
  OUT CHAR16       *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  IN  VA_LIST      Marker
  )
{
  return BasePrintLibVSPrint ((CHAR8 *)StartOfBuffer, BufferSize >> 1, OUTPUT_UNICODE,FormatString, Marker);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated 
  ASCII format string and  variable argument list.
  
  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the 
  format string.
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
UnicodeSPrintAsciiFormat (
  OUT CHAR16       *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, FormatString);
  return UnicodeVSPrintAsciiFormat (StartOfBuffer, BufferSize >> 1, FormatString, Marker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and a VA_LIST argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on 
  the contents of the format string.
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
AsciiVSPrint (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  IN  VA_LIST       Marker
  )
{
  return BasePrintLibVSPrint (StartOfBuffer, BufferSize, 0, FormatString, Marker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and  variable argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the 
  format string.
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
AsciiSPrint (
  OUT CHAR8        *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, FormatString);
  return AsciiVSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and a VA_LIST argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on 
  the contents of the format string.
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
AsciiVSPrintUnicodeFormat (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  )
{
  return BasePrintLibVSPrint (StartOfBuffer, BufferSize, FORMAT_UNICODE, (CHAR8 *)FormatString, Marker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and  variable argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the 
  format string.
  The length of the produced output buffer is returned.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize is not 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize is not 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than 
  PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength ASCII characters, then ASSERT().

  @param  StartOfBuffer   APointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  
  @return Length of the produced output buffer.

**/
UINTN
EFIAPI
AsciiSPrintUnicodeFormat (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, FormatString);
  return AsciiVSPrintUnicodeFormat (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Converts a decimal value to a Null-terminated Unicode string.
  
  Converts the decimal number specified by Value to a Null-terminated Unicode 
  string specified by Buffer containing at most Width characters.
  If Width is 0 then a width of  MAXIMUM_VALUE_CHARACTERS is assumed.
  The total number of characters placed in Buffer is returned.
  If the conversion contains more than Width characters, then only the first
  Width characters are returned, and the total number of characters 
  required to perform the conversion is returned.
  Additional conversion parameters are specified in Flags.  
  The Flags bit LEFT_JUSTIFY is always ignored.
  All conversions are left justified in Buffer.
  If Width is 0, PREFIX_ZERO is ignored in Flags.
  If COMMA_TYPE is set in Flags, then PREFIX_ZERO is ignored in Flags, and commas
  are inserted every 3rd digit starting from the right.
  If Value is < 0, then the fist character in Buffer is a '-'.
  If PREFIX_ZERO is set in Flags and PREFIX_ZERO is not being ignored, 
  then Buffer is padded with '0' characters so the combination of the optional '-' 
  sign character, '0' characters, digit characters for Value, and the Null-terminator
  add up to Width characters.

  If Buffer is NULL, then ASSERT().
  If unsupported bits are set in Flags, then ASSERT().
  If Width >= MAXIMUM_VALUE_CHARACTERS, then ASSERT()

  @param  Buffer  Pointer to the output buffer for the produced Null-terminated
                  Unicode string.
  @param  Flags   The bitmask of flags that specify left justification, zero pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width	  The maximum number of Unicode characters to place in Buffer.
  
  @return Total number of characters required to perform the conversion.

**/
UINTN
EFIAPI
UnicodeValueToString (
  IN OUT CHAR16  *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width
  )
{
  return BasePrintLibConvertValueToString ((CHAR8 *)Buffer, Flags, Value, Width, 2);
}

/**
  Converts a decimal value to a Null-terminated ASCII string.
  
  Converts the decimal number specified by Value to a Null-terminated ASCII string 
  specified by Buffer containing at most Width characters.
  If Width is 0 then a width of  MAXIMUM_VALUE_CHARACTERS is assumed.
  The total number of characters placed in Buffer is returned.
  If the conversion contains more than Width characters, then only the first Width
  characters are returned, and the total number of characters required to perform
  the conversion is returned.
  Additional conversion parameters are specified in Flags.  
  The Flags bit LEFT_JUSTIFY is always ignored.
  All conversions are left justified in Buffer.
  If Width is 0, PREFIX_ZERO is ignored in Flags.
  If COMMA_TYPE is set in Flags, then PREFIX_ZERO is ignored in Flags, and commas 
  are inserted every 3rd digit starting from the right.
  If Value is < 0, then the fist character in Buffer is a '-'.
  If PREFIX_ZERO is set in Flags and PREFIX_ZERO is not being ignored, then Buffer
  is padded with '0' characters so the combination of the optional '-'
  sign character, '0' characters, digit characters for Value, and the 
  Null-terminator add up to Width characters.

  If Buffer is NULL, then ASSERT().
  If unsupported bits are set in Flags, then ASSERT().
  If Width >= MAXIMUM_VALUE_CHARACTERS, then ASSERT()

  @param  Buffer  Pointer to the output buffer for the produced Null-terminated
                  ASCII string.
  @param  Flags   The bitmask of flags that specify left justification, zero pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width	  The maximum number of ASCII characters to place in Buffer.
  
  @return Total number of characters required to perform the conversion.

**/
UINTN
EFIAPI
AsciiValueToString (
  IN OUT CHAR8  *Buffer,
  IN UINTN      Flags,
  IN INT64      Value,
  IN UINTN      Width
  )
{
  return BasePrintLibConvertValueToString ((CHAR8 *)Buffer, Flags, Value, Width, 1);
}
