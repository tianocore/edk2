/** @file
  Main file for attrib shell level 2 function.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Print out each environment variable registered with the Shell 2.0 GUID.

  If you spawn a pre 2.0 shell from the Shell 2.0 the environment variable may not carry through.

  @retval STATUS_SUCCESS  the printout was sucessful
  @return any return code from GetNextVariableName except EFI_NOT_FOUND
**/
SHELL_STATUS
EFIAPI
PrintAllShellEnvVars(
  VOID
  )
{
  CONST CHAR16      *Value;
  CONST CHAR16      *ConstEnvNameList;

  ConstEnvNameList = gEfiShellProtocol->GetEnv(NULL);
  if (ConstEnvNameList == NULL) {
    return (SHELL_SUCCESS);
  }
  while (*ConstEnvNameList != CHAR_NULL){
    Value = gEfiShellProtocol->GetEnv(ConstEnvNameList);
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SET_DISP), gShellLevel2HiiHandle, ConstEnvNameList, Value);
    ConstEnvNameList += StrLen(ConstEnvNameList)+1;
  }

  return (SHELL_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM SetParamList[] = {
  {L"-d", TypeValue},
  {L"-v", TypeFlag},
  {NULL, TypeMax}
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
  CONST CHAR16  *KeyName;
  CONST CHAR16  *Value;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // Make sure globals are good...
  //
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (SetParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
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
    } else if (ShellCommandLineGetRawValue(Package, 3) != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetRawValue(Package, 1) != NULL && ShellCommandLineGetFlag(Package, L"-d")) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetFlag(Package, L"-d")) {
      //
      // delete a environment variable
      //
      KeyName = ShellCommandLineGetValue(Package, L"-d");
      if (KeyName == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellLevel2HiiHandle, L"-d");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Status = ShellSetEnvironmentVariable(KeyName, L"", ShellCommandLineGetFlag(Package, L"-v"));
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SET_ND), gShellLevel2HiiHandle, KeyName, Status);
          ShellStatus = SHELL_DEVICE_ERROR;
        }
      }
    } else if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      //
      // print out all current environment variables
      //
      return(PrintAllShellEnvVars());
    } else {
      //
      // we are either printing one or assigning one
      //
      KeyName = ShellCommandLineGetRawValue(Package, 1);
      Value   = ShellCommandLineGetRawValue(Package, 2);
      if (KeyName != NULL && Value != NULL) {
        //
        // assigning one
        //
        Status = ShellSetEnvironmentVariable(KeyName, Value, ShellCommandLineGetFlag(Package, L"-v"));
      } else {
        if (KeyName != NULL) {
          //
          // print out value for this one only.
          //
          Value = ShellGetEnvironmentVariable(KeyName);
          if (Value == NULL) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SET_NF), gShellLevel2HiiHandle, KeyName);
            ShellStatus = SHELL_SUCCESS;
          } else {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SET_DISP), gShellLevel2HiiHandle, KeyName, Value);
            ShellStatus = SHELL_SUCCESS;
          }
        } else {
          ASSERT(FALSE);
        }
      }
    }
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}
