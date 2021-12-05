/** @file
  Main file for mv shell level 2 function.

  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  function to determine if a move is between file systems.

  @param FullName [in]    The name of the file to move.
  @param Cwd      [in]    The current working directory
  @param DestPath [in]    The target location to move to

  @retval TRUE            The move is across file system.
  @retval FALSE           The move is within a file system.
**/
BOOLEAN
IsBetweenFileSystem (
  IN CONST CHAR16  *FullName,
  IN CONST CHAR16  *Cwd,
  IN CONST CHAR16  *DestPath
  )
{
  CHAR16  *Test;
  CHAR16  *Test1;
  UINTN   Result;

  Test = StrStr (FullName, L":");
  if ((Test == NULL) && (Cwd != NULL)) {
    Test = StrStr (Cwd, L":");
  }

  Test1 = StrStr (DestPath, L":");
  if ((Test1 == NULL) && (Cwd != NULL)) {
    Test1 = StrStr (Cwd, L":");
  }

  if ((Test1 != NULL) && (Test != NULL)) {
    *Test  = CHAR_NULL;
    *Test1 = CHAR_NULL;
    Result = StringNoCaseCompare (&FullName, &DestPath);
    *Test  = L':';
    *Test1 = L':';
    if (Result != 0) {
      return (TRUE);
    }
  }

  return (FALSE);
}

/**
  function to determine if SrcPath is valid to mv.

  if SrcPath equal CWD then it's invalid.
  if SrcPath is the parent path of CWD then it's invalid.
  is SrcPath is NULL return FALSE.

  if CwdPath is NULL then ASSERT()

  @param SrcPath  [in]    The source path.
  @param CwdPath  [in]    The current working directory.

  @retval TRUE            The source path is valid.
  @retval FALSE           The source path is invalid.
**/
BOOLEAN
IsSoucePathValid (
  IN CONST CHAR16  *SrcPath,
  IN CONST CHAR16  *CwdPath
  )
{
  CHAR16   *SrcPathBuffer;
  CHAR16   *CwdPathBuffer;
  BOOLEAN  Ret;

  ASSERT (CwdPath != NULL);
  if (SrcPath == NULL) {
    return FALSE;
  }

  Ret = TRUE;

  SrcPathBuffer = AllocateCopyPool (StrSize (SrcPath), SrcPath);
  if (SrcPathBuffer == NULL) {
    return FALSE;
  }

  CwdPathBuffer = AllocateCopyPool (StrSize (CwdPath), CwdPath);
  if (CwdPathBuffer == NULL) {
    FreePool (SrcPathBuffer);
    return FALSE;
  }

  gUnicodeCollation->StrUpr (gUnicodeCollation, SrcPathBuffer);
  gUnicodeCollation->StrUpr (gUnicodeCollation, CwdPathBuffer);

  if (SrcPathBuffer[StrLen (SrcPathBuffer) -1] == L'\\') {
    SrcPathBuffer[StrLen (SrcPathBuffer) - 1] = CHAR_NULL;
  }

  if (CwdPathBuffer[StrLen (CwdPathBuffer) - 1] == L'\\') {
    CwdPathBuffer[StrLen (CwdPathBuffer) - 1] = CHAR_NULL;
  }

  if ((StrCmp (CwdPathBuffer, SrcPathBuffer) == 0) ||
      ((StrStr (CwdPathBuffer, SrcPathBuffer) == CwdPathBuffer) &&
       (CwdPathBuffer[StrLen (SrcPathBuffer)] == L'\\'))
      )
  {
    Ret = FALSE;
  }

  FreePool (SrcPathBuffer);
  FreePool (CwdPathBuffer);

  return Ret;
}

