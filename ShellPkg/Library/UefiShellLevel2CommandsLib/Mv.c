/** @file
  Main file for mv shell level 2 function.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Function to validate that moving a specific file (FileName) to a specific
  location (DestPath) is valid.

  This function will verify that the destination is not a subdirectory of
  FullName, that the Current working Directory is not being moved, and that
  the directory is not read only.

  if the move is invalid this function will report the error to StdOut.

  @param FullName [in]    The name of the file to move.
  @param Cwd      [in]    The current working directory
  @param DestPath [in]    The target location to move to
  @param Attribute[in]    The Attribute of the file

  @retval TRUE        The move is valid
  @retval FALSE       The move is not
**/
BOOLEAN
EFIAPI
IsValidMove(
  IN CONST CHAR16   *FullName,
  IN CONST CHAR16   *Cwd,
  IN CONST CHAR16   *DestPath,
  IN CONST UINT64   Attribute
  )
{
  CHAR16  *Test;
  CHAR16  *Test1;
  CHAR16  *TestWalker;
  INTN    Result;
  UINTN   TempLen;
  if (Cwd != NULL && StrCmp(FullName, Cwd) == 0) {
    //
    // Invalid move
    //
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MV_INV_CWD), gShellLevel2HiiHandle);
    return (FALSE);
  }
  Test = NULL;
  Test = StrnCatGrow(&Test, NULL, DestPath, 0);
  TestWalker = Test;
  ASSERT(TestWalker != NULL);
  while(*TestWalker == L'\\') {
    TestWalker++;
  }
  while(TestWalker != NULL && TestWalker[StrLen(TestWalker)-1] == L'\\') {
    TestWalker[StrLen(TestWalker)-1] = CHAR_NULL;
  }
  ASSERT(TestWalker != NULL);
  ASSERT(FullName   != NULL);
  if (StrStr(FullName, TestWalker) != 0) {
    TempLen = StrLen(FullName);
    if (StrStr(FullName, TestWalker) != FullName                    // not the first items... (could below it)
      && TempLen <= (StrLen(TestWalker) + 1)
      && StrStr(FullName+StrLen(TestWalker) + 1, L"\\") == NULL) {
      //
      // Invalid move
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MV_INV_SUB), gShellLevel2HiiHandle);
      FreePool(Test);
      return (FALSE);
    }
  }
  FreePool(Test);
  if (StrStr(DestPath, FullName) != 0 && StrStr(DestPath, FullName) != DestPath) {
    //
    // Invalid move
    //
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MV_INV_SUB), gShellLevel2HiiHandle);
    return (FALSE);
  }
  if ((Attribute & EFI_FILE_READ_ONLY) != 0) {
    //
    // invalid to move read only
    //
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MV_INV_RO), gShellLevel2HiiHandle);
    return (FALSE);
  }
  Test  = StrStr(FullName, L":");
  Test1 = StrStr(DestPath, L":");
  if (Test1 != NULL && Test  != NULL) {
    *Test  = CHAR_NULL;
    *Test1 = CHAR_NULL;
    Result = StringNoCaseCompare(&FullName, &DestPath);
    *Test  = L':';
    *Test1 = L':';
    if (Result != 0) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MV_INV_FS), gShellLevel2HiiHandle);
      return (FALSE);
    }
  }
  return (TRUE);
}

