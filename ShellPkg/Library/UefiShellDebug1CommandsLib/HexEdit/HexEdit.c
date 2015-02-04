/** @file
  Main entry point of editor
  
  (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include "HexEditor.h"

//
// Global Variables
//
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-f", TypeFlag},
  {L"-d", TypeFlag},
  {L"-m", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'hexedit' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunHexEdit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS              Status;
  CHAR16                  *Buffer;
  CHAR16                  *ProblemParam;
  SHELL_STATUS            ShellStatus;
  LIST_ENTRY              *Package;
  CHAR16                  *NewName;
  CONST CHAR16            *Name;
  UINTN                   Offset;
  UINTN                   Size;
  EDIT_FILE_TYPE          WhatToDo;

  Buffer      = NULL;
  ShellStatus = SHELL_SUCCESS;
  NewName         = NULL;
  Buffer      = NULL;
  Name        = NULL;
  Offset      = 0;
  Size        = 0;
  WhatToDo    = FileTypeNone;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"hexedit", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // Check for -d
    //
    if (ShellCommandLineGetFlag(Package, L"-d")){
      if (ShellCommandLineGetCount(Package) < 4) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) > 4) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        WhatToDo = FileTypeDiskBuffer;
        Name    = ShellCommandLineGetRawValue(Package, 1);
        Offset  = ShellStrToUintn(ShellCommandLineGetRawValue(Package, 2));
        Size    = ShellStrToUintn(ShellCommandLineGetRawValue(Package, 3));
      }
      if (Offset == (UINTN)-1 || Size == (UINTN)-1) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDebug1HiiHandle, L"hexedit", L"-d");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      }
    }

    //
    // check for -f
    //
    if (ShellCommandLineGetFlag(Package, L"-f") && (WhatToDo == FileTypeNone)){
      if (ShellCommandLineGetCount(Package) < 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) > 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Name      = ShellCommandLineGetRawValue(Package, 1);
        if (Name == NULL || !IsValidFileName(Name)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"hexedit", Name);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          WhatToDo  = FileTypeFileBuffer;
        }
      }
    }

    //
    // check for -m
    //
    if (ShellCommandLineGetFlag(Package, L"-m") && (WhatToDo == FileTypeNone)){
      if (ShellCommandLineGetCount(Package) < 3) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (ShellCommandLineGetCount(Package) > 3) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        WhatToDo = FileTypeMemBuffer;
        Offset  = ShellStrToUintn(ShellCommandLineGetRawValue(Package, 1));
        Size    = ShellStrToUintn(ShellCommandLineGetRawValue(Package, 2));
      }
    }
    Name = ShellCommandLineGetRawValue(Package, 1);
    if (WhatToDo == FileTypeNone && Name != NULL) {
      if (ShellCommandLineGetCount(Package) > 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"hexedit");  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else if (!IsValidFileName(Name)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"hexedit", Name);  
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        WhatToDo  = FileTypeFileBuffer;
      }
    } else if (WhatToDo == FileTypeNone) {
      if (gEfiShellProtocol->GetCurDir(NULL) == NULL) {
        ShellStatus = SHELL_NOT_FOUND;
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellDebug1HiiHandle, L"hexedit");  
      } else {
        NewName = EditGetDefaultFileName(L"bin");
        Name = NewName;
        WhatToDo  = FileTypeFileBuffer;
      }
    }

    if (ShellStatus == SHELL_SUCCESS && WhatToDo == FileTypeNone) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"hexedit");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (WhatToDo == FileTypeFileBuffer && ShellGetCurrentDir(NULL) == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellDebug1HiiHandle, L"hexedit");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    }

    if (ShellStatus == SHELL_SUCCESS) {
      //
      // Do the editor
      //
      Status = HMainEditorInit ();
      if (EFI_ERROR (Status)) {
        gST->ConOut->ClearScreen (gST->ConOut);
        gST->ConOut->EnableCursor (gST->ConOut, TRUE);
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_INIT_FAILED), gShellDebug1HiiHandle);
      } else {
        HMainEditorBackup ();
        switch (WhatToDo) {
        case FileTypeFileBuffer:
          Status = HBufferImageRead (
                    Name==NULL?L"":Name,
                    NULL,
                    0,
                    0,
                    0,
                    0,
                    FileTypeFileBuffer,
                    FALSE
                    );
          break;

        case FileTypeDiskBuffer:
          Status = HBufferImageRead (
                    NULL,
                    Name==NULL?L"":Name,
                    Offset,
                    Size,
                    0,
                    0,
                    FileTypeDiskBuffer,
                    FALSE
                    );
          break;

        case FileTypeMemBuffer:
          Status = HBufferImageRead (
                    NULL,
                    NULL,
                    0,
                    0,
                    (UINT32) Offset,
                    Size,
                    FileTypeMemBuffer,
                    FALSE
                    );
          break;

        default:
          Status = EFI_NOT_FOUND;
          break;
        }
        if (!EFI_ERROR (Status)) {
          HMainEditorRefresh ();
          Status = HMainEditorKeyInput ();
        }
        if (Status != EFI_OUT_OF_RESOURCES) {
          //
          // back up the status string
          //
          Buffer = CatSPrint (NULL, L"%s\r\n", StatusBarGetString());
        }
      }

      //
      // cleanup
      //
      HMainEditorCleanup ();

      if (EFI_ERROR (Status)) {
        if (ShellStatus == SHELL_SUCCESS) {
          ShellStatus = SHELL_UNSUPPORTED;
        }
      }

      //
      // print editor exit code on screen
      //
      if (Status == EFI_OUT_OF_RESOURCES) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"hexedit");  
      } else if (EFI_ERROR(Status)){
        if (Buffer != NULL) {
          if (StrCmp (Buffer, L"") != 0) {
            //
            // print out the status string
            //
            ShellPrintEx(-1, -1, L"%s", Buffer);
          } else {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_UNKNOWN_EDITOR), gShellDebug1HiiHandle);
          }
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_HEXEDIT_UNKNOWN_EDITOR), gShellDebug1HiiHandle);
        }
      }
    }
    ShellCommandLineFreeVarList (Package);
  }

  SHELL_FREE_NON_NULL (Buffer);
  SHELL_FREE_NON_NULL (NewName);
  return ShellStatus;
}
