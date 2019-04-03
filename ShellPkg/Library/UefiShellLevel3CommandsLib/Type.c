/** @file
  Main file for Type shell level 3 function.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
TypeFileByHandle (
  IN SHELL_FILE_HANDLE Handle,
  IN BOOLEAN Ascii,
  IN BOOLEAN UCS2
  )
{
  UINTN       ReadSize;
  VOID        *Buffer;
  VOID        *AllocatedBuffer;
  EFI_STATUS  Status;
  UINTN       LoopVar;
  UINTN       LoopSize;
  CHAR16      AsciiChar;
  CHAR16      Ucs2Char;

  ReadSize = PcdGet32(PcdShellFileOperationSize);
  AllocatedBuffer = AllocateZeroPool(ReadSize);
  if (AllocatedBuffer == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  Status = ShellSetFilePosition(Handle, 0);
  ASSERT_EFI_ERROR(Status);

  while (ReadSize == ((UINTN)PcdGet32(PcdShellFileOperationSize))) {
    Buffer = AllocatedBuffer;
    ZeroMem(Buffer, ReadSize);
    Status = ShellReadFile(Handle, &ReadSize, Buffer);
    if (EFI_ERROR(Status)){
      break;
    }

    if (!(Ascii|UCS2)) {
      if (*(UINT16*)Buffer == gUnicodeFileTag) {
        UCS2 = TRUE;
      } else {
        Ascii = TRUE;
      }
    }

    if (Ascii) {
      LoopSize = ReadSize;
      for (LoopVar = 0 ; LoopVar < LoopSize ; LoopVar++) {
        //
        // The valid range of ASCII characters is 0x20-0x7E.
        // Display "." when there is an invalid character.
        //
        AsciiChar = CHAR_NULL;
        AsciiChar = ((CHAR8*)Buffer)[LoopVar];
        if (AsciiChar == '\r' || AsciiChar == '\n') {
          //
          // Allow Line Feed (LF) (0xA) & Carriage Return (CR) (0xD)
          // characters to be displayed as is.
          //
          if (AsciiChar == '\n' && ((CHAR8*)Buffer)[LoopVar-1] != '\r') {
            //
            // In case Line Feed (0xA) is encountered & Carriage Return (0xD)
            // was not the previous character, print CR and LF. This is because
            // Shell 2.0 requires carriage return with line feed for displaying
            // each new line from left.
            //
            ShellPrintEx (-1, -1, L"\r\n");
            continue;
          }
        } else {
          //
          // For all other characters which are not printable, display '.'
          //
          if (AsciiChar < 0x20 || AsciiChar >= 0x7F) {
            AsciiChar = '.';
          }
        }
        ShellPrintEx (-1, -1, L"%c", AsciiChar);
      }
    } else {
      if (*(UINT16*)Buffer == gUnicodeFileTag) {
        //
        // For unicode files, skip displaying the byte order marker.
        //
        Buffer = ((UINT16*)Buffer) + 1;
        LoopSize = (ReadSize / (sizeof (CHAR16))) - 1;
      } else {
        LoopSize = ReadSize / (sizeof (CHAR16));
      }

      for (LoopVar = 0 ; LoopVar < LoopSize ; LoopVar++) {
        //
        // An invalid range of characters is 0x0-0x1F.
        // Display "." when there is an invalid character.
        //
        Ucs2Char = CHAR_NULL;
        Ucs2Char = ((CHAR16*)Buffer)[LoopVar];
        if (Ucs2Char == '\r' || Ucs2Char == '\n') {
          //
          // Allow Line Feed (LF) (0xA) & Carriage Return (CR) (0xD)
          // characters to be displayed as is.
          //
          if (Ucs2Char == '\n' && ((CHAR16*)Buffer)[LoopVar-1] != '\r') {
            //
            // In case Line Feed (0xA) is encountered & Carriage Return (0xD)
            // was not the previous character, print CR and LF. This is because
            // Shell 2.0 requires carriage return with line feed for displaying
            // each new line from left.
            //
            ShellPrintEx (-1, -1, L"\r\n");
            continue;
          }
        }
        else if (Ucs2Char < 0x20) {
          //
          // For all other characters which are not printable, display '.'
          //
          Ucs2Char = L'.';
        }
        ShellPrintEx (-1, -1, L"%c", Ucs2Char);
      }
    }

    if (ShellGetExecutionBreakFlag()) {
      break;
    }
  }
  FreePool (AllocatedBuffer);
  ShellPrintEx (-1, -1, L"\r\n");
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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"type", ProblemParam);
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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"type", L"-a & -u");
      ShellStatus = SHELL_INVALID_PARAMETER;
   } else if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      //
      // we insufficient parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle, L"type");
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
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, L"type", (CHAR16*)Param);
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
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel3HiiHandle, L"type", Param);
            continue;
          } else {
            //
            // loop through the list and make sure we are not aborting...
            //
            for ( Node = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link)
                ; !IsNull(&FileList->Link, &Node->Link) && !ShellGetExecutionBreakFlag()
                ; Node = (EFI_SHELL_FILE_INFO*)GetNextNode(&FileList->Link, &Node->Link)
               ){

              if (ShellGetExecutionBreakFlag()) {
                break;
              }

              //
              // make sure the file opened ok
              //
              if (EFI_ERROR(Node->Status)){
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, L"type", Node->FileName);
                ShellStatus = SHELL_NOT_FOUND;
                continue;
              }

              //
              // make sure its not a directory
              //
              if (FileHandleIsDirectory(Node->Handle) == EFI_SUCCESS) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_IS_DIR), gShellLevel3HiiHandle, L"type", Node->FileName);
                ShellStatus = SHELL_NOT_FOUND;
                continue;
              }

              //
              // do it
              //
              Status = TypeFileByHandle (Node->Handle, AsciiMode, UnicodeMode);
              if (EFI_ERROR(Status)) {
                ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_TYP_ERROR), gShellLevel3HiiHandle, L"type", Node->FileName);
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

