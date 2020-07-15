/** @file
  Main file for Reconnect shell Driver1 function.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-r", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Connect all the possible console devices.

**/
VOID
ConnectAllConsoles (
  VOID
  )
{
  ShellConnectFromDevPaths(L"ConInDev");
  ShellConnectFromDevPaths(L"ConOutDev");
  ShellConnectFromDevPaths(L"ErrOutDev");

  ShellConnectFromDevPaths(L"ErrOut");
  ShellConnectFromDevPaths(L"ConIn");
  ShellConnectFromDevPaths(L"ConOut");
}


/**
  Function for 'reconnect' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunReconnect (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  SHELL_STATUS        ShellStatus;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  EFI_STATUS          Status;

  gInReconnect = TRUE;
  ShellStatus = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"reconnect", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    ShellStatus = ShellCommandRunDisconnect(ImageHandle, SystemTable);
    if (ShellStatus == SHELL_SUCCESS) {
      if (ShellCommandLineGetFlag(Package, L"-r")) {
        ConnectAllConsoles();
      }
      ShellStatus = ShellCommandRunConnect(ImageHandle, SystemTable);
    }
  }

  gInReconnect = FALSE;

  return (ShellStatus);
}

