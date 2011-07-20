/** @file
  implements menubar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EditMenuBar.h"
#include "UefiShellDebug1CommandsLib.h"
#include "EditStatusBar.h"

EDITOR_MENU_ITEM   *MenuItems;
MENU_ITEM_FUNCTION *ControlBasedMenuFunctions;
UINTN                 NumItems;

/**
  Cleanup function for a menu bar.  frees all allocated memory.
**/
VOID
EFIAPI
MenuBarCleanup (
  VOID
  )
{
  SHELL_FREE_NON_NULL(MenuItems);
}

/**
  Initialize the menu bar with the specified items.

  @param[in] Items              The items to display and their functions.

  @retval EFI_SUCCESS           The initialization was correct.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
MenuBarInit (
  IN CONST EDITOR_MENU_ITEM  *Items
  )
{
  CONST EDITOR_MENU_ITEM  *ItemsWalker;

  for (NumItems = 0, ItemsWalker = Items ; ItemsWalker != NULL && ItemsWalker->Function != NULL ; ItemsWalker++,NumItems++);
  
  MenuItems = AllocateZeroPool((NumItems+1) * sizeof(EDITOR_MENU_ITEM));
  if (MenuItems == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem(MenuItems, Items, (NumItems+1) * sizeof(EDITOR_MENU_ITEM));
  return EFI_SUCCESS;
}

/**
  Initialize the control hot-key with the specified items.

  @param[in] Items              The hot-key functions.

  @retval EFI_SUCCESS           The initialization was correct.
**/
EFI_STATUS
EFIAPI
ControlHotKeyInit (
  IN MENU_ITEM_FUNCTION  *Items
  )
{
  ControlBasedMenuFunctions = Items;
  return EFI_SUCCESS; 
}
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
  )
{
  EDITOR_MENU_ITEM  *Item;
  UINTN                 Col;
  UINTN                 Row;
  UINTN                 Width;
  CHAR16                *NameString;
  CHAR16                *FunctionKeyString;

  //
  // variable initialization
  //
  Col = 1;
  Row = (LastRow - 2);

  //
  // clear menu bar rows
  //
  EditorClearLine (LastRow - 2, LastCol, LastRow);
  EditorClearLine (LastRow - 1, LastCol, LastRow);
  EditorClearLine (LastRow    , LastCol, LastRow);


  //
  // print out the menu items
  //
  for (Item = MenuItems; Item != NULL && Item->Function != NULL; Item++) {


    NameString = HiiGetString(gShellDebug1HiiHandle, Item->NameToken, NULL);


    Width             = MAX ((StrLen (NameString) + 6), 20);
    if (((Col + Width) > LastCol)) {
      Row++;
      Col = 1;
    }

    FunctionKeyString = HiiGetString(gShellDebug1HiiHandle, Item->FunctionKeyToken, NULL);

    ShellPrintEx ((INT32)(Col) - 1, (INT32)(Row) - 1, L"%E%s%N  %H%s%N  ", FunctionKeyString, NameString);

    FreePool (NameString);
    FreePool (FunctionKeyString);
    Col += Width;
  }

  return EFI_SUCCESS;
}

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
  )
{
  UINTN                 Index;

  Index     = Key->ScanCode - SCAN_F1;

  //
  // check whether in range
  //
  if (Index > (NumItems - 1)) {
    StatusBarSetStatusString (L"Unknown Command");
    return EFI_SUCCESS;
  }

  return (MenuItems[Index].Function ());
}

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
  )
{
  
  if ((SCAN_CONTROL_Z < Key->UnicodeChar)
    ||(NULL == ControlBasedMenuFunctions[Key->UnicodeChar]))
  {
      return EFI_NOT_FOUND;
  }

  ControlBasedMenuFunctions[Key->UnicodeChar]();
  return EFI_SUCCESS;
}


