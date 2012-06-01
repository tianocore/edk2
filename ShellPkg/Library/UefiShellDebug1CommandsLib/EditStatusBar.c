/** @file
  Implements statusbar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EditStatusBar.h"
#include "UefiShellDebug1CommandsLib.h"

CHAR16  *StatusString;
BOOLEAN StatusBarNeedRefresh;
BOOLEAN StatusStringChanged;

/**
  Initialization function for Status Bar.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @sa StatusBarSetStatusString
**/
EFI_STATUS
EFIAPI
StatusBarInit (
  VOID
  )
{
  //
  // initialize the statusbar
  //
  StatusString         = NULL;
  StatusBarNeedRefresh = TRUE;
  StatusStringChanged  = FALSE;

  //
  // status string set to ""
  //
  return (StatusBarSetStatusString (L""));
}

/**
  Cleanup function for the status bar.
**/
VOID
EFIAPI
StatusBarCleanup (
  VOID
  )
{
  //
  // free the status string and backvar's status string
  //
  SHELL_FREE_NON_NULL (StatusString);
}

typedef struct {
  UINT32  Foreground : 4;
  UINT32  Background : 4;
} STATUS_BAR_COLOR_ATTRIBUTES;

typedef union {
  STATUS_BAR_COLOR_ATTRIBUTES  Colors;
  UINTN                       Data;
} STATUS_BAR_COLOR_UNION;

/**
  Cause the status bar to refresh it's printing on the screen.

  @param[in] EditorFirst      TRUE to indicate the first launch of the editor.  
                              FALSE otherwise.
  @param[in] LastRow          LastPrintable row.
  @param[in] LastCol          Last printable column.
  @param[in] FileRow          Row in the file.
  @param[in] FileCol          Column in the file.
  @param[in] InsertMode       TRUE to indicate InsertMode.  FALSE otherwise.

  @retval EFI_SUCCESS         The operation was successful.
**/
EFI_STATUS
EFIAPI
StatusBarRefresh (
  IN BOOLEAN  EditorFirst,
  IN UINTN    LastRow,
  IN UINTN    LastCol,
  IN UINTN    FileRow,
  IN UINTN    FileCol,
  IN BOOLEAN  InsertMode
  )
{
  STATUS_BAR_COLOR_UNION  Orig;
  STATUS_BAR_COLOR_UNION  New;

  if (!StatusStringChanged && StatusBarNeedRefresh) {
    StatusBarSetStatusString (L"\0");
  }
  //
  // when it's called first time after editor launch, so refresh is mandatory
  //
  if (!StatusBarNeedRefresh && !StatusStringChanged) {
    return EFI_SUCCESS;
  }

  //
  // back up the screen attributes
  //
  Orig.Data             = gST->ConOut->Mode->Attribute;
  New.Colors.Foreground = Orig.Colors.Background;
  New.Colors.Background = Orig.Colors.Foreground;

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->SetAttribute (gST->ConOut, New.Data);

  //
  // clear status bar
  //
  EditorClearLine (LastRow, LastCol, LastRow);

  //
  // print row, column fields
  //
  if (FileRow != (UINTN)(-1) && FileCol != (UINTN)(-1)) {
    ShellPrintEx (
      0,
      (INT32)(LastRow) - 1,
      L" %d,%d       %s",
      FileRow,
      FileCol,
      StatusString
      );
  } else {
    ShellPrintEx (
      0,
      (INT32)(LastRow) - 1,
      L"  %s",
      StatusString
      );
  }

  //
  // print insert mode field
  //
  if (InsertMode) {
    ShellPrintEx ((INT32)(LastCol) - 21, (INT32)(LastRow) - 1, L"|%s|   Help: Ctrl-E", L"INS");
  } else {
    ShellPrintEx ((INT32)(LastCol) - 21, (INT32)(LastRow) - 1, L"|%s|   Help: Ctrl-E", L"OVR");
  }
  //
  // restore the old screen attributes
  //
  gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);

  //
  // restore position in edit area
  //
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  StatusBarNeedRefresh  = FALSE;
  StatusStringChanged   = FALSE;

  return EFI_SUCCESS;
}

/**
  Set the status string text part.

  @param[in] Str                The string to use.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
StatusBarSetStatusString (
  IN CHAR16 *Str
  )
{
  StatusStringChanged = TRUE;

  //
  // free the old status string
  //
  SHELL_FREE_NON_NULL (StatusString);
  StatusString = CatSPrint (NULL, L"%s", Str);
  if (StatusString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Function to retrieve the current status string.

  @return The string that is used.
**/
CONST CHAR16*
EFIAPI
StatusBarGetString (
  VOID
  )
{
  return (StatusString);
}

/**
  Function to set the need refresh boolean to TRUE.
**/
VOID
EFIAPI
StatusBarSetRefresh(
  VOID
  )
{
  StatusBarNeedRefresh = TRUE;
}

/**
  Function to get the need refresh boolean to TRUE.

  @retval TRUE    The status bar needs to be refreshed.
**/
BOOLEAN
EFIAPI
StatusBarGetRefresh(
  VOID
  )
{
  return (StatusBarNeedRefresh);
}
