/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Sprint.c

Abstract:

  Basic Ascii AvSPrintf() function named VSPrint(). VSPrint() enables very
  simple implemenation of SPrint() and Print() to support debug. 

  You can not Print more than EFI_DRIVER_LIB_MAX_PRINT_BUFFER characters at a 
  time. This makes the implementation very simple.

  VSPrint, Print, SPrint format specification has the follwoing form

  %[flags][width]type

  flags:
    '-' - Left justify
    '+' - Prefix a sign
    ' ' - Prefix a blank
    ',' - Place commas in numberss
    '0' - Prefix for width with zeros
    'l' - UINT64
    'L' - UINT64

  width:
    '*' - Get width from a UINTN argumnet from the argument list
    Decimal number that represents width of print

  type:
    'X' - argument is a UINTN hex number, prefix '0'
    'x' - argument is a hex number
    'd' - argument is a decimal number
    'a' - argument is an ascii string 
    'S','s' - argument is an Unicode string
    'g' - argument is a pointer to an EFI_GUID
    't' - argument is a pointer to an EFI_TIME structure
    'c' - argument is an ascii character
    'r' - argument is EFI_STATUS
    '%' - Print a %

--*/

#include "TianoCommon.h"
#include "PrintWidth.h"
#include "EfiPrintLib.h"
#include "Print.h"


UINTN
ASPrint (
  OUT CHAR8         *Buffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *Format,
  ...
  )
/*++

Routine Description:

  Process format and place the results in Buffer for narrow chars.

Arguments:

  Buffer      - Narrow char buffer to print the results of the parsing of Format into.
  BufferSize  - Maximum number of characters to put into buffer.
  Format      - Format string
  ...         - Vararg list consumed by processing Format.

Returns:

  Number of characters printed.

--*/
{
  UINTN   Return;
  VA_LIST Marker;

  VA_START (Marker, Format);
  Return = AvSPrint (Buffer, BufferSize, Format, Marker);
  VA_END (Marker);

  return Return;
}


UINTN
AvSPrint (
  OUT CHAR8         *Buffer,
  IN  UINTN         BufferSize,
  IN  CONST CHAR8   *FormatString,
  IN  VA_LIST       Marker
  )
/*++

Routine Description:

  Internal implementation of ASPrint. 
  Process format and place the results in Buffer for narrow chars.

Arguments:

  Buffer        - Narrow char buffer to print the results of the parsing of Format into.
  BufferSize    - Maximum number of characters to put into buffer.
  FormatString  - Format string
  Marker        - Vararg list consumed by processing Format.

Returns:

  Number of characters printed.

--*/
{
  UINTN   Index;
  CHAR16  UnicodeFormat[EFI_DRIVER_LIB_MAX_PRINT_BUFFER+1];
  CHAR16  UnicodeResult[EFI_DRIVER_LIB_MAX_PRINT_BUFFER+1];

  for (Index = 0; Index < EFI_DRIVER_LIB_MAX_PRINT_BUFFER && FormatString[Index] != '\0'; Index++) {
    UnicodeFormat[Index] = (CHAR16) FormatString[Index];
  }

  UnicodeFormat[Index]  = '\0';

  Index                 = VSPrint (UnicodeResult, sizeof (UnicodeResult), UnicodeFormat, Marker);

  for (Index = 0; (Index < (BufferSize - 1)) && UnicodeResult[Index] != '\0'; Index++) {
    Buffer[Index] = (CHAR8) UnicodeResult[Index];
  }

  Buffer[Index] = '\0';

  return Index++;
}
