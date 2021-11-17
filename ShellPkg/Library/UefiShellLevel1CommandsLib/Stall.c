/** @file
  Main file for stall shell level 1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel1CommandsLib.h"

/**
  Function for 'stall' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunStall (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  UINT64        Intermediate;

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
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel1HiiHandle, L"stall", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetRawValue (Package, 2) != NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel1HiiHandle, L"stall");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel1HiiHandle, L"stall");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Status = ShellConvertStringToUint64 (ShellCommandLineGetRawValue (Package, 1), &Intermediate, FALSE, FALSE);
      if (EFI_ERROR (Status) || (((UINT64)(UINTN)(Intermediate)) != Intermediate)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel1HiiHandle, L"stall", ShellCommandLineGetRawValue (Package, 1));
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = gBS->Stall ((UINTN)Intermediate);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_STALL_FAILED), gShellLevel1HiiHandle, L"stall");
          ShellStatus = SHELL_DEVICE_ERROR;
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}
