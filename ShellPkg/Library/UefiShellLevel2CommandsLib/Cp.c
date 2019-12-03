/** @file
  Main file for cp shell level 2 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

/**
  Function to take a list of files to copy and a destination location and do
  the verification and copying of those files to that location.  This function
  will report any errors to the user and halt.

  @param[in] FileList           A LIST_ENTRY* based list of files to move.
  @param[in] DestDir            The destination location.
  @param[in] SilentMode         TRUE to eliminate screen output.
  @param[in] RecursiveMode      TRUE to copy directories.
  @param[in] Resp               The response to the overwrite query (if always).

  @retval SHELL_SUCCESS             the files were all moved.
  @retval SHELL_INVALID_PARAMETER   a parameter was invalid
  @retval SHELL_SECURITY_VIOLATION  a security violation ocurred
  @retval SHELL_WRITE_PROTECTED     the destination was write protected
  @retval SHELL_OUT_OF_RESOURCES    a memory allocation failed
**/
SHELL_STATUS
ValidateAndCopyFiles(
  IN CONST EFI_SHELL_FILE_INFO  *FileList,
  IN CONST CHAR16               *DestDir,
  IN BOOLEAN                    SilentMode,
  IN BOOLEAN                    RecursiveMode,
  IN VOID                       **Resp
  );

