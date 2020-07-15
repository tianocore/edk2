/** @file
  Main file for Alias shell level 3 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>

/**
  Print out single alias registered with the Shell.

  @param[in]  Alias             Points to the NULL-terminated shell alias.
                                If this parameter is NULL, then all
                                aliases will be returned in ReturnedData.
  @retval     SHELL_SUCCESS     the printout was sucessful
**/
SHELL_STATUS
PrintSingleShellAlias(
  IN  CONST CHAR16 *Alias
  )
{
  CONST CHAR16        *ConstAliasVal;
  SHELL_STATUS        ShellStatus;
  BOOLEAN             Volatile;

  ShellStatus = SHELL_SUCCESS;
  ConstAliasVal = gEfiShellProtocol->GetAlias (Alias, &Volatile);
  if (ConstAliasVal == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"alias", Alias);
    ShellStatus = SHELL_INVALID_PARAMETER;
  } else {
    if (ShellCommandIsOnAliasList (Alias)) {
      Volatile = FALSE;
    }
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_ALIAS_OUTPUT), gShellLevel3HiiHandle, !Volatile ? L' ' : L'*', Alias, ConstAliasVal);
  }
  return ShellStatus;
}

/**
  Print out each alias registered with the Shell.

  @retval STATUS_SUCCESS  the printout was sucessful
  @return any return code from GetNextVariableName except EFI_NOT_FOUND
**/
SHELL_STATUS
PrintAllShellAlias(
  VOID
  )
{
  CONST CHAR16      *ConstAllAliasList;
  CHAR16            *Alias;
  CHAR16            *Walker;

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
    PrintSingleShellAlias(Alias);
  } while (Walker != NULL && Walker[0] != CHAR_NULL);

  FreePool(Alias);

  return (SHELL_SUCCESS);
}

/**
  Changes a shell command alias.

  This function creates an alias for a shell command or if Alias is NULL it will delete an existing alias.


  @param[in] Command            Points to the NULL-terminated shell command or existing alias.
  @param[in] Alias              Points to the NULL-terminated alias for the shell command. If this is NULL, and
                                Command refers to an alias, that alias will be deleted.
  @param[in] Replace            If TRUE and the alias already exists, then the existing alias will be replaced. If
                                FALSE and the alias already exists, then the existing alias is unchanged and
                                EFI_ACCESS_DENIED is returned.
  @param[in] Volatile           if TRUE the Alias being set will be stored in a volatile fashion.  if FALSE the
                                Alias being set will be stored in a non-volatile fashion.

  @retval SHELL_SUCCESS        Alias created or deleted successfully.
  @retval SHELL_NOT_FOUND       the Alias intended to be deleted was not found
  @retval SHELL_ACCESS_DENIED   The alias is a built-in alias or already existed and Replace was set to
                                FALSE.
  @retval SHELL_DEVICE_ERROR    Command is null or the empty string.
**/
SHELL_STATUS
ShellLevel3CommandsLibSetAlias(
  IN CONST CHAR16 *Command,
  IN CONST CHAR16 *Alias,
  IN BOOLEAN Replace,
  IN BOOLEAN Volatile
  )
{
  SHELL_STATUS        ShellStatus;
  EFI_STATUS          Status;

  ShellStatus = SHELL_SUCCESS;
  Status = gEfiShellProtocol->SetAlias (Command, Alias, Replace, Volatile);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_ACCESS_DENIED) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_AD), gShellLevel3HiiHandle, L"alias");
      ShellStatus = SHELL_ACCESS_DENIED;
    } else if (Status == EFI_NOT_FOUND) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_NOT_FOUND), gShellLevel3HiiHandle, L"alias", Command);
      ShellStatus = SHELL_NOT_FOUND;
    } else {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel3HiiHandle, L"alias", Status);
      ShellStatus = SHELL_DEVICE_ERROR;
    }
  }
  return ShellStatus;
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-v", TypeFlag},
  {L"-d", TypeValue},
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
  CONST CHAR16        *ParamStrD;
  CHAR16              *CleanParam2;
  BOOLEAN             DeleteFlag;
  BOOLEAN             VolatileFlag;

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

    DeleteFlag   = ShellCommandLineGetFlag (Package, L"-d");
    VolatileFlag = ShellCommandLineGetFlag (Package, L"-v");

    if (Param2 != NULL) {
      CleanParam2 = AllocateCopyPool (StrSize(Param2), Param2);
      if (CleanParam2 == NULL) {
        ShellCommandLineFreeVarList (Package);
        return SHELL_OUT_OF_RESOURCES;
      }

      if (CleanParam2[0] == L'\"' && CleanParam2[StrLen(CleanParam2)-1] == L'\"') {
        CleanParam2[StrLen(CleanParam2)-1] = L'\0';
        CopyMem (CleanParam2, CleanParam2 + 1, StrSize(CleanParam2) - sizeof(CleanParam2[0]));
      }
    }

    if (!DeleteFlag && !VolatileFlag) {
      switch (ShellCommandLineGetCount (Package)) {
        case 1:
          //
          // "alias"
          //
          ShellStatus = PrintAllShellAlias ();
          break;
        case 2:
          //
          // "alias Param1"
          //
          ShellStatus = PrintSingleShellAlias (Param1);
          break;
        case 3:
          //
          // "alias Param1 CleanParam2"
          //
          ShellStatus = ShellLevel3CommandsLibSetAlias (CleanParam2, Param1, FALSE, VolatileFlag);
          break;
        default:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"alias");
          ShellStatus = SHELL_INVALID_PARAMETER;
      }
    } else if (DeleteFlag) {
      if (VolatileFlag || ShellCommandLineGetCount (Package) > 1) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"alias");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        ParamStrD = ShellCommandLineGetValue (Package, L"-d");
        if (ParamStrD == NULL) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle, L"alias");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          //
          // Delete an alias: "alias -d ParamStrD"
          //
          ShellStatus = ShellLevel3CommandsLibSetAlias (ParamStrD, NULL, TRUE, FALSE);
        }
      }
    } else {
      //
      // Set volatile alias.
      //
      ASSERT (VolatileFlag);
      ASSERT (!DeleteFlag);
      switch (ShellCommandLineGetCount (Package)) {
        case 1:
        case 2:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle, L"alias");
          ShellStatus = SHELL_INVALID_PARAMETER;
          break;
        case 3:
          //
          // "alias -v Param1 CleanParam2"
          //
          ShellStatus = ShellLevel3CommandsLibSetAlias (CleanParam2, Param1, FALSE, VolatileFlag);
          break;
        default:
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"alias");
          ShellStatus = SHELL_INVALID_PARAMETER;
      }
    }
    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  SHELL_FREE_NON_NULL (CleanParam2);
  return (ShellStatus);
}
