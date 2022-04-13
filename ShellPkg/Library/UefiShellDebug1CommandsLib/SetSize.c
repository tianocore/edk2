/** @file
  Main file for SetSize shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"

/**
  Function for 'setsize' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunSetSize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  LIST_ENTRY         *Package;
  CHAR16             *ProblemParam;
  SHELL_STATUS       ShellStatus;
  CONST CHAR16       *Temp1;
  UINTN              NewSize;
  UINTN              LoopVar;
  SHELL_FILE_HANDLE  FileHandle;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;

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
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"setsize", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) < 3) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"setsize");
      ShellStatus = SHELL_INVALID_PARAMETER;
      NewSize     = 0;
    } else {
      Temp1 = ShellCommandLineGetRawValue (Package, 1);
      if (!ShellIsHexOrDecimalNumber (Temp1, FALSE, FALSE)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SIZE_NOT_SPEC), gShellDebug1HiiHandle, L"setsize");
        ShellStatus = SHELL_INVALID_PARAMETER;
        NewSize     = 0;
      } else {
        NewSize = ShellStrToUintn (Temp1);
      }
    }

    for (LoopVar = 2; LoopVar < ShellCommandLineGetCount (Package) && ShellStatus == SHELL_SUCCESS; LoopVar++) {
      Status = ShellOpenFileByName (ShellCommandLineGetRawValue (Package, LoopVar), &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);
      if (EFI_ERROR (Status)) {
        Status = ShellOpenFileByName (ShellCommandLineGetRawValue (Package, LoopVar), &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
      }

      if (EFI_ERROR (Status) && (LoopVar == 2)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_FILE_NOT_SPEC), gShellDebug1HiiHandle, L"setsize");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellDebug1HiiHandle, L"setsize", ShellCommandLineGetRawValue (Package, LoopVar));
        ShellStatus = SHELL_INVALID_PARAMETER;
        break;
      } else {
        Status = FileHandleSetSize (FileHandle, NewSize);
        if (Status == EFI_VOLUME_FULL) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_VOLUME_FULL), gShellDebug1HiiHandle, L"setsize");
          ShellStatus = SHELL_VOLUME_FULL;
        } else if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SET_SIZE_FAIL), gShellDebug1HiiHandle, L"setsize", ShellCommandLineGetRawValue (Package, LoopVar));
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SET_SIZE_DONE), gShellDebug1HiiHandle, ShellCommandLineGetRawValue (Package, LoopVar));
        }

        ShellCloseFile (&FileHandle);
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
