/** @file
  Main file for Parse shell level 2 function.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Check if data is coming from StdIn output.

  @param[in] None

  @retval TRUE  StdIn stream data available to parse
  @retval FALSE StdIn stream data is not available to parse.
**/
BOOLEAN
IsStdInDataAvailable (
  VOID
  )
{
  SHELL_FILE_HANDLE FileHandle;
  EFI_STATUS        Status;
  CHAR16            CharBuffer;
  UINTN             CharSize;
  UINT64            OriginalFilePosition;

  Status               = EFI_SUCCESS;
  FileHandle           = NULL;
  OriginalFilePosition = 0;

  if (ShellOpenFileByName (L">i", &FileHandle, EFI_FILE_MODE_READ, 0) == EFI_SUCCESS) {
    CharSize = sizeof(CHAR16);
    gEfiShellProtocol->GetFilePosition (FileHandle, &OriginalFilePosition);
    Status = gEfiShellProtocol->ReadFile (FileHandle, &CharSize, &CharBuffer);
    if (EFI_ERROR (Status) || (CharSize != sizeof(CHAR16))) {
      return FALSE;
    }
    gEfiShellProtocol->SetFilePosition(FileHandle, OriginalFilePosition);
  }

  if (FileHandle == NULL) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Handle stings for SFO Output with escape character ^ in a string
  1. Quotation marks in the string must be escaped by using a ^ character (i.e. ^").
  2. The ^ character may be inserted using ^^.

  @param[in]  String  The Unicode NULL-terminated string.

  @retval NewString   The new string handled for SFO.
**/
EFI_STRING
HandleStringWithEscapeCharForParse (
  IN      CHAR16  *String
  )
{
  EFI_STRING  NewStr;
  EFI_STRING  StrWalker;
  EFI_STRING  ReturnStr;

  if (String == NULL) {
    return NULL;
  }

  //
  // start to parse the input string.
  //
  NewStr = AllocateZeroPool (StrSize (String));
  if (NewStr == NULL) {
    return NULL;
  }
  ReturnStr = NewStr;
  StrWalker = String;
  while (*StrWalker != CHAR_NULL) {
    if (*StrWalker == L'^' && (*(StrWalker + 1) == L'^' || *(StrWalker + 1) == L'"')) {
      *NewStr = *(StrWalker + 1);
      StrWalker++;
    } else {
      *NewStr = *StrWalker;
    }
    StrWalker++;
    NewStr++;
  }

  return ReturnStr;
}


/**
  Do the actual parsing of the file.  the file should be SFO output from a
  shell command or a similar format.

  @param[in] FileName               The filename to open.
  @param[in] TableName              The name of the table to find.
  @param[in] ColumnIndex            The column number to get.
  @param[in] TableNameInstance      Which instance of the table to get (row).
  @param[in] ShellCommandInstance   Which instance of the command to get.
  @param[in] StreamingUnicode       Indicates Input file is StdIn Unicode streaming data or not

  @retval SHELL_NOT_FOUND     The requested instance was not found.
  @retval SHELL_SUCCESS       The operation was successful.
**/
SHELL_STATUS
PerformParsing(
  IN CONST CHAR16 *FileName,
  IN CONST CHAR16 *TableName,
  IN CONST UINTN  ColumnIndex,
  IN CONST UINTN  TableNameInstance,
  IN CONST UINTN  ShellCommandInstance,
  IN BOOLEAN      StreamingUnicode
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
  CHAR16            *SfoString;

  ASSERT(FileName   != NULL);
  ASSERT(TableName  != NULL);

  ShellStatus       = SHELL_SUCCESS;

  Status = ShellOpenFileByName(FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel2HiiHandle, L"parse", FileName);
    ShellStatus = SHELL_NOT_FOUND;
  } else if (!EFI_ERROR (FileHandleIsDirectory (FileHandle))) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NOT_FILE), gShellLevel2HiiHandle, L"parse", FileName);
    ShellStatus = SHELL_NOT_FOUND;
  } else {
    for (LoopVariable = 0 ; LoopVariable < ShellCommandInstance && !ShellFileHandleEof(FileHandle);) {
     TempLine = ShellFileHandleReturnLine (FileHandle, &Ascii);

      if ((TempLine == NULL) || (*TempLine == CHAR_NULL && StreamingUnicode)) {
         break;
      }

      //
      // Search for "ShellCommand," in the file to start the SFO table
      // for a given ShellCommand.  The UEFI Shell spec does not specify
      // a space after the comma.
      //
      if (StrStr (TempLine, L"ShellCommand,") == TempLine) {
        LoopVariable++;
      }
      SHELL_FREE_NON_NULL(TempLine);
    }
    if (LoopVariable == ShellCommandInstance) {
      LoopVariable = 0;
      while(1) {
        TempLine = ShellFileHandleReturnLine (FileHandle, &Ascii);
        if (TempLine == NULL
            || *TempLine == CHAR_NULL
            || StrStr (TempLine, L"ShellCommand,") == TempLine) {
          SHELL_FREE_NON_NULL(TempLine);
          break;
        }
        if (StrStr (TempLine, TableName) == TempLine) {
          LoopVariable++;
          if (LoopVariable == TableNameInstance
              || (TableNameInstance == (UINTN)-1)) {
            for (ColumnLoop = 1, ColumnPointer = TempLine; ColumnLoop < ColumnIndex && ColumnPointer != NULL && *ColumnPointer != CHAR_NULL; ColumnLoop++) {
              ColumnPointer = StrStr (ColumnPointer, L",\"");
              if (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL){
                ColumnPointer++;
              }
            }
            if (ColumnLoop == ColumnIndex) {
              if (ColumnPointer == NULL) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellLevel2HiiHandle, L"parse", L"Column Index");
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                TempSpot = StrStr (ColumnPointer, L",\"");
                if (TempSpot != NULL) {
                  *TempSpot = CHAR_NULL;
                }
                while (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL && ColumnPointer[0] == L' '){
                  ColumnPointer++;
                }
                if (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL && ColumnPointer[0] == L'\"'){
                  ColumnPointer++;
                }
                if (ColumnPointer != NULL && *ColumnPointer != CHAR_NULL && ColumnPointer[StrLen (ColumnPointer) - 1] == L'\"'){
                  ColumnPointer[StrLen (ColumnPointer) - 1] = CHAR_NULL;
                }
                SfoString = HandleStringWithEscapeCharForParse (ColumnPointer);
                if (SfoString != NULL) {
                  ShellPrintEx (-1, -1, L"%s\r\n", SfoString);
                  SHELL_FREE_NON_NULL (SfoString);
                }
              }
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
  BOOLEAN             StreamingUnicode;

  ShellStatus      = SHELL_SUCCESS;
  ProblemParam     = NULL;
  StreamingUnicode = FALSE;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParseEx (ParamList, &Package, &ProblemParam, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"parse", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    StreamingUnicode = IsStdInDataAvailable ();
    if ((!StreamingUnicode && (ShellCommandLineGetCount(Package) < 4)) ||
        (ShellCommandLineGetCount(Package) < 3)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"parse");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if ((StreamingUnicode && (ShellCommandLineGetCount(Package) > 3)) ||
                (ShellCommandLineGetCount(Package) > 4)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"parse");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (StreamingUnicode) {
        FileName         = L">i";
        TableName        = ShellCommandLineGetRawValue(Package, 1);
        ColumnString     = ShellCommandLineGetRawValue(Package, 2);
      } else {
        FileName         = ShellCommandLineGetRawValue(Package, 1);
        TableName        = ShellCommandLineGetRawValue(Package, 2);
        ColumnString     = ShellCommandLineGetRawValue(Package, 3);
      }
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

      ShellStatus = PerformParsing(FileName, TableName, ShellStrToUintn(ColumnString), TableNameInstance, ShellCommandInstance, StreamingUnicode);
    }
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}

