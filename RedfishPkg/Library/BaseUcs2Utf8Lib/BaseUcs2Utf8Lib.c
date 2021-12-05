/** @file
  UCS2 to UTF8 manipulation library.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseUcs2Utf8Lib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Since each UCS2 character can be represented by 1-3 UTF8 encoded characters,
  this function is used to retrieve the UTF8 encoding size for a UCS2 character.

  @param[in]   Utf8Buffer       The buffer for UTF8 encoded data.

  @retval      Return the size of UTF8 encoding string or 0 if it is not for
               UCS2 format.

**/
UINT8
GetUTF8SizeForUCS2 (
  IN    CHAR8  *Utf8Buffer
  )
{
  CHAR8  TempChar;
  UINT8  Utf8Size;

  ASSERT (Utf8Buffer != NULL);

  TempChar = *Utf8Buffer;
  if ((TempChar & 0xF0) == 0xF0) {
    //
    // This format is not for UCS2.
    //
    return 0;
  }

  Utf8Size = 1;
  if ((TempChar & 0x80) == 0x80) {
    if ((TempChar & 0xC0) == 0xC0) {
      Utf8Size++;
      if ((TempChar & 0xE0) == 0xE0) {
        Utf8Size++;
      }
    }
  }

  return Utf8Size;
}

/**
  Since each UCS2 character can be represented by the format: \uXXXX, this function
  is used to retrieve the UCS2 character from a Unicode format.
  Call MUST make sure there are at least 6 Bytes in the input UTF8 buffer.

  @param[in]    Utf8Buffer             The buffer for UTF8 encoded data.
  @param[out]   Ucs2Char               The converted UCS2 character.

  @retval       EFI_INVALID_PARAMETER  Non-Ascii characters found in the hexadecimal
                                       digits string, and can't be converted to a UCS2
                                       character.
  @retval       EFI_SUCCESS            The UCS2 character has been retrieved.

**/
EFI_STATUS
GetUCS2CharByFormat (
  IN    CHAR8   *Utf8Buffer,
  OUT   CHAR16  *Ucs2Char
  )
{
  UINT8  Num1;
  UINT8  Num2;
  UINT8  Index;
  CHAR8  Ucs2CharFormat[UNICODE_FORMAT_CHAR_SIZE];     /// two Hexadecimal digits Ascii string, like "3F"

  for (Index = 0; Index < 4; Index++) {
    if ((*(Utf8Buffer + 2 + Index) & 0x80) != 0x00) {
      return EFI_INVALID_PARAMETER;
    }
  }

  ZeroMem (Ucs2CharFormat, UNICODE_FORMAT_CHAR_SIZE);

  //
  // Get the First Number, Offset is 2
  //
  CopyMem (Ucs2CharFormat, Utf8Buffer + 2, UNICODE_FORMAT_CHAR_LEN);
  Num1 = (UINT8)AsciiStrHexToUintn (Ucs2CharFormat);

  //
  // Get the Second Number, Offset is 4
  //
  CopyMem (Ucs2CharFormat, Utf8Buffer + 4, UNICODE_FORMAT_CHAR_LEN);
  Num2 = (UINT8)AsciiStrHexToUintn (Ucs2CharFormat);

  //
  // Ucs2Char is Little-Endian
  //
  *((CHAR8 *)Ucs2Char)       = Num2;
  *(((CHAR8 *)Ucs2Char) + 1) = Num1;

  return EFI_SUCCESS;
}

/**
  Convert a UCS2 character to UTF8 encoding string.

  @param[in]    Ucs2Char               The provided UCS2 character.
  @param[out]   Utf8Buffer             The converted UTF8 encoded data.

  @retval      Return the size of UTF8 encoding data for this UCS2 character.

**/
UINT8
UCS2CharToUTF8 (
  IN  CHAR16  Ucs2Char,
  OUT CHAR8   *Utf8Buffer
  )
{
  UINT16  Ucs2Number;

  ASSERT (Utf8Buffer != NULL);

  Ucs2Number = (UINT16)Ucs2Char;
  if (Ucs2Number <= 0x007F) {
    //
    // UTF8 format: 0xxxxxxx
    //
    *Utf8Buffer = Ucs2Char & 0x7F;
    return 1;
  } else if ((Ucs2Number >= 0x0080) && (Ucs2Number <= 0x07FF)) {
    //
    // UTF8 format: 110xxxxx 10xxxxxx
    //
    *(Utf8Buffer + 1) = (Ucs2Char & 0x3F) | 0x80;
    *Utf8Buffer       = ((Ucs2Char >> 6) & 0x1F) | 0xC0;
    return 2;
  } else {
    /// Ucs2Number >= 0x0800 && Ucs2Number <= 0xFFFF

    //
    // UTF8 format: 1110xxxx 10xxxxxx 10xxxxxx
    //
    *(Utf8Buffer + 2) = (Ucs2Char & 0x3F) | 0x80;
    *(Utf8Buffer + 1) = ((Ucs2Char >> 6) & 0x3F) | 0x80;
    *Utf8Buffer       = ((Ucs2Char >> 12) & 0x0F) | 0xE0;
    return 3;
  }
}

