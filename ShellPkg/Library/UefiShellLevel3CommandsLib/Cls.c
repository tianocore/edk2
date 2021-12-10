/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel3CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-sfo", TypeFlag },
  { NULL,    TypeMax  }
};

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
  UINTN         Background;
  UINTN         Foreground;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *BackColorStr;
  CONST CHAR16  *ForeColorStr;

  //
  // Initialize variables
  //
  ShellStatus  = SHELL_SUCCESS;
  ProblemParam = NULL;
  Background   = 0;
  Foreground   = 0;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"cls", ProblemParam);
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
    } else if (ShellCommandLineGetFlag (Package, L"-sfo")) {
      if (ShellCommandLineGetCount (Package) > 1) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"cls");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        Background = (gST->ConOut->Mode->Attribute >> 4) & 0x7;
        Foreground = gST->ConOut->Mode->Attribute & 0x0F;
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_SFO_HEADER), gShellLevel3HiiHandle, L"cls");
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_CLS_OUTPUT_SFO),
          gShellLevel3HiiHandle,
          gST->ConOut->Mode->Attribute,
          Foreground,
          Background
          );
      }
    } else {
      //
      // If there are 0 value parameters, clear sceen
      //
      BackColorStr = ShellCommandLineGetRawValue (Package, 1);
      ForeColorStr = ShellCommandLineGetRawValue (Package, 2);

      if ((BackColorStr == NULL) && (ForeColorStr == NULL)) {
        //
        // clear screen
        //
        gST->ConOut->ClearScreen (gST->ConOut);
      } else if (ShellCommandLineGetCount (Package) > 3) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"cls");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        if (BackColorStr != NULL) {
          if ((ShellStrToUintn (BackColorStr) > 7) || (StrLen (BackColorStr) > 1) || (!ShellIsDecimalDigitCharacter (*BackColorStr))) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"cls", BackColorStr);
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            switch (ShellStrToUintn (BackColorStr)) {
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

            if (ForeColorStr != NULL) {
              if ((ShellStrToUintn (ForeColorStr) > 15) || (StrLen (ForeColorStr) > 2) || (!ShellIsDecimalDigitCharacter (*ForeColorStr))) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"cls", ForeColorStr);
                ShellStatus = SHELL_INVALID_PARAMETER;
              } else {
                switch (ShellStrToUintn (ForeColorStr)) {
                  case 0:
                    Foreground = EFI_BLACK;
                    break;
                  case 1:
                    Foreground = EFI_BLUE;
                    break;
                  case 2:
                    Foreground = EFI_GREEN;
                    break;
                  case 3:
                    Foreground = EFI_CYAN;
                    break;
                  case 4:
                    Foreground = EFI_RED;
                    break;
                  case 5:
                    Foreground = EFI_MAGENTA;
                    break;
                  case 6:
                    Foreground = EFI_BROWN;
                    break;
                  case 7:
                    Foreground = EFI_LIGHTGRAY;
                    break;
                  case 8:
                    Foreground = EFI_DARKGRAY;
                    break;
                  case 9:
                    Foreground = EFI_LIGHTBLUE;
                    break;
                  case 10:
                    Foreground = EFI_LIGHTGREEN;
                    break;
                  case 11:
                    Foreground = EFI_LIGHTCYAN;
                    break;
                  case 12:
                    Foreground = EFI_LIGHTRED;
                    break;
                  case 13:
                    Foreground = EFI_LIGHTMAGENTA;
                    break;
                  case 14:
                    Foreground = EFI_YELLOW;
                    break;
                  case 15:
                    Foreground = EFI_WHITE;
                    break;
                }
              }
            } else {
              //
              // Since foreground color is not modified, so retain
              // existing foreground color without any change to it.
              //
              Foreground = gST->ConOut->Mode->Attribute & 0x0F;
            }

            if (ShellStatus == SHELL_SUCCESS) {
              Status = gST->ConOut->SetAttribute (gST->ConOut, (Foreground | Background) & 0x7F);
              ASSERT_EFI_ERROR (Status);
              Status = gST->ConOut->ClearScreen (gST->ConOut);
              ASSERT_EFI_ERROR (Status);
            }
          }
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
