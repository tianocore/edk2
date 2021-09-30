/** @file

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ACPI_HELPER_LIB_H_
#define ACPI_HELPER_LIB_H_

/** Is a character upper case
*/
#define IS_UPPER_CHAR(x) ((x >= 'A') && (x <= 'Z'))

/** Is a character a decimal digit
*/
#define IS_DIGIT(x) ((x >= '0') && (x <= '9'))

/** Is a character an upper case hexadecimal digit
*/
#define IS_UPPER_HEX(x) (((x >= 'A') && (x <= 'F')) || IS_DIGIT (x))

/** Convert a hex number to its ASCII code.

 @param [in]  x   Hex number to convert.
                  Must be 0 <= x < 16.

 @return The ASCII code corresponding to x.
**/
UINT8
EFIAPI
AsciiFromHex (
  IN  UINT8   x
  );

/** Check if a HID is a valid PNP ID.

  @param     [in] Hid     The Hid to validate.

  @retval    TRUE         The Hid is a valid PNP ID.
  @retval    FALSE        The Hid is not a valid PNP ID.
**/
BOOLEAN
IsValidPnpId (
  IN  CONST CHAR8  * Hid
  );

/** Check if a HID is a valid ACPI ID.

  @param     [in] Hid     The Hid to validate.

  @retval    TRUE         The Hid is a valid ACPI ID.
  @retval    FALSE        The Hid is not a valid ACPI ID.
**/
BOOLEAN
IsValidAcpiId (
  IN  CONST CHAR8  * Hid
  );

#endif // ACPI_HELPER_LIB_H_
