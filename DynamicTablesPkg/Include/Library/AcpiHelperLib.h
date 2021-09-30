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

 @param [in]  Hex   Hex number to convert.
                    Must be 0 <= x < 16.

 @return The ASCII code corresponding to x.
         -1 if error.
**/
UINT8
EFIAPI
AsciiFromHex (
  IN  UINT8   Hex
  );

/** Convert an ASCII char representing an hexadecimal number
    to its integer value.

 @param [in]  Char  Char to convert.
                    Must be between '0'-'9' or 'A'-'F' or 'a'-'f'.

 @return The corresponding integer (between 0-16).
         -1 if error.
**/
UINT8
EFIAPI
HexFromAscii (
  IN  CHAR8   Char
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

/** Convert a EisaId string to its compressed UINT32 equivalent.

  Cf. ACPI 6.4 specification, s19.3.4 "ASL Macros": "Eisaid"

  @param  [in]  EisaIdStr   Input EisaId string.
  @param  [out] EisaIdInt   Output EisaId UINT32 (compressed).

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlGetEisaIdFromString (
  IN  CONST CHAR8   * EisaIdStr,
  OUT       UINT32  * EisaIdInt
  );

#endif // ACPI_HELPER_LIB_H_
