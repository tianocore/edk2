/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Function for 'mkdir' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMkDir (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS         Status;
  CONST CHAR16       *NewDirName;
  CHAR16             *NewDirNameCopy;
  CHAR16             *SplitName;
  CHAR16             SaveSplitChar;
  UINTN              DirCreateCount;
  LIST_ENTRY         *Package;
  CHAR16             *ProblemParam;
  SHELL_FILE_HANDLE  FileHandle;
  SHELL_STATUS       ShellStatus;

  ShellStatus    = SHELL_SUCCESS;
  NewDirNameCopy = NULL;
  SplitName      = NULL;
  SaveSplitChar  = CHAR_NULL;
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
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"mkdir", ProblemParam);
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

    //
    // create a set of directories
    //
    if (ShellCommandLineGetRawValue (Package, 1) == NULL) {
      //
      // we didnt get a single parameter
      //
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"mkdir");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      for ( DirCreateCount = 1
            ;
            ; DirCreateCount++
            )
      {
        //
        // loop through each directory specified
        //

        NewDirName = ShellCommandLineGetRawValue (Package, DirCreateCount);
        if (NewDirName == NULL) {
          break;
        }

        //
        // check if that already exists... if yes fail
        //
        FileHandle = NULL;
        Status     = ShellOpenFileByName (
                       NewDirName,
                       &FileHandle,
                       EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                       EFI_FILE_DIRECTORY
                       );
        if (!EFI_ERROR (Status)) {
          ShellCloseFile (&FileHandle);
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MKDIR_ALREADY), gShellLevel2HiiHandle, NewDirName);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          ASSERT (FileHandle == NULL);
          //
          // create the nested directory from parent to child.
          // if NewDirName = test1\test2\test3, first create "test1\" directory, then "test1\test2\", finally "test1\test2\test3".
          //
          NewDirNameCopy = AllocateCopyPool (StrSize (NewDirName), NewDirName);
          NewDirNameCopy = PathCleanUpDirectories (NewDirNameCopy);
          if (NewDirNameCopy == NULL) {
            ShellStatus = SHELL_OUT_OF_RESOURCES;
            break;
          }

          SplitName = NewDirNameCopy;
          while (SplitName != NULL) {
            SplitName = StrStr (SplitName + 1, L"\\");
            if (SplitName != NULL) {
              SaveSplitChar    = *(SplitName + 1);
              *(SplitName + 1) = '\0';
            }

            //
            // check if current nested directory already exists... continue to create the child directory.
            //
            Status = ShellOpenFileByName (
                       NewDirNameCopy,
                       &FileHandle,
                       EFI_FILE_MODE_READ,
                       EFI_FILE_DIRECTORY
                       );
            if (!EFI_ERROR (Status)) {
              ShellCloseFile (&FileHandle);
            } else {
              Status = ShellCreateDirectory (NewDirNameCopy, &FileHandle);
              if (EFI_ERROR (Status)) {
                break;
              }

              if (FileHandle != NULL) {
                gEfiShellProtocol->CloseFile (FileHandle);
              }
            }

            if (SplitName != NULL) {
              *(SplitName + 1) = SaveSplitChar;
            }
          }

          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MKDIR_CREATEFAIL), gShellLevel2HiiHandle, NewDirName);
            ShellStatus = SHELL_ACCESS_DENIED;
            break;
          }

          SHELL_FREE_NON_NULL (NewDirNameCopy);
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
