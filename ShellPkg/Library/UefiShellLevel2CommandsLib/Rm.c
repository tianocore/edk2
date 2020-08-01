/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-q", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Determine if a directory has no files in it.

  @param[in] FileHandle   The EFI_HANDLE to the directory.

  @retval TRUE  The directory has no files (or directories).
  @retval FALSE The directory has at least 1 file or directory in it.
**/
BOOLEAN
IsDirectoryEmpty (
  IN SHELL_FILE_HANDLE   FileHandle
  )
{
  EFI_STATUS      Status;
  EFI_FILE_INFO   *FileInfo;
  BOOLEAN         NoFile;
  BOOLEAN         RetVal;

  RetVal = TRUE;
  NoFile = FALSE;
  FileInfo = NULL;

  for (Status = FileHandleFindFirstFile(FileHandle, &FileInfo)
    ;  !NoFile && !EFI_ERROR (Status)
    ;  FileHandleFindNextFile(FileHandle, FileInfo, &NoFile)
   ){
    if (StrStr(FileInfo->FileName, L".") != FileInfo->FileName
      &&StrStr(FileInfo->FileName, L"..") != FileInfo->FileName) {
      RetVal = FALSE;
    }
  }
  return (RetVal);
}

/**
  Delete a node and all nodes under it (including sub directories).

  @param[in] Node   The node to start deleting with.
  @param[in] Quiet  TRUE to print no messages.

  @retval SHELL_SUCCESS       The operation was successful.
  @retval SHELL_ACCESS_DENIED A file was read only.
  @retval SHELL_ABORTED       The abort message was received.
  @retval SHELL_DEVICE_ERROR  A device error occurred reading this Node.
**/
SHELL_STATUS
CascadeDelete(
  IN EFI_SHELL_FILE_INFO  *Node,
  IN CONST BOOLEAN        Quiet
  )
{
  SHELL_STATUS          ShellStatus;
  EFI_SHELL_FILE_INFO   *List;
  EFI_SHELL_FILE_INFO   *Node2;
  EFI_STATUS            Status;
  SHELL_PROMPT_RESPONSE *Resp;
  CHAR16                *TempName;
  UINTN                 NewSize;

  Resp                  = NULL;
  ShellStatus           = SHELL_SUCCESS;
  List                  = NULL;
  Status                = EFI_SUCCESS;

  if ((Node->Info->Attribute & EFI_FILE_READ_ONLY) == EFI_FILE_READ_ONLY) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_RM_LOG_DETELE_RO), gShellLevel2HiiHandle, L"rm", Node->FullName);
    return (SHELL_ACCESS_DENIED);
  }

  if ((Node->Info->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY) {
    if (!IsDirectoryEmpty(Node->Handle)) {
      if (!Quiet) {
        Status = ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_RM_LOG_DELETE_CONF), gShellLevel2HiiHandle, Node->FullName);
        Status = ShellPromptForResponse(ShellPromptResponseTypeYesNo, NULL, (VOID**)&Resp);
        ASSERT(Resp != NULL);
        if (EFI_ERROR(Status) || *Resp != ShellPromptResponseYes) {
          SHELL_FREE_NON_NULL(Resp);
          return (SHELL_ABORTED);
        }
        SHELL_FREE_NON_NULL(Resp);
      }
      //
      // empty out the directory
      //
      Status = gEfiShellProtocol->FindFilesInDir(Node->Handle, &List);
      if (EFI_ERROR(Status)) {
        if (List!=NULL) {
          gEfiShellProtocol->FreeFileList(&List);
        }
        return (SHELL_DEVICE_ERROR);
      }
      for (Node2 = (EFI_SHELL_FILE_INFO   *)GetFirstNode(&List->Link)
        ;  !IsNull(&List->Link, &Node2->Link)
        ;  Node2 = (EFI_SHELL_FILE_INFO   *)GetNextNode(&List->Link, &Node2->Link)
       ){
        //
        // skip the directory traversing stuff...
        //
        if (StrCmp(Node2->FileName, L".") == 0 || StrCmp(Node2->FileName, L"..") == 0) {
          continue;
        }
        Node2->Status = gEfiShellProtocol->OpenFileByName (Node2->FullName, &Node2->Handle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE);
        if (EFI_ERROR(Node2->Status) && StrStr(Node2->FileName, L":") == NULL) {
          //
          // Update the node filename to have full path with file system identifier
          //
          NewSize = StrSize(Node->FullName) + StrSize(Node2->FullName);
          TempName = AllocateZeroPool(NewSize);
          if (TempName == NULL) {
            ShellStatus = SHELL_OUT_OF_RESOURCES;
          } else {
            StrCpyS(TempName, NewSize/sizeof(CHAR16), Node->FullName);
            TempName[StrStr(TempName, L":")+1-TempName] = CHAR_NULL;
            StrCatS(TempName, NewSize/sizeof(CHAR16), Node2->FullName);
            FreePool((VOID*)Node2->FullName);
            Node2->FullName = TempName;

            //
            // Now try again to open the file
            //
            Node2->Status = gEfiShellProtocol->OpenFileByName (Node2->FullName, &Node2->Handle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE);
          }
        }
        if (!EFI_ERROR(Node2->Status)) {
          ShellStatus = CascadeDelete(Node2, Quiet);
        } else if (ShellStatus == SHELL_SUCCESS) {
          ShellStatus = (SHELL_STATUS)(Node2->Status&(~0x80000000));
        }
        if (ShellStatus != SHELL_SUCCESS) {
          if (List!=NULL) {
            gEfiShellProtocol->FreeFileList(&List);
          }
          return (ShellStatus);
        }
      }
      if (List!=NULL) {
        gEfiShellProtocol->FreeFileList(&List);
      }
    }
  }

  if (!(StrCmp(Node->FileName, L".") == 0 || StrCmp(Node->FileName, L"..") == 0)) {
    //
    // now delete the current node...
    //
    if (!Quiet) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_RM_LOG_DELETE), gShellLevel2HiiHandle, Node->FullName);
    }
    Status = gEfiShellProtocol->DeleteFile(Node->Handle);
    Node->Handle = NULL;
  }

  //
  // We cant allow for the warning here! (Dont use EFI_ERROR Macro).
  //
  if (Status != EFI_SUCCESS){
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_RM_LOG_DELETE_ERR), gShellLevel2HiiHandle, Status);
    return (SHELL_ACCESS_DENIED);
  } else {
    if (!Quiet) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_RM_LOG_DELETE_COMP), gShellLevel2HiiHandle);
    }
    return (SHELL_SUCCESS);
  }
}

