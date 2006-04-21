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

///
/// Define the maximum number of characters that are required to encode
/// a decimal, hexidecimal, GUID, or TIME value with a Nll terminator.
///   Maximum Length Decimal String     = 28    "-9,223,372,036,854,775,808"
///   Maximum Length Hexidecimal String = 17    "FFFFFFFFFFFFFFFF"
///   Maximum Length GUID               = 37    "00000000-0000-0000-0000-000000000000"
///   Maximum Length TIME               = 17    "12/12/2006  12:12"
///
#define MAXIMUM_VALUE_CHARACTERS  38

//
//
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

UINTN
BasePrintLibSPrint (
  OUT CHAR8        *Buffer,
  IN  UINTN        BufferSize,
  IN  UINTN        Flags,
  IN  CONST CHAR8  *FormatString,
  ...
  );

CHAR8 *
BasePrintLibFillBuffer (
  CHAR8   *Buffer,
  INTN    Length,
  UINTN   Character,
  INTN    Increment
  );

UINTN
EFIAPI
BasePrintLibValueToString (
  IN OUT CHAR8  *Buffer, 
  IN INT64      Value, 
  IN UINTN      Radix
  );

