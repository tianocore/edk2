/** @file
  Main file for Type shell level 3 function.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved. <BR>
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
  Display a single file to StdOut.

  If both Ascii and UCS2 are FALSE attempt to discover the file type.

  @param[in] Handle   The handle to the file to display.
  @param[in] Ascii    TRUE to force ASCII, FALSE othewise.
  @param[in] UCS2     TRUE to force UCS2, FALSE othewise.

  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
TypeFileByHandle (
  IN EFI_HANDLE Handle,
  BOOLEAN Ascii,
  BOOLEAN UCS2
  )
{
  UINTN       ReadSize;
  VOID        *Buffer;
  EFI_STATUS  Status;
  UINTN       LoopVar;
  CHAR16      AsciiChar;

  ReadSize = PcdGet16(PcdShellFileOperationSize);
  Buffer = AllocateZeroPool(ReadSize);
  if (Buffer == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  Status = ShellSetFilePosition(Handle, 0);
  ASSERT_EFI_ERROR(Status);

  while (ReadSize == ((UINTN)PcdGet16(PcdShellFileOperationSize))){
    ZeroMem(Buffer, ReadSize);
    Status = ShellReadFile(Handle, &ReadSize, Buffer);
    if (EFI_ERROR(Status)){
      break;
    }

    if (!(Ascii|UCS2)){
      if (*(UINT16*)Buffer == gUnicodeFileTag) {
        UCS2 = TRUE;
        Buffer = ((UINT16*)Buffer) + 1;
      } else {
        Ascii = TRUE;
      }
    }

    //
    // We want to use plain Print function here! (no color support for files)
    //
    if (Ascii){
      for (LoopVar = 0 ; LoopVar < ReadSize ; LoopVar++) {
      AsciiChar = CHAR_NULL;
      AsciiChar = ((CHAR8*)Buffer)[LoopVar];
      if (AsciiChar == CHAR_NULL) {
        AsciiChar = '.';
      }
      Print(L"%c", AsciiChar);
      }
    } else {
      Print(L"%s", Buffer);
    }
  }
  Print(L"\r\n", Buffer);
  return (Status);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-a", TypeFlag},
  {L"-u", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'type' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunType (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  CONST CHAR16        *Param;
  SHELL_STATUS        ShellStatus;
  UINTN               ParamCount;
  EFI_SHELL_FILE_INFO *FileList;
  EFI_SHELL_FILE_INFO *Node;
  BOOLEAN             AsciiMode;
  BOOLEAN             UnicodeMode;

  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  ParamCount          = 0;
  FileList            = NULL;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    }
    AsciiMode   = ShellCommandLineGetFlag(Package, L"-a");
    UnicodeMode = ShellCommandLineGetFlag(Package, L"-u");

    if (AsciiMode && UnicodeMode) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"-a & -u");
      ShellStatus = SHELL_INVALID_PARAMETER;
   } else if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      //
      // we insufficient parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // get a list with each file specified by parameters
      // if parameter is a directory then add all the files below it to the list
      //
      for ( ParamCount = 1, Param = ShellCommandLineGetRawValue(Package, ParamCount)
          ; Param != NULL
          ; ParamCount++, Param = ShellCommandLineGetRawValue(Package, ParamCount)
         ){
        Status = ShellOpenFileMetaArg((CHAR16*)Param, EFI_FILE_MODE_READ, &FileList);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, (CHAR16*)Param);
          ShellStatus = SHELL_NOT_FOUND;
          break;
        }
        //
        // make sure we completed the param parsing sucessfully...
        // Also make sure that any previous action was sucessful
        //
        if (ShellStatus == SHELL_SUCCESS) {
          //
          // check that we have at least 1 file
          //
          if (FileList == NULL || IsListEmpty(&FileList->Link)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel3HiiHandle, Param);
            continue;
          } else {
            //
            // loop through the list and make sure we are not aborting...
            //
            for ( Node = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link)
                ; !IsNull(&FileList->Link, &Node->Link) && !ShellGetExecutionBreakFlag()
                ; Node = (EFI_SHELL_FILE_INFO*)GetNextNode(&FileList->Link, &Node->Link)
               ){
              //
              // make sure the file opened ok
              //
              if (EFI_ERROR(Node->Status)){
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NO_OPEN), gShellLevel3HiiHandle, Node->FileName, Node->Status);
                ShellStatus = SHELL_NOT_FOUND;
                continue;
              }

              //
              // make sure its not a directory
              //
              if (FileHandleIsDirectory(Node->Handle) == EFI_SUCCESS) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_IS_DIR), gShellLevel3HiiHandle, Node->FileName);
                ShellStatus = SHELL_NOT_FOUND;
                continue;
              }

              //
              // do it
              //
              Status = TypeFileByHandle(Node->Handle, AsciiMode, UnicodeMode);
              if (EFI_ERROR(Status)) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TYP_ERROR), gShellLevel3HiiHandle, Node->FileName, Status);
                ShellStatus = SHELL_INVALID_PARAMETER;
              }
              ASSERT(ShellStatus == SHELL_SUCCESS);
            }
          }
        }
        //
        // Free the fileList
        //
        if (FileList != NULL && !IsListEmpty(&FileList->Link)) {
          Status = ShellCloseFileMetaArg(&FileList);
        }
        ASSERT_EFI_ERROR(Status);
        FileList = NULL;
      }
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  if (ShellGetExecutionBreakFlag()) {
    return (SHELL_ABORTED);
  }

  return (ShellStatus);
}

