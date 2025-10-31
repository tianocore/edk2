/** @file
  Main file for Mode shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"

/**
  Function for 'mode' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMode (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  UINTN         NewCol;
  UINTN         NewRow;
  UINTN         Col;
  UINTN         Row;
  CONST CHAR16  *Temp;
  BOOLEAN       Done;
  INT32         LoopVar;

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
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"mode", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 3) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"mode");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) == 2) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"mode");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount (Package) == 3) {
      Temp = ShellCommandLineGetRawValue (Package, 1);
      if (Temp == NULL) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mode", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (!ShellIsHexOrDecimalNumber (Temp, FALSE, FALSE)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mode", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }

      NewCol = ShellStrToUintn (Temp);
      Temp   = ShellCommandLineGetRawValue (Package, 2);
      if (Temp == NULL) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mode", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (!ShellIsHexOrDecimalNumber (Temp, FALSE, FALSE)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"mode", Temp);
        ShellStatus = SHELL_INVALID_PARAMETER;
      }

      NewRow = ShellStrToUintn (Temp);

      for (LoopVar = 0, Done = FALSE; LoopVar < gST->ConOut->Mode->MaxMode && ShellStatus == SHELL_SUCCESS; LoopVar++) {
        Status = gST->ConOut->QueryMode (gST->ConOut, LoopVar, &Col, &Row);
        if (EFI_ERROR (Status)) {
          continue;
        }

        if ((Col == NewCol) && (Row == NewRow)) {
          Status = gST->ConOut->SetMode (gST->ConOut, LoopVar);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiDefaultEx (STRING_TOKEN (STR_MODE_SET_FAIL), gShellDebug1HiiHandle, L"mode");
            ShellStatus = SHELL_DEVICE_ERROR;
          } else {
            // worked fine...
            Done = TRUE;
          }

          break;
        }
      }

      if (!Done) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_MODE_NO_MATCH), gShellDebug1HiiHandle, L"mode");
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
    } else if (ShellCommandLineGetCount (Package) == 1) {
      //
      // print out valid
      //
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_MODE_LIST_HEAD), gShellDebug1HiiHandle);
      for (LoopVar = 0, Done = FALSE; LoopVar < gST->ConOut->Mode->MaxMode && ShellStatus == SHELL_SUCCESS; LoopVar++) {
        Status = gST->ConOut->QueryMode (gST->ConOut, LoopVar, &Col, &Row);
        if (EFI_ERROR (Status)) {
          continue;
        }

        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_MODE_LIST_ITEM), gShellDebug1HiiHandle, Col, Row, LoopVar == gST->ConOut->Mode->Mode ? L'*' : L' ');
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
