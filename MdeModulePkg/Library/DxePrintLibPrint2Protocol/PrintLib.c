/** @file
  Instance of Print Library based on gEfiPrint2ProtocolGuid.

  Implement the print library instance by wrap the interface 
  provided in the Print2 protocol. This protocol is defined as the internal
  protocol related to this implementation, not in the public spec. So, this 
  library instance is only for this code base.

Copyright (c) 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/Print2.h>

#include <Library/PrintLib.h>

#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

EFI_PRINT2_PROTOCOL  *mPrint2Protocol = NULL;

/**
  The constructor function caches the pointer to Print2 protocol.
  
  The constructor function locates Print2 protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS. 

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PrintLibConstructor (
  IN EFI_HANDLE                ImageHandle,
  IN EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_STATUS                   Status;

  Status = gBS->LocateProtocol (
                  &gEfiPrint2ProtocolGuid,
                  NULL,
                  (VOID**) &mPrint2Protocol
                  );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mPrint2Protocol != NULL);

  return Status;
}


/**
  Worker function that converts a VA_LIST to a BASE_LIST based on a Null-terminated 
  format string.

  @param  AsciiFormat     TRUE if Format is an ASCII string.  FALSE if Format is a Unicode string.
  @param  Format          Null-terminated format string.
  @param  VaListMarker    VA_LIST style variable argument list consumed by processing Format.
  @param  BaseListMarker  BASE_LIST style variable argument list consumed by processing Format.
  @param  Size            The size, in bytes, of the BaseListMarker buffer.

  @return The number of bytes in BaseListMarker.  0 if BaseListMarker is too small.

**/
BOOLEAN
DxePrintLibPrint2ProtocolVaListToBaseList (
  IN  BOOLEAN      AsciiFormat,
  IN  CONST CHAR8  *Format,
  IN  VA_LIST      VaListMarker,
  OUT BASE_LIST    BaseListMarker,
  IN  UINTN        Size
  )
{
  BASE_LIST  BaseListStart;
  UINTN      BytesPerFormatCharacter;
  UINTN      FormatMask;
  UINTN      FormatCharacter;
  BOOLEAN    Long;
  BOOLEAN    Done;

  ASSERT (Format         != NULL);
  ASSERT (VaListMarker   != NULL);
  ASSERT (BaseListMarker != NULL);

  BaseListStart = BaseListMarker;

  if (AsciiFormat) {
    ASSERT (AsciiStrSize (Format) != 0);
    BytesPerFormatCharacter = 1;
    FormatMask = 0xff;
  } else {
    ASSERT (StrSize ((CHAR16 *) Format) != 0);
    BytesPerFormatCharacter = 2;
    FormatMask = 0xffff;
  }

  //
  // Get the first character from the format string
  //
  FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;

  while (FormatCharacter != 0) {
    if (FormatCharacter == '%') {
      Long = FALSE;

      //
      // Parse Flags and Width
      //
      for (Done = FALSE; !Done; ) {
        //
        // Get the next character from the format string
        //
        Format += BytesPerFormatCharacter;

        //
        // Get the next character from the format string
        //
        FormatCharacter = ((*Format & 0xff) | (*(Format + 1) << 8)) & FormatMask;

        switch (FormatCharacter) {
        case '.': 
        case '-': 
        case '+': 
        case ' ': 
        case ',': 
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          break;
        case 'L':
        case 'l': 
          Long = TRUE;
          break;
        case '*':
          BASE_ARG (BaseListMarker, UINTN) = VA_ARG (VaListMarker, UINTN);
          break;
        case '\0':
          //
          // Make no output if Format string terminates unexpectedly when
          // looking up for flag, width, precision and type. 
          //
          Format -= BytesPerFormatCharacter;
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
        if (sizeof (VOID *) > 4) {
          Long = TRUE;
        }
      case 'X':
      case 'x':
      case 'd':
        if (Long) {
          BASE_ARG (BaseListMarker, INT64) = VA_ARG (VaListMarker, INT64);
        } else {
          BASE_ARG (BaseListMarker, int) = VA_ARG (VaListMarker, int);
        }
        break;
      case 's':
      case 'S':
      case 'a':
      case 'g':
      case 't':
        BASE_ARG (BaseListMarker, VOID *) = VA_ARG (VaListMarker, VOID *);
        break;
      case 'c':
        BASE_ARG (BaseListMarker, UINTN) = VA_ARG (VaListMarker, UINTN);
        break;
      case 'r':
        BASE_ARG (BaseListMarker, RETURN_STATUS) = VA_ARG (VaListMarker, RETURN_STATUS);
        break;
      }
    }

    //
    // If BASE_LIST is larger than Size, then return FALSE
    //
    if ((UINTN)((UINT8 *)BaseListMarker - (UINT8 *)BaseListStart) > Size) {
      return FALSE;
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
  return TRUE;
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
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

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
  UINT8  BaseListMarker[256];

  DxePrintLibPrint2ProtocolVaListToBaseList (
    FALSE, 
    (CHAR8 *)FormatString, 
    Marker, 
    (BASE_LIST)BaseListMarker, 
    sizeof (BaseListMarker) - 8
    );

  return UnicodeBSPrint (StartOfBuffer, BufferSize, FormatString, (BASE_LIST)BaseListMarker);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on 
  a Null-terminated Unicode format string and a BASE_LIST argument list
  
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
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          BASE_LIST marker for the variable argument list.
  
  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
EFIAPI
UnicodeBSPrint (
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  BASE_LIST     Marker
  )
{
  return mPrint2Protocol->UnicodeBSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
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
  @param  FormatString    Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the 
                          format string specified by FormatString.

  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

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
  The number of Unicode characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0 or 1, then no output buffer is produced and 0 is returned.

  If BufferSize > 1 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 1 and StartOfBuffer is not aligned on a 16-bit boundary, then ASSERT().
  If BufferSize > 1 and FormatString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and FormatString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

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
  UINT8  BaseListMarker[256];

  DxePrintLibPrint2ProtocolVaListToBaseList (
    TRUE, 
    FormatString, 
    Marker, 
    (BASE_LIST)BaseListMarker, 
    sizeof (BaseListMarker) - 8
    );

  return UnicodeBSPrintAsciiFormat (StartOfBuffer, BufferSize, FormatString, (BASE_LIST)BaseListMarker);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated
  ASCII format string and a BASE_LIST argument list
  
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
  If PcdMaximumAsciiStringLength is not zero, and FormatString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          BASE_LIST marker for the variable argument list.
  
  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
EFIAPI
UnicodeBSPrintAsciiFormat (
  OUT CHAR16       *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  IN  BASE_LIST    Marker
  )
{
  return mPrint2Protocol->UnicodeBSPrintAsciiFormat (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Produces a Null-terminated Unicode string in an output buffer based on a Null-terminated 
  ASCII format string and  variable argument list.
  
  Produces a Null-terminated Unicode string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The Unicode string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the 
  format string.
  The number of Unicode characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0 or 1, then no output buffer is produced and 0 is returned.

  If BufferSize > 1 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 1 and StartOfBuffer is not aligned on a 16-bit boundary, then ASSERT().
  If BufferSize > 1 and FormatString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and FormatString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and produced Null-terminated Unicode string
  contains more than PcdMaximumUnicodeStringLength Unicode characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          Unicode string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the 
                          format string specified by FormatString.
  
  @return The number of Unicode characters in the produced output buffer not including the
          Null-terminator.

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
  return UnicodeVSPrintAsciiFormat (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Converts a decimal value to a Null-terminated Unicode string.
  
  Converts the decimal number specified by Value to a Null-terminated Unicode 
  string specified by Buffer containing at most Width characters. No padding of spaces 
  is ever performed. If Width is 0 then a width of MAXIMUM_VALUE_CHARACTERS is assumed.
  The number of Unicode characters in Buffer is returned not including the Null-terminator.
  If the conversion contains more than Width characters, then only the first
  Width characters are returned, and the total number of characters 
  required to perform the conversion is returned.
  Additional conversion parameters are specified in Flags.  
  
  The Flags bit LEFT_JUSTIFY is always ignored.
  All conversions are left justified in Buffer.
  If Width is 0, PREFIX_ZERO is ignored in Flags.
  If COMMA_TYPE is set in Flags, then PREFIX_ZERO is ignored in Flags, and commas
  are inserted every 3rd digit starting from the right.
  If HEX_RADIX is set in Flags, then the output buffer will be 
  formatted in hexadecimal format.
  If Value is < 0 and HEX_RADIX is not set in Flags, then the fist character in Buffer is a '-'.
  If PREFIX_ZERO is set in Flags and PREFIX_ZERO is not being ignored, 
  then Buffer is padded with '0' characters so the combination of the optional '-' 
  sign character, '0' characters, digit characters for Value, and the Null-terminator
  add up to Width characters.
  If both COMMA_TYPE and HEX_RADIX are set in Flags, then ASSERT().
  If Buffer is NULL, then ASSERT().
  If Buffer is not aligned on a 16-bit boundary, then ASSERT().
  If unsupported bits are set in Flags, then ASSERT().
  If both COMMA_TYPE and HEX_RADIX are set in Flags, then ASSERT().
  If Width >= MAXIMUM_VALUE_CHARACTERS, then ASSERT()

  @param  Buffer  Pointer to the output buffer for the produced Null-terminated
                  Unicode string.
  @param  Flags   The bitmask of flags that specify left justification, zero pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width   The maximum number of Unicode characters to place in Buffer, not including
                  the Null-terminator.
  
  @return The number of Unicode characters in Buffer not including the Null-terminator.

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
  return mPrint2Protocol->UnicodeValueToString (Buffer, Flags, Value, Width);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and a VA_LIST argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on 
  the contents of the format string.
  The number of ASCII characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize > 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and FormatString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and produced Null-terminated ASCII string
  contains more than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

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
  UINT8  BaseListMarker[256];

  DxePrintLibPrint2ProtocolVaListToBaseList (
    TRUE, 
    FormatString, 
    Marker, 
    (BASE_LIST)BaseListMarker, 
    sizeof (BaseListMarker) - 8
    );

  return AsciiBSPrint (StartOfBuffer, BufferSize, FormatString, (BASE_LIST)BaseListMarker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and a BASE_LIST argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on 
  the contents of the format string.
  The number of ASCII characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize > 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and FormatString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and produced Null-terminated ASCII string
  contains more than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          BASE_LIST marker for the variable argument list.
  
  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
EFIAPI
AsciiBSPrint (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  IN  BASE_LIST     Marker
  )
{
  return mPrint2Protocol->AsciiBSPrint (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and  variable argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the 
  format string.
  The number of ASCII characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize > 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is NULL, then ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and FormatString contains more than
  PcdMaximumAsciiStringLength ASCII characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and produced Null-terminated ASCII string
  contains more than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the 
                          format string specified by FormatString.

  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

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
  The number of ASCII characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize > 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and produced Null-terminated ASCII string
  contains more than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          VA_LIST marker for the variable argument list.
  
  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

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
  UINT8  BaseListMarker[256];

  DxePrintLibPrint2ProtocolVaListToBaseList (
    FALSE, 
    (CHAR8 *)FormatString, 
    Marker, 
    (BASE_LIST)BaseListMarker, 
    sizeof (BaseListMarker) - 8
    );

  return AsciiBSPrintUnicodeFormat (StartOfBuffer, BufferSize, FormatString, (BASE_LIST)BaseListMarker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and a BASE_LIST argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list specified by Marker based on 
  the contents of the format string.
  The number of ASCII characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize > 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and produced Null-terminated ASCII string
  contains more than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  Marker          BASE_LIST marker for the variable argument list.
  
  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

**/
UINTN
EFIAPI
AsciiBSPrintUnicodeFormat (
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  BASE_LIST     Marker
  )
{
  return mPrint2Protocol->AsciiBSPrintUnicodeFormat (StartOfBuffer, BufferSize, FormatString, Marker);
}

/**
  Produces a Null-terminated ASCII string in an output buffer based on a Null-terminated
  ASCII format string and  variable argument list.
  
  Produces a Null-terminated ASCII string in the output buffer specified by StartOfBuffer
  and BufferSize.
  The ASCII string is produced by parsing the format string specified by FormatString.
  Arguments are pulled from the variable argument list based on the contents of the 
  format string.
  The number of ASCII characters in the produced output buffer is returned not including
  the Null-terminator.
  If BufferSize is 0, then no output buffer is produced and 0 is returned.

  If BufferSize > 0 and StartOfBuffer is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is NULL, then ASSERT().
  If BufferSize > 0 and FormatString is not aligned on a 16-bit boundary, then ASSERT().
  If PcdMaximumUnicodeStringLength is not zero, and FormatString contains more than
  PcdMaximumUnicodeStringLength Unicode characters not including the Null-terminator, then
  ASSERT().
  If PcdMaximumAsciiStringLength is not zero, and produced Null-terminated ASCII string
  contains more than PcdMaximumAsciiStringLength ASCII characters not including the
  Null-terminator, then ASSERT().

  @param  StartOfBuffer   A pointer to the output buffer for the produced Null-terminated 
                          ASCII string.
  @param  BufferSize      The size, in bytes, of the output buffer specified by StartOfBuffer.
  @param  FormatString    Null-terminated Unicode format string.
  @param  ...             Variable argument list whose contents are accessed based on the 
                          format string specified by FormatString.

  @return The number of ASCII characters in the produced output buffer not including the
          Null-terminator.

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
  Converts a decimal value to a Null-terminated ASCII string.
  
  Converts the decimal number specified by Value to a Null-terminated ASCII string 
  specified by Buffer containing at most Width characters. No padding of spaces 
  is ever performed.
  If Width is 0 then a width of  MAXIMUM_VALUE_CHARACTERS is assumed.
  The number of ASCII characters in Buffer is returned not including the Null-terminator.
  If the conversion contains more than Width characters, then only the first Width
  characters are returned, and the total number of characters required to perform
  the conversion is returned.
  Additional conversion parameters are specified in Flags.  
  The Flags bit LEFT_JUSTIFY is always ignored.
  All conversions are left justified in Buffer.
  If Width is 0, PREFIX_ZERO is ignored in Flags.
  If COMMA_TYPE is set in Flags, then PREFIX_ZERO is ignored in Flags, and commas
  are inserted every 3rd digit starting from the right.
  If HEX_RADIX is set in Flags, then the output buffer will be 
  formatted in hexadecimal format.
  If Value is < 0 and HEX_RADIX is not set in Flags, then the fist character in Buffer is a '-'.
  If PREFIX_ZERO is set in Flags and PREFIX_ZERO is not being ignored, 
  then Buffer is padded with '0' characters so the combination of the optional '-' 
  sign character, '0' characters, digit characters for Value, and the Null-terminator
  add up to Width characters.
  
  If Buffer is NULL, then ASSERT().
  If unsupported bits are set in Flags, then ASSERT().
  If both COMMA_TYPE and HEX_RADIX are set in Flags, then ASSERT().
  If Width >= MAXIMUM_VALUE_CHARACTERS, then ASSERT()

  @param  Buffer  Pointer to the output buffer for the produced Null-terminated
                  ASCII string.
  @param  Flags   The bitmask of flags that specify left justification, zero pad, and commas.
  @param  Value   The 64-bit signed value to convert to a string.
  @param  Width   The maximum number of ASCII characters to place in Buffer, not including
                  the Null-terminator.
  
  @return The number of ASCII characters in Buffer not including the Null-terminator.

**/
UINTN
EFIAPI
AsciiValueToString (
  OUT CHAR8      *Buffer,
  IN  UINTN      Flags,
  IN  INT64      Value,
  IN  UINTN      Width
  )
{
  return mPrint2Protocol->AsciiValueToString (Buffer, Flags, Value, Width);
}
