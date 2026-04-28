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

/** Mapping of Background colors.
**/
STATIC CONST UINTN  mClsBackgroundColorMap[] = {
  EFI_BACKGROUND_BLACK,
  EFI_BACKGROUND_BLUE,
  EFI_BACKGROUND_GREEN,
  EFI_BACKGROUND_CYAN,
  EFI_BACKGROUND_RED,
  EFI_BACKGROUND_MAGENTA,
  EFI_BACKGROUND_BROWN,
  EFI_BACKGROUND_LIGHTGRAY
};

/** Mapping of Foreground colors.
**/
STATIC CONST UINTN  mClsForegroundColorMap[] = {
  EFI_BLACK,
  EFI_BLUE,
  EFI_GREEN,
  EFI_CYAN,
  EFI_RED,
  EFI_MAGENTA,
  EFI_BROWN,
  EFI_LIGHTGRAY,
  EFI_DARKGRAY,
  EFI_LIGHTBLUE,
  EFI_LIGHTGREEN,
  EFI_LIGHTCYAN,
  EFI_LIGHTRED,
  EFI_LIGHTMAGENTA,
  EFI_YELLOW,
  EFI_WHITE
};

/**
  Parse a decimal cls color argument.

  @param[in]  ColorStr   Command-line argument to parse.
  @param[in]  MaxValue   Maximum accepted value.
  @param[in]  MaxLength  Maximum accepted string length.
  @param[out] Color      Parsed color index.

  @retval SHELL_SUCCESS            The argument is valid.
  @retval SHELL_INVALID_PARAMETER  The argument is invalid.
**/
STATIC
SHELL_STATUS
GetClsColorIndex (
  IN  CONST CHAR16  *ColorStr,
  IN  UINTN         MaxValue,
  IN  UINTN         MaxLength,
  OUT UINTN         *Color
  )
{
  ASSERT (Color != NULL);

  if ((ShellStrToUintn (ColorStr) > MaxValue) || (StrLen (ColorStr) > MaxLength) || !ShellIsDecimalDigitCharacter (*ColorStr)) {
    return SHELL_INVALID_PARAMETER;
  }

  *Color = ShellStrToUintn (ColorStr);
  return SHELL_SUCCESS;
}

/** Main function of the 'Cls' command.

  @param[in] Package    List of input parameter for the command.
**/
STATIC
SHELL_STATUS
MainCmdCls (
  LIST_ENTRY  *Package
  )
{
  EFI_STATUS    Status;
  UINTN         Background;
  UINTN         Foreground;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *BackColorStr;
  CONST CHAR16  *ForeColorStr;
  UINTN         ColorIndex;

  //
  // Initialize variables
  //
  ShellStatus = SHELL_SUCCESS;
  Background  = 0;
  Foreground  = 0;

  //
  // check for "-?"
  //
  if (ShellCommandLineGetFlag (Package, L"-?")) {
    ASSERT (FALSE);
  } else if (ShellCommandLineGetFlag (Package, L"-sfo")) {
    if (ShellCommandLineGetCount (Package) > 1) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"cls");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Background = (gST->ConOut->Mode->Attribute >> 4) & 0x7;
      Foreground = gST->ConOut->Mode->Attribute & 0x0F;
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_SFO_HEADER), gShellLevel3HiiHandle, L"cls");
      ShellPrintHiiDefaultEx (
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
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel3HiiHandle, L"cls");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      if (BackColorStr != NULL) {
        ShellStatus = GetClsColorIndex (BackColorStr, 7, 1, &ColorIndex);
        if (ShellStatus != SHELL_SUCCESS) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"cls", BackColorStr);
        } else {
          Background = mClsBackgroundColorMap[ColorIndex];

          if (ForeColorStr != NULL) {
            ShellStatus = GetClsColorIndex (ForeColorStr, 15, 2, &ColorIndex);
            if (ShellStatus != SHELL_SUCCESS) {
              ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel3HiiHandle, L"cls", ForeColorStr);
            } else {
              Foreground = mClsForegroundColorMap[ColorIndex];
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

  return ShellStatus;
}

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
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;

  //
  // Initialize variables
  //
  ShellStatus  = SHELL_SUCCESS;
  ProblemParam = NULL;

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
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel3HiiHandle, L"cls", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }

    return ShellStatus;
  }

  ShellStatus = MainCmdCls (Package);

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  //
  // return the status
  //
  return (ShellStatus);
}
