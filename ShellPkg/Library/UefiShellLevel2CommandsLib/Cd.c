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
  Function for 'cd' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunCd (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  LIST_ENTRY        *Package;
  CONST CHAR16      *Directory;
  CHAR16            *Path;
  CHAR16            *Drive;
  UINTN             DriveSize;
  CHAR16            *ProblemParam;
  SHELL_STATUS      ShellStatus;
  SHELL_FILE_HANDLE Handle;
  CONST CHAR16      *Param1;

  ProblemParam = NULL;
  ShellStatus = SHELL_SUCCESS;
  Drive = NULL;
  DriveSize = 0;

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

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
  }

  //
  // check for "-?"
  //
  if (ShellCommandLineGetFlag(Package, L"-?")) {
    ASSERT(FALSE);
  } else if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
    ShellStatus = SHELL_INVALID_PARAMETER;
  } else {
    //
    // remember that param 0 is the command name
    // If there are 0 value parameters, then print the current directory
    // else If there are 2 value parameters, then print the error message
    // else If there is  1 value paramerer , then change the directory
    //
    Param1 = ShellCommandLineGetRawValue(Package, 1);
    if (Param1 == NULL) {
      //
      // display the current directory
      //
      Directory = ShellGetCurrentDir(NULL);
      if (Directory != NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CD_PRINT), gShellLevel2HiiHandle, Directory);
      } else {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
        ShellStatus = SHELL_NOT_FOUND;
      }
    } else {
      if (StrCmp(Param1, L".") == 0) {
        //
        // nothing to do... change to current directory
        //
      } else if (StrCmp(Param1, L"..") == 0) {
        //
        // Change up one directory...
        //
        Directory = ShellGetCurrentDir(NULL);
        if (Directory == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
          ShellStatus = SHELL_NOT_FOUND;
        } else {
          Drive = GetFullyQualifiedPath(Directory);
          ChopLastSlash(Drive);
        }
        if (ShellStatus == SHELL_SUCCESS && Drive != NULL) {
          //
          // change directory on current drive letter
          //
          Status = gEfiShellProtocol->SetCurDir(NULL, Drive);
          if (Status == EFI_NOT_FOUND) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CD_NF), gShellLevel2HiiHandle);
            ShellStatus = SHELL_NOT_FOUND;
          }
        }
      } else if (StrCmp(Param1, L"\\") == 0) {
        //
        // Move to root of current drive
        //
        Directory = ShellGetCurrentDir(NULL);
        if (Directory == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
          ShellStatus = SHELL_NOT_FOUND;
        } else {
          Drive = GetFullyQualifiedPath(Directory);
          while (ChopLastSlash(Drive)) ;
        }
        if (ShellStatus == SHELL_SUCCESS && Drive != NULL) {
          //
          // change directory on current drive letter
          //
          Status = gEfiShellProtocol->SetCurDir(NULL, Drive);
          if (Status == EFI_NOT_FOUND) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CD_NF), gShellLevel2HiiHandle);
            ShellStatus = SHELL_NOT_FOUND;
          }
        }
      } else if (StrStr(Param1, L":") == NULL) {
        if (ShellGetCurrentDir(NULL) == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
          ShellStatus = SHELL_NOT_FOUND;
        } else {
          ASSERT((Drive == NULL && DriveSize == 0) || (Drive != NULL));
          Drive = StrnCatGrow(&Drive, &DriveSize, ShellGetCurrentDir(NULL), 0);
          if (*Param1 == L'\\') {
            while (ChopLastSlash(Drive)) ;
            Drive = StrnCatGrow(&Drive, &DriveSize, Param1+1, 0);
          } else {
            Drive = StrnCatGrow(&Drive, &DriveSize, Param1, 0);
          }
          //
          // Verify that this is a valid directory
          //
          Status = gEfiShellProtocol->OpenFileByName(Drive, &Handle, EFI_FILE_MODE_READ);
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_DIR_NF), gShellLevel2HiiHandle, Drive);
            ShellStatus = SHELL_NOT_FOUND;
          } else if (EFI_ERROR(FileHandleIsDirectory(Handle))) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NOT_DIR), gShellLevel2HiiHandle, Drive);
            ShellStatus = SHELL_NOT_FOUND;
          }
          if (ShellStatus == SHELL_SUCCESS && Drive != NULL) {
            //
            // change directory on current drive letter
            //
            Status = gEfiShellProtocol->SetCurDir(NULL, Drive);
            if (Status == EFI_NOT_FOUND) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CD_NF), gShellLevel2HiiHandle);
              ShellStatus = SHELL_NOT_FOUND;
            }
          }
          if (Handle != NULL) {
            gEfiShellProtocol->CloseFile(Handle);
            DEBUG_CODE(Handle = NULL;);
          }
        }
      } else {
        //
        // change directory on other drive letter
        //
        Drive = AllocateZeroPool(StrSize(Param1));
        Drive = StrCpy(Drive, Param1);
        Path = StrStr(Drive, L":");
        *(++Path) = CHAR_NULL;
        Status = gEfiShellProtocol->SetCurDir(Drive, ++Path);

        if (Status == EFI_NOT_FOUND) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_CD_NF), gShellLevel2HiiHandle);
          Status = SHELL_NOT_FOUND;
        } else if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_DIR_NF), gShellLevel2HiiHandle, Param1);
          Status = SHELL_NOT_FOUND;
        }
      }
    }
  }

  if (Drive != NULL) {
    FreePool(Drive);
  }
  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  //
  // return the status
  //
  return (ShellStatus);
}

