/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Print out each environment variable registered with the Shell 2.0 GUID.

  If you spawn a pre 2.0 shell from the Shell 2.0 the environment variable may not carry through.

  @retval STATUS_SUCCESS  the printout was successful
  @return any return code from GetNextVariableName except EFI_NOT_FOUND
**/
SHELL_STATUS
PrintAllShellEnvVars (
  VOID
  )
{
  CONST CHAR16  *Value;
  CONST CHAR16  *ConstEnvNameList;

  ConstEnvNameList = gEfiShellProtocol->GetEnv (NULL);
  if (ConstEnvNameList == NULL) {
    return (SHELL_SUCCESS);
  }

  while (*ConstEnvNameList != CHAR_NULL) {
    Value = gEfiShellProtocol->GetEnv (ConstEnvNameList);
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_SET_DISP), gShellLevel2HiiHandle, ConstEnvNameList, Value);
    ConstEnvNameList += StrLen (ConstEnvNameList)+1;
  }

  return (SHELL_SUCCESS);
}

/** Main function of the 'Set' command.

  @param[in] Package    List of input parameter for the command.
**/
STATIC
SHELL_STATUS
MainCmdSet (
  LIST_ENTRY  *Package
  )
{
  EFI_STATUS    Status;
  CONST CHAR16  *KeyName;
  CONST CHAR16  *Value;
  SHELL_STATUS  ShellStatus;

  ShellStatus = SHELL_SUCCESS;

  //
  // check for "-?"
  //
  if (ShellCommandLineGetFlag (Package, L"-?")) {
    ASSERT (FALSE);
    return ShellStatus;
  } else if (ShellCommandLineGetRawValue (Package, 3) != NULL) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"set");
    return SHELL_INVALID_PARAMETER;
  } else if ((ShellCommandLineGetRawValue (Package, 1) != NULL) && ShellCommandLineGetFlag (Package, L"-d")) {
    ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"set");
    return SHELL_INVALID_PARAMETER;
  } else if (ShellCommandLineGetFlag (Package, L"-d")) {
    //
    // delete a environment variable
    //
    KeyName = ShellCommandLineGetValue (Package, L"-d");
    if (KeyName == NULL) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_NO_VALUE), gShellLevel2HiiHandle, L"set", L"-d");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Status = ShellSetEnvironmentVariable (KeyName, L"", ShellCommandLineGetFlag (Package, L"-v"));
      if (EFI_ERROR (Status)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_SET_ND), gShellLevel2HiiHandle, L"set", KeyName);
        ShellStatus = SHELL_DEVICE_ERROR;
      }
    }

    return ShellStatus;
  } else if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
    //
    // print out all current environment variables
    //
    return (PrintAllShellEnvVars ());
  }

  //
  // we are either printing one or assigning one
  //
  KeyName = ShellCommandLineGetRawValue (Package, 1);
  Value   = ShellCommandLineGetRawValue (Package, 2);
  if ((KeyName != NULL) && (Value != NULL)) {
    //
    // assigning one
    //
    Status = ShellSetEnvironmentVariable (KeyName, Value, ShellCommandLineGetFlag (Package, L"-v"));
    if (EFI_ERROR (Status)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_SET_ERROR_SET), gShellLevel2HiiHandle, L"set", KeyName);
      ShellStatus = (SHELL_STATUS)(Status & (~MAX_BIT));
    }
  } else {
    if (KeyName != NULL) {
      //
      // print out value for this one only.
      //
      Value = ShellGetEnvironmentVariable (KeyName);
      if (Value == NULL) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_SET_NF), gShellLevel2HiiHandle, L"set", KeyName);
        ShellStatus = SHELL_SUCCESS;
      } else {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_SET_DISP), gShellLevel2HiiHandle, KeyName, Value);
        ShellStatus = SHELL_SUCCESS;
      }
    } else {
      ASSERT (FALSE);
    }
  }

  return ShellStatus;
}

STATIC CONST SHELL_PARAM_ITEM  SetParamList[] = {
  { L"-d", TypeValue },
  { L"-v", TypeFlag  },
  { NULL,  TypeMax   }
};

/**
  Function for 'set' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunSet (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Make sure globals are good...
  //
  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (SetParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"set", ProblemParam);
      FreePool (ProblemParam);
      return (SHELL_INVALID_PARAMETER);
    } else {
      ASSERT (FALSE);
    }

    return ShellStatus;
  }

  ShellStatus = MainCmdSet (Package);

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}
