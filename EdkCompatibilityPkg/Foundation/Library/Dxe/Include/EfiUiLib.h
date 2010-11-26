/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiUiLib.h

Abstract:
  Collection of usefull UI functions.

Revision History:

--*/

#ifndef _EFI_UI_LIB_H_
#define _EFI_UI_LIB_H_

#include "Tiano.h"
#include "TianoTypes.h"
#include "EfiDriverLib.h"

CHAR16  *
StrHzToString (
  OUT CHAR16         *String,
  IN  UINT64         Val
  )
/*++

Routine Description:
  Converts frequency in Hz to Unicode string. 
  Three significant digits are delivered. Used for processor info display.

Arguments:
  String - string that will contain the frequency.
  Val    - value to convert, minimum is  100000 i.e., 0.1 MHz.

Returns:
  String that contains the frequency.

--*/
;

CHAR16  *
StrBytesToString (
  OUT CHAR16         *String,
  IN  UINT64         Val
  )
/*++

Routine Description:
  Converts size in bytes to Unicode string.
  Used for memory/cache size display.

Arguments:
  String - string that will contain the value
  Val    - value to convert in bytes

Returns:
  String that contains the value.

--*/
;

CHAR16  *
StrVersionToString (
  OUT CHAR16          *String,
  IN  UINT8           Version
  )
/*++

Routine Description:
  Converts 8 bit version value to Unicode string.
  The upper nibble contains the upper part, the lower nibble contains the minor part.
  The output format is <major>.<minor>.

Arguments:
  String  - string that will contain the version value
  Version - Version value to convert
  
Returns:
  String that contains the version value.

--*/
;

CHAR16  *
StrMacToString (
  OUT CHAR16              *String,
  IN  EFI_MAC_ADDRESS     *MacAddr,
  IN  UINT32              AddrSize
  )
/*++

Routine Description:
  Converts MAC address to Unicode string.
  The value is 64-bit and the resulting string will be 12
  digit hex number in pairs of digits separated by dashes.

Arguments:
  String - string that will contain the value
  MacAddr     - MAC address to convert
  AddrSize    - Size of address
  
Returns:
  String that contains the value.

--*/
;

CHAR16  *
StrIp4AdrToString (
  OUT CHAR16             *String,
  IN  EFI_IPv4_ADDRESS   *Ip4Addr
  )
/*++

Routine Description:
  Converts IP v4 address to Unicode string.
  The value is 64-bit and the resulting string will
  be four decimal values 0-255 separated by dots.

Arguments:
  String  - string that will contain the value
  Ip4Addr - IP v4 address to convert from

Returns:

  String that contain the value

--*/
;

EFI_STATUS
StrStringToIp4Adr (
  OUT EFI_IPv4_ADDRESS   *Ip4Addr,
  IN  CHAR16             *String
  )
/*++

Routine Description:
  Parses and converts Unicode string to IP v4 address.
  The value will 64-bit.
  The string must be four decimal values 0-255 separated by dots.
  The string is parsed and format verified.

Arguments:
  Ip4Addr - pointer to the variable to store the value to
  String  - string that contains the value to parse and convert

Returns:
  EFI_SUCCESS           - if successful
  EFI_INVALID_PARAMETER - if String contains invalid IP v4 format

--*/
;

CHAR16  *
Ascii2Unicode (
  OUT CHAR16         *UnicodeStr,
  IN  CHAR8          *AsciiStr
  )
/*++

Routine Description:
  Converts ASCII characters to Unicode.

Arguments:
  UnicodeStr - the Unicode string to be written to. The buffer must be large enough.
  AsciiStr   - The ASCII string to be converted.

Returns:
  The address to the Unicode string - same as UnicodeStr.

--*/
;

CHAR8   *
Unicode2Ascii (
  OUT CHAR8          *AsciiStr,
  IN  CHAR16         *UnicodeStr
  )
/*++

Routine Description:
  Converts ASCII characters to Unicode.
  Assumes that the Unicode characters are only these defined in the ASCII set.

Arguments:
  AsciiStr   - The ASCII string to be written to. The buffer must be large enough.
  UnicodeStr - the Unicode string to be converted.

Returns:
  The address to the ASCII string - same as AsciiStr.

--*/
;

EFI_STATUS
EfiStringToValue (
  OUT UINT64        *Val,
  IN  CHAR16        *String,
  OUT UINT8         *EndIdx OPTIONAL
  )
/*++

Routine Description:
  Parses and converts Unicode string to decimal value.
  The returned value is 64-bit.
  The string is expected in decimal format,
  the string is parsed and format verified.

Arguments:
  Val    - pointer to the variable to store the value to
  String - string that contains the value to parse and convert
  EndIdx - index on which the parsing stopped. It points to the
           first character that was not part of the returned Val.
           It's valid only if the function returns success.
           It's optional and it could be NULL.

Returns:
  EFI_SUCCESS           - if successful
  EFI_INVALID_PARAMETER - if String is in unexpected format

--*/
;

#endif
