/** @file
  Main file for Edit shell Debug1 function.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include "TextEditor.h"

/**
  Function for 'edit' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunEdit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  CHAR16              *Buffer;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  LIST_ENTRY          *Package;
  CONST CHAR16        *Cwd;
  CHAR16              *Nfs;
  CHAR16              *Spot;
  CONST CHAR16        *TempParam;
//  SHELL_FILE_HANDLE   TempHandle;

  Buffer      = NULL;
  ShellStatus = SHELL_SUCCESS;
  Nfs         = NULL;

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
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Cwd = gEfiShellProtocol->GetCurDir(NULL);
      if (Cwd == NULL) {
        Cwd = ShellGetEnvironmentVariable(L"path");
        if (Cwd != NULL) {
          Nfs = StrnCatGrow(&Nfs, NULL, Cwd+3, 0);
          if (Nfs != NULL) {
            Spot = StrStr(Nfs, L";");
            if (Spot != NULL) {
              *Spot = CHAR_NULL;
            }
            Spot = StrStr(Nfs, L"\\");
            if (Spot != NULL) {
              Spot[1] = CHAR_NULL;
            }
            gEfiShellProtocol->SetCurDir(NULL, Nfs);
            FreePool(Nfs);
          } 
        }
      }

      Status = MainEditorInit ();

      if (EFI_ERROR (Status)) {
        gST->ConOut->ClearScreen (gST->ConOut);
        gST->ConOut->EnableCursor (gST->ConOut, TRUE);
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_MAIN_INIT_FAILED), gShellDebug1HiiHandle);
      } else {
        MainEditorBackup ();

        //
        // if editor launched with file named
        //
        if (ShellCommandLineGetCount(Package) == 2) {
          TempParam = ShellCommandLineGetRawValue(Package, 1);
          ASSERT(TempParam != NULL);
          FileBufferSetFileName (TempParam);
//          if (EFI_ERROR(ShellFileExists(MainEditor.FileBuffer->FileName))) {
//            Status = ShellOpenFileByName(MainEditor.FileBuffer->FileName, &TempHandle, EFI_FILE_MODE_CREATE|EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE, 0);
//            if (!EFI_ERROR(Status)) {
//              ShellCloseFile(&TempHandle);
//            }
//          }
        }

        Status = FileBufferRead (MainEditor.FileBuffer->FileName, FALSE);
        if (!EFI_ERROR (Status)) {
          MainEditorRefresh ();

          Status = MainEditorKeyInput ();
        }

        if (Status != EFI_OUT_OF_RESOURCES) {
          //
          // back up the status string
          //
          Buffer = CatSPrint (NULL, L"%s", StatusBarGetString());
        }

        MainEditorCleanup ();

        //
        // print editor exit code on screen
        //
        if (Status == EFI_SUCCESS) {
        } else if (Status == EFI_OUT_OF_RESOURCES) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_GEN_OUT_MEM), gShellDebug1HiiHandle);
        } else {
          if (Buffer != NULL) {
            if (StrCmp (Buffer, L"") != 0) {
              //
              // print out the status string
              //
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_MAIN_BUFFER), gShellDebug1HiiHandle, Buffer);
            } else {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_MAIN_UNKNOWN_EDITOR_ERR), gShellDebug1HiiHandle);
            }
          } else {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_MAIN_UNKNOWN_EDITOR_ERR), gShellDebug1HiiHandle);
          }
        }

        if (Status != EFI_OUT_OF_RESOURCES) {
          SHELL_FREE_NON_NULL (Buffer);
        }
      }
    }
    ShellCommandLineFreeVarList (Package);
  }
  return ShellStatus;
}