/**
  Function to Copy one file to another location

  If the destination exists the user will be prompted and the result put into *resp

  @param[in] Source     pointer to source file name
  @param[in] Dest       pointer to destination file name
  @param[out] Resp      pointer to response from question.  Pass back on looped calling
  @param[in] SilentMode whether to run in quiet mode or not
  @param[in] CmdName    Source command name requesting single file copy

  @retval SHELL_SUCCESS   The source file was copied to the destination
**/
SHELL_STATUS
CopySingleFile(
  IN CONST CHAR16 *Source,
  IN CONST CHAR16 *Dest,
  OUT VOID        **Resp,
  IN BOOLEAN      SilentMode,
  IN CONST CHAR16 *CmdName
  )
{
  VOID                  *Response;
  UINTN                 ReadSize;
  SHELL_FILE_HANDLE     SourceHandle;
  SHELL_FILE_HANDLE     DestHandle;
  EFI_STATUS            Status;
  VOID                  *Buffer;
  CHAR16                *TempName;
  UINTN                 Size;
  EFI_SHELL_FILE_INFO   *List;
  SHELL_STATUS          ShellStatus;
  UINT64                SourceFileSize;
  UINT64                DestFileSize;
  EFI_FILE_PROTOCOL     *DestVolumeFP;
  EFI_FILE_SYSTEM_INFO  *DestVolumeInfo;
  UINTN                 DestVolumeInfoSize;

  ASSERT(Resp != NULL);

  SourceHandle    = NULL;
  DestHandle      = NULL;
  Response        = *Resp;
  List            = NULL;
  DestVolumeInfo  = NULL;
  ShellStatus     = SHELL_SUCCESS;

  ReadSize = PcdGet32(PcdShellFileOperationSize);
  // Why bother copying a file to itself
  if (StrCmp(Source, Dest) == 0) {
    return (SHELL_SUCCESS);
  }

  //
  // if the destination file existed check response and possibly prompt user
  //
  if (ShellFileExists(Dest) == EFI_SUCCESS) {
    if (Response == NULL && !SilentMode) {
      Status = ShellPromptForResponseHii(ShellPromptResponseTypeYesNoAllCancel, STRING_TOKEN (STR_GEN_DEST_EXIST_OVR), gShellLevel2HiiHandle, &Response);
    }
    //
    // possibly return based on response
    //
    if (!SilentMode) {
      if (Response == NULL) {
        return SHELL_ABORTED;
      }
      switch (*(SHELL_PROMPT_RESPONSE*)Response) {
        case ShellPromptResponseNo:
          //
          // return success here so we dont stop the process
          //
          return (SHELL_SUCCESS);
        case ShellPromptResponseCancel:
          *Resp = Response;
          //
          // indicate to stop everything
          //
          return (SHELL_ABORTED);
        case ShellPromptResponseAll:
          *Resp = Response;
        case ShellPromptResponseYes:
          break;
        default:
          return SHELL_ABORTED;
      }
    }
  }

  if (ShellIsDirectory(Source) == EFI_SUCCESS) {
    Status = ShellCreateDirectory(Dest, &DestHandle);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_DEST_DIR_FAIL), gShellLevel2HiiHandle, CmdName, Dest);
      return (SHELL_ACCESS_DENIED);
    }

    //
    // Now copy all the files under the directory...
    //
    TempName    = NULL;
    Size        = 0;
    StrnCatGrow(&TempName, &Size, Source, 0);
    StrnCatGrow(&TempName, &Size, L"\\*", 0);
    if (TempName != NULL) {
      ShellOpenFileMetaArg((CHAR16*)TempName, EFI_FILE_MODE_READ, &List);
      *TempName = CHAR_NULL;
      StrnCatGrow(&TempName, &Size, Dest, 0);
      StrnCatGrow(&TempName, &Size, L"\\", 0);
      ShellStatus = ValidateAndCopyFiles(List, TempName, SilentMode, TRUE, Resp);
      ShellCloseFileMetaArg(&List);
      SHELL_FREE_NON_NULL(TempName);
      Size = 0;
    }
  } else {
    Status = ShellDeleteFileByName(Dest);

    //
    // open file with create enabled
    //
    Status = ShellOpenFileByName(Dest, &DestHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_DEST_OPEN_FAIL), gShellLevel2HiiHandle, CmdName, Dest);
      return (SHELL_ACCESS_DENIED);
    }

    //
    // open source file
    //
    Status = ShellOpenFileByName (Source, &SourceHandle, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_CP_SRC_OPEN_FAIL), gShellLevel2HiiHandle, CmdName, Source);
      return (SHELL_ACCESS_DENIED);
    }

    //
    //get file size of source file and freespace available on destination volume
    //
    ShellGetFileSize(SourceHandle, &SourceFileSize);
    ShellGetFileSize(DestHandle, &DestFileSize);

    //
    //if the destination file already exists then it will be replaced, meaning the sourcefile effectively needs less storage space
    //
    if(DestFileSize < SourceFileSize){
      SourceFileSize -= DestFileSize;
    } else {
      SourceFileSize = 0;
    }

    //
    //get the system volume info to check the free space
    //
    DestVolumeFP = ConvertShellHandleToEfiFileProtocol(DestHandle);
    DestVolumeInfo = NULL;
    DestVolumeInfoSize = 0;
    Status = DestVolumeFP->GetInfo(
      DestVolumeFP,
      &gEfiFileSystemInfoGuid,
      &DestVolumeInfoSize,
      DestVolumeInfo
      );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      DestVolumeInfo = AllocateZeroPool(DestVolumeInfoSize);
      Status = DestVolumeFP->GetInfo(
        DestVolumeFP,
        &gEfiFileSystemInfoGuid,
        &DestVolumeInfoSize,
        DestVolumeInfo
        );
    }

    //
    //check if enough space available on destination drive to complete copy
    //
    if (DestVolumeInfo!= NULL && (DestVolumeInfo->FreeSpace < SourceFileSize)) {
      //
      //not enough space on destination directory to copy file
      //
      SHELL_FREE_NON_NULL(DestVolumeInfo);
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_CPY_FAIL), gShellLevel2HiiHandle, CmdName);
      return(SHELL_VOLUME_FULL);
    } else {
      //
      // copy data between files
      //
      Buffer = AllocateZeroPool(ReadSize);
      if (Buffer == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellLevel2HiiHandle, CmdName);
        return SHELL_OUT_OF_RESOURCES;
      }
      while (ReadSize == PcdGet32(PcdShellFileOperationSize) && !EFI_ERROR(Status)) {
        Status = ShellReadFile(SourceHandle, &ReadSize, Buffer);
        if (!EFI_ERROR(Status)) {
          Status = ShellWriteFile(DestHandle, &ReadSize, Buffer);
          if (EFI_ERROR(Status)) {
            ShellStatus = (SHELL_STATUS) (Status & (~MAX_BIT));
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_CPY_WRITE_ERROR), gShellLevel2HiiHandle, CmdName, Dest);
            break;
          }
        } else {
          ShellStatus = (SHELL_STATUS) (Status & (~MAX_BIT));
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_CPY_READ_ERROR), gShellLevel2HiiHandle, CmdName, Source);
          break;
        }
      }
    }
    SHELL_FREE_NON_NULL(DestVolumeInfo);
  }

  //
  // close files
  //
  if (DestHandle != NULL) {
    ShellCloseFile(&DestHandle);
    DestHandle   = NULL;
  }
  if (SourceHandle != NULL) {
    ShellCloseFile(&SourceHandle);
    SourceHandle = NULL;
  }

  //
  // return
  //
  return ShellStatus;
}