/**
  Function to validate that moving a specific file (FileName) to a specific
  location (DestPath) is valid.

  This function will verify that the destination is not a subdirectory of
  FullName, that the Current working Directory is not being moved, and that
  the directory is not read only.

  if the move is invalid this function will report the error to StdOut.

  @param SourcePath [in]    The name of the file to move.
  @param Cwd        [in]    The current working directory
  @param DestPath   [in]    The target location to move to
  @param Attribute  [in]    The Attribute of the file
  @param DestAttr   [in]    The Attribute of the destination
  @param FileStatus [in]    The Status of the file when opened

  @retval TRUE        The move is valid
  @retval FALSE       The move is not
**/
BOOLEAN
IsValidMove (
  IN CONST CHAR16      *SourcePath,
  IN CONST CHAR16      *Cwd,
  IN CONST CHAR16      *DestPath,
  IN CONST UINT64      Attribute,
  IN CONST UINT64      DestAttr,
  IN CONST EFI_STATUS  FileStatus
  )
{
  CHAR16  *DestPathCopy;
  CHAR16  *DestPathWalker;

  if ((Cwd != NULL) && ((Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY)) {
    if (!IsSoucePathValid (SourcePath, Cwd)) {
      //
      // Invalid move
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MV_INV_CWD), gShellLevel2HiiHandle);
      return FALSE;
    }
  }

  //
  // invalid to move read only or move to a read only destination
  //
  if (  ((Attribute & EFI_FILE_READ_ONLY) != 0)
     || (FileStatus == EFI_WRITE_PROTECTED)
     || ((DestAttr & EFI_FILE_READ_ONLY) != 0)
        )
  {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MV_INV_RO), gShellLevel2HiiHandle, SourcePath);
    return (FALSE);
  }

  DestPathCopy = AllocateCopyPool (StrSize (DestPath), DestPath);
  if (DestPathCopy == NULL) {
    return (FALSE);
  }

  for (DestPathWalker = DestPathCopy; *DestPathWalker == L'\\'; DestPathWalker++) {
  }

  while (DestPathWalker != NULL && DestPathWalker[StrLen (DestPathWalker)-1] == L'\\') {
    DestPathWalker[StrLen (DestPathWalker)-1] = CHAR_NULL;
  }

  ASSERT (DestPathWalker != NULL);
  ASSERT (SourcePath   != NULL);

  //
  // If they're the same, or if source is "above" dest on file path tree
  //
  if ((StringNoCaseCompare (&DestPathWalker, &SourcePath) == 0) ||
      ((StrStr (DestPathWalker, SourcePath) == DestPathWalker) &&
       (DestPathWalker[StrLen (SourcePath)] == '\\')
      )
      )
  {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MV_INV_SUB), gShellLevel2HiiHandle);
    FreePool (DestPathCopy);
    return (FALSE);
  }

  FreePool (DestPathCopy);

  return (TRUE);
}

