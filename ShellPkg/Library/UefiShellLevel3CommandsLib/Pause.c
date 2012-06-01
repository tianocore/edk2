/** @file
  Main file for Pause shell level 3 function.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel3CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-q", TypeFlag},
  {NULL, TypeMax}
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
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  SHELL_PROMPT_RESPONSE            *Resp;

  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_SCRIPT_ONLY), gShellLevel3HiiHandle);
    return (SHELL_UNSUPPORTED);
  }

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    } else if (ShellCommandLineGetRawValue(Package, 1) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (!ShellCommandLineGetFlag(Package, L"-q")) {
        Status = ShellPromptForResponseHii(ShellPromptResponseTypeQuitContinue, STRING_TOKEN (STR_PAUSE_PROMPT), gShellLevel3HiiHandle, (VOID**)&Resp);
      } else {
        Status = ShellPromptForResponse(ShellPromptResponseTypeQuitContinue, NULL, (VOID**)&Resp);
      }

      if (EFI_ERROR(Status) || Resp == NULL || *Resp == ShellPromptResponseQuit) {
        ShellCommandRegisterExit(TRUE, 0);
        ShellStatus = SHELL_ABORTED;
      }

      if (Resp != NULL) {
        FreePool(Resp);
      }
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }


  return (ShellStatus);
}

