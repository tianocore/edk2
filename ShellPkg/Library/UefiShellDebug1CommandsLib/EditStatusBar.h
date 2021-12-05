/** @file
  Declares statusbar interface functions.

  Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved. <BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LIB_STATUS_BAR_H_
#define _LIB_STATUS_BAR_H_

/**
  Initialization function for Status Bar.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @sa StatusBarSetStatusString
**/
EFI_STATUS
StatusBarInit (
  VOID
  );

/**
  Cleanup function for the status bar.
**/
VOID
StatusBarCleanup (
  VOID
  );

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
  );

/**
  Set the status string text part.

  @param[in] Str                The string to use.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
StatusBarSetStatusString (
  IN CHAR16  *Str
  );

/**
  Function to retrieve the current status string.

  @return The string that is used.
**/
CONST CHAR16 *
StatusBarGetString (
  VOID
  );

/**
  Function to set the need refresh boolean to TRUE.
**/
VOID
StatusBarSetRefresh (
  VOID
  );

/**
  Function to get the need refresh boolean to TRUE.

  @retval TRUE    The status bar needs to be refreshed.
**/
BOOLEAN
StatusBarGetRefresh (
  VOID
  );

#endif
