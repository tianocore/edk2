/** @file
  Main file for Echo shell level 3 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-on",  TypeFlag },
  { L"-off", TypeFlag },
  { NULL,    TypeMax  }
};

/**
  Function for 'echo' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEcho (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  SHELL_STATUS  ShellStatus;
  UINTN         ParamCount;
  CHAR16        *ProblemParam;
  UINTN         Size;
  CHAR16        *PrintString;

  Size         = 0;
  ProblemParam = NULL;
  PrintString  = NULL;
  ShellStatus  = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParseEx (ParamList, &Package, &ProblemParam, TRUE, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"echo", ProblemParam);
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
    }

    if (ShellCommandLineGetFlag (Package, L"-on")) {
      //
      // Turn it on
      //
      ShellCommandSetEchoState (TRUE);
    } else if (ShellCommandLineGetFlag (Package, L"-off")) {
      //
      // turn it off
      //
      ShellCommandSetEchoState (FALSE);
    } else if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
      //
      // output its current state
      //
      if (ShellCommandGetEchoState ()) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_ECHO_ON), gShellLevel3HiiHandle);
      } else {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_ECHO_OFF), gShellLevel3HiiHandle);
      }
    } else {
      //
      // print the line
      //
      for ( ParamCount = 1
            ; ShellCommandLineGetRawValue (Package, ParamCount) != NULL
            ; ParamCount++
            )
      {
        StrnCatGrow (&PrintString, &Size, ShellCommandLineGetRawValue (Package, ParamCount), 0);
        if (ShellCommandLineGetRawValue (Package, ParamCount+1) != NULL) {
          StrnCatGrow (&PrintString, &Size, L" ", 0);
        }
      }

      ShellPrintDefaultEx (L"%s\r\n", PrintString);
      SHELL_FREE_NON_NULL (PrintString);
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
