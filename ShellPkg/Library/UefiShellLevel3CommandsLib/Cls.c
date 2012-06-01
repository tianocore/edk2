/** @file
  Main file for attrib shell level 2 function.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel3CommandsLib.h"

/**
  Function for 'cls' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunCls (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *Message;
  UINTN         Background;
  UINTN         ForeColor;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *Param1;

  ShellStatus   = SHELL_SUCCESS;
  ProblemParam  = NULL;
  Background    = 0;

  //
  // Initialize variables
  //
  Message = NULL;

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
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, ProblemParam);
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
    } else {
      //
      // If there are 0 value parameters, clear sceen
      //
      Param1 = ShellCommandLineGetRawValue(Package, 1);
      if (Param1 == NULL) {
        //
        // clear screen
        //
        gST->ConOut->ClearScreen (gST->ConOut);
      } else if (ShellCommandLineGetCount(Package) > 2) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        if (ShellStrToUintn(Param1) > 7 || StrLen(Param1) > 1 || !ShellIsDecimalDigitCharacter(*Param1)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, Param1);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else {
          switch (ShellStrToUintn(Param1)) {
            case 0:
              Background = EFI_BACKGROUND_BLACK;
              break;
            case 1:
              Background = EFI_BACKGROUND_BLUE;
              break;
            case 2:
              Background = EFI_BACKGROUND_GREEN;
              break;
            case 3:
              Background = EFI_BACKGROUND_CYAN;
              break;
            case 4:
              Background = EFI_BACKGROUND_RED;
              break;
            case 5:
              Background = EFI_BACKGROUND_MAGENTA;
              break;
            case 6:
              Background = EFI_BACKGROUND_BROWN;
              break;
            case 7:
              Background = EFI_BACKGROUND_LIGHTGRAY;
              break;
          }
          ForeColor = (~ShellStrToUintn(Param1)) & 0xF;
          Status = gST->ConOut->SetAttribute (gST->ConOut, ForeColor | Background);
          ASSERT_EFI_ERROR(Status);
          Status = gST->ConOut->ClearScreen (gST->ConOut);
          ASSERT_EFI_ERROR(Status);
        }
      }
    }
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

