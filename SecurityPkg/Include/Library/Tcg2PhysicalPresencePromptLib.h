/** @file -- Tcg2PhysicalPresencePromptLib.h
This library abstracts the action of prompting the user so that it may be overridden in a platform-specific way.
Rather than just printing to the screen.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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
  );

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
  );
