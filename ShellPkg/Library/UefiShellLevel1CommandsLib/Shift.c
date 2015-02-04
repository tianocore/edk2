/** @file
  Main file for Shift shell level 1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
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
  Function for 'shift' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunShift (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  SCRIPT_FILE         *CurrentScriptFile;
  UINTN               LoopVar;

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  if (!gEfiShellProtocol->BatchIsActive()) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_NO_SCRIPT), gShellLevel1HiiHandle, L"shift");  
    return (SHELL_UNSUPPORTED);
  }

  CurrentScriptFile = ShellCommandGetCurrentScriptFile();
  ASSERT(CurrentScriptFile != NULL);

  if (CurrentScriptFile->Argc < 2) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle, L"shift");  
    return (SHELL_UNSUPPORTED);
  }

  for (LoopVar = 0 ; LoopVar < CurrentScriptFile->Argc ; LoopVar++) {
    if (LoopVar == 0) {
      SHELL_FREE_NON_NULL(CurrentScriptFile->Argv[LoopVar]);
    }
    if (LoopVar < CurrentScriptFile->Argc -1) {
      CurrentScriptFile->Argv[LoopVar] = CurrentScriptFile->Argv[LoopVar+1];
    } else {
      CurrentScriptFile->Argv[LoopVar] = NULL;
    }
  }
  CurrentScriptFile->Argc--;
  return (SHELL_SUCCESS);
}

