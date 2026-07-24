/** @file

  This instance of the Tcg2PhysicalPresencePromptLib uses the
  console and basic key input to prompt the user.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/Tcg2PhysicalPresencePromptLib.h>

/**
  Simple function to inform any callers of whether the lib is ready to present a prompt.
  Since the prompt itself only returns TRUE or FALSE, make sure all other technical requirements
  are out of the way.

  @retval     EFI_SUCCESS       Prompt is ready.
  @retval     EFI_NOT_READY     Prompt is not ready.
  @retval     EFI_DEVICE_ERROR  Library failed to prepare resources.

**/
EFI_STATUS
EFIAPI
Tcg2IsPromptReady (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Read the specified key for user confirmation as specified by
  TCG PC Client Platform Physical Presence Interface Specification
  version 1.3 Revision 00.52.

  @param[in]  CautionKey  If true, F12 is used as confirm key.
                          If false, F10 is used as confirm key.

  @retval     TRUE        User confirmed the changes by input.
  @retval     FALSE       User discarded the changes.
**/
STATIC
BOOLEAN
ReadUserKey (
  IN     BOOLEAN  CautionKey
  )
{
  EFI_STATUS     Status;
  EFI_INPUT_KEY  Key;

  while (TRUE) {
    Status = gBS->CheckEvent (gST->ConIn->WaitForKey);
    if (EFI_ERROR (Status)) {
      continue;
    }

    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (Key.ScanCode == SCAN_ESC) {
      return FALSE;
    }

    if ((Key.ScanCode == SCAN_F10) && !CautionKey) {
      return TRUE;
    }

    if ((Key.ScanCode == SCAN_F12) && CautionKey) {
      return TRUE;
    }
  }
}

/**
  Presents the given prompt string to the user and returns whether the user
  confirmed the requested action.

  @param[in]  PromptString  The string that should occupy the body of the prompt.
  @param[in]  CautionKey    If TRUE, the caller has instructed the user to press
                            the CAUTION key to confirm.
                            If FALSE, the caller has instructed the user to press
                            the ACCEPT key to confirm.

  @retval     TRUE    User confirmed the action.
  @retval     FALSE   User rejected the action or a failure occurred.

**/
BOOLEAN
EFIAPI
Tcg2PromptForUserConfirmation (
  IN  CHAR16   *PromptString,
  IN  BOOLEAN  CautionKey
  )
{
  UINTN   Index;
  CHAR16  DstStr[81];

  DstStr[80] = L'\0';
  for (Index = 0; Index < StrLen (PromptString); Index += 80) {
    StrnCpyS (DstStr, sizeof (DstStr) / sizeof (CHAR16), PromptString + Index, sizeof (DstStr) / sizeof (CHAR16) - 1);
    Print (DstStr);
  }

  if (ReadUserKey (CautionKey)) {
    return TRUE;
  }

  return FALSE;
}
