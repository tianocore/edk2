/** @file
  Implementation of translation upon VT-UTF8.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Terminal.h"

/**
  Translate all VT-UTF8 characters in the Raw FIFI into unicode characters,
  and insert them into Unicode FIFO.

  @param TerminalDevice          The terminal device.

**/
VOID
VTUTF8RawDataToUnicode (
  IN  TERMINAL_DEV    *TerminalDevice
  )
{
  UTF8_CHAR Utf8Char;
  UINT8     ValidBytes;
  UINT16    UnicodeChar;

  ValidBytes = 0;
  //
  // pop the raw data out from the raw fifo,
  // and translate it into unicode, then push
  // the unicode into unicode fifo, until the raw fifo is empty.
  //
  while (!IsRawFiFoEmpty (TerminalDevice) && !IsUnicodeFiFoFull (TerminalDevice)) {

    GetOneValidUtf8Char (TerminalDevice, &Utf8Char, &ValidBytes);

    if (ValidBytes < 1 || ValidBytes > 3) {
      continue;
    }

    Utf8ToUnicode (Utf8Char, ValidBytes, (CHAR16 *) &UnicodeChar);

    UnicodeFiFoInsertOneKey (TerminalDevice, UnicodeChar);
  }
}

/**
  Get one valid VT-UTF8 characters set from Raw Data FIFO.

  @param  Utf8Device          The terminal device.
  @param  Utf8Char            Returned valid VT-UTF8 characters set.
  @param  ValidBytes          The count of returned VT-VTF8 characters.
                              If ValidBytes is zero, no valid VT-UTF8 returned.

**/
VOID
GetOneValidUtf8Char (
  IN  TERMINAL_DEV      *Utf8Device,
  OUT UTF8_CHAR         *Utf8Char,
  OUT UINT8             *ValidBytes
  )
{
  UINT8   Temp;
  UINT8   Index;
  BOOLEAN FetchFlag;

  Temp      = 0;
  Index     = 0;
  FetchFlag = TRUE;

  //
  // if no valid Utf8 char is found in the RawFiFo,
  // then *ValidBytes will be zero.
  //
  *ValidBytes = 0;

  while (!IsRawFiFoEmpty (Utf8Device)) {

    RawFiFoRemoveOneKey (Utf8Device, &Temp);

    switch (*ValidBytes) {

    case 0:
      if ((Temp & 0x80) == 0) {
        //
        // one-byte utf8 char
        //
        *ValidBytes       = 1;

        Utf8Char->Utf8_1  = Temp;

        FetchFlag         = FALSE;

      } else if ((Temp & 0xe0) == 0xc0) {
        //
        // two-byte utf8 char
        //
        *ValidBytes         = 2;

        Utf8Char->Utf8_2[1] = Temp;

      } else if ((Temp & 0xf0) == 0xe0) {
        //
        // three-byte utf8 char
        //
        *ValidBytes         = 3;

        Utf8Char->Utf8_3[2] = Temp;

        Index++;

      } else {
        //
        // reset *ValidBytes to zero, let valid utf8 char search restart
        //
        *ValidBytes = 0;
      }

      break;

    case 2:
      //
      // two-byte utf8 char go on
      //
      if ((Temp & 0xc0) == 0x80) {

        Utf8Char->Utf8_2[0] = Temp;

        FetchFlag           = FALSE;

      } else {

        *ValidBytes = 0;
      }
      break;

    case 3:
      //
      // three-byte utf8 char go on
      //
      if ((Temp & 0xc0) == 0x80) {
        if (Index == 1) {
          Utf8Char->Utf8_3[1] = Temp;
          Index++;
        } else {
          Utf8Char->Utf8_3[0] = Temp;
          FetchFlag = FALSE;
        }
      } else {
        //
        // reset *ValidBytes and Index to zero, let valid utf8 char search restart
        //
        *ValidBytes = 0;
        Index       = 0;
      }
      break;

    default:
      break;
    }

    if (!FetchFlag) {
      break;
    }
  }

  return ;
}

