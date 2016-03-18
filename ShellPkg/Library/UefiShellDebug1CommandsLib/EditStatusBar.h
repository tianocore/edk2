/** @file
  Declares statusbar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
EFIAPI
StatusBarInit (
  VOID
  );

/**
  Cleanup function for the status bar.
**/
VOID
EFIAPI
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
EFIAPI
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
EFIAPI
StatusBarSetStatusString (
  IN CHAR16 *Str
  );

/**
  Function to retrieve the current status string.

  @return The string that is used.
**/
CONST CHAR16*
EFIAPI
StatusBarGetString (
  VOID
  );

/**
  Function to set the need refresh boolean to TRUE.
**/
VOID
EFIAPI
StatusBarSetRefresh(
  VOID
  );

/**
  Function to get the need refresh boolean to TRUE.

  @retval TRUE    The status bar needs to be refreshed.
**/
BOOLEAN
EFIAPI
StatusBarGetRefresh(
  VOID
  );

#endif
