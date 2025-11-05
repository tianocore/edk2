/** @file
  Implements statusbar interface functions.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EditStatusBar.h"
#include "UefiShellDebug1CommandsLib.h"

CHAR16   *StatusString;
BOOLEAN  StatusBarNeedRefresh;
BOOLEAN  StatusStringChanged;

/**
  Initialization function for Status Bar.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @sa StatusBarSetStatusString
**/
EFI_STATUS
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
  UINT32    Foreground : 4;
  UINT32    Background : 3;
} STATUS_BAR_COLOR_ATTRIBUTES;

typedef union {
  STATUS_BAR_COLOR_ATTRIBUTES    Colors;
  UINTN                          Data;
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
  New.Data              = 0;
  New.Colors.Foreground = Orig.Colors.Background & 0xF;
  New.Colors.Background = Orig.Colors.Foreground & 0x7;

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->SetAttribute (gST->ConOut, New.Data & 0x7F);

  //
  // clear status bar
  //
  EditorClearLine (LastRow, LastCol, LastRow);

  //
  // print row, column fields
  //
  if ((FileRow != (UINTN)(-1)) && (FileCol != (UINTN)(-1))) {
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

  StatusBarNeedRefresh = FALSE;
  StatusStringChanged  = FALSE;

  return EFI_SUCCESS;
}

/**
  Set the status string text part.

  @param[in] Str                The string to use.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
StatusBarSetStatusString (
  IN CHAR16  *Str
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
CONST CHAR16 *
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
StatusBarSetRefresh (
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
StatusBarGetRefresh (
  VOID
  )
{
  return (StatusBarNeedRefresh);
}