/**
  Convert a UTF8 encoded data to a UCS2 character.

  @param[in]    Utf8Buffer             The provided UTF8 encoded data.
  @param[out]   Ucs2Char               The converted UCS2 character.

  @retval       EFI_INVALID_PARAMETER  The UTF8 encoded string is not valid or
                                       not for UCS2 character.
  @retval       EFI_SUCCESS            The converted UCS2 character.

**/
EFI_STATUS
UTF8ToUCS2Char (
  IN   CHAR8   *Utf8Buffer,
  OUT  CHAR16  *Ucs2Char
  )
{
  UINT8  Utf8Size;
  CHAR8  *Ucs2Buffer;
  CHAR8  TempChar1;
  CHAR8  TempChar2;
  CHAR8  TempChar3;

  ASSERT (Utf8Buffer != NULL && Ucs2Char != NULL);
  ZeroMem (Ucs2Char, sizeof (CHAR16));
  Ucs2Buffer = (CHAR8 *)Ucs2Char;

  Utf8Size = GetUTF8SizeForUCS2 (Utf8Buffer);
  switch (Utf8Size) {
    case 1:

      //
      // UTF8 format: 0xxxxxxx
      //
      TempChar1 = *Utf8Buffer;
      if ((TempChar1 & 0x80) != 0x00) {
        return EFI_INVALID_PARAMETER;
      }

      *Ucs2Buffer       = TempChar1;
      *(Ucs2Buffer + 1) = 0;
      break;

    case 2:

      //
      // UTF8 format: 110xxxxx 10xxxxxx
      //
      TempChar1 = *Utf8Buffer;
      if ((TempChar1 & 0xE0) != 0xC0) {
        return EFI_INVALID_PARAMETER;
      }

      TempChar2 = *(Utf8Buffer + 1);
      if ((TempChar2 & 0xC0) != 0x80) {
        return EFI_INVALID_PARAMETER;
      }

      *Ucs2Buffer       = (TempChar1 << 6) + (TempChar2 & 0x3F);
      *(Ucs2Buffer + 1) = (TempChar1 >> 2) & 0x07;
      break;

    case 3:

      //
      // UTF8 format: 1110xxxx 10xxxxxx 10xxxxxx
      //
      TempChar1 = *Utf8Buffer;
      if ((TempChar1 & 0xF0) != 0xE0) {
        return EFI_INVALID_PARAMETER;
      }

      TempChar2 = *(Utf8Buffer + 1);
      if ((TempChar2 & 0xC0) != 0x80) {
        return EFI_INVALID_PARAMETER;
      }

      TempChar3 = *(Utf8Buffer + 2);
      if ((TempChar3 & 0xC0) != 0x80) {
        return EFI_INVALID_PARAMETER;
      }

      *Ucs2Buffer       = (TempChar2 << 6) + (TempChar3 & 0x3F);
      *(Ucs2Buffer + 1) = (TempChar1 << 4) + ((TempChar2 >> 2) & 0x0F);

      break;

    default:

      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/**
  Convert a UCS2 string to a UTF8 encoded string.

  @param[in]    Ucs2Str                The provided UCS2 string.
  @param[out]   Utf8StrAddr            The converted UTF8 string address. Caller
                                       is responsible for Free this string.

  @retval       EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval       EFI_OUT_OF_RESOURCES   System runs out of resources.
  @retval       EFI_SUCCESS            The UTF8 encoded string has been converted.

**/
EFI_STATUS
UCS2StrToUTF8 (
  IN  CHAR16  *Ucs2Str,
  OUT CHAR8   **Utf8StrAddr
  )
{
  UINTN  Ucs2StrIndex;
  UINTN  Ucs2StrLength;
  CHAR8  *Utf8Str;
  UINTN  Utf8StrLength;
  UINTN  Utf8StrIndex;
  CHAR8  Utf8Buffer[UTF8_BUFFER_FOR_UCS2_MAX_SIZE];
  UINT8  Utf8BufferSize;

  if ((Ucs2Str == NULL) || (Utf8StrAddr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Ucs2StrLength = StrLen (Ucs2Str);
  Utf8StrLength = 0;

  for (Ucs2StrIndex = 0; Ucs2StrIndex < Ucs2StrLength; Ucs2StrIndex++) {
    ZeroMem (Utf8Buffer, sizeof (Utf8Buffer));
    Utf8BufferSize = UCS2CharToUTF8 (Ucs2Str[Ucs2StrIndex], Utf8Buffer);
    Utf8StrLength += Utf8BufferSize;
  }

  Utf8Str = AllocateZeroPool (Utf8StrLength + 1);
  if (Utf8Str == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Utf8StrIndex = 0;
  for (Ucs2StrIndex = 0; Ucs2StrIndex < Ucs2StrLength; Ucs2StrIndex++) {
    ZeroMem (Utf8Buffer, sizeof (Utf8Buffer));
    Utf8BufferSize = UCS2CharToUTF8 (Ucs2Str[Ucs2StrIndex], Utf8Buffer);

    CopyMem (Utf8Str + Utf8StrIndex, Utf8Buffer, Utf8BufferSize);
    Utf8StrIndex += Utf8BufferSize;
  }

  Utf8Str[Utf8StrIndex] = '\0';
  *Utf8StrAddr          = Utf8Str;

  return EFI_SUCCESS;
}

/**
  Convert a UTF8 encoded string to a UCS2 string.

  @param[in]    Utf8Str                The provided UTF8 encoded string.
  @param[out]   Ucs2StrAddr            The converted UCS2 string address. Caller
                                       is responsible for Free this string.

  @retval       EFI_INVALID_PARAMETER  The UTF8 encoded string is not valid to
                                       convert to UCS2 string.
                                       One or more parameters are invalid.
  @retval       EFI_OUT_OF_RESOURCES   System runs out of resources.
  @retval       EFI_SUCCESS            The UCS2 string has been converted.

**/
EFI_STATUS
UTF8StrToUCS2 (
  IN  CHAR8   *Utf8Str,
  OUT CHAR16  **Ucs2StrAddr
  )
{
  EFI_STATUS  Status;
  UINTN       Utf8StrIndex;
  UINTN       Utf8StrLength;
  UINTN       Ucs2StrIndex;
  UINT8       Utf8BufferSize;
  CHAR16      *Ucs2StrTemp;

  if ((Utf8Str == NULL) || (Ucs2StrAddr == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // It is not an Ascii string, calculate string length.
  //
  Utf8StrLength = 0;
  while (*(Utf8Str + Utf8StrLength) != '\0') {
    Utf8StrLength++;
  }

  //
  // UCS2 string shall not be longer than the UTF8 string.
  //
  Ucs2StrTemp = AllocateZeroPool ((Utf8StrLength + 1) * sizeof (CHAR16));
  if (Ucs2StrTemp == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Utf8StrIndex = 0;
  Ucs2StrIndex = 0;
  while (Utf8Str[Utf8StrIndex] != '\0') {
    if ((CompareMem (Utf8Str + Utf8StrIndex, "\\u", 2) == 0) &&
        (Utf8StrLength - Utf8StrIndex >= UNICODE_FORMAT_LEN))
    {
      Status = GetUCS2CharByFormat (Utf8Str + Utf8StrIndex, Ucs2StrTemp + Ucs2StrIndex);
      if (!EFI_ERROR (Status)) {
        Utf8StrIndex += UNICODE_FORMAT_LEN;
        Ucs2StrIndex++;
      } else {
        StrCpyS (Ucs2StrTemp + Ucs2StrIndex, 3, L"\\u");

        Ucs2StrIndex += 2;
        Utf8StrIndex += 2;
      }
    } else {
      Utf8BufferSize = GetUTF8SizeForUCS2 (Utf8Str + Utf8StrIndex);
      if ((Utf8BufferSize == 0) || (Utf8StrLength - Utf8StrIndex < Utf8BufferSize)) {
        FreePool (Ucs2StrTemp);
        return EFI_INVALID_PARAMETER;
      }

      Status = UTF8ToUCS2Char (Utf8Str + Utf8StrIndex, Ucs2StrTemp + Ucs2StrIndex);
      if (EFI_ERROR (Status)) {
        FreePool (Ucs2StrTemp);
        return EFI_INVALID_PARAMETER;
      }

      Ucs2StrIndex++;
      Utf8StrIndex += Utf8BufferSize;
    }
  }

  *Ucs2StrAddr = AllocateZeroPool ((Ucs2StrIndex + 1) * sizeof (CHAR16));
  if (*Ucs2StrAddr == NULL) {
    FreePool (Ucs2StrTemp);
    return EFI_OUT_OF_RESOURCES;
  }

  StrCpyS (*Ucs2StrAddr, Ucs2StrIndex + 1, Ucs2StrTemp);
  *(*Ucs2StrAddr + Ucs2StrIndex) = L'\0';
  FreePool (Ucs2StrTemp);

  return EFI_SUCCESS;
}
