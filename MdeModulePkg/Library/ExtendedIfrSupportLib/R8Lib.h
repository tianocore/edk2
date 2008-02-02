/**@file
  Copyright (c) 2007, Intel Corporation

  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/



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
;




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
;

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
;