/**
  Translate VT-UTF8 characters into one Unicode character.

  UTF8 Encoding Table
  Bits per Character | Unicode Character Range | Unicode Binary  Encoding |  UTF8 Binary Encoding
        0-7           |     0x0000 - 0x007F     |     00000000 0xxxxxxx    |   0xxxxxxx
        8-11          |     0x0080 - 0x07FF     |     00000xxx xxxxxxxx     |   110xxxxx 10xxxxxx
       12-16         |     0x0800 - 0xFFFF     |     xxxxxxxx xxxxxxxx    |   1110xxxx 10xxxxxx 10xxxxxx


  @param  Utf8Char         VT-UTF8 character set needs translating.
  @param  ValidBytes       The count of valid VT-UTF8 characters.
  @param  UnicodeChar      Returned unicode character.

**/
VOID
Utf8ToUnicode (
  IN  UTF8_CHAR       Utf8Char,
  IN  UINT8           ValidBytes,
  OUT CHAR16          *UnicodeChar
  )
{
  UINT8 UnicodeByte0;
  UINT8 UnicodeByte1;
  UINT8 Byte0;
  UINT8 Byte1;
  UINT8 Byte2;

  *UnicodeChar = 0;

  //
  // translate utf8 code to unicode, in terminal standard,
  // up to 3 bytes utf8 code is supported.
  //
  switch (ValidBytes) {
  case 1:
    //
    // one-byte utf8 code
    //
    *UnicodeChar = (UINT16) Utf8Char.Utf8_1;
    break;

  case 2:
    //
    // two-byte utf8 code
    //
    Byte0         = Utf8Char.Utf8_2[0];
    Byte1         = Utf8Char.Utf8_2[1];

    UnicodeByte0  = (UINT8) ((Byte1 << 6) | (Byte0 & 0x3f));
    UnicodeByte1  = (UINT8) ((Byte1 >> 2) & 0x07);
    *UnicodeChar  = (UINT16) (UnicodeByte0 | (UnicodeByte1 << 8));
    break;

  case 3:
    //
    // three-byte utf8 code
    //
    Byte0         = Utf8Char.Utf8_3[0];
    Byte1         = Utf8Char.Utf8_3[1];
    Byte2         = Utf8Char.Utf8_3[2];

    UnicodeByte0  = (UINT8) ((Byte1 << 6) | (Byte0 & 0x3f));
    UnicodeByte1  = (UINT8) ((Byte2 << 4) | ((Byte1 >> 2) & 0x0f));
    *UnicodeChar  = (UINT16) (UnicodeByte0 | (UnicodeByte1 << 8));

  default:
    break;
  }

  return ;
}

/**
  Translate one Unicode character into VT-UTF8 characters.

  UTF8 Encoding Table
  Bits per Character | Unicode Character Range | Unicode Binary  Encoding |  UTF8 Binary Encoding
        0-7           |     0x0000 - 0x007F     |     00000000 0xxxxxxx    |   0xxxxxxx
        8-11          |     0x0080 - 0x07FF     |     00000xxx xxxxxxxx     |   110xxxxx 10xxxxxx
       12-16         |     0x0800 - 0xFFFF     |     xxxxxxxx xxxxxxxx    |   1110xxxx 10xxxxxx 10xxxxxx


  @param  Unicode          Unicode character need translating.
  @param  Utf8Char         Return VT-UTF8 character set.
  @param  ValidBytes       The count of valid VT-UTF8 characters. If
                           ValidBytes is zero, no valid VT-UTF8 returned.

**/
VOID
UnicodeToUtf8 (
  IN  CHAR16      Unicode,
  OUT UTF8_CHAR   *Utf8Char,
  OUT UINT8       *ValidBytes
  )
{
  UINT8 UnicodeByte0;
  UINT8 UnicodeByte1;
  //
  // translate unicode to utf8 code
  //
  UnicodeByte0  = (UINT8) Unicode;
  UnicodeByte1  = (UINT8) (Unicode >> 8);

  if (Unicode < 0x0080) {

    Utf8Char->Utf8_1  = (UINT8) (UnicodeByte0 & 0x7f);
    *ValidBytes       = 1;

  } else if (Unicode < 0x0800) {
    //
    // byte sequence: high -> low
    //                Utf8_2[0], Utf8_2[1]
    //
    Utf8Char->Utf8_2[1] = (UINT8) ((UnicodeByte0 & 0x3f) + 0x80);
    Utf8Char->Utf8_2[0] = (UINT8) ((((UnicodeByte1 << 2) + (UnicodeByte0 >> 6)) & 0x1f) + 0xc0);

    *ValidBytes         = 2;

  } else {
    //
    // byte sequence: high -> low
    //                Utf8_3[0], Utf8_3[1], Utf8_3[2]
    //
    Utf8Char->Utf8_3[2] = (UINT8) ((UnicodeByte0 & 0x3f) + 0x80);
    Utf8Char->Utf8_3[1] = (UINT8) ((((UnicodeByte1 << 2) + (UnicodeByte0 >> 6)) & 0x3f) + 0x80);
    Utf8Char->Utf8_3[0] = (UINT8) (((UnicodeByte1 >> 4) & 0x0f) + 0xe0);

    *ValidBytes         = 3;
  }
}


/**
  Check if input string is valid VT-UTF8 string.

  @param  TerminalDevice          The terminal device.
  @param  WString                 The input string.

  @retval EFI_SUCCESS             If all input characters are valid.

**/
EFI_STATUS
VTUTF8TestString (
  IN  TERMINAL_DEV    *TerminalDevice,
  IN  CHAR16          *WString
  )
{
  //
  // to utf8, all kind of characters are supported.
  //
  return EFI_SUCCESS;
}
