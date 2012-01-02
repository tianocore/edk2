/** @file
  Main file for Parse shell level 2 function.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Do the actual parsing of the file.  the file should be SFO output from a 
  shell command or a similar format.

  @param[in] FileName               The filename to open.
  @param[in] TableName              The name of the table to find.
  @param[in] ColumnIndex            The column number to get.
  @param[in] TableNameInstance      Which instance of the table to get (row).
  @param[in] ShellCommandInstance   Which instance of the command to get.

  @retval SHELL_NOT_FOUND     The requested instance was not found.
  @retval SHELL_SUCCESS       The operation was successful.
**/
SHELL_STATUS
EFIAPI
PerformParsing(
  IN CONST CHAR16 *FileName,
  IN CONST CHAR16 *TableName,
  IN CONST UINTN  ColumnIndex,
  IN CONST UINTN  TableNameInstance,
  IN CONST UINTN  ShellCommandInstance
  )
{
  SHELL_FILE_HANDLE FileHandle;
  EFI_STATUS        Status;
  BOOLEAN           Ascii;
  UINTN             LoopVariable;
  UINTN             ColumnLoop;
  CHAR16            *TempLine;
  CHAR16            *ColumnPointer;
  SHELL_STATUS      ShellStatus;
  CHAR16            *TempSpot;

  ASSERT(FileName   != NULL);
  ASSERT(TableName  != NULL);

  ShellStatus       = SHELL_SUCCESS;

  Status = ShellOpenFileByName(FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel2HiiHandle, FileName);
    ShellStatus = SHELL_NOT_FOUND;
  } else {
    for (LoopVariable = 0 ; LoopVariable < ShellCommandInstance && !ShellFileHandleEof(FileHandle);) {
      TempLine = ShellFileHandleReturnLine(FileHandle, &Ascii);
      if (TempLine == NULL) {
        break;
      }
      if (StrStr(TempLine, L"ShellCommand, \"") == TempLine) {
        LoopVariable++;
      }
      SHELL_FREE_NON_NULL(TempLine);
    }
    if (LoopVariable == ShellCommandInstance) {
      LoopVariable = 0;
      while(1) {
        TempLine = ShellFileHandleReturnLine(FileHandle, &Ascii);
        if ( TempLine == NULL
          || *TempLine == CHAR_NULL
          || StrStr(TempLine, L"ShellCommand, \"") == TempLine
         ){
          SHELL_FREE_NON_NULL(TempLine);
          break;
        }
        if (StrStr(TempLine, TableName) == TempLine) {
          LoopVariable++;
        }
        if ( LoopVariable == TableNameInstance
          || (TableNameInstance == (UINTN)-1 && StrStr(TempLine, TableName) == TempLine)
         ){
          for (ColumnLoop = 1, ColumnPointer = TempLine; ColumnLoop < ColumnIndex && ColumnPointer != NULL && *ColumnPointer != CHAR_NULL; ColumnLoop++) {
            ColumnPointer = StrStr(ColumnPointer, L",");
            if (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL){
              ColumnPointer++;
            }
          }
          if (ColumnLoop == ColumnIndex) {
            if (ColumnPointer == NULL) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellLevel2HiiHandle, L"Column Index");
              ShellStatus = SHELL_INVALID_PARAMETER;
            } else {
              TempSpot = StrStr(ColumnPointer, L",");
              if (TempSpot != NULL) {
                *TempSpot = CHAR_NULL;
              }
              while (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL && ColumnPointer[0] == L' '){
                ColumnPointer++;
              }
              if (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL && ColumnPointer[0] == L'\"'){
                ColumnPointer++;
              }
              if (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL && ColumnPointer[StrLen(ColumnPointer)-1] == L'\"'){
                ColumnPointer[StrLen(ColumnPointer)-1] = CHAR_NULL;
              }

              ShellPrintEx(-1, -1, L"%s\r\n", ColumnPointer);
            }
          }
        }
        SHELL_FREE_NON_NULL(TempLine);
      }
    }
  }
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-i", TypeValue},
  {L"-s", TypeValue},
  {NULL, TypeMax}
  };

/**
  Function for 'parse' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunParse (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  CONST CHAR16        *FileName;
  CONST CHAR16        *TableName;
  CONST CHAR16        *ColumnString;
  SHELL_STATUS        ShellStatus;
  UINTN               ShellCommandInstance;
  UINTN               TableNameInstance;

  ShellStatus   = SHELL_SUCCESS;
  ProblemParam  = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) < 4) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) > 4) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      FileName      = ShellCommandLineGetRawValue(Package, 1);
      TableName     = ShellCommandLineGetRawValue(Package, 2);
      ColumnString  = ShellCommandLineGetRawValue(Package, 3);

      if (ShellCommandLineGetValue(Package, L"-i") == NULL) {
        TableNameInstance = (UINTN)-1;
      } else {
        TableNameInstance = ShellStrToUintn(ShellCommandLineGetValue(Package, L"-i"));
      }
      if (ShellCommandLineGetValue(Package, L"-s") == NULL) {
        ShellCommandInstance = 1;
      } else {
        ShellCommandInstance = ShellStrToUintn(ShellCommandLineGetValue(Package, L"-s"));
      }

      ShellStatus = PerformParsing(FileName, TableName, ShellStrToUintn(ColumnString), TableNameInstance, ShellCommandInstance);
    }
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}

