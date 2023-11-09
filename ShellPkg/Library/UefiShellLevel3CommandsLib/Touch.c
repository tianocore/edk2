/** @file
  Main file for Touch shell level 3 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

#include <Library/ShellLib.h>

/**
  Do the touch operation on a single handle.

  @param[in] Handle   The handle to update the date/time on.

  @retval EFI_ACCESS_DENIED The file referenced by Handle is read only.
  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
TouchFileByHandle (
  IN SHELL_FILE_HANDLE  Handle
  )
{
  EFI_STATUS     Status;
  EFI_FILE_INFO  *FileInfo;

  FileInfo = gEfiShellProtocol->GetFileInfo (Handle);
  if ((FileInfo->Attribute & EFI_FILE_READ_ONLY) != 0) {
    return (EFI_ACCESS_DENIED);
  }

  Status = gRT->GetTime (&FileInfo->ModificationTime, NULL);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"gRT->GetTime", Status);
    return (SHELL_DEVICE_ERROR);
  }

  CopyMem (&FileInfo->LastAccessTime, &FileInfo->ModificationTime, sizeof (EFI_TIME));

  Status = gEfiShellProtocol->SetFileInfo (Handle, FileInfo);

  FreePool (FileInfo);

  return (Status);
}

/**
  Touch a given file and potantially recurse down if it was a directory.

  @param[in] Name   The name of this file.
  @param[in] FS     The name of the file system this file is on.
  @param[in] Handle The handle of this file already opened.
  @param[in] Rec    TRUE to recurse if possible.

  @retval EFI_INVALID_PARAMETER A parameter was invalid.
  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
DoTouchByHandle (
  IN CONST CHAR16       *Name,
  IN       CHAR16       *FS,
  IN SHELL_FILE_HANDLE  Handle,
  IN BOOLEAN            Rec
  )
{
  EFI_STATUS           Status;
  EFI_SHELL_FILE_INFO  *FileList;
  EFI_SHELL_FILE_INFO  *Walker;
  CHAR16               *TempSpot;

  Status   = EFI_SUCCESS;
  FileList = NULL;
  Walker   = NULL;

  if (FS == NULL) {
    FS = StrnCatGrow (&FS, NULL, Name, 0);
    if (FS != NULL) {
      TempSpot = StrStr (FS, L"\\");
      if (TempSpot != NULL) {
        *TempSpot = CHAR_NULL;
      }
    }
  }

  if (FS == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // do it
  //
  Status = TouchFileByHandle (Handle);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, L"touch", Name);
    return (Status);
  }

  //
  // if it's a directory recurse...
  //
  if ((FileHandleIsDirectory (Handle) == EFI_SUCCESS) && Rec) {
    //
    // get each file under this directory
    //
    if (EFI_ERROR (gEfiShellProtocol->FindFilesInDir (Handle, &FileList))) {
      Status = EFI_INVALID_PARAMETER;
    }

    //
    // recurse on each
    //
    for (Walker = (EFI_SHELL_FILE_INFO *)GetFirstNode (&FileList->Link)
         ; FileList != NULL && !IsNull (&FileList->Link, &Walker->Link) && !EFI_ERROR (Status)
         ; Walker = (EFI_SHELL_FILE_INFO *)GetNextNode (&FileList->Link, &Walker->Link)
         )
    {
      if (  (StrCmp (Walker->FileName, L".") != 0)
         && (StrCmp (Walker->FileName, L"..") != 0)
            )
      {
        //
        // Open the file since we need that handle.
        //
        Status = gEfiShellProtocol->OpenFileByName (Walker->FullName, &Walker->Handle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, L"touch", Walker->FullName);
          Status = EFI_ACCESS_DENIED;
        } else {
          Status = DoTouchByHandle (Walker->FullName, FS, Walker->Handle, TRUE);
          gEfiShellProtocol->CloseFile (Walker->Handle);
          Walker->Handle = NULL;
        }
      }
    }

    //
    // free stuff
    //
    if ((FileList != NULL) && EFI_ERROR (gEfiShellProtocol->FreeFileList (&FileList))) {
      Status = EFI_INVALID_PARAMETER;
    }
  }

  return (Status);
}

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-r", TypeFlag },
  { NULL,  TypeMax  }
};

/**
  Function for 'touch' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunTouch (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS           Status;
  LIST_ENTRY           *Package;
  CHAR16               *ProblemParam;
  CONST CHAR16         *Param;
  SHELL_STATUS         ShellStatus;
  UINTN                ParamCount;
  EFI_SHELL_FILE_INFO  *FileList;
  EFI_SHELL_FILE_INFO  *Node;

  ProblemParam = NULL;
  ShellStatus  = SHELL_SUCCESS;
  ParamCount   = 0;
  FileList     = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"touch", ProblemParam);
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

    if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
      //
      // we insufficient parameters
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel3HiiHandle, L"touch");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      //
      // get a list with each file specified by parameters
      // if parameter is a directory then add all the files below it to the list
      //
      for ( ParamCount = 1, Param = ShellCommandLineGetRawValue (Package, ParamCount)
            ; Param != NULL
            ; ParamCount++, Param = ShellCommandLineGetRawValue (Package, ParamCount)
            )
      {
        Status = ShellOpenFileMetaArg ((CHAR16 *)Param, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, &FileList);
        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"touch", (CHAR16 *)Param);
          ShellStatus = SHELL_NOT_FOUND;
          break;
        }

        //
        // make sure we completed the param parsing successfully...
        // Also make sure that any previous action was successful
        //
        if (ShellStatus == SHELL_SUCCESS) {
          //
          // check that we have at least 1 file
          //
          if ((FileList == NULL) || IsListEmpty (&FileList->Link)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel3HiiHandle, L"touch", Param);
            continue;
          } else {
            //
            // loop through the list and make sure we are not aborting...
            //
            for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode (&FileList->Link)
                  ; !IsNull (&FileList->Link, &Node->Link) && !ShellGetExecutionBreakFlag ()
                  ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode (&FileList->Link, &Node->Link)
                  )
            {
              //
              // make sure the file opened ok
              //
              if (EFI_ERROR (Node->Status)) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, L"touch", Node->FileName);
                ShellStatus = SHELL_NOT_FOUND;
                continue;
              }

              Status = DoTouchByHandle (Node->FullName, NULL, Node->Handle, ShellCommandLineGetFlag (Package, L"-r"));
              if (EFI_ERROR (Status) && (Status != EFI_ACCESS_DENIED)) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel3HiiHandle, L"touch", Node->FileName);
                ShellStatus = SHELL_NOT_FOUND;
              }
            }
          }
        }

        //
        // Free the fileList
        //
        if ((FileList != NULL) && !IsListEmpty (&FileList->Link)) {
          Status = ShellCloseFileMetaArg (&FileList);
          ASSERT_EFI_ERROR (Status);
        }

        FileList = NULL;
      }
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  if (ShellGetExecutionBreakFlag ()) {
    return (SHELL_ABORTED);
  }

  return (ShellStatus);
}