/**
  function to take a list of files to copy and a destination location and do
  the verification and copying of those files to that location.  This function
  will report any errors to the user and halt.

  The key is to have this function called ONLY once.  this allows for the parameter
  verification to happen correctly.

  @param[in] FileList           A LIST_ENTRY* based list of files to move.
  @param[in] DestDir            The destination location.
  @param[in] SilentMode         TRUE to eliminate screen output.
  @param[in] RecursiveMode      TRUE to copy directories.
  @param[in] Resp               The response to the overwrite query (if always).

  @retval SHELL_SUCCESS             the files were all moved.
  @retval SHELL_INVALID_PARAMETER   a parameter was invalid
  @retval SHELL_SECURITY_VIOLATION  a security violation ocurred
  @retval SHELL_WRITE_PROTECTED     the destination was write protected
  @retval SHELL_OUT_OF_RESOURCES    a memory allocation failed
**/
SHELL_STATUS
ValidateAndCopyFiles(
  IN CONST EFI_SHELL_FILE_INFO  *FileList,
  IN CONST CHAR16               *DestDir,
  IN BOOLEAN                    SilentMode,
  IN BOOLEAN                    RecursiveMode,
  IN VOID                       **Resp
  )
{
  CHAR16                    *HiiOutput;
  CHAR16                    *HiiResultOk;
  CONST EFI_SHELL_FILE_INFO *Node;
  SHELL_STATUS              ShellStatus;
  EFI_STATUS                Status;
  CHAR16                    *DestPath;
  VOID                      *Response;
  UINTN                     PathSize;
  CONST CHAR16              *Cwd;
  UINTN                     NewSize;
  CHAR16                    *CleanFilePathStr;

  if (Resp == NULL) {
    Response = NULL;
  } else {
    Response = *Resp;
  }

  DestPath         = NULL;
  ShellStatus      = SHELL_SUCCESS;
  PathSize         = 0;
  Cwd              = ShellGetCurrentDir(NULL);
  CleanFilePathStr = NULL;

  ASSERT(FileList != NULL);
  ASSERT(DestDir  != NULL);


  Status = ShellLevel2StripQuotes (DestDir, &CleanFilePathStr);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_OUT_OF_RESOURCES) {
      return SHELL_OUT_OF_RESOURCES;
    } else {
      return SHELL_INVALID_PARAMETER;
    }
  }

  ASSERT (CleanFilePathStr != NULL);

  //
  // If we are trying to copy multiple files... make sure we got a directory for the target...
  //
  if (EFI_ERROR(ShellIsDirectory(CleanFilePathStr)) && FileList->Link.ForwardLink != FileList->Link.BackLink) {
    //
    // Error for destination not a directory
    //
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NOT_DIR), gShellLevel2HiiHandle, L"cp", CleanFilePathStr);
    FreePool (CleanFilePathStr);
    return (SHELL_INVALID_PARAMETER);
  }
  for (Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&FileList->Link)
    ;  !IsNull(&FileList->Link, &Node->Link)
    ;  Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&FileList->Link, &Node->Link)
    ){
    //
    // skip the directory traversing stuff...
    //
    if (StrCmp(Node->FileName, L".") == 0 || StrCmp(Node->FileName, L"..") == 0) {
      continue;
    }

    NewSize =  StrSize(CleanFilePathStr);
    NewSize += StrSize(Node->FullName);
    NewSize += (Cwd == NULL)? 0 : (StrSize(Cwd) + sizeof(CHAR16));
    if (NewSize > PathSize) {
      PathSize = NewSize;
    }

    //
    // Make sure got -r if required
    //
    if (!RecursiveMode && !EFI_ERROR(ShellIsDirectory(Node->FullName))) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_DIR_REQ), gShellLevel2HiiHandle, L"cp");
      FreePool (CleanFilePathStr);
      return (SHELL_INVALID_PARAMETER);
    }

    //
    // make sure got dest as dir if needed
    //
    if (!EFI_ERROR(ShellIsDirectory(Node->FullName)) && EFI_ERROR(ShellIsDirectory(CleanFilePathStr))) {
      //
      // Error for destination not a directory
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NOT_DIR), gShellLevel2HiiHandle, L"cp", CleanFilePathStr);
      FreePool (CleanFilePathStr);
      return (SHELL_INVALID_PARAMETER);
    }
  }

  HiiOutput   = HiiGetString (gShellLevel2HiiHandle, STRING_TOKEN (STR_CP_OUTPUT), NULL);
  HiiResultOk = HiiGetString (gShellLevel2HiiHandle, STRING_TOKEN (STR_GEN_RES_OK), NULL);
  DestPath    = AllocateZeroPool(PathSize);

  if (DestPath == NULL || HiiOutput == NULL || HiiResultOk == NULL) {
    SHELL_FREE_NON_NULL(DestPath);
    SHELL_FREE_NON_NULL(HiiOutput);
    SHELL_FREE_NON_NULL(HiiResultOk);
    FreePool (CleanFilePathStr);
    return (SHELL_OUT_OF_RESOURCES);
  }

  //
  // Go through the list of files to copy...
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

    if (FileList->Link.ForwardLink == FileList->Link.BackLink // 1 item
      && EFI_ERROR(ShellIsDirectory(CleanFilePathStr))                 // not an existing directory
      ) {
      if (StrStr(CleanFilePathStr, L":") == NULL) {
        //
        // simple copy of a single file
        //
        if (Cwd != NULL) {
          StrCpyS(DestPath, PathSize / sizeof(CHAR16), Cwd);
          StrCatS(DestPath, PathSize / sizeof(CHAR16), L"\\");
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_DIR_NF), gShellLevel2HiiHandle, L"cp", CleanFilePathStr);
          FreePool (CleanFilePathStr);
          return (SHELL_INVALID_PARAMETER);
        }
        if (DestPath[StrLen(DestPath)-1] != L'\\' && CleanFilePathStr[0] != L'\\') {
          StrCatS(DestPath, PathSize / sizeof(CHAR16), L"\\");
        } else if (DestPath[StrLen(DestPath)-1] == L'\\' && CleanFilePathStr[0] == L'\\') {
          ((CHAR16*)DestPath)[StrLen(DestPath)-1] = CHAR_NULL;
        }
        StrCatS(DestPath, PathSize/sizeof(CHAR16), CleanFilePathStr);
      } else {
        StrCpyS(DestPath, PathSize/sizeof(CHAR16), CleanFilePathStr);
      }
    } else {
      //
      // we have multiple files or a directory in the DestDir
      //

      //
      // Check for leading slash
      //
      if (CleanFilePathStr[0] == L'\\') {
         //
         // Copy to the root of CWD
         //
        if (Cwd != NULL) {
          StrCpyS(DestPath, PathSize/sizeof(CHAR16), Cwd);
          StrCatS(DestPath, PathSize/sizeof(CHAR16), L"\\");
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_DIR_NF), gShellLevel2HiiHandle, L"cp",  CleanFilePathStr);
          FreePool(CleanFilePathStr);
          return (SHELL_INVALID_PARAMETER);
        }
        while (PathRemoveLastItem(DestPath));
        StrCatS(DestPath, PathSize/sizeof(CHAR16), CleanFilePathStr+1);
        StrCatS(DestPath, PathSize/sizeof(CHAR16), Node->FileName);
      } else if (StrStr(CleanFilePathStr, L":") == NULL) {
        if (Cwd != NULL) {
          StrCpyS(DestPath, PathSize/sizeof(CHAR16), Cwd);
          StrCatS(DestPath, PathSize/sizeof(CHAR16), L"\\");
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_DIR_NF), gShellLevel2HiiHandle, L"cp", CleanFilePathStr);
          FreePool(CleanFilePathStr);
          return (SHELL_INVALID_PARAMETER);
        }
        if (DestPath[StrLen(DestPath)-1] != L'\\' && CleanFilePathStr[0] != L'\\') {
          StrCatS(DestPath, PathSize/sizeof(CHAR16), L"\\");
        } else if (DestPath[StrLen(DestPath)-1] == L'\\' && CleanFilePathStr[0] == L'\\') {
          ((CHAR16*)DestPath)[StrLen(DestPath)-1] = CHAR_NULL;
        }
        StrCatS(DestPath, PathSize/sizeof(CHAR16), CleanFilePathStr);
        if (CleanFilePathStr[StrLen(CleanFilePathStr)-1] != L'\\' && Node->FileName[0] != L'\\') {
          StrCatS(DestPath, PathSize/sizeof(CHAR16), L"\\");
        } else if (CleanFilePathStr[StrLen(CleanFilePathStr)-1] == L'\\' && Node->FileName[0] == L'\\') {
          ((CHAR16*)DestPath)[StrLen(DestPath)-1] = CHAR_NULL;
        }
        StrCatS(DestPath, PathSize/sizeof(CHAR16), Node->FileName);

      } else {
        StrCpyS(DestPath, PathSize/sizeof(CHAR16), CleanFilePathStr);
        if (CleanFilePathStr[StrLen(CleanFilePathStr)-1] != L'\\' && Node->FileName[0] != L'\\') {
          StrCatS(DestPath, PathSize/sizeof(CHAR16), L"\\");
        } else if (CleanFilePathStr[StrLen(CleanFilePathStr)-1] == L'\\' && Node->FileName[0] == L'\\') {
          ((CHAR16*)CleanFilePathStr)[StrLen(CleanFilePathStr)-1] = CHAR_NULL;
        }
        StrCatS(DestPath, PathSize/sizeof(CHAR16), Node->FileName);
      }
    }

    //
    // Make sure the path exists
    //
    if (EFI_ERROR(VerifyIntermediateDirectories(DestPath))) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_DIR_WNF), gShellLevel2HiiHandle, L"cp", DestPath);
      ShellStatus = SHELL_DEVICE_ERROR;
      break;
    }

    if ( !EFI_ERROR(ShellIsDirectory(Node->FullName))
      && !EFI_ERROR(ShellIsDirectory(DestPath))
      && StrniCmp(Node->FullName, DestPath, StrLen(DestPath)) == 0
      ){
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_SD_PARENT), gShellLevel2HiiHandle, L"cp");
      ShellStatus = SHELL_INVALID_PARAMETER;
      break;
    }
    if (StringNoCaseCompare(&Node->FullName, &DestPath) == 0) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_SD_SAME), gShellLevel2HiiHandle, L"cp");
      ShellStatus = SHELL_INVALID_PARAMETER;
      break;
    }

    if ((StrniCmp(Node->FullName, DestPath, StrLen(Node->FullName)) == 0)
      && (DestPath[StrLen(Node->FullName)] == CHAR_NULL || DestPath[StrLen(Node->FullName)] == L'\\')
      ) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_SD_SAME), gShellLevel2HiiHandle, L"cp");
      ShellStatus = SHELL_INVALID_PARAMETER;
      break;
    }

    PathCleanUpDirectories(DestPath);

    if (!SilentMode) {
      ShellPrintEx(-1, -1, HiiOutput, Node->FullName, DestPath);
    }

    //
    // copy single file...
    //
    ShellStatus = CopySingleFile(Node->FullName, DestPath, &Response, SilentMode, L"cp");
    if (ShellStatus != SHELL_SUCCESS) {
      break;
    }
  }
  if (ShellStatus == SHELL_SUCCESS && Resp == NULL) {
    ShellPrintEx(-1, -1, L"%s", HiiResultOk);
  }

  SHELL_FREE_NON_NULL(DestPath);
  SHELL_FREE_NON_NULL(HiiOutput);
  SHELL_FREE_NON_NULL(HiiResultOk);
  SHELL_FREE_NON_NULL(CleanFilePathStr);
  if (Resp == NULL) {
    SHELL_FREE_NON_NULL(Response);
  }

  return (ShellStatus);

}

