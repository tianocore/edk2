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
  IN  UINT8   Hex
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

/** Check if a HID is a valid PNP ID.

  @param     [in] Hid     The Hid to validate.

  @retval    TRUE         The Hid is a valid PNP ID.
  @retval    FALSE        The Hid is not a valid PNP ID.
**/
BOOLEAN
IsValidPnpId (
  IN  CONST CHAR8  * Hid
  )
{
  UINTN Index;

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
  IN  CONST CHAR8  * Hid
  )
{
  UINTN Index;

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