/**
  Function to take a destination path that might contain wildcards and verify
  that there is only a single possible target (IE we cant have wildcards that
  have 2 possible destination).

  if the result is sucessful the caller must free *DestPathPointer.

  @param[in] DestDir               The original path to the destination.
  @param[in, out] DestPathPointer  A pointer to the callee allocated final path.
  @param[in] Cwd                   A pointer to the current working directory.

  @retval SHELL_INVALID_PARAMETER  The DestDir could not be resolved to a location.
  @retval SHELL_INVALID_PARAMETER  The DestDir could be resolved to more than 1 location.
  @retval SHELL_INVALID_PARAMETER  Cwd is required and is NULL.
  @retval SHELL_SUCCESS            The operation was sucessful.
**/
SHELL_STATUS
EFIAPI
GetDestinationLocation(
  IN CONST CHAR16               *DestDir,
  IN OUT CHAR16                 **DestPathPointer,
  IN CONST CHAR16               *Cwd
  )
{
  EFI_SHELL_FILE_INFO       *DestList;
  EFI_SHELL_FILE_INFO       *Node;
  EFI_STATUS                Status;
  CHAR16                    *DestPath;
  CHAR16                    *TempLocation;
  UINTN                     NewSize;

  DestList = NULL;
  DestPath = NULL;

  if (StrStr(DestDir, L"\\") == DestDir) {
    if (Cwd == NULL) {
      return SHELL_INVALID_PARAMETER;
    }
    DestPath = AllocateZeroPool(StrSize(Cwd));
    if (DestPath == NULL) {
      return (SHELL_OUT_OF_RESOURCES);
    }
    StrCpy(DestPath, Cwd);
    while (PathRemoveLastItem(DestPath)) ;
    *DestPathPointer =  DestPath;
    return (SHELL_SUCCESS);
  }
  //
  // get the destination path
  //
  Status = ShellOpenFileMetaArg((CHAR16*)DestDir, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE, &DestList);
  if (DestList == NULL || IsListEmpty(&DestList->Link)) {
    //
    // Not existing... must be renaming
    //
    if ((TempLocation = StrStr(DestDir, L":")) == NULL) {
      if (Cwd == NULL) {
        ShellCloseFileMetaArg(&DestList);
        return (SHELL_INVALID_PARAMETER);
      }
      NewSize = StrSize(Cwd);
      NewSize += StrSize(DestDir);
      DestPath = AllocateZeroPool(NewSize);
      if (DestPath == NULL) {
        ShellCloseFileMetaArg(&DestList);
        return (SHELL_OUT_OF_RESOURCES);
      }
      StrCpy(DestPath, Cwd);
      if (DestPath[StrLen(DestPath)-1] != L'\\' && DestDir[0] != L'\\') {
        StrCat(DestPath, L"\\");
      } else if (DestPath[StrLen(DestPath)-1] == L'\\' && DestDir[0] == L'\\') {
        ((CHAR16*)DestPath)[StrLen(DestPath)-1] = CHAR_NULL;
      }
      StrCat(DestPath, DestDir);
    } else {
      ASSERT(DestPath == NULL);
      DestPath = StrnCatGrow(&DestPath, NULL, DestDir, 0);
      if (DestPath == NULL) {
        ShellCloseFileMetaArg(&DestList);
        return (SHELL_OUT_OF_RESOURCES);
      }
    }
  } else {
    Node = (EFI_SHELL_FILE_INFO*)GetFirstNode(&DestList->Link);
    //
    // Make sure there is only 1 node in the list.
    //
    if (!IsNodeAtEnd(&DestList->Link, &Node->Link)) {
      ShellCloseFileMetaArg(&DestList);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_MARG_ERROR), gShellLevel2HiiHandle, DestDir);
      return (SHELL_INVALID_PARAMETER);
    }
    if (ShellIsDirectory(Node->FullName)==EFI_SUCCESS) {
      DestPath = AllocateZeroPool(StrSize(Node->FullName)+sizeof(CHAR16));
      if (DestPath == NULL) {
        ShellCloseFileMetaArg(&DestList);
        return (SHELL_OUT_OF_RESOURCES);
      }
      StrCpy(DestPath, Node->FullName);
      StrCat(DestPath, L"\\");
    } else {
      //
      // cant move onto another file.
      //
      ShellCloseFileMetaArg(&DestList);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_ERROR), gShellLevel2HiiHandle, DestDir);
      return (SHELL_INVALID_PARAMETER);
    }
  }

  *DestPathPointer =  DestPath;
  ShellCloseFileMetaArg(&DestList);

  return (SHELL_SUCCESS);
}

