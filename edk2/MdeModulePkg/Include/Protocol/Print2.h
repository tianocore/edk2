/** @file

  This print protocol defines six basic print functions to 
  print the format unicode and ascii string.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PPRINT2_H__
#define __PPRINT2_H__

#define EFI_PRINT2_PROTOCOL_GUID  \
   { 0x5bcc3dbc, 0x8c57, 0x450a, { 0xbb, 0xc, 0xa1, 0xc0, 0xbd, 0xde, 0x48, 0xc } }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_PRINT2_PROTOCOL  EFI_PRINT2_PROTOCOL;

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
typedef
UINTN
(EFIAPI *UNI_VSPRINT2)(
  OUT CHAR16        *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  );

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
typedef
UINTN
(EFIAPI *UNI_VSPRINT_ASCII)(
  OUT CHAR16       *StartOfBuffer,
  IN  UINTN        BufferSize,
  IN  CONST CHAR8  *FormatString,
  IN  VA_LIST      Marker
  );

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
typedef
UINTN
(EFIAPI *VALUE_TO_UNISTRING)(
  IN OUT CHAR16  *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width
  );

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
typedef
UINTN
(EFIAPI *ASCII_VSPRINT)(
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  IN  VA_LIST       Marker
  );

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
typedef
UINTN
(EFIAPI *ASCII_VSPRINT_UNI)(
  OUT CHAR8         *StartOfBuffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR16  *FormatString,
  IN  VA_LIST       Marker
  );

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
typedef
UINTN
(EFIAPI *VALUE_TO_ASCIISTRING)(
  IN OUT CHAR8  *Buffer,
  IN UINTN      Flags,
  IN INT64      Value,
  IN UINTN      Width
  );

struct _EFI_PRINT2_PROTOCOL {
  UNI_VSPRINT2              VSPrint;
  UNI_VSPRINT_ASCII         UniVSPrintAscii;
  VALUE_TO_UNISTRING        UniValueToString;                         
  ASCII_VSPRINT             AsciiVSPrint;          
  ASCII_VSPRINT_UNI         AsciiVSPrintUni;
  VALUE_TO_ASCIISTRING      AsciiValueToString;
};

extern EFI_GUID gEfiPrint2ProtocolGuid;

#endif
