/*++

Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  EfiUiLib.c

Abstract:
  Collection of usefull UI functions.

Revision History:

--*/

#include "EfiUiLib.h"

#define IS_DIGIT(Ch)  (((Ch) >= L'0') && ((Ch) <= L'9'))

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
{
  UINT8   i;
  UINT64  TempVal;

  TempVal = 0;
  //
  // Iterate upto 20 digits, only so many could fit in the UINT64
  //
  for (i = 0; i <= 20; i++) {
    //
    // test if the next character is not a digit
    //
    if (!IS_DIGIT (String[i])) {
      //
      // If here, there is no more digits,
      // return with success if there was at least one to process
      //
      if (i == 0) {
        break;
      }

      *Val = TempVal;

      if (EndIdx != NULL) {
        *EndIdx = i;
      }

      return EFI_SUCCESS;
    }
    //
    // If here, there is a digit to process
    //
    TempVal = MultU64x32 (TempVal, 10) + String[i] - L'0';
  }
  //
  // if here, there was some sort of format error
  //
  return EFI_INVALID_PARAMETER;
}

CHAR16 *
StrHzToString (
  OUT CHAR16          *String,
  IN  UINT64          Val
  )
/*++

Routine Description:
  Converts frequency in Hz to Unicode string. 
  Three significant digits are delivered. 
  Used for things like processor info display.

Arguments:
  String - string that will contain the frequency.
  Val    - value to convert, minimum is  100000 i.e., 0.1 MHz.

--*/
// GC_TODO: function comment is missing 'Returns:'
{
  CHAR16        HlpStr[8];
  UINT32        i;
  UINT32        IdxPoint;
  UINT32        IdxUnits;
  static CHAR16 *FreqUnits[] = { L" Hz", L" kHz", L" MHz", L" GHz", L" THz", L" PHz" };

  //
  // Normalize to 9999 or less.
  //
  i = 0;
  while (Val >= 10000) {
    Val = DivU64x32 (Val, 10, NULL);
    i++;
  }
  //
  // Make it rounded to the nearest, but only by
  // a .3. This assures that .6 is not rounded.
  //
  if (Val >= 1000) {
    Val += 3;
    Val = DivU64x32 (Val, 10, NULL);
    i++;
  }

  EfiValueToString (String, Val, 0, 0);

  //
  // Get rid of that cursed number!
  //
  if (!EfiStrCmp (&String[1], L"66")) {
    String[2] = L'7';
  }
  //
  // Compute index to the units substrings.
  //
  IdxUnits = (i + 2) / 3;

  if (IdxUnits >= (sizeof (FreqUnits) / sizeof (FreqUnits)[0])) {
    //
    // Frequency is too high.
    //
    EfiStrCpy (String, L"OVERFLOW");
    return String;
  }
  //
  // Compute the position of the decimal point.
  //
  IdxPoint = i % 3;

  //
  // Test if decimal point needs to be inserted.
  //
  if (IdxPoint != 0) {
    //
    // Save the part after decimal point.
    //
    EfiStrCpy (HlpStr, &String[IdxPoint]);

    //
    // Insert the point.
    //
    String[IdxPoint] = L'.';

    //
    // Reattach the saved part.
    //
    EfiStrCpy (&String[IdxPoint + 1], HlpStr);

    //
    // Clear the insignificant zero.
    //
    if (String[3] == L'0') {
      String[4 - IdxPoint] = L'\0';
    }
  }
  //
  // Attach units.
  //
  EfiStrCat (String, FreqUnits[IdxUnits]);

  return String;
}

CHAR16 *
StrBytesToString (
  OUT CHAR16          *String,
  IN  UINT64          Val
  )
