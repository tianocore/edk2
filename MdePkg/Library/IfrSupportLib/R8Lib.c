/**@file
  Copyright (c) 2007, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "UefiIfrLibraryInternal.h"


CHAR16
InternalNibbleToHexChar (
  IN UINT8      Nibble
  )
/*++

  Routine Description:
    Converts the low nibble of a byte  to hex unicode character.

  Arguments:
    Nibble - lower nibble of a byte.

  Returns:
    Hex unicode character.

--*/
{
  Nibble &= 0x0F;
  if (Nibble <= 0x9) {
    return (CHAR16)(Nibble + L'0');
  }

  return (CHAR16)(Nibble - 0xA + L'A');
}


/**
  Converts binary buffer to Unicode string.
  At a minimum, any blob of data could be represented as a hex string.

  @param  Str                    Pointer to the string.
  @param  HexStringBufferLength  Length in bytes of buffer to hold the hex string.
                                 Includes tailing '\0' character. If routine return
                                 with EFI_SUCCESS, containing length of hex string
                                 buffer. If routine return with
                                 EFI_BUFFER_TOO_SMALL, containg length of hex
                                 string buffer desired.
  @param  Buf                    Buffer to be converted from.
  @param  Len                    Length in bytes of the buffer to be converted.

  @retval EFI_SUCCESS            Routine success.
  @retval EFI_BUFFER_TOO_SMALL   The hex string buffer is too small.

**/
EFI_STATUS
R8_BufToHexString (
  IN OUT CHAR16                    *Str,
  IN OUT UINTN                     *HexStringBufferLength,
  IN     UINT8                     *Buf,
  IN     UINTN                      Len
  )
{
  //
  // Porting Guide:
  // This library interface is simply obsolete.
  // Include the source code to user code.
  //
  UINTN       Idx;
  UINT8       Byte;
  UINTN       StrLen;

  //
  // Make sure string is either passed or allocate enough.
  // It takes 2 Unicode characters (4 bytes) to represent 1 byte of the binary buffer.
  // Plus the Unicode termination character.
  //
  StrLen = Len * 2;
  if (StrLen > ((*HexStringBufferLength) - 1)) {
    *HexStringBufferLength = StrLen + 1;
    return EFI_BUFFER_TOO_SMALL;
  }

  *HexStringBufferLength = StrLen + 1;
  //
  // Ends the string.
  //
  Str[StrLen] = L'\0';

  for (Idx = 0; Idx < Len; Idx++) {

    Byte = Buf[Idx];
    Str[StrLen - 1 - Idx * 2] = InternalNibbleToHexChar (Byte);
    Str[StrLen - 2 - Idx * 2] = InternalNibbleToHexChar ((UINT8)(Byte >> 4));
  }

  return EFI_SUCCESS;
}




/**
  Converts Unicode string to binary buffer.
  The conversion may be partial.
  The first character in the string that is not hex digit stops the conversion.
  At a minimum, any blob of data could be represented as a hex string.

  @param  Buf                    Pointer to buffer that receives the data.
  @param  Len                    Length in bytes of the buffer to hold converted
                                 data. If routine return with EFI_SUCCESS,
                                 containing length of converted data. If routine
                                 return with EFI_BUFFER_TOO_SMALL, containg length
                                 of buffer desired.
  @param  Str                    String to be converted from.
  @param  ConvertedStrLen        Length of the Hex String consumed.

  @retval EFI_SUCCESS            Routine Success.
  @retval EFI_BUFFER_TOO_SMALL   The buffer is too small to hold converted data.

**/
EFI_STATUS
R8_HexStringToBuf (
  IN OUT UINT8                     *Buf,
  IN OUT UINTN                    *Len,
  IN     CHAR16                    *Str,
  OUT    UINTN                     *ConvertedStrLen  OPTIONAL
  )
{
  //
  // Porting Guide:
  // This library interface is simply obsolete.
  // Include the source code to user code.
  //

  UINTN       HexCnt;
  UINTN       Idx;
  UINTN       BufferLength;
  UINT8       Digit;
  UINT8       Byte;

  //
  // Find out how many hex characters the string has.
  //
  for (Idx = 0, HexCnt = 0; R8_IsHexDigit (&Digit, Str[Idx]); Idx++, HexCnt++);

  if (HexCnt == 0) {
    *Len = 0;
    return EFI_SUCCESS;
  }
  //
  // Two Unicode characters make up 1 buffer byte. Round up.
  //
  BufferLength = (HexCnt + 1) / 2;

  //
  // Test if  buffer is passed enough.
  //
  if (BufferLength > (*Len)) {
    *Len = BufferLength;
    return EFI_BUFFER_TOO_SMALL;
  }

  *Len = BufferLength;

  for (Idx = 0; Idx < HexCnt; Idx++) {

    R8_IsHexDigit (&Digit, Str[HexCnt - 1 - Idx]);

    //
    // For odd charaters, write the lower nibble for each buffer byte,
    // and for even characters, the upper nibble.
    //
    if ((Idx & 1) == 0) {
      Byte = Digit;
    } else {
      Byte = Buf[Idx / 2];
      Byte &= 0x0F;
      Byte = (UINT8) (Byte | Digit << 4);
    }

    Buf[Idx / 2] = Byte;
  }

  if (ConvertedStrLen != NULL) {
    *ConvertedStrLen = HexCnt;
  }

  return EFI_SUCCESS;
}


/**
  Determines if a Unicode character is a hexadecimal digit.
  The test is case insensitive.

  @param  Digit                  Pointer to byte that receives the value of the hex
                                 character.
  @param  Char                   Unicode character to test.

  @retval TRUE                   If the character is a hexadecimal digit.
  @retval FALSE                  Otherwise.

**/
BOOLEAN
R8_IsHexDigit (
  OUT UINT8      *Digit,
  IN  CHAR16      Char
  )
{
  //
  // Porting Guide:
  // This library interface is simply obsolete.
  // Include the source code to user code.
  //

  if ((Char >= L'0') && (Char <= L'9')) {
    *Digit = (UINT8) (Char - L'0');
    return TRUE;
  }

  if ((Char >= L'A') && (Char <= L'F')) {
    *Digit = (UINT8) (Char - L'A' + 0x0A);
    return TRUE;
  }

  if ((Char >= L'a') && (Char <= L'f')) {
    *Digit = (UINT8) (Char - L'a' + 0x0A);
    return TRUE;
  }

  return FALSE;
}