/**
  Validate and if successful copy all the files from the list into
  destination directory.

  @param[in] FileList       The list of files to copy.
  @param[in] DestDir        The directory to copy files to.
  @param[in] SilentMode     TRUE to eliminate screen output.
  @param[in] RecursiveMode  TRUE to copy directories.

  @retval SHELL_INVALID_PARAMETER   A parameter was invalid.
  @retval SHELL_SUCCESS             The operation was successful.
**/
SHELL_STATUS
ProcessValidateAndCopyFiles(
  IN       EFI_SHELL_FILE_INFO  *FileList,
  IN CONST CHAR16               *DestDir,
  IN BOOLEAN                    SilentMode,
  IN BOOLEAN                    RecursiveMode
  )
{
  SHELL_STATUS        ShellStatus;
  EFI_SHELL_FILE_INFO *List;
  EFI_FILE_INFO       *FileInfo;
  CHAR16              *FullName;

  List      = NULL;
  FullName  = NULL;
  FileInfo  = NULL;

  ShellOpenFileMetaArg((CHAR16*)DestDir, EFI_FILE_MODE_READ, &List);
  if (List != NULL && List->Link.ForwardLink != List->Link.BackLink) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_MARG_ERROR), gShellLevel2HiiHandle, L"cp", DestDir);
    ShellStatus = SHELL_INVALID_PARAMETER;
    ShellCloseFileMetaArg(&List);
  } else if (List != NULL) {
    ASSERT(((EFI_SHELL_FILE_INFO *)List->Link.ForwardLink) != NULL);
    ASSERT(((EFI_SHELL_FILE_INFO *)List->Link.ForwardLink)->FullName != NULL);
    FileInfo = gEfiShellProtocol->GetFileInfo(((EFI_SHELL_FILE_INFO *)List->Link.ForwardLink)->Handle);
    ASSERT(FileInfo != NULL);
    StrnCatGrow(&FullName, NULL, ((EFI_SHELL_FILE_INFO *)List->Link.ForwardLink)->FullName, 0);
    ShellCloseFileMetaArg(&List);
    if ((FileInfo->Attribute & EFI_FILE_READ_ONLY) == 0) {
      ShellStatus = ValidateAndCopyFiles(FileList, FullName, SilentMode, RecursiveMode, NULL);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CP_DEST_ERROR), gShellLevel2HiiHandle, L"cp");
      ShellStatus = SHELL_ACCESS_DENIED;
    }
  } else {
    ShellCloseFileMetaArg(&List);
    ShellStatus = ValidateAndCopyFiles(FileList, DestDir, SilentMode, RecursiveMode, NULL);
  }

  SHELL_FREE_NON_NULL(FileInfo);
  SHELL_FREE_NON_NULL(FullName);
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-r", TypeFlag},
  {L"-q", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'cp' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunCp (
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
  BOOLEAN             SilentMode;
  BOOLEAN             RecursiveMode;
  CONST CHAR16        *Cwd;
  CHAR16              *FullCwd;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"cp", ProblemParam);
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

    //
    // Initialize SilentMode and RecursiveMode
    //
    if (gEfiShellProtocol->BatchIsActive()) {
      SilentMode = TRUE;
    } else {
      SilentMode = ShellCommandLineGetFlag(Package, L"-q");
    }
    RecursiveMode = ShellCommandLineGetFlag(Package, L"-r");

    switch (ParamCount = ShellCommandLineGetCount(Package)) {
      case 0:
      case 1:
        //
        // we have insufficient parameters
        //
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"cp");
        ShellStatus = SHELL_INVALID_PARAMETER;
        break;
      case 2:
        //
        // must have valid CWD for single parameter...
        //
        Cwd = ShellGetCurrentDir(NULL);
        if (Cwd == NULL){
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle, L"cp");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Status = ShellOpenFileMetaArg((CHAR16*)ShellCommandLineGetRawValue(Package, 1), EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
          if (FileList == NULL || IsListEmpty(&FileList->Link) || EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, L"cp", ShellCommandLineGetRawValue(Package, 1));
            ShellStatus = SHELL_NOT_FOUND;
          } else  {
            FullCwd = AllocateZeroPool(StrSize(Cwd) + sizeof(CHAR16));
            if (FullCwd == NULL) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellLevel2HiiHandle, L"cp");
              ShellStatus = SHELL_OUT_OF_RESOURCES;
            } else {
              StrCpyS (FullCwd, StrSize (Cwd) / sizeof (CHAR16) + 1, Cwd);
              ShellStatus = ProcessValidateAndCopyFiles (FileList, FullCwd, SilentMode, RecursiveMode);
              FreePool (FullCwd);
            }
          }
        }

        break;
      default:
        //
        // Make a big list of all the files...
        //
        for (ParamCount--, LoopCounter = 1 ; LoopCounter < ParamCount && ShellStatus == SHELL_SUCCESS ; LoopCounter++) {
          if (ShellGetExecutionBreakFlag()) {
            break;
          }
          Status = ShellOpenFileMetaArg((CHAR16*)ShellCommandLineGetRawValue(Package, LoopCounter), EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
          if (EFI_ERROR(Status) || FileList == NULL || IsListEmpty(&FileList->Link)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, L"cp", ShellCommandLineGetRawValue(Package, LoopCounter));
            ShellStatus = SHELL_NOT_FOUND;
          }
        }
        if (ShellStatus != SHELL_SUCCESS) {
          Status = ShellCloseFileMetaArg(&FileList);
        } else {
          //
          // now copy them all...
          //
          if (FileList != NULL && !IsListEmpty(&FileList->Link)) {
            ShellStatus = ProcessValidateAndCopyFiles(FileList, PathCleanUpDirectories((CHAR16*)ShellCommandLineGetRawValue(Package, ParamCount)), SilentMode, RecursiveMode);
            Status = ShellCloseFileMetaArg(&FileList);
            if (EFI_ERROR(Status) && ShellStatus == SHELL_SUCCESS) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_FILE), gShellLevel2HiiHandle, L"cp", ShellCommandLineGetRawValue(Package, ParamCount), ShellStatus|MAX_BIT);
              ShellStatus = SHELL_ACCESS_DENIED;
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

  if (ShellGetExecutionBreakFlag()) {
    return (SHELL_ABORTED);
  }

  return (ShellStatus);
}

