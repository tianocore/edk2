/** @file
  Implements inputbar interface functions.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved. <BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EditInputBar.h"
#include "UefiShellDebug1CommandsLib.h"

CHAR16  *mPrompt;        // Input bar mPrompt string.
CHAR16  *mReturnString;  // The returned string.
UINTN   StringSize;      // Size of mReturnString space size.

/**
  Initialize the input bar.
**/
VOID
EFIAPI
InputBarInit (
  VOID
  )
{
  mPrompt       = NULL;
  mReturnString = NULL;
  StringSize    = 0;
}

/**
  Cleanup function for input bar.
**/
VOID
EFIAPI
InputBarCleanup (
  VOID
  )
{
  //
  // free input bar's prompt and input string
  //
  SHELL_FREE_NON_NULL (mPrompt);
  SHELL_FREE_NON_NULL (mReturnString);
  mPrompt       = NULL;
  mReturnString = NULL;
}

/**
  Display the prompt.
  Do the requesting of input.

  @param[in]  LastColumn   The last printable column.
  @param[in]  LastRow      The last printable row.
**/
VOID
EFIAPI
InputBarPrintInput (
  IN UINTN LastColumn,
  IN UINTN LastRow
  )
{
  UINTN   Limit;
  UINTN   Size;
  CHAR16  *Buffer;
  UINTN   Index;
  UINTN   mPromptLen;

  mPromptLen = StrLen (mPrompt);
  Limit     = LastColumn - mPromptLen - 1;
  Size      = StrLen (mReturnString);

  //
  // check whether the mPrompt length and input length will
  // exceed limit
  //
  if (Size <= Limit) {
    Buffer = mReturnString;
  } else {
    Buffer = mReturnString + Size - Limit;
  }

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  ShellPrintEx (((INT32)mPromptLen), ((INT32)LastRow) - 1, L"%s", Buffer);
  Size = StrLen (Buffer);

  //
  // print " " after mPrompt
  //
  for (Index = Size; Index < Limit; Index++) {
    ShellPrintEx ((INT32)(mPromptLen + Size), ((INT32)LastRow) - 1, L" ");
  }

  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  gST->ConOut->SetCursorPosition (gST->ConOut, Size + mPromptLen, LastRow - 1);
}

typedef struct {
  UINT32  Foreground : 4;
  UINT32  Background : 4;
} INPUT_BAR_COLOR_ATTRIBUTES;

typedef union {
  INPUT_BAR_COLOR_ATTRIBUTES  Colors;
  UINTN                       Data;
} INPUT_BAR_COLOR_UNION;


/**
  The refresh function for InputBar, it will wait for user input

  @param[in] LastRow            The last printable row.
  @param[in] LastColumn         The last printable column.

  @retval EFI_SUCCESS           The operation was successful.
**/
EFI_STATUS
EFIAPI
InputBarRefresh (
  UINTN LastRow,
  UINTN LastColumn
  )
{
  INPUT_BAR_COLOR_UNION   Orig;
  INPUT_BAR_COLOR_UNION   New;
  EFI_INPUT_KEY           Key;
  UINTN                   Size;
  EFI_STATUS              Status;
  BOOLEAN                 NoDisplay;
  UINTN                   Limit;
  UINTN                   mPromptLen;
  UINTN                   EventIndex;
  UINTN                   CursorRow;
  UINTN                   CursorCol;

  //
  // variable initialization
  //
  Size    = 0;
  Status  = EFI_SUCCESS;

  //
  // back up the old screen attributes
  //
  CursorCol             = gST->ConOut->Mode->CursorColumn;
  CursorRow             = gST->ConOut->Mode->CursorRow;
  Orig.Data             = gST->ConOut->Mode->Attribute;
  New.Colors.Foreground = Orig.Colors.Background;
  New.Colors.Background = Orig.Colors.Foreground;

  gST->ConOut->SetAttribute (gST->ConOut, New.Data);

  //
  // clear input bar
  //
  EditorClearLine (LastRow , LastColumn, LastRow);

  gST->ConOut->SetCursorPosition (gST->ConOut, 0, LastRow - 1);
  ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN(STR_EDIT_LIBINPUTBAR_MAININPUTBAR), gShellDebug1HiiHandle, mPrompt);

  //
  // that's the maximum input length that can be displayed on screen
  //
  mPromptLen = StrLen (mPrompt);
  Limit     = LastColumn - mPromptLen;

  //
  // this is a selection mPrompt, cursor will stay in edit area
  // actually this is for search , search/replace
  //
  if (StrStr (mPrompt, L"Yes/No")) {
    NoDisplay = TRUE;
    gST->ConOut->SetCursorPosition (gST->ConOut, CursorCol, CursorRow);
    gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);
  } else {
    NoDisplay = FALSE;
  }
  //
  // wait for user input
  //
  for (;;) {
    gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &EventIndex);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // pressed ESC
    //
    if (Key.ScanCode == SCAN_ESC) {
      Size    = 0;
      Status  = EFI_NOT_READY;
      break;
    }
    //
    // return pressed
    //
    if (Key.UnicodeChar == CHAR_LINEFEED || Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
      break;
    } else if (Key.UnicodeChar == CHAR_BACKSPACE) {
      //
      // backspace
      //
      if (Size > 0) {
        Size--;
        mReturnString[Size] = CHAR_NULL;
        if (!NoDisplay) {

          InputBarPrintInput (LastColumn, LastRow);

        }
      }
    } else if (Key.UnicodeChar <= 127 && Key.UnicodeChar >= 32) {
      //
      // VALID ASCII char pressed
      //
      mReturnString[Size] = Key.UnicodeChar;

      //
      // should be less than specified length
      //
      if (Size >= StringSize) {
        continue;
      }

      Size++;

      mReturnString[Size] = CHAR_NULL;

      if (!NoDisplay) {

        InputBarPrintInput (LastColumn, LastRow);

      } else {
        //
        // if just choose yes/no
        //
        break;
      }

    }
  }

  mReturnString[Size] = CHAR_NULL;
  

  //
  // restore screen attributes
  //
  gST->ConOut->SetCursorPosition (gST->ConOut, CursorCol, CursorRow);
  gST->ConOut->SetAttribute (gST->ConOut, Orig.Data);

  return Status;
}

/**
  SetPrompt and wait for input.

  @param[in] Str                The prompt string.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
InputBarSetPrompt (
  IN CONST CHAR16 *Str
  )
{
  //
  // FREE the old mPrompt string
  //
  SHELL_FREE_NON_NULL (mPrompt);

  mPrompt = CatSPrint (NULL, L"%s ", Str);
  if (mPrompt == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Set the size of the string in characters.

  @param[in] Size               The max number of characters to accept.

  @retval EFI_SUCCESS           The operation was successful.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
**/
EFI_STATUS
EFIAPI
InputBarSetStringSize (
  UINTN   Size
  )
{
  //
  // free the old ReturnStirng
  //
  SHELL_FREE_NON_NULL (mReturnString);

  StringSize = Size;
  mReturnString = AllocateZeroPool ((StringSize + 1) * sizeof(mReturnString[0]));
  if (mReturnString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Function to retrieve the input from the user.

  @retval NULL                  No input has been received.
  @return The string that was input.
**/
CONST CHAR16*
EFIAPI
InputBarGetString (
  VOID
  )
{
  return (mReturnString);
}
