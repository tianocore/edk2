/** @file
  Declares menubar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_MENU_BAR_H_
#define _LIB_MENU_BAR_H_

#define SCAN_CONTROL_E  5
#define SCAN_CONTROL_F  6
#define SCAN_CONTROL_G  7
#define SCAN_CONTROL_K  11
#define SCAN_CONTROL_O  15
#define SCAN_CONTROL_Q  17
#define SCAN_CONTROL_R  18
#define SCAN_CONTROL_S  19
#define SCAN_CONTROL_T  20
#define SCAN_CONTROL_U  21
#define SCAN_CONTROL_W  23
#define SCAN_CONTROL_Z  26


typedef
EFI_STATUS
(*MENU_ITEM_FUNCTION) (
  VOID
  );

typedef struct _EDITOR_MENU_ITEM {
  EFI_STRING_ID           NameToken;
  CHAR16                  FunctionKeyToken;
  MENU_ITEM_FUNCTION  Function;
} EDITOR_MENU_ITEM;

/**
  Initializa the menu bar with the specified items.

  @param[in] Items              The items to display and their functions.

  @retval EFI_SUCCESS           The initialization was correct.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
MenuBarInit (
  IN CONST EDITOR_MENU_ITEM  *Items
  );

/**
  Initialize the control hot-key with the specified items.

  @param[in] Items              The hot-key functions.

  @retval EFI_SUCCESS           The initialization was correct.
**/
EFI_STATUS
EFIAPI
ControlHotKeyInit (
  IN MENU_ITEM_FUNCTION  *Items
  );

/**
  Cleanup function for a menu bar.  frees all allocated memory.
**/
VOID
EFIAPI
MenuBarCleanup (
  VOID
  );

/**
  Refresh function for the menu bar.

  @param[in] LastRow            The last printable row.
  @param[in] LastCol            The last printable column.

  @retval EFI_SUCCESS           The refresh was successful.
**/
EFI_STATUS
EFIAPI
MenuBarRefresh (
  IN CONST UINTN LastRow,
  IN CONST UINTN LastCol
  );

/**
  Function to dispatch the correct function based on a function key (F1...)

  @param[in] Key                The pressed key.

  @retval EFI_NOT_FOUND         The key was not a valid function key 
                                (an error was sent to the status bar).
  @return The return value from the called dispatch function.
**/
EFI_STATUS
EFIAPI
MenuBarDispatchFunctionKey (
  IN CONST EFI_INPUT_KEY   *Key
  );

/**
  Function to dispatch the correct function based on a control-based key (ctrl+o...)

  @param[in] Key                The pressed key.

  @retval EFI_NOT_FOUND         The key was not a valid control-based key 
                                (an error was sent to the status bar).
  @return EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
MenuBarDispatchControlHotKey (
  IN CONST EFI_INPUT_KEY   *Key
  );

#endif
