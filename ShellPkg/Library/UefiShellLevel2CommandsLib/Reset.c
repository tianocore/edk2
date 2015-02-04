/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ResetParamList[] = {
  {L"-w", TypeValue},
  {L"-s", TypeValue},
  {L"-c", TypeValue},
  {NULL, TypeMax}
  };

/**
  Function for 'reset' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunReset (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CONST CHAR16  *String;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;

  ShellStatus = SHELL_SUCCESS;
  ProblemParam = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ResetParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"reset", ProblemParam);  
      FreePool(ProblemParam);
      return (SHELL_INVALID_PARAMETER);
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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"reset");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // check for warm reset flag, then shutdown reset flag, then cold (default) reset flag
      //
      if (ShellCommandLineGetFlag(Package, L"-w")) {
        if (ShellCommandLineGetFlag(Package, L"-s") || ShellCommandLineGetFlag(Package, L"-c")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"reset");  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          String = ShellCommandLineGetValue(Package, L"-w");
          if (String != NULL) {
            gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, StrSize(String), (VOID*)String);
          } else {
            gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
          }
        }
      } else if (ShellCommandLineGetFlag(Package, L"-s")) {
        if (ShellCommandLineGetFlag(Package, L"-c")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"reset");  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          String = ShellCommandLineGetValue(Package, L"-s");
          DEBUG_CODE(ShellPrintEx(-1,-1,L"Reset with %s (%d bytes)", String, String!=NULL?StrSize(String):0););
          if (String != NULL) {
            gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, StrSize(String), (VOID*)String);
          } else {
            gRT->ResetSystem(EfiResetShutdown, EFI_SUCCESS, 0, NULL);
          }
        }
      } else {
        //
        // this is default so dont worry about flag...
        //
        String = ShellCommandLineGetValue(Package, L"-c");
        if (String != NULL) {
          gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, StrSize(String), (VOID*)String);
        } else {
          gRT->ResetSystem(EfiResetCold, EFI_SUCCESS, 0, NULL);
        }
      }
    }
  }

  //
  // we should never get here... so the free and return are for formality more than use
  // as the ResetSystem function should not return...
  //

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  //
  // return the status
  //
  return (ShellStatus);
}

