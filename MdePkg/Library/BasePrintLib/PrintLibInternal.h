/** @file
  Print Library.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PrintLibInternal.h

**/

//
// Print primitives
//
//#define LEFT_JUSTIFY      0x01
#define PREFIX_SIGN       0x02
#define PREFIX_BLANK      0x04
//#define COMMA_TYPE        0x08
#define LONG_TYPE         0x10
//#define PREFIX_ZERO       0x20
#define OUTPUT_UNICODE    0x40
#define RADIX_HEX         0x80
#define FORMAT_UNICODE    0x100
#define PAD_TO_WIDTH      0x200
#define ARGUMENT_UNICODE  0x400
#define PRECISION         0x800
#define ARGUMENT_REVERSED 0x1000

//
// Record date and time information
//
typedef struct {
  UINT16  Year;
  UINT8   Month;
  UINT8   Day;
  UINT8   Hour;
  UINT8   Minute;
  UINT8   Second;
  UINT8   Pad1;
  UINT32  Nanosecond;
  INT16   TimeZone;
  UINT8   Daylight;
  UINT8   Pad2;
} TIME;

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
  OUT CHAR8        *Buffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *FormatString,
  ...
  );

/**
  Internal function that places the character into the Buffer.

  Internal function that places ASCII or Unicode character into the Buffer.

  @param  Buffer      Buffer to place the Unicode or ASCII string.
  @param  Length      Count of character to be placed into Buffer.
  @param  Character   Character to be placed into Buffer.
  @param  Increment   Character increment in Buffer.

  @return Number of characters printed.

**/
CHAR8 *
BasePrintLibFillBuffer (
  CHAR8   *Buffer,
  INTN    Length,
  UINTN   Character,
  INTN    Increment
  );

/**
  Internal function that convert a decimal number to a string in Buffer.

  Print worker function that convert a decimal number to a string in Buffer.

  @param  Buffer    Location to place the Unicode or ASCII string of Value.
  @param  Value     Value to convert to a Decimal or Hexidecimal string in Buffer.
  @param  Radix     Radix of the value

  @return Number of characters printed.

**/
UINTN
EFIAPI
BasePrintLibValueToString (
  IN OUT CHAR8  *Buffer, 
  IN INT64      Value, 
  IN UINTN      Radix
  );

/**
  Internal function that converts a decimal value to a Null-terminated string.
  
  Converts the decimal number specified by Value to a Null-terminated  
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

  @param  Buffer    Pointer to the output buffer for the produced Null-terminated
                    string.
  @param  Flags     The bitmask of flags that specify left justification, zero pad,
                    and commas.
  @param  Value     The 64-bit signed value to convert to a string.
  @param  Width	    The maximum number of characters to place in Buffer.
  @param  Increment Character increment in Buffer.
  
  @return Total number of characters required to perform the conversion.

**/
UINTN
BasePrintLibConvertValueToString (
  IN OUT CHAR8   *Buffer,
  IN UINTN       Flags,
  IN INT64       Value,
  IN UINTN       Width,
  IN UINTN       Increment
  );
