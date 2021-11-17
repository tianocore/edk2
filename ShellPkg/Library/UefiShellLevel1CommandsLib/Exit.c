/** @file
  Main file for exit shell level 1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"/b", TypeFlag },
  { NULL,  TypeMax  }
};

/**
  Function for 'exit' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunExit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  UINT64        RetVal;
  CONST CHAR16  *Return;

  ShellStatus = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel1HiiHandle, L"exit", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // return the specified error code
    //
    Return = ShellCommandLineGetRawValue (Package, 1);
    if (Return != NULL) {
      Status = ShellConvertStringToUint64 (Return, &RetVal, FALSE, FALSE);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel1HiiHandle, L"exit", Return);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // If we are in a batch file and /b then pass TRUE otherwise false...
        //
        ShellCommandRegisterExit ((BOOLEAN)(gEfiShellProtocol->BatchIsActive () && ShellCommandLineGetFlag (Package, L"/b")), RetVal);

        ShellStatus = SHELL_SUCCESS;
      }
    } else {
      // If we are in a batch file and /b then pass TRUE otherwise false...
      //
      ShellCommandRegisterExit ((BOOLEAN)(gEfiShellProtocol->BatchIsActive () && ShellCommandLineGetFlag (Package, L"/b")), 0);

      ShellStatus = SHELL_SUCCESS;
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