/*++

Routine Description:
  Converts size in bytes to Unicode string.
  Used for memory/cache size display.

Arguments:
  String - string that will contain the value
  Val    - value to convert in bytes

--*/
// GC_TODO: function comment is missing 'Returns:'
{
  UINTN         i;
  UINTN         Rem;
  static CHAR16 *SizeUnits[] = { L" B", L" kB", L" MB", L" GB", L" TB", L" PB" };

  Rem = 0;

  for (i = 0; i < (sizeof (SizeUnits) / sizeof (SizeUnits)[0]); i++) {

    DivU64x32 (Val, 1024, &Rem);

    //
    // Done if:
    // 1. less than 1k
    // 2. less than 8k and there are fractions of 1k
    //
    if ((Val < 1024) || ((Val < 8192) && (Rem != 0))) {

      EfiValueToString (String, Val, 0, 0);

      //
      // attach units
      //
      EfiStrCat (String, SizeUnits[i]);
      return String;
    }
    //
    // prescale down by 1k with rounding to the nearest
    //
    Val = DivU64x32 (Val + 511, 1024, NULL);
  }

  EfiStrCpy (String, L"OVERFLOW");

  return String;
}

CHAR16 *
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
  String  - string that will contain the value
  Version - value to convert

--*/
// GC_TODO: function comment is missing 'Returns:'
{
  CHAR16  HlpStr[4];

  EfiValueToString (String, 0x0F & Version, 0, 0);
  EfiStrCat (String, L".");
  EfiValueToString (HlpStr, 0x0F & (Version >> 4), 0, 0);
  EfiStrCat (String, HlpStr);

  return String;
}

CHAR16 *
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
  Val    - value to convert

--*/
// GC_TODO: function comment is missing 'Returns:'
// GC_TODO:    MacAddr - add argument and description to function comment
// GC_TODO:    AddrSize - add argument and description to function comment
{
  UINT32  i;

  for (i = 0; i < AddrSize; i++) {

    EfiValueToHexStr (
      &String[2 * i],
      MacAddr->Addr[i] & 0xFF,
      PREFIX_ZERO,
      2
      );
  }
  //
  // Terminate the string.
  //
  String[2 * AddrSize] = L'\0';

  return String;
}

CHAR16 *
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
  Ip4Addr - value to convert from

--*/
// GC_TODO: function comment is missing 'Returns:'
{
  INT32   i;
  CHAR16  HlpStr[4];

  String[0] = L'\0';

  for (i = 0; i < 4; i++) {

    EfiValueToString (HlpStr, Ip4Addr->Addr[i], 0, 0);
    EfiStrCat (String, HlpStr);

    if (i < 3) {
      EfiStrCat (String, L".");
    }
  }

  return String;
}

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
{
  EFI_STATUS        Status;

  EFI_IPv4_ADDRESS  RetVal;
  UINT64            TempVal;
  UINT8             Idx;
  UINT8             i;

  Idx = 0;
  TempVal = 0;
  //
  // Iterate the decimal values separated by dots
  //
  for (i = 0; i < 4; i++) {
    //
    // get the value of a decimal
    //
    Status = EfiStringToValue (&TempVal, String, &Idx);
    if ((EFI_ERROR (Status)) || (TempVal > 255)) {
      break;
    }

    RetVal.Addr[i] = (UINT8) TempVal;
    String += Idx;

    //
    // test if it is the last decimal value
    //
    if (i == 3) {
      if (String[0] != L'\0') {
        //
        // the string must end with string termination character
        //
        break;
      }

      *Ip4Addr = RetVal;
      return EFI_SUCCESS;
    }
    //
    // Test for presence of a dot, it is required between the values
    //
    if (String++[0] != L'.') {
      break;
    }
  }

  return EFI_INVALID_PARAMETER;
}

CHAR16 *
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
{
  CHAR16  *Str;

  Str = UnicodeStr;

  while (TRUE) {

    *(UnicodeStr++) = (CHAR16) *AsciiStr;

    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}

CHAR8 *
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
{
  CHAR8 *Str;

  Str = AsciiStr;

  while (TRUE) {

    *AsciiStr = (CHAR8) *(UnicodeStr++);

    if (*(AsciiStr++) == '\0') {
      return Str;
    }
  }
}
