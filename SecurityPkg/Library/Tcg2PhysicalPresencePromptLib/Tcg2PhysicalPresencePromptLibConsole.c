/** @file -- Tcg2PhysicalPresencePromptLibConsole.c
This instance of the Tcg2PhysicalPresencePromptLib uses the console and basic key input
to prompt the user.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>

/**
  Read the specified key for user confirmation.

  @param[in]  CautionKey  If true,  F12 is used as confirm key;
                          If false, F10 is used as confirm key.

  @retval     TRUE        User confirmed the changes by input.
  @retval     FALSE       User discarded the changes.
**/
BOOLEAN
Tcg2ReadUserKey (
  IN     BOOLEAN  CautionKey
  )
{
  EFI_STATUS     Status;
  EFI_INPUT_KEY  Key;
  UINT16         InputKey;

  InputKey = 0;
  do {
    Status = gBS->CheckEvent (gST->ConIn->WaitForKey);
    if (!EFI_ERROR (Status)) {
      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      if (Key.ScanCode == SCAN_ESC) {
        InputKey = Key.ScanCode;
      }

      if ((Key.ScanCode == SCAN_F10) && !CautionKey) {
        InputKey = Key.ScanCode;
      }

      if ((Key.ScanCode == SCAN_F12) && CautionKey) {
        InputKey = Key.ScanCode;
      }
    }
  } while (InputKey == 0);

  if (InputKey != SCAN_ESC) {
    return TRUE;
  }

  return FALSE;
}

/**
  This function will take in a prompt string to present to the user in a
  OK/Cancel dialog box and return TRUE if the user actively pressed OK. Returns
  FALSE on Cancel or any errors.

  @param[in]  PromptString  The string that should occupy the body of the prompt.

  @retval     TRUE    User confirmed action.
  @retval     FALSE   User rejected action or a failure occurred.

**/
BOOLEAN
EFIAPI
PromptForUserConfirmation (
  IN  CHAR16  *PromptString
  )
{
  UINTN   Index;
  CHAR16  DstStr[81];

  DstStr[80] = L'\0';
  for (Index = 0; Index < StrLen (PromptString); Index += 80) {
    StrnCpyS (DstStr, sizeof (DstStr) / sizeof (CHAR16), PromptString + Index, sizeof (DstStr) / sizeof (CHAR16) - 1);
    Print (DstStr);
  }

  // if (Tcg2ReadUserKey (CautionKey)) {
  if (Tcg2ReadUserKey (FALSE)) {
    return TRUE;
  }

  return FALSE;
} // PromptForUserConfirmation()
