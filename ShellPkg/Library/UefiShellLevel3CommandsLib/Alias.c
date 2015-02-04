/** @file
  Main file for Alias shell level 3 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved. <BR>
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
  Print out each alias registered with the Shell.

  @retval STATUS_SUCCESS  the printout was sucessful
  @return any return code from GetNextVariableName except EFI_NOT_FOUND
**/
SHELL_STATUS
EFIAPI
PrintAllShellAlias(
  VOID
  )
{
  CONST CHAR16      *ConstAllAliasList;
  CHAR16            *Alias;
  CONST CHAR16      *Command;
  CHAR16            *Walker;
  BOOLEAN           Volatile;

  Volatile = FALSE;

  ConstAllAliasList = gEfiShellProtocol->GetAlias(NULL, NULL);
  if (ConstAllAliasList == NULL) {
    return (SHELL_SUCCESS);
  }
  Alias = AllocateZeroPool(StrSize(ConstAllAliasList));
  if (Alias == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }
  Walker = (CHAR16*)ConstAllAliasList;

  do {
    CopyMem(Alias, Walker, StrSize(Walker));
    Walker = StrStr(Alias, L";");
    if (Walker != NULL) {
      Walker[0] = CHAR_NULL;
      Walker = Walker + 1;
    }
    Command = gEfiShellProtocol->GetAlias(Alias, &Volatile);
    if (ShellCommandIsOnAliasList(Alias)) {
      Volatile = FALSE;
    }
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_ALIAS_OUTPUT), gShellLevel3HiiHandle, !Volatile?L' ':L'*', Alias, Command);
  } while (Walker != NULL && Walker[0] != CHAR_NULL);

  FreePool(Alias);

  return (SHELL_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},
  {L"-d", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'alias' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunAlias (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST CHAR16        *Param1;
  CONST CHAR16        *Param2;
  CHAR16              *CleanParam2;

  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  CleanParam2         = NULL;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"alias", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    Param1 = ShellCommandLineGetRawValue(Package, 1);
    Param2 = ShellCommandLineGetRawValue(Package, 2);

    if (Param2 != NULL) {
      CleanParam2 = AllocateCopyPool (StrSize(Param2), Param2);
      if (CleanParam2 == NULL) {
        return SHELL_OUT_OF_RESOURCES;
      }

      if (CleanParam2[0] == L'\"' && CleanParam2[StrLen(CleanParam2)-1] == L'\"') {
        CleanParam2[StrLen(CleanParam2)-1] = L'\0';
        CopyMem (CleanParam2, CleanParam2 + 1, StrSize(CleanParam2) - sizeof(CleanParam2[0]));
      }
    }

    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    }
    if (ShellCommandLineGetCount(Package) == 1) {
      //
      // print out alias'
      //
      Status = PrintAllShellAlias();
    } else if (ShellCommandLineGetFlag(Package, L"-d")) {
      //
      // delete an alias
      //
      Status = gEfiShellProtocol->SetAlias(Param1, NULL, TRUE, FALSE);
    } else if (ShellCommandLineGetCount(Package) == 3) {
      //
      // must be adding an alias
      //
      Status = gEfiShellProtocol->SetAlias(CleanParam2, Param1, FALSE, ShellCommandLineGetFlag(Package, L"-v"));
      if (EFI_ERROR(Status)) {
        if (Status == EFI_ACCESS_DENIED) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellLevel3HiiHandle, L"alias");  
          ShellStatus = SHELL_ACCESS_DENIED;
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel3HiiHandle, L"alias", Status);  
          ShellStatus = SHELL_DEVICE_ERROR;
        }
      }
    } else if (ShellCommandLineGetCount(Package) == 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle, L"alias");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"alias");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    }
    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  SHELL_FREE_NON_NULL (CleanParam2);
  return (ShellStatus);
}
