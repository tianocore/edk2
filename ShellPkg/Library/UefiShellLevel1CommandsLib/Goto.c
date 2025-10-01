/** @file
  Main file for goto shell level 1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel1CommandsLib.h"

/**
  Function for 'goto' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunGoto (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CHAR16        *CompareString;
  UINTN         Size;
  SCRIPT_FILE   *CurrentScriptFile;

  ShellStatus   = SHELL_SUCCESS;
  CompareString = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  if (!gEfiShellProtocol->BatchIsActive ()) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"Goto");
    return (SHELL_UNSUPPORTED);
  }

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel1HiiHandle, L"goto", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetRawValue (Package, 2) != NULL) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle, L"goto");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle, L"goto");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Size = 0;
      ASSERT ((CompareString == NULL && Size == 0) || (CompareString != NULL));
      CompareString = StrnCatGrow (&CompareString, &Size, L":", 0);
      CompareString = StrnCatGrow (&CompareString, &Size, ShellCommandLineGetRawValue (Package, 1), 0);
      if (CompareString == NULL) {
        ShellCommandLineFreeVarList (Package);
        return SHELL_OUT_OF_RESOURCES;
      }

      //
      // Check forwards and then backwards for a label...
      //
      if (!MoveToTag (GetNextNode, L"endfor", L"for", CompareString, ShellCommandGetCurrentScriptFile (), FALSE, FALSE, TRUE)) {
        CurrentScriptFile = ShellCommandGetCurrentScriptFile ();
        ShellPrintHiiDefaultEx (
          STRING_TOKEN (STR_SYNTAX_NO_MATCHING),
          gShellLevel1HiiHandle,
          CompareString,
          L"Goto",
          CurrentScriptFile != NULL
                               && CurrentScriptFile->CurrentCommand != NULL
            ? CurrentScriptFile->CurrentCommand->Line : 0
          );
        ShellStatus = SHELL_NOT_FOUND;
      }

      FreePool (CompareString);
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