/**
  function to take a list of files to move and a destination location and do
  the verification and moving of those files to that location.  This function
  will report any errors to the user and continue to move the rest of the files.

  @param[in] FileList           A LIST_ENTRY* based list of files to move
  @param[out] Resp              pointer to response from question.  Pass back on looped calling
  @param[in] DestDir            the destination location

  @retval SHELL_SUCCESS             the files were all moved.
  @retval SHELL_INVALID_PARAMETER   a parameter was invalid
  @retval SHELL_SECURITY_VIOLATION  a security violation ocurred
  @retval SHELL_WRITE_PROTECTED     the destination was write protected
  @retval SHELL_OUT_OF_RESOURCES    a memory allocation failed
**/
SHELL_STATUS
EFIAPI
ValidateAndMoveFiles(
  IN CONST EFI_SHELL_FILE_INFO  *FileList,
  OUT VOID                      **Resp,
  IN CONST CHAR16               *DestDir
  )
{
  EFI_STATUS                Status;
  CHAR16                    *HiiOutput;
  CHAR16                    *HiiResultOk;
  CHAR16                    *DestPath;
  CONST CHAR16              *Cwd;
  SHELL_STATUS              ShellStatus;
  CONST EFI_SHELL_FILE_INFO *Node;
  EFI_FILE_INFO             *NewFileInfo;
  CHAR16                    *TempLocation;
  UINTN                     NewSize;
  UINTN                     Length;
  VOID                      *Response;
  SHELL_FILE_HANDLE         DestHandle;

  ASSERT(FileList != NULL);
  ASSERT(DestDir  != NULL);

  DestPath = NULL;
  Cwd      = ShellGetCurrentDir(NULL);
  Response = *Resp;

  //
  // Get and validate the destination location
  //
  ShellStatus = GetDestinationLocation(DestDir, &DestPath, Cwd);
  if (ShellStatus != SHELL_SUCCESS) {
    return (ShellStatus);
  }
  DestPath = PathCleanUpDirectories(DestPath);

  HiiOutput   = HiiGetString (gShellLevel2HiiHandle, STRING_TOKEN (STR_MV_OUTPUT), NULL);
  HiiResultOk = HiiGetString (gShellLevel2HiiHandle, STRING_TOKEN (STR_GEN_RES_OK), NULL);
  ASSERT  (DestPath     != NULL);
  ASSERT  (HiiResultOk  != NULL);
  ASSERT  (HiiOutput    != NULL);
//  ASSERT  (Cwd          != NULL);

  //
  // Go through the list of files and directories to move...
  //
  for (Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&FileList->Link)
    ;  !IsNull(&FileList->Link, &Node->Link)
    ;  Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&FileList->Link, &Node->Link)
   ){
    if (ShellGetExecutionBreakFlag()) {
      break;
    }
    ASSERT(Node->FileName != NULL);
    ASSERT(Node->FullName != NULL);

    //
    // skip the directory traversing stuff...
    //
    if (StrCmp(Node->FileName, L".") == 0 || StrCmp(Node->FileName, L"..") == 0) {
      continue;
    }

    //
    // Validate that the move is valid
    //
    if (!IsValidMove(Node->FullName, Cwd, DestPath, Node->Info->Attribute)) {
      ShellStatus = SHELL_INVALID_PARAMETER;
      continue;
    }

    //
    // Chop off map info from "DestPath"
    //
    if ((TempLocation = StrStr(DestPath, L":")) != NULL) {
      CopyMem(DestPath, TempLocation+1, StrSize(TempLocation+1));
    }

    //
    // construct the new file info block
    //
    NewSize = StrSize(DestPath);
    NewSize += StrSize(Node->FileName) + SIZE_OF_EFI_FILE_INFO + sizeof(CHAR16);
    NewFileInfo = AllocateZeroPool(NewSize);
    if (NewFileInfo == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_MEM), gShellLevel2HiiHandle);
      ShellStatus = SHELL_OUT_OF_RESOURCES;
    } else {
      CopyMem(NewFileInfo, Node->Info, SIZE_OF_EFI_FILE_INFO);
      if (DestPath[0] != L'\\') {
        StrCpy(NewFileInfo->FileName, L"\\");
        StrCat(NewFileInfo->FileName, DestPath);
      } else {
        StrCpy(NewFileInfo->FileName, DestPath);
      }
      Length = StrLen(NewFileInfo->FileName);
      if (Length > 0) {
        Length--;
      }
      if (NewFileInfo->FileName[Length] == L'\\') {
        if (Node->FileName[0] == L'\\') {
          //
          // Don't allow for double slashes. Eliminate one of them.
          //
          NewFileInfo->FileName[Length] = CHAR_NULL;
        }
        StrCat(NewFileInfo->FileName, Node->FileName);
      }
      NewFileInfo->Size = SIZE_OF_EFI_FILE_INFO + StrSize(NewFileInfo->FileName);
      ShellPrintEx(-1, -1, HiiOutput, Node->FullName, NewFileInfo->FileName);

      if (!EFI_ERROR(ShellFileExists(NewFileInfo->FileName))) {
        if (Response == NULL) {
          ShellPromptForResponseHii(ShellPromptResponseTypeYesNoAllCancel, STRING_TOKEN (STR_GEN_DEST_EXIST_OVR), gShellLevel2HiiHandle, &Response);
        }
        switch (*(SHELL_PROMPT_RESPONSE*)Response) {
          case ShellPromptResponseNo:
            FreePool(NewFileInfo);
            continue;
          case ShellPromptResponseCancel:
            *Resp = Response;
            //
            // indicate to stop everything
            //
            FreePool(NewFileInfo);
            FreePool(DestPath);
            FreePool(HiiOutput);
            FreePool(HiiResultOk);
            return (SHELL_ABORTED);
          case ShellPromptResponseAll:
            *Resp = Response;
            break;
          case ShellPromptResponseYes:
            FreePool(Response);
            break;
          default:
            FreePool(Response);
            FreePool(NewFileInfo);
            FreePool(DestPath);
            FreePool(HiiOutput);
            FreePool(HiiResultOk);
            return SHELL_ABORTED;
        }
        Status = ShellOpenFileByName(NewFileInfo->FileName, &DestHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);
        ShellDeleteFile(&DestHandle);
      }


      //
      // Perform the move operation
      //
      Status = ShellSetFileInfo(Node->Handle, NewFileInfo);

      //
      // Free the info object we used...
      //
      FreePool(NewFileInfo);

      //
      // Check our result
      //
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel2HiiHandle, Status);
        //
        // move failed
        //
        switch(Status){
          default:
            ShellStatus = SHELL_INVALID_PARAMETER;
          case EFI_SECURITY_VIOLATION:
            ShellStatus = SHELL_SECURITY_VIOLATION;
          case EFI_WRITE_PROTECTED:
            ShellStatus = SHELL_WRITE_PROTECTED;
          case EFI_OUT_OF_RESOURCES:
            ShellStatus = SHELL_OUT_OF_RESOURCES;
          case EFI_DEVICE_ERROR:
            ShellStatus = SHELL_DEVICE_ERROR;
          case EFI_ACCESS_DENIED:
            ShellStatus = SHELL_ACCESS_DENIED;
        } // switch
      } else {
        ShellPrintEx(-1, -1, L"%s", HiiResultOk);
      }
    }
  } // for loop

  FreePool(DestPath);
  FreePool(HiiOutput);
  FreePool(HiiResultOk);
  return (ShellStatus);
}

