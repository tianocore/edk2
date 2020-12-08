/** @file
  UCS2 to UTF8 manipulation library header file.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef BASE_UCS2UTF8_LIB_H_
#define BASE_UCS2UTF8_LIB_H_

///
///  L"\u0000"
///
#define UNICODE_FORMAT_LEN         6
#define UNICODE_FORMAT_CHAR_LEN    2
#define UNICODE_FORMAT_CHAR_SIZE   3

#define UTF8_BUFFER_FOR_UCS2_MAX_SIZE   3

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
  IN  CHAR16     *Ucs2Str,
  OUT CHAR8      **Utf8StrAddr
  );

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
  IN  CHAR8      *Utf8Str,
  OUT CHAR16     **Ucs2StrAddr
  );

#endif