/**
  Function to take a destination path that might contain wildcards and verify
  that there is only a single possible target (IE we cant have wildcards that
  have 2 possible destination).

  if the result is sucessful the caller must free *DestPathPointer.

  @param[in] DestParameter               The original path to the destination.
  @param[in, out] DestPathPointer  A pointer to the callee allocated final path.
  @param[in] Cwd                   A pointer to the current working directory.
  @param[in] SingleSource          TRUE to have only one source file.
  @param[in, out] DestAttr         A pointer to the destination information attribute.

  @retval SHELL_INVALID_PARAMETER  The DestParameter could not be resolved to a location.
  @retval SHELL_INVALID_PARAMETER  The DestParameter could be resolved to more than 1 location.
  @retval SHELL_INVALID_PARAMETER  Cwd is required and is NULL.
  @retval SHELL_SUCCESS            The operation was sucessful.
**/
SHELL_STATUS
GetDestinationLocation (
  IN CONST CHAR16   *DestParameter,
  IN OUT CHAR16     **DestPathPointer,
  IN CONST CHAR16   *Cwd,
  IN CONST BOOLEAN  SingleSource,
  IN OUT UINT64     *DestAttr
  )
{
  EFI_SHELL_FILE_INFO  *DestList;
  EFI_SHELL_FILE_INFO  *Node;
  CHAR16               *DestPath;
  UINTN                NewSize;
  UINTN                CurrentSize;

  DestList = NULL;
  DestPath = NULL;

  ASSERT (DestAttr != NULL);

  if (StrStr (DestParameter, L"\\") == DestParameter) {
    if (Cwd == NULL) {
      return SHELL_INVALID_PARAMETER;
    }

    DestPath = AllocateZeroPool (StrSize (Cwd));
    if (DestPath == NULL) {
      return (SHELL_OUT_OF_RESOURCES);
    }

    StrCpyS (DestPath, StrSize (Cwd) / sizeof (CHAR16), Cwd);
    while (PathRemoveLastItem (DestPath)) {
    }

    //
    // Append DestParameter beyond '\' which may be present
    //
    CurrentSize = StrSize (DestPath);
    StrnCatGrow (&DestPath, &CurrentSize, &DestParameter[1], 0);

    *DestPathPointer =  DestPath;
    return (SHELL_SUCCESS);
  }

  //
  // get the destination path
  //
  ShellOpenFileMetaArg ((CHAR16 *)DestParameter, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE, &DestList);
  if ((DestList == NULL) || IsListEmpty (&DestList->Link)) {
    //
    // Not existing... must be renaming
    //
    if (StrStr (DestParameter, L":") == NULL) {
      if (Cwd == NULL) {
        ShellCloseFileMetaArg (&DestList);
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
        return (SHELL_INVALID_PARAMETER);
      }

      NewSize  = StrSize (Cwd);
      NewSize += StrSize (DestParameter);
      DestPath = AllocateZeroPool (NewSize);
      if (DestPath == NULL) {
        ShellCloseFileMetaArg (&DestList);
        return (SHELL_OUT_OF_RESOURCES);
      }

      StrCpyS (DestPath, NewSize / sizeof (CHAR16), Cwd);
      if ((DestPath[StrLen (DestPath)-1] != L'\\') && (DestParameter[0] != L'\\')) {
        StrCatS (DestPath, NewSize / sizeof (CHAR16), L"\\");
      } else if ((DestPath[StrLen (DestPath)-1] == L'\\') && (DestParameter[0] == L'\\')) {
        ((CHAR16 *)DestPath)[StrLen (DestPath)-1] = CHAR_NULL;
      }

      StrCatS (DestPath, NewSize / sizeof (CHAR16), DestParameter);
    } else {
      ASSERT (DestPath == NULL);
      DestPath = StrnCatGrow (&DestPath, NULL, DestParameter, 0);
      if (DestPath == NULL) {
        ShellCloseFileMetaArg (&DestList);
        return (SHELL_OUT_OF_RESOURCES);
      }
    }
  } else {
    Node      = (EFI_SHELL_FILE_INFO *)GetFirstNode (&DestList->Link);
    *DestAttr = Node->Info->Attribute;
    //
    // Make sure there is only 1 node in the list.
    //
    if (!IsNodeAtEnd (&DestList->Link, &Node->Link)) {
      ShellCloseFileMetaArg (&DestList);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_MARG_ERROR), gShellLevel2HiiHandle, L"mv", DestParameter);
      return (SHELL_INVALID_PARAMETER);
    }

    //
    // If we are a directory or a single file, then one node is fine.
    //
    if ((ShellIsDirectory (Node->FullName) == EFI_SUCCESS) || SingleSource) {
      DestPath = AllocateZeroPool (StrSize (Node->FullName)+sizeof (CHAR16));
      if (DestPath == NULL) {
        ShellCloseFileMetaArg (&DestList);
        return (SHELL_OUT_OF_RESOURCES);
      }

      StrCpyS (DestPath, (StrSize (Node->FullName)+sizeof (CHAR16)) / sizeof (CHAR16), Node->FullName);
      StrCatS (DestPath, (StrSize (Node->FullName)+sizeof (CHAR16)) / sizeof (CHAR16), L"\\");
    } else {
      //
      // cant move multiple files onto a single file.
      //
      ShellCloseFileMetaArg (&DestList);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_ERROR), gShellLevel2HiiHandle, L"mv", DestParameter);
      return (SHELL_INVALID_PARAMETER);
    }
  }

  *DestPathPointer =  DestPath;
  ShellCloseFileMetaArg (&DestList);

  return (SHELL_SUCCESS);
}

/**
  Function to do a move across file systems.

  @param[in] Node               A pointer to the file to be removed.
  @param[in] DestPath           A pointer to the destination file path.
  @param[out] Resp              A pointer to response from question.  Pass back on looped calling

  @retval SHELL_SUCCESS     The source file was moved to the destination.
**/
EFI_STATUS
MoveBetweenFileSystems (
  IN EFI_SHELL_FILE_INFO  *Node,
  IN CONST CHAR16         *DestPath,
  OUT VOID                **Resp
  )
{
  SHELL_STATUS  ShellStatus;

  //
  // First we copy the file
  //
  ShellStatus = CopySingleFile (Node->FullName, DestPath, Resp, TRUE, L"mv");

  //
  // Check our result
  //
  if (ShellStatus == SHELL_SUCCESS) {
    //
    // The copy was successful.  delete the source file.
    //
    CascadeDelete (Node, TRUE);
    Node->Handle = NULL;
  } else if (ShellStatus == SHELL_ABORTED) {
    return EFI_ABORTED;
  } else if (ShellStatus == SHELL_ACCESS_DENIED) {
    return EFI_ACCESS_DENIED;
  } else if (ShellStatus == SHELL_VOLUME_FULL) {
    return EFI_VOLUME_FULL;
  } else {
    return EFI_UNSUPPORTED;
  }

  return (EFI_SUCCESS);
}

