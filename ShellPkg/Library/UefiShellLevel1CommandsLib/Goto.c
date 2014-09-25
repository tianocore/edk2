/** @file
  Main file for goto shell level 1 function.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CHAR16              *CompareString;
  UINTN               Size;
  SCRIPT_FILE         *CurrentScriptFile;

  ShellStatus         = SHELL_SUCCESS;
  CompareString       = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"Goto");
    return (SHELL_UNSUPPORTED);
  }

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Size = 0;
      ASSERT((CompareString == NULL && Size == 0) || (CompareString != NULL));
      CompareString = StrnCatGrow(&CompareString, &Size, L":", 0);
      CompareString = StrnCatGrow(&CompareString, &Size, ShellCommandLineGetRawValue(Package, 1), 0);
      //
      // Check forwards and then backwards for a label...
      //
      if (!MoveToTag(GetNextNode, L"endfor", L"for", CompareString, ShellCommandGetCurrentScriptFile(), FALSE, FALSE, TRUE)) {
        CurrentScriptFile = ShellCommandGetCurrentScriptFile();
        ShellPrintHiiEx(
          -1, 
          -1, 
          NULL, 
          STRING_TOKEN (STR_SYNTAX_NO_MATCHING), 
          gShellLevel1HiiHandle, 
          CompareString, 
          L"Goto", 
          CurrentScriptFile!=NULL 
            && CurrentScriptFile->CurrentCommand!=NULL
            ? CurrentScriptFile->CurrentCommand->Line:0);
        ShellStatus = SHELL_NOT_FOUND;
      }
    FreePool(CompareString);
    }
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}

