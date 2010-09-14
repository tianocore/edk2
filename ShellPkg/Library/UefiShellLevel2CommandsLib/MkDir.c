/** @file
  Main file for attrib shell level 2 function.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  EFI_STATUS      Status;
  CONST CHAR16    *NewDirName;
  UINTN           DirCreateCount;
  LIST_ENTRY      *Package;
  CHAR16          *ProblemParam;
  SHELL_FILE_HANDLE          FileHandle;
  SHELL_STATUS    ShellStatus;

  ShellStatus  = SHELL_SUCCESS;

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

    //
    // create a set of directories
    //
    if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      //
      // we didnt get a single parameter
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      for ( DirCreateCount = 1
          ;
          ; DirCreateCount++
       ){
        //
        // loop through each directory specified
        //

        NewDirName = ShellCommandLineGetRawValue(Package, DirCreateCount);
        if (NewDirName == NULL) {
          break;
        }
        //
        // check if that already exists... if yes fail
        //
        FileHandle = NULL;
        Status = ShellOpenFileByName(NewDirName,
                                    &FileHandle,
                                    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE,
                                    EFI_FILE_DIRECTORY
                                   );
        if (!EFI_ERROR(Status)) {
          ShellCloseFile(&FileHandle);
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MKDIR_ALREADY), gShellLevel2HiiHandle, NewDirName);
          ShellStatus = SHELL_INVALID_PARAMETER;
          break;
        } else {
          ASSERT(FileHandle == NULL);
          //
          // create the directory named NewDirName
          //
          Status = ShellCreateDirectory(NewDirName, &FileHandle);
          if (FileHandle != NULL) {
            gEfiShellProtocol->CloseFile(FileHandle);
          }
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MKDIR_CREATEFAIL), gShellLevel2HiiHandle, NewDirName);
            ShellStatus = SHELL_ACCESS_DENIED;
            break;
          }
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