/**
  Function to take the destination path and target file name to generate the full destination path.

  @param[in] DestPath           A pointer to the destination file path string.
  @param[out] FullDestPath      A pointer to the full destination path string.
  @param[in] FileName           Name string of  the targe file.

  @retval SHELL_SUCCESS             the files were all moved.
  @retval SHELL_INVALID_PARAMETER   a parameter was invalid
  @retval SHELL_OUT_OF_RESOURCES    a memory allocation failed
**/
EFI_STATUS
CreateFullDestPath (
  IN CONST CHAR16  **DestPath,
  OUT CHAR16       **FullDestPath,
  IN CONST CHAR16  *FileName
  )
{
  UINTN  Size;

  if ((FullDestPath == NULL) || (FileName == NULL) || (DestPath == NULL) || (*DestPath == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  Size = StrSize (*DestPath) + StrSize (FileName);

  *FullDestPath = AllocateZeroPool (Size);
  if (*FullDestPath == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  StrCpyS (*FullDestPath, Size / sizeof (CHAR16), *DestPath);
  if (((*FullDestPath)[StrLen (*FullDestPath)-1] != L'\\') && (FileName[0] != L'\\')) {
    StrCatS (*FullDestPath, Size / sizeof (CHAR16), L"\\");
  }

  StrCatS (*FullDestPath, Size / sizeof (CHAR16), FileName);

  return (EFI_SUCCESS);
}

/**
  Function to do a move within a file system.

  @param[in] Node               A pointer to the file to be removed.
  @param[in] DestPath           A pointer to the destination file path.
  @param[out] Resp              A pointer to response from question.  Pass back on looped calling.

  @retval SHELL_SUCCESS           The source file was moved to the destination.
  @retval SHELL_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
MoveWithinFileSystems (
  IN EFI_SHELL_FILE_INFO  *Node,
  IN CHAR16               *DestPath,
  OUT VOID                **Resp
  )
{
  EFI_FILE_INFO  *NewFileInfo;
  CHAR16         *TempLocation;
  UINTN          NewSize;
  UINTN          Length;
  EFI_STATUS     Status;

  //
  // Chop off map info from DestPath
  //
  if ((TempLocation = StrStr (DestPath, L":")) != NULL) {
    CopyMem (DestPath, TempLocation+1, StrSize (TempLocation+1));
  }

  //
  // construct the new file info block
  //
  NewSize     = StrSize (DestPath);
  NewSize    += StrSize (Node->FileName) + SIZE_OF_EFI_FILE_INFO + sizeof (CHAR16);
  NewFileInfo = AllocateZeroPool (NewSize);
  if (NewFileInfo == NULL) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_MEM), gShellLevel2HiiHandle);
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    CopyMem (NewFileInfo, Node->Info, SIZE_OF_EFI_FILE_INFO);
    if (DestPath[0] != L'\\') {
      StrCpyS (NewFileInfo->FileName, (NewSize - SIZE_OF_EFI_FILE_INFO) / sizeof (CHAR16), L"\\");
      StrCatS (NewFileInfo->FileName, (NewSize - SIZE_OF_EFI_FILE_INFO) / sizeof (CHAR16), DestPath);
    } else {
      StrCpyS (NewFileInfo->FileName, (NewSize - SIZE_OF_EFI_FILE_INFO) / sizeof (CHAR16), DestPath);
    }

    Length = StrLen (NewFileInfo->FileName);
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

      StrCatS (NewFileInfo->FileName, (NewSize - SIZE_OF_EFI_FILE_INFO) / sizeof (CHAR16), Node->FileName);
    }

    NewFileInfo->Size = SIZE_OF_EFI_FILE_INFO + StrSize (NewFileInfo->FileName);

    //
    // Perform the move operation
    //
    Status = ShellSetFileInfo (Node->Handle, NewFileInfo);

    //
    // Free the info object we used...
    //
    FreePool (NewFileInfo);
  }

  return (Status);
}

/**
  function to take a list of files to move and a destination location and do
  the verification and moving of those files to that location.  This function
  will report any errors to the user and continue to move the rest of the files.

  @param[in] FileList           A LIST_ENTRY* based list of files to move
  @param[out] Resp              pointer to response from question.  Pass back on looped calling
  @param[in] DestParameter      the originally specified destination location

  @retval SHELL_SUCCESS             the files were all moved.
  @retval SHELL_INVALID_PARAMETER   a parameter was invalid
  @retval SHELL_SECURITY_VIOLATION  a security violation ocurred
  @retval SHELL_WRITE_PROTECTED     the destination was write protected
  @retval SHELL_OUT_OF_RESOURCES    a memory allocation failed
**/
SHELL_STATUS
ValidateAndMoveFiles (
  IN EFI_SHELL_FILE_INFO  *FileList,
  OUT VOID                **Resp,
  IN CONST CHAR16         *DestParameter
  )
{
  EFI_STATUS           Status;
  CHAR16               *HiiOutput;
  CHAR16               *HiiResultOk;
  CHAR16               *DestPath;
  CHAR16               *FullDestPath;
  CONST CHAR16         *Cwd;
  CHAR16               *FullCwd;
  SHELL_STATUS         ShellStatus;
  EFI_SHELL_FILE_INFO  *Node;
  VOID                 *Response;
  UINT64               Attr;
  CHAR16               *CleanFilePathStr;

  ASSERT (FileList != NULL);
  ASSERT (DestParameter  != NULL);

  DestPath         = NULL;
  FullDestPath     = NULL;
  Cwd              = ShellGetCurrentDir (NULL);
  Response         = *Resp;
  Attr             = 0;
  CleanFilePathStr = NULL;
  FullCwd          = NULL;

  if (Cwd != NULL) {
    FullCwd = AllocateZeroPool (StrSize (Cwd) + sizeof (CHAR16));
    if (FullCwd == NULL) {
      return SHELL_OUT_OF_RESOURCES;
    } else {
      StrCpyS (FullCwd, StrSize (Cwd)/sizeof (CHAR16)+1, Cwd);
      StrCatS (FullCwd, StrSize (Cwd)/sizeof (CHAR16)+1, L"\\");
    }
  }

  Status = ShellLevel2StripQuotes (DestParameter, &CleanFilePathStr);
  if (EFI_ERROR (Status)) {
    SHELL_FREE_NON_NULL (FullCwd);
    if (Status == EFI_OUT_OF_RESOURCES) {
      return SHELL_OUT_OF_RESOURCES;
    } else {
      return SHELL_INVALID_PARAMETER;
    }
  }

  ASSERT (CleanFilePathStr != NULL);

  //
  // Get and validate the destination location
  //
  ShellStatus = GetDestinationLocation (CleanFilePathStr, &DestPath, FullCwd, (BOOLEAN)(FileList->Link.ForwardLink == FileList->Link.BackLink), &Attr);
  FreePool (CleanFilePathStr);

  if (ShellStatus != SHELL_SUCCESS) {
    SHELL_FREE_NON_NULL (FullCwd);
    return (ShellStatus);
  }

  DestPath = PathCleanUpDirectories (DestPath);
  if (DestPath == NULL) {
    FreePool (FullCwd);
    return (SHELL_OUT_OF_RESOURCES);
  }

  HiiOutput   = HiiGetString (gShellLevel2HiiHandle, STRING_TOKEN (STR_MV_OUTPUT), NULL);
  HiiResultOk = HiiGetString (gShellLevel2HiiHandle, STRING_TOKEN (STR_GEN_RES_OK), NULL);
  if ((HiiOutput == NULL) || (HiiResultOk == NULL)) {
    SHELL_FREE_NON_NULL (DestPath);
    SHELL_FREE_NON_NULL (HiiOutput);
    SHELL_FREE_NON_NULL (HiiResultOk);
    SHELL_FREE_NON_NULL (FullCwd);
    return (SHELL_OUT_OF_RESOURCES);
  }

  //
  // Go through the list of files and directories to move...
  //
  for (Node = (EFI_SHELL_FILE_INFO *)GetFirstNode (&FileList->Link)
       ; !IsNull (&FileList->Link, &Node->Link)
       ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode (&FileList->Link, &Node->Link)
       )
  {
    if (ShellGetExecutionBreakFlag ()) {
      break;
    }

    //
    // These should never be NULL
    //
    ASSERT (Node->FileName != NULL);
    ASSERT (Node->FullName != NULL);
    ASSERT (Node->Info     != NULL);

    //
    // skip the directory traversing stuff...
    //
    if ((StrCmp (Node->FileName, L".") == 0) || (StrCmp (Node->FileName, L"..") == 0)) {
      continue;
    }

    SHELL_FREE_NON_NULL (FullDestPath);
    FullDestPath = NULL;
    if (ShellIsDirectory (DestPath) == EFI_SUCCESS) {
      CreateFullDestPath ((CONST CHAR16 **)&DestPath, &FullDestPath, Node->FileName);
    }

    //
    // Validate that the move is valid
    //
    if (!IsValidMove (Node->FullName, FullCwd, (FullDestPath != NULL) ? FullDestPath : DestPath, Node->Info->Attribute, Attr, Node->Status)) {
      ShellStatus = SHELL_INVALID_PARAMETER;
      continue;
    }

    ShellPrintEx (-1, -1, HiiOutput, Node->FullName, FullDestPath != NULL ? FullDestPath : DestPath);

    //
    // See if destination exists
    //
    if (!EFI_ERROR (ShellFileExists ((FullDestPath != NULL) ? FullDestPath : DestPath))) {
      if (Response == NULL) {
        ShellPromptForResponseHii (ShellPromptResponseTypeYesNoAllCancel, STRING_TOKEN (STR_GEN_DEST_EXIST_OVR), gShellLevel2HiiHandle, &Response);
      }

      if (Response == NULL) {
        return SHELL_ABORTED;
      }

      switch (*(SHELL_PROMPT_RESPONSE *)Response) {
        case ShellPromptResponseNo:
          FreePool (Response);
          Response = NULL;
          continue;
        case ShellPromptResponseCancel:
          *Resp = Response;
          //
          // indicate to stop everything
          //
          SHELL_FREE_NON_NULL (FullCwd);
          return (SHELL_ABORTED);
        case ShellPromptResponseAll:
          *Resp = Response;
          break;
        case ShellPromptResponseYes:
          FreePool (Response);
          Response = NULL;
          break;
        default:
          FreePool (Response);
          SHELL_FREE_NON_NULL (FullCwd);
          return SHELL_ABORTED;
      }

      Status = ShellDeleteFileByName (FullDestPath != NULL ? FullDestPath : DestPath);
    }

    if (IsBetweenFileSystem (Node->FullName, FullCwd, DestPath)) {
      while (FullDestPath == NULL && DestPath != NULL && DestPath[0] != CHAR_NULL && DestPath[StrLen (DestPath) - 1] == L'\\') {
        DestPath[StrLen (DestPath) - 1] = CHAR_NULL;
      }

      Status = MoveBetweenFileSystems (Node, FullDestPath != NULL ? FullDestPath : DestPath, &Response);
    } else {
      Status = MoveWithinFileSystems (Node, DestPath, &Response);
      //
      // Display error status
      //
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_UK), gShellLevel2HiiHandle, L"mv", Status);
      }
    }

    //
    // Check our result
    //
    if (EFI_ERROR (Status)) {
      ShellStatus = SHELL_INVALID_PARAMETER;
      if (Status == EFI_SECURITY_VIOLATION) {
        ShellStatus = SHELL_SECURITY_VIOLATION;
      } else if (Status == EFI_WRITE_PROTECTED) {
        ShellStatus = SHELL_WRITE_PROTECTED;
      } else if (Status == EFI_OUT_OF_RESOURCES) {
        ShellStatus = SHELL_OUT_OF_RESOURCES;
      } else if (Status == EFI_DEVICE_ERROR) {
        ShellStatus = SHELL_DEVICE_ERROR;
      } else if (Status == EFI_ACCESS_DENIED) {
        ShellStatus = SHELL_ACCESS_DENIED;
      }
    } else {
      ShellPrintEx (-1, -1, L"%s", HiiResultOk);
    }
  } // main for loop

  SHELL_FREE_NON_NULL (FullDestPath);
  SHELL_FREE_NON_NULL (DestPath);
  SHELL_FREE_NON_NULL (HiiOutput);
  SHELL_FREE_NON_NULL (HiiResultOk);
  SHELL_FREE_NON_NULL (FullCwd);
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
  EFI_STATUS           Status;
  LIST_ENTRY           *Package;
  CHAR16               *ProblemParam;
  CHAR16               *Cwd;
  UINTN                CwdSize;
  SHELL_STATUS         ShellStatus;
  UINTN                ParamCount;
  UINTN                LoopCounter;
  EFI_SHELL_FILE_INFO  *FileList;
  VOID                 *Response;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;
  ParamCount   = 0;
  FileList     = NULL;
  Response     = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"mv", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag (Package, L"-?")) {
      ASSERT (FALSE);
    }

    switch (ParamCount = ShellCommandLineGetCount (Package)) {
      case 0:
      case 1:
        //
        // we have insufficient parameters
        //
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"mv");
        ShellStatus = SHELL_INVALID_PARAMETER;
        break;
      case 2:
        //
        // must have valid CWD for single parameter...
        //
        if (ShellGetCurrentDir (NULL) == NULL) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle, L"mv");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          Status = ShellOpenFileMetaArg ((CHAR16 *)ShellCommandLineGetRawValue (Package, 1), EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
          if ((FileList == NULL) || IsListEmpty (&FileList->Link) || EFI_ERROR (Status)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, L"mv", ShellCommandLineGetRawValue (Package, 1));
            ShellStatus = SHELL_NOT_FOUND;
          } else {
            //
            // ValidateAndMoveFiles will report errors to the screen itself
            //
            CwdSize = StrSize (ShellGetCurrentDir (NULL)) + sizeof (CHAR16);
            Cwd     = AllocateZeroPool (CwdSize);
            if (Cwd == NULL) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellLevel2HiiHandle, L"mv");
              ShellStatus = SHELL_OUT_OF_RESOURCES;
            } else {
              StrCpyS (Cwd, CwdSize / sizeof (CHAR16), ShellGetCurrentDir (NULL));
              StrCatS (Cwd, CwdSize / sizeof (CHAR16), L"\\");
              ShellStatus = ValidateAndMoveFiles (FileList, &Response, Cwd);
              FreePool (Cwd);
            }
          }
        }

        break;
      default:
        /// @todo make sure this works with error half way through and continues...
        for (ParamCount--, LoopCounter = 1; LoopCounter < ParamCount; LoopCounter++) {
          if (ShellGetExecutionBreakFlag ()) {
            break;
          }

          Status = ShellOpenFileMetaArg ((CHAR16 *)ShellCommandLineGetRawValue (Package, LoopCounter), EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ, &FileList);
          if ((FileList == NULL) || IsListEmpty (&FileList->Link) || EFI_ERROR (Status)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, L"mv", ShellCommandLineGetRawValue (Package, LoopCounter));
            ShellStatus = SHELL_NOT_FOUND;
          } else {
            //
            // ValidateAndMoveFiles will report errors to the screen itself
            // Only change ShellStatus if it's sucessful
            //
            if (ShellStatus == SHELL_SUCCESS) {
              ShellStatus = ValidateAndMoveFiles (FileList, &Response, ShellCommandLineGetRawValue (Package, ParamCount));
            } else {
              ValidateAndMoveFiles (FileList, &Response, ShellCommandLineGetRawValue (Package, ParamCount));
            }
          }

          if ((FileList != NULL) && !IsListEmpty (&FileList->Link)) {
            Status = ShellCloseFileMetaArg (&FileList);
            if (EFI_ERROR (Status) && (ShellStatus == SHELL_SUCCESS)) {
              ShellStatus = SHELL_ACCESS_DENIED;
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_ERR_FILE), gShellLevel2HiiHandle, L"mv", ShellCommandLineGetRawValue (Package, 1), ShellStatus|MAX_BIT);
            }
          }
        }

        break;
    } // switch on parameter count

    if (FileList != NULL) {
      ShellCloseFileMetaArg (&FileList);
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  SHELL_FREE_NON_NULL (Response);

  if (ShellGetExecutionBreakFlag ()) {
    return (SHELL_ABORTED);
  }

  return (ShellStatus);
}
