/** @file
  Acpi Helper

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <Library/AcpiHelperLib.h>

/** Convert a hex number to its ASCII code.

 @param [in]  Hex   Hex number to convert.
                    Must be 0 <= x < 16.

 @return The ASCII code corresponding to x.
         -1 if error.
**/
UINT8
EFIAPI
AsciiFromHex (
  IN  UINT8  Hex
  )
{
  if (Hex < 10) {
    return (UINT8)(Hex + '0');
  }

  if (Hex < 16) {
    return (UINT8)(Hex - 10 + 'A');
  }

  ASSERT (FALSE);
  return (UINT8)-1;
}

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
  IN  CHAR8  Char
  )
{
  if ((Char >= '0') && (Char <= '9')) {
    return (UINT8)(Char - '0');
  }

  if ((Char >= 'A') && (Char <= 'F')) {
    return (UINT8)(Char - 'A' + 10);
  }

  if ((Char >= 'a') && (Char <= 'f')) {
    return (UINT8)(Char - 'a' + 10);
  }

  ASSERT (FALSE);
  return (UINT8)-1;
}

/** Check if a HID is a valid PNP ID.

  @param     [in] Hid     The Hid to validate.

  @retval    TRUE         The Hid is a valid PNP ID.
  @retval    FALSE        The Hid is not a valid PNP ID.
**/
BOOLEAN
IsValidPnpId (
  IN  CONST CHAR8  *Hid
  )
{
  UINTN  Index;

  if (AsciiStrLen (Hid) != 7) {
    return FALSE;
  }

  // A valid PNP ID must be of the form "AAA####"
  // where A is an uppercase letter and # is a hex digit.
  for (Index = 0; Index < 3; Index++) {
    if (!IS_UPPER_CHAR (Hid[Index])) {
      return FALSE;
    }
  }

  for (Index = 3; Index < 7; Index++) {
    if (!IS_UPPER_HEX (Hid[Index])) {
      return FALSE;
    }
  }

  return TRUE;
}

/** Check if a HID is a valid ACPI ID.

  @param     [in] Hid     The Hid to validate.

  @retval    TRUE         The Hid is a valid ACPI ID.
  @retval    FALSE        The Hid is not a valid ACPI ID.
**/
BOOLEAN
IsValidAcpiId (
  IN  CONST CHAR8  *Hid
  )
{
  UINTN  Index;

  if (AsciiStrLen (Hid) != 8) {
    return FALSE;
  }

  // A valid ACPI ID must be of the form "NNNN####"
  // where N is an uppercase letter or a digit ('0'-'9')
  // and # is a hex digit.
  for (Index = 0; Index < 4; Index++) {
    if (!(IS_UPPER_CHAR (Hid[Index]) || IS_DIGIT (Hid[Index]))) {
      return FALSE;
    }
  }

  for (Index = 4; Index < 8; Index++) {
    if (!IS_UPPER_HEX (Hid[Index])) {
      return FALSE;
    }
  }

  return TRUE;
}

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
  IN  CONST CHAR8   *EisaIdStr,
  OUT       UINT32  *EisaIdInt
  )
{
  if ((EisaIdStr == NULL)         ||
      (!IsValidPnpId (EisaIdStr)) ||
      (EisaIdInt == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  /* Cf. ACPI 6.4 specification, s19.3.4 "ASL Macros": "Eisaid"

  Converts and compresses the 7-character text argument into its corresponding
  4-byte numeric EISA ID encoding (Integer). This can be used when declaring
  IDs for devices that are EISA IDs.

  The algorithm used to convert the TextID is as shown in the following
  example:
    Starting with a seven character input string "PNP0303", we want to create
    a DWordConst. This string contains a three character manufacturer code
    "PNP", a three character hex product identifier "030", and a one character
    revision identifier "3".
    The compressed manufacturer code is created as follows:
      1) Find hex ASCII value for each letter
      2) Subtract 40h from each ASCII value
      3) Retain 5 least significant bits for each letter and discard remaining
         0's:

      Byte 0:
        Bit 7: reserved (0)
        Bit 6-2: 1st character of compressed mfg code "P"
        Bit 1-0: Upper 2 bits of 2nd character of mfg code "N"
      Byte 1:
        Bit 7-5: Lower 3 bits of 2nd character of mfg code "N"
        Bit 4-0: 3rd character of mfg code "P"
      Byte 2:
        Bit 7-4: 1st hex digit of product number "0"
        Bit 3-0: 2nd hex digit of product number "3"
      Byte 3:
        Bit 7-4: 3rd hex digit of product number "0"
        Bit 3-0: 4th hex digit of product number "3"
  */
  *EisaIdInt = SwapBytes32 (
                 ((EisaIdStr[0] - 0x40) << 26)       |
                 ((EisaIdStr[1] - 0x40) << 21)       |
                 ((EisaIdStr[2] - 0x40) << 16)       |
                 (HexFromAscii (EisaIdStr[3]) << 12) |
                 (HexFromAscii (EisaIdStr[4]) << 8)  |
                 (HexFromAscii (EisaIdStr[5]) << 4)  |
                 (HexFromAscii (EisaIdStr[6]))
                 );

  return EFI_SUCCESS;
}
