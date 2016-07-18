/** @file
  Main file for Echo shell level 3 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>

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
  CHAR16              *RawCmdLine;
  SHELL_STATUS        Status;
  UINTN               Size;
  CHAR16              *Walker;
  CHAR16              *TempParameter;
  BOOLEAN             OnFlag;
  BOOLEAN             OffFlag;
  UINTN               Count;

  RawCmdLine = ShellGetRawCmdLine ();
  if (RawCmdLine == NULL) {
    return SHELL_OUT_OF_RESOURCES;
  }

  OnFlag  = FALSE;
  OffFlag = FALSE;

  Size = StrSize (RawCmdLine);
  TempParameter  = AllocateZeroPool(Size);
  if (TempParameter == NULL) {
    Status = SHELL_OUT_OF_RESOURCES;
    goto Done;
  }

  for ( Count = 0
      , Walker = RawCmdLine
      ; Walker != NULL && *Walker != CHAR_NULL
      ; Count++
      ) {
    if (EFI_ERROR (ShellGetNextParameter (&Walker, TempParameter, Size, FALSE))) {
      break;
    }

    if (Count == 1) {
      if (gUnicodeCollation->StriColl(gUnicodeCollation, TempParameter, L"-on") == 0 ) {
        OnFlag = TRUE;
      }
      if (gUnicodeCollation->StriColl(gUnicodeCollation, TempParameter, L"-off") == 0 ) {
        OffFlag = TRUE;
      }
    }
  }

  if (OnFlag || OffFlag) {
    if (Count != 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_ECHO_INVALID_PARAM), gShellLevel3HiiHandle, L"echo", L"-on/-off");
      Status = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    ShellCommandSetEchoState(OnFlag);
    Status = SHELL_SUCCESS;
    goto Done;
  }

  Walker = RawCmdLine + StrLen (L"echo");
  if  (*Walker != CHAR_NULL) {
    Walker++;
    ShellPrintEx (-1, -1, L"%s\r\n", Walker);
  }

  Status = SHELL_SUCCESS;

Done:
  SHELL_FREE_NON_NULL (TempParameter);
  SHELL_FREE_NON_NULL (RawCmdLine);
  return Status;

}