/**
  Determines if a Node is a valid delete target.  Will prevent deleting the root directory.

  @param[in] List       RESERVED.  Not used.
  @param[in] Node       The node to analyze.
  @param[in] Package    RESERVED.  Not used.
**/
BOOLEAN
IsValidDeleteTarget(
  IN CONST EFI_SHELL_FILE_INFO  *List,
  IN CONST EFI_SHELL_FILE_INFO  *Node,
  IN CONST LIST_ENTRY           *Package
  )
{
  CONST CHAR16        *TempLocation;
  BOOLEAN             RetVal;
  CHAR16              *SearchString;
  CHAR16              *Pattern;
  UINTN               Size;

  if (Node == NULL || Node->FullName == NULL) {
    return (FALSE);
  }

  TempLocation = StrStr(Node->FullName, L":");
  if (StrLen(TempLocation) <= 2) {
    //
    // Deleting the root directory is invalid.
    //
    return (FALSE);
  }

  TempLocation = ShellGetCurrentDir(NULL);
  if (TempLocation == NULL) {
    //
    // No working directory is specified so whatever is left is ok.
    //
    return (TRUE);
  }

  Pattern       = NULL;
  SearchString  = NULL;
  Size          = 0;
  Pattern       = StrnCatGrow(&Pattern, &Size, TempLocation  , 0);
  Pattern       = StrnCatGrow(&Pattern, &Size, L"\\"  , 0);
  Size = 0;
  SearchString  = StrnCatGrow(&SearchString, &Size, Node->FullName, 0);
  if (!EFI_ERROR(ShellIsDirectory(SearchString))) {
    SearchString  = StrnCatGrow(&SearchString, &Size, L"\\", 0);
    SearchString  = StrnCatGrow(&SearchString, &Size, L"*", 0);
  }

  if (Pattern == NULL || SearchString == NULL) {
    RetVal = FALSE;
  } else {
    RetVal = TRUE;
    if (gUnicodeCollation->MetaiMatch(gUnicodeCollation, Pattern, SearchString)) {
      RetVal = FALSE;
    }
  }

  SHELL_FREE_NON_NULL(Pattern     );
  SHELL_FREE_NON_NULL(SearchString);

  return (RetVal);
}

/**
  Function for 'rm' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunRm (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS            Status;
  LIST_ENTRY            *Package;
  CHAR16                *ProblemParam;
  CONST CHAR16          *Param;
  SHELL_STATUS          ShellStatus;
  UINTN                 ParamCount;
  EFI_SHELL_FILE_INFO   *FileList;
  EFI_SHELL_FILE_INFO   *Node;

  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  ParamCount          = 0;
  FileList            = NULL;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"rm", ProblemParam);
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
    if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      //
      // we insufficient parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"rm");
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
        Status = ShellOpenFileMetaArg((CHAR16*)Param, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
        if (EFI_ERROR(Status) || FileList == NULL || IsListEmpty(&FileList->Link)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, L"rm", (CHAR16*)Param);
          ShellStatus = SHELL_NOT_FOUND;
          break;
        }
      }

      if (ShellStatus == SHELL_SUCCESS){
        //
        // loop through the list and make sure we are not aborting...
        //
        for ( Node = (EFI_SHELL_FILE_INFO*)GetFirstNode(&FileList->Link)
            ; !IsNull(&FileList->Link, &Node->Link) && !ShellGetExecutionBreakFlag()
            ; Node = (EFI_SHELL_FILE_INFO*)GetNextNode(&FileList->Link, &Node->Link)
           ){
          //
          // skip the directory traversing stuff...
          //
          if (StrCmp(Node->FileName, L".") == 0 || StrCmp(Node->FileName, L"..") == 0) {
            continue;
          }

          //
          // do the deleting of nodes
          //
          if (EFI_ERROR(Node->Status)){
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_RM_LOG_DELETE_ERR2), gShellLevel2HiiHandle, Node->Status);
            ShellStatus = SHELL_ACCESS_DENIED;
            break;
          }
          if (!IsValidDeleteTarget(FileList, Node, Package)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_RM_LOG_DELETE_ERR3), gShellLevel2HiiHandle, Node->FullName);
            ShellStatus = SHELL_INVALID_PARAMETER;
            break;
          }

          ShellStatus = CascadeDelete(Node, ShellCommandLineGetFlag(Package, L"-q"));
        }
      }
      //
      // Free the fileList
      //
      if (FileList != NULL) {
        Status = ShellCloseFileMetaArg(&FileList);
      }
      FileList = NULL;
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  return (ShellStatus);
}

