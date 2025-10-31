/** @file
  Main file for Pause shell level 3 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-q", TypeFlag },
  { NULL,  TypeMax  }
};

/**
  Function for 'pause' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunPause (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  LIST_ENTRY             *Package;
  CHAR16                 *ProblemParam;
  SHELL_STATUS           ShellStatus;
  SHELL_PROMPT_RESPONSE  *Resp;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;
  Resp         = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  if (!gEfiShellProtocol->BatchIsActive ()) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_NO_SCRIPT), gShellLevel3HiiHandle, L"pause");
    return (SHELL_UNSUPPORTED);
  }

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"pause", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag (Package, L"-?")) {
      ASSERT (FALSE);
    } else if (ShellCommandLineGetRawValue (Package, 1) != NULL) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"pause");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (!ShellCommandLineGetFlag (Package, L"-q")) {
        Status = ShellPromptForResponseHii (ShellPromptResponseTypeQuitContinue, STRING_TOKEN (STR_PAUSE_PROMPT), gShellLevel3HiiHandle, (VOID **)&Resp);
      } else {
        Status = ShellPromptForResponse (ShellPromptResponseTypeQuitContinue, NULL, (VOID **)&Resp);
      }

      if (EFI_ERROR (Status) || (Resp == NULL) || (*Resp == ShellPromptResponseQuit)) {
        ShellCommandRegisterExit (TRUE, 0);
        ShellStatus = SHELL_ABORTED;
      }

      if (Resp != NULL) {
        FreePool (Resp);
      }
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