/**
  Function for 'mv' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMv (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  UINTN               ParamCount;
  UINTN               LoopCounter;
  EFI_SHELL_FILE_INFO *FileList;
  VOID                *Response;

  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  ParamCount          = 0;
  FileList            = NULL;
  Response            = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
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

    switch (ParamCount = ShellCommandLineGetCount(Package)) {
      case 0:
      case 1:
        //
        // we have insufficient parameters
        //
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
        break;
      case 2:
        //
        // must have valid CWD for single parameter...
        //
        if (ShellGetCurrentDir(NULL) == NULL){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Status = ShellOpenFileMetaArg((CHAR16*)ShellCommandLineGetRawValue(Package, 1), EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
          if (FileList == NULL || IsListEmpty(&FileList->Link) || EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, ShellCommandLineGetRawValue(Package, 1));
            ShellStatus = SHELL_NOT_FOUND;
          } else  {
            //
            // ValidateAndMoveFiles will report errors to the screen itself
            //
            ShellStatus = ValidateAndMoveFiles(FileList, &Response, ShellGetCurrentDir(NULL));
          }
        }

        break;
      default:
        ///@todo make sure this works with error half way through and continues...
        for (ParamCount--, LoopCounter = 1 ; LoopCounter < ParamCount ; LoopCounter++) {
          if (ShellGetExecutionBreakFlag()) {
            break;
          }
          Status = ShellOpenFileMetaArg((CHAR16*)ShellCommandLineGetRawValue(Package, LoopCounter), EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
          if (FileList == NULL || IsListEmpty(&FileList->Link) || EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, ShellCommandLineGetRawValue(Package, LoopCounter));
            ShellStatus = SHELL_NOT_FOUND;
          } else  {
            //
            // ValidateAndMoveFiles will report errors to the screen itself
            // Only change ShellStatus if it's sucessful
            //
            if (ShellStatus == SHELL_SUCCESS) {
              ShellStatus = ValidateAndMoveFiles(FileList, &Response, ShellCommandLineGetRawValue(Package, ParamCount));
            } else {
              ValidateAndMoveFiles(FileList, &Response, ShellCommandLineGetRawValue(Package, ParamCount));
            }
          }
          if (FileList != NULL && !IsListEmpty(&FileList->Link)) {
            Status = ShellCloseFileMetaArg(&FileList);
            if (EFI_ERROR(Status) && ShellStatus == SHELL_SUCCESS) {
              ShellStatus = SHELL_ACCESS_DENIED;
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_FILE), gShellLevel2HiiHandle, ShellCommandLineGetRawValue(Package, 1), ShellStatus|MAX_BIT);
            }
          }
        }
        break;
    } // switch on parameter count

    if (FileList != NULL) {
      ShellCloseFileMetaArg(&FileList);
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  SHELL_FREE_NON_NULL(Response);

  if (ShellGetExecutionBreakFlag()) {
    return (SHELL_ABORTED);
  }

  return (ShellStatus);
}
