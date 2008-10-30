/** @file
Utility functions for UI presentation.

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"
#include "Ui.h"

BOOLEAN            mHiiPackageListUpdated;
UI_MENU_SELECTION  *gCurrentSelection;


/**
  Clear retangle with specified text attribute.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
ClearLines (
  UINTN                                       LeftColumn,
  UINTN                                       RightColumn,
  UINTN                                       TopRow,
  UINTN                                       BottomRow,
  UINTN                                       TextAttribute
  )
{
  CHAR16  *Buffer;
  UINTN   Row;

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  //
  // Set foreground and background as defined
  //
  gST->ConOut->SetAttribute (gST->ConOut, TextAttribute);

  //
  // Much faster to buffer the long string instead of print it a character at a time
  //
  SetUnicodeMem (Buffer, RightColumn - LeftColumn, L' ');

  //
  // Clear the desired area with the appropriate foreground/background
  //
  for (Row = TopRow; Row <= BottomRow; Row++) {
    PrintStringAt (LeftColumn, Row, Buffer);
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, LeftColumn, TopRow);

  gBS->FreePool (Buffer);
  return ;
}

/**
  Concatenate a narrow string to another string.

  @param Destination The destination string.
  @param Source      The source string. The string to be concatenated.
                     to the end of Destination.

**/
VOID
NewStrCat (
  CHAR16                                      *Destination,
  CHAR16                                      *Source
  )
{
  UINTN Length;

  for (Length = 0; Destination[Length] != 0; Length++)
    ;

  //
  // We now have the length of the original string
  // We can safely assume for now that we are concatenating a narrow value to this string.
  // For instance, the string is "XYZ" and cat'ing ">"
  // If this assumption changes, we need to make this routine a bit more complex
  //
  Destination[Length] = NARROW_CHAR;
  Length++;

  StrCpy (Destination + Length, Source);
}

/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is 
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
GetStringWidth (
  CHAR16                                      *String
  )
{
  UINTN Index;
  UINTN Count;
  UINTN IncrementValue;

  Index           = 0;
  Count           = 0;
  IncrementValue  = 1;

  do {
    //
    // Advance to the null-terminator or to the first width directive
    //
    for (;
         (String[Index] != NARROW_CHAR) && (String[Index] != WIDE_CHAR) && (String[Index] != 0);
         Index++, Count = Count + IncrementValue
        )
      ;

    //
    // We hit the null-terminator, we now have a count
    //
    if (String[Index] == 0) {
      break;
    }
    //
    // We encountered a narrow directive - strip it from the size calculation since it doesn't get printed
    // and also set the flag that determines what we increment by.(if narrow, increment by 1, if wide increment by 2)
    //
    if (String[Index] == NARROW_CHAR) {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 1;
    } else {
      //
      // Skip to the next character
      //
      Index++;
      IncrementValue = 2;
    }
  } while (String[Index] != 0);

  //
  // Increment by one to include the null-terminator in the size
  //
  Count++;

  return Count * sizeof (CHAR16);
}

/**
  This function displays the page frame.

**/
VOID
DisplayPageFrame (
  VOID
  )
{
  UINTN                  Index;
  UINT8                  Line;
  UINT8                  Alignment;
  CHAR16                 Character;
  CHAR16                 *Buffer;
  CHAR16                 *StrFrontPageBanner;
  UINTN                  Row;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &LocalScreen.RightColumn, &LocalScreen.BottomRow);
  ClearLines (0, LocalScreen.RightColumn, 0, LocalScreen.BottomRow, KEYHELP_BACKGROUND);

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  Character = BOXDRAW_HORIZONTAL;

  for (Index = 0; Index + 2 < (LocalScreen.RightColumn - LocalScreen.LeftColumn); Index++) {
    Buffer[Index] = Character;
  }

  if (gClassOfVfr == EFI_FRONT_PAGE_SUBCLASS) {
    //
    //    ClearLines(0, LocalScreen.RightColumn, 0, BANNER_HEIGHT-1, BANNER_TEXT | BANNER_BACKGROUND);
    //
    ClearLines (
      LocalScreen.LeftColumn,
      LocalScreen.RightColumn,
      LocalScreen.TopRow,
      FRONT_PAGE_HEADER_HEIGHT - 1 + LocalScreen.TopRow,
      BANNER_TEXT | BANNER_BACKGROUND
      );
    //
    //    for (Line = 0; Line < BANNER_HEIGHT; Line++) {
    //
    for (Line = (UINT8) LocalScreen.TopRow; Line < BANNER_HEIGHT + (UINT8) LocalScreen.TopRow; Line++) {
      //
      //      for (Alignment = 0; Alignment < BANNER_COLUMNS; Alignment++) {
      //
      for (Alignment = (UINT8) LocalScreen.LeftColumn;
           Alignment < BANNER_COLUMNS + (UINT8) LocalScreen.LeftColumn;
           Alignment++
          ) {
        if (BannerData->Banner[Line - (UINT8) LocalScreen.TopRow][Alignment - (UINT8) LocalScreen.LeftColumn] != 0x0000) {
          StrFrontPageBanner = GetToken (
                                BannerData->Banner[Line - (UINT8) LocalScreen.TopRow][Alignment - (UINT8) LocalScreen.LeftColumn],
                                FrontPageHandle
                                );
        } else {
          continue;
        }

        switch (Alignment - LocalScreen.LeftColumn) {
        case 0:
          //
          // Handle left column
          //
          PrintStringAt (LocalScreen.LeftColumn, Line, StrFrontPageBanner);
          break;

        case 1:
          //
          // Handle center column
          //
          PrintStringAt (
            LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3,
            Line,
            StrFrontPageBanner
            );
          break;

        case 2:
          //
          // Handle right column
          //
          PrintStringAt (
            LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) * 2 / 3,
            Line,
            StrFrontPageBanner
            );
          break;
        }

        gBS->FreePool (StrFrontPageBanner);
      }
    }
  }

  ClearLines (
    LocalScreen.LeftColumn,
    LocalScreen.RightColumn,
    LocalScreen.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT,
    LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 1,
    KEYHELP_TEXT | KEYHELP_BACKGROUND
    );

  if (gClassOfVfr != EFI_FRONT_PAGE_SUBCLASS) {
    ClearLines (
      LocalScreen.LeftColumn,
      LocalScreen.RightColumn,
      LocalScreen.TopRow,
      LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT - 1,
      TITLE_TEXT | TITLE_BACKGROUND
      );
    //
    // Print Top border line
    // +------------------------------------------------------------------------------+
    // ?                                                                             ?
    // +------------------------------------------------------------------------------+
    //
    Character = BOXDRAW_DOWN_RIGHT;

    PrintChar (Character);
    PrintString (Buffer);

    Character = BOXDRAW_DOWN_LEFT;
    PrintChar (Character);

    Character = BOXDRAW_VERTICAL;
    for (Row = LocalScreen.TopRow + 1; Row <= LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT - 2; Row++) {
      PrintCharAt (LocalScreen.LeftColumn, Row, Character);
      PrintCharAt (LocalScreen.RightColumn - 1, Row, Character);
    }

    Character = BOXDRAW_UP_RIGHT;
    PrintCharAt (LocalScreen.LeftColumn, LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT - 1, Character);
    PrintString (Buffer);

    Character = BOXDRAW_UP_LEFT;
    PrintChar (Character);

    if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
      //
      // Print Bottom border line
      // +------------------------------------------------------------------------------+
      // ?                                                                             ?
      // +------------------------------------------------------------------------------+
      //
      Character = BOXDRAW_DOWN_RIGHT;
      PrintCharAt (LocalScreen.LeftColumn, LocalScreen.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT, Character);

      PrintString (Buffer);

      Character = BOXDRAW_DOWN_LEFT;
      PrintChar (Character);
      Character = BOXDRAW_VERTICAL;
      for (Row = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT + 1;
           Row <= LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 2;
           Row++
          ) {
        PrintCharAt (LocalScreen.LeftColumn, Row, Character);
        PrintCharAt (LocalScreen.RightColumn - 1, Row, Character);
      }

      Character = BOXDRAW_UP_RIGHT;
      PrintCharAt (LocalScreen.LeftColumn, LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 1, Character);

      PrintString (Buffer);

      Character = BOXDRAW_UP_LEFT;
      PrintChar (Character);
    }
  }

  gBS->FreePool (Buffer);

}


/**
  Evaluate all expressions in a Form.

  @param  FormSet        FormSet this Form belongs to.
  @param  Form           The Form.

  @retval EFI_SUCCESS    The expression evaluated successfuly

**/
EFI_STATUS
EvaluateFormExpressions (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN FORM_BROWSER_FORM     *Form
  )
{
  EFI_STATUS       Status;
  LIST_ENTRY       *Link;
  FORM_EXPRESSION  *Expression;

  Link = GetFirstNode (&Form->ExpressionListHead);
  while (!IsNull (&Form->ExpressionListHead, Link)) {
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    Link = GetNextNode (&Form->ExpressionListHead, Link);

    if (Expression->Type == EFI_HII_EXPRESSION_INCONSISTENT_IF ||
        Expression->Type == EFI_HII_EXPRESSION_NO_SUBMIT_IF) {
      //
      // Postpone Form validation to Question editing or Form submiting
      //
      continue;
    }

    Status = EvaluateExpression (FormSet, Form, Expression);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/*
+------------------------------------------------------------------------------+
?F2=Previous Page                 Setup Page                                  ?
+------------------------------------------------------------------------------+

















+------------------------------------------------------------------------------+
?F1=Scroll Help                 F9=Reset to Defaults        F10=Save and Exit ?
| ^"=Move Highlight          <Spacebar> Toggles Checkbox   Esc=Discard Changes |
+------------------------------------------------------------------------------+
*/

/**

  
  Display form and wait for user to select one menu option, then return it.
  
  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.
  @retval EFI_SUCESSS            This function always return successfully for now.

**/
EFI_STATUS
DisplayForm (
  IN OUT UI_MENU_SELECTION           *Selection
  )
{
  CHAR16                 *StringPtr;
  UINT16                 MenuItemCount;
  EFI_HII_HANDLE         Handle;
  BOOLEAN                Suppress;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  UINT16                 Width;
  UINTN                  ArrayEntry;
  CHAR16                 *OutputString;
  LIST_ENTRY             *Link;
  FORM_BROWSER_STATEMENT *Statement;
  UINT16                 NumberOfLines;
  EFI_STATUS             Status;

  Handle        = Selection->Handle;
  MenuItemCount = 0;
  ArrayEntry    = 0;
  OutputString  = NULL;

  UiInitMenu ();

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  StringPtr = GetToken (Selection->Form->FormTitle, Handle);

  if (gClassOfVfr != EFI_FRONT_PAGE_SUBCLASS) {
    gST->ConOut->SetAttribute (gST->ConOut, TITLE_TEXT | TITLE_BACKGROUND);
    PrintStringAt (
      (LocalScreen.RightColumn + LocalScreen.LeftColumn - GetStringWidth (StringPtr) / 2) / 2,
      LocalScreen.TopRow + 1,
      StringPtr
      );
  }

  if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
    gST->ConOut->SetAttribute (gST->ConOut, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    //
    // Display the infrastructure strings
    //
    if (!IsListEmpty (&gMenuList)) {
      PrintStringAt (LocalScreen.LeftColumn + 2, LocalScreen.TopRow + 1, gFunctionTwoString);
    }

    PrintStringAt (LocalScreen.LeftColumn + 2, LocalScreen.BottomRow - 4, gFunctionOneString);
    PrintStringAt (
      LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3,
      LocalScreen.BottomRow - 4,
      gFunctionNineString
      );
    PrintStringAt (
      LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) * 2 / 3,
      LocalScreen.BottomRow - 4,
      gFunctionTenString
      );
    PrintAt (LocalScreen.LeftColumn + 2, LocalScreen.BottomRow - 3, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
    PrintStringAt (
      LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3,
      LocalScreen.BottomRow - 3,
      gEscapeString
      );
  }
  //
  // Remove Buffer allocated for StringPtr after it has been used.
  //
  gBS->FreePool (StringPtr);

  //
  // Evaluate all the Expressions in this Form
  //
  Status = EvaluateFormExpressions (Selection->FormSet, Selection->Form);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Link = GetFirstNode (&Selection->Form->StatementListHead);
  while (!IsNull (&Selection->Form->StatementListHead, Link)) {
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);

    if (Statement->SuppressExpression != NULL) {
      Suppress = Statement->SuppressExpression->Result.Value.b;
    } else {
      Suppress = FALSE;
    }

    if (!Suppress) {
      StringPtr = GetToken (Statement->Prompt, Handle);

      Width     = GetWidth (Statement, Handle);

      NumberOfLines = 1;
      ArrayEntry = 0;
      for (; GetLineByWidth (StringPtr, Width, &ArrayEntry, &OutputString) != 0x0000;) {
        //
        // If there is more string to process print on the next row and increment the Skip value
        //
        if (StrLen (&StringPtr[ArrayEntry])) {
          NumberOfLines++;
        }

        gBS->FreePool (OutputString);
      }

      //
      // We are NOT!! removing this StringPtr buffer via FreePool since it is being used in the menuoptions, we will do
      // it in UiFreeMenu.
      //
      UiAddMenuOption (StringPtr, Selection->Handle, Statement, NumberOfLines, MenuItemCount);
      MenuItemCount++;
    }

    Link = GetNextNode (&Selection->Form->StatementListHead, Link);
  }

  Status = UiDisplayMenu (Selection);

  UiFreeMenu ();

  return Status;
}

/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeBrowserStrings (
  VOID
  )
{
  gFunctionOneString    = GetToken (STRING_TOKEN (FUNCTION_ONE_STRING), gHiiHandle);
  gFunctionTwoString    = GetToken (STRING_TOKEN (FUNCTION_TWO_STRING), gHiiHandle);
  gFunctionNineString   = GetToken (STRING_TOKEN (FUNCTION_NINE_STRING), gHiiHandle);
  gFunctionTenString    = GetToken (STRING_TOKEN (FUNCTION_TEN_STRING), gHiiHandle);
  gEnterString          = GetToken (STRING_TOKEN (ENTER_STRING), gHiiHandle);
  gEnterCommitString    = GetToken (STRING_TOKEN (ENTER_COMMIT_STRING), gHiiHandle);
  gEscapeString         = GetToken (STRING_TOKEN (ESCAPE_STRING), gHiiHandle);
  gSaveFailed           = GetToken (STRING_TOKEN (SAVE_FAILED), gHiiHandle);
  gMoveHighlight        = GetToken (STRING_TOKEN (MOVE_HIGHLIGHT), gHiiHandle);
  gMakeSelection        = GetToken (STRING_TOKEN (MAKE_SELECTION), gHiiHandle);
  gDecNumericInput      = GetToken (STRING_TOKEN (DEC_NUMERIC_INPUT), gHiiHandle);
  gHexNumericInput      = GetToken (STRING_TOKEN (HEX_NUMERIC_INPUT), gHiiHandle);
  gToggleCheckBox       = GetToken (STRING_TOKEN (TOGGLE_CHECK_BOX), gHiiHandle);
  gPromptForData        = GetToken (STRING_TOKEN (PROMPT_FOR_DATA), gHiiHandle);
  gPromptForPassword    = GetToken (STRING_TOKEN (PROMPT_FOR_PASSWORD), gHiiHandle);
  gPromptForNewPassword = GetToken (STRING_TOKEN (PROMPT_FOR_NEW_PASSWORD), gHiiHandle);
  gConfirmPassword      = GetToken (STRING_TOKEN (CONFIRM_PASSWORD), gHiiHandle);
  gConfirmError         = GetToken (STRING_TOKEN (CONFIRM_ERROR), gHiiHandle);
  gPassowordInvalid     = GetToken (STRING_TOKEN (PASSWORD_INVALID), gHiiHandle);
  gPressEnter           = GetToken (STRING_TOKEN (PRESS_ENTER), gHiiHandle);
  gEmptyString          = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
  gAreYouSure           = GetToken (STRING_TOKEN (ARE_YOU_SURE), gHiiHandle);
  gYesResponse          = GetToken (STRING_TOKEN (ARE_YOU_SURE_YES), gHiiHandle);
  gNoResponse           = GetToken (STRING_TOKEN (ARE_YOU_SURE_NO), gHiiHandle);
  gMiniString           = GetToken (STRING_TOKEN (MINI_STRING), gHiiHandle);
  gPlusString           = GetToken (STRING_TOKEN (PLUS_STRING), gHiiHandle);
  gMinusString          = GetToken (STRING_TOKEN (MINUS_STRING), gHiiHandle);
  gAdjustNumber         = GetToken (STRING_TOKEN (ADJUST_NUMBER), gHiiHandle);
  gSaveChanges          = GetToken (STRING_TOKEN (SAVE_CHANGES), gHiiHandle);
  return ;
}

/**
  Free up the resource allocated for all strings required
  by Setup Browser.

**/
VOID
FreeBrowserStrings (
  VOID
  )
{
  FreePool (gFunctionOneString);
  FreePool (gFunctionTwoString);
  FreePool (gFunctionNineString);
  FreePool (gFunctionTenString);
  FreePool (gEnterString);
  FreePool (gEnterCommitString);
  FreePool (gEscapeString);
  FreePool (gMoveHighlight);
  FreePool (gMakeSelection);
  FreePool (gDecNumericInput);
  FreePool (gHexNumericInput);
  FreePool (gToggleCheckBox);
  FreePool (gPromptForData);
  FreePool (gPromptForPassword);
  FreePool (gPromptForNewPassword);
  FreePool (gConfirmPassword);
  FreePool (gPassowordInvalid);
  FreePool (gConfirmError);
  FreePool (gPressEnter);
  FreePool (gEmptyString);
  FreePool (gAreYouSure);
  FreePool (gYesResponse);
  FreePool (gNoResponse);
  FreePool (gMiniString);
  FreePool (gPlusString);
  FreePool (gMinusString);
  FreePool (gAdjustNumber);
  FreePool (gSaveChanges);
  return ;
}


/**
  Update key's help imformation.

  @param  MenuOption     The Menu option
  @param  Selected       Whether or not a tag be selected

**/
VOID
UpdateKeyHelp (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected
  )
{
  UINTN                  SecCol;
  UINTN                  ThdCol;
  UINTN                  LeftColumnOfHelp;
  UINTN                  RightColumnOfHelp;
  UINTN                  TopRowOfHelp;
  UINTN                  BottomRowOfHelp;
  UINTN                  StartColumnOfHelp;
  EFI_SCREEN_DESCRIPTOR  LocalScreen;
  FORM_BROWSER_STATEMENT *Statement;

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  SecCol            = LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3;
  ThdCol            = LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) * 2 / 3;

  StartColumnOfHelp = LocalScreen.LeftColumn + 2;
  LeftColumnOfHelp  = LocalScreen.LeftColumn + 1;
  RightColumnOfHelp = LocalScreen.RightColumn - 2;
  TopRowOfHelp      = LocalScreen.BottomRow - 4;
  BottomRowOfHelp   = LocalScreen.BottomRow - 3;

  if (gClassOfVfr == EFI_GENERAL_APPLICATION_SUBCLASS) {
    return ;
  }

  gST->ConOut->SetAttribute (gST->ConOut, KEYHELP_TEXT | KEYHELP_BACKGROUND);

  Statement = MenuOption->ThisTag;
  switch (Statement->Operand) {
  case EFI_IFR_ORDERED_LIST_OP:
  case EFI_IFR_ONE_OF_OP:
  case EFI_IFR_NUMERIC_OP:
  case EFI_IFR_TIME_OP:
  case EFI_IFR_DATE_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    if (!Selected) {
      if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
        PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gFunctionOneString);
        PrintStringAt (SecCol, TopRowOfHelp, gFunctionNineString);
        PrintStringAt (ThdCol, TopRowOfHelp, gFunctionTenString);
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }

      if ((Statement->Operand == EFI_IFR_DATE_OP) ||
          (Statement->Operand == EFI_IFR_TIME_OP) ||
          (Statement->Operand == EFI_IFR_NUMERIC_OP && Statement->Step != 0)) {
        PrintAt (
          StartColumnOfHelp,
          BottomRowOfHelp,
          L"%c%c%c%c%s",
          ARROW_UP,
          ARROW_DOWN,
          ARROW_RIGHT,
          ARROW_LEFT,
          gMoveHighlight
          );
        PrintStringAt (SecCol, BottomRowOfHelp, gAdjustNumber);
      } else {
        PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
        PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
      }
    } else {
      PrintStringAt (SecCol, BottomRowOfHelp, gEnterCommitString);

      //
      // If it is a selected numeric with manual input, display different message
      //
      if ((Statement->Operand == EFI_IFR_NUMERIC_OP) && (Statement->Step == 0)) {
        PrintStringAt (
          SecCol,
          TopRowOfHelp,
          ((Statement->Flags & EFI_IFR_DISPLAY_UINT_HEX) == EFI_IFR_DISPLAY_UINT_HEX) ? gHexNumericInput : gDecNumericInput
          );
      } else if (Statement->Operand != EFI_IFR_ORDERED_LIST_OP) {
        PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      }

      if (Statement->Operand == EFI_IFR_ORDERED_LIST_OP) {
        PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gPlusString);
        PrintStringAt (ThdCol, TopRowOfHelp, gMinusString);
      }

      PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
    }
    break;

  case EFI_IFR_CHECKBOX_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
      PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gFunctionOneString);
      PrintStringAt (SecCol, TopRowOfHelp, gFunctionNineString);
      PrintStringAt (ThdCol, TopRowOfHelp, gFunctionTenString);
      PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
    }

    PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
    PrintStringAt (SecCol, BottomRowOfHelp, gToggleCheckBox);
    break;

  case EFI_IFR_REF_OP:
  case EFI_IFR_PASSWORD_OP:
  case EFI_IFR_STRING_OP:
  case EFI_IFR_TEXT_OP:
  case EFI_IFR_ACTION_OP:
  case EFI_IFR_RESET_BUTTON_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    if (!Selected) {
      if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
        PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gFunctionOneString);
        PrintStringAt (SecCol, TopRowOfHelp, gFunctionNineString);
        PrintStringAt (ThdCol, TopRowOfHelp, gFunctionTenString);
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }

      PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      if (Statement->Operand != EFI_IFR_TEXT_OP) {
        PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
      }
    } else {
      if (Statement->Operand != EFI_IFR_REF_OP) {
        PrintStringAt (
          (LocalScreen.RightColumn - GetStringWidth (gEnterCommitString) / 2) / 2,
          BottomRowOfHelp,
          gEnterCommitString
          );
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }
    }
    break;

  default:
    break;
  }
}

/**
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  @param PackageType  Package type of the notification.

  @param PackageGuid  If PackageType is
                      EFI_HII_PACKAGE_TYPE_GUID, then this is
                      the pointer to the GUID from the Guid
                      field of EFI_HII_PACKAGE_GUID_HEADER.
                      Otherwise, it must be NULL.

  @param Package  Points to the package referred to by the
                  notification Handle The handle of the package
                  list which contains the specified package.

  @param Handle       The HII handle.

  @param NotifyType   The type of change concerning the
                      database. See
                      EFI_HII_DATABASE_NOTIFY_TYPE.

**/
EFI_STATUS
FormUpdateNotify (
  IN UINT8                              PackageType,
  IN CONST EFI_GUID                     *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER       *Package,
  IN EFI_HII_HANDLE                     Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE       NotifyType
  )
{
  mHiiPackageListUpdated = TRUE;

  return EFI_SUCCESS;
}

/**
  The worker function that send the displays to the screen. On output,
  the selection made by user is returned.

  @param Selection       On input, Selection tell setup browser the information
                         about the Selection, form and formset to be displayed.
                         On output, Selection return the screen item that is selected
                         by user.

  @retval EFI_SUCCESS    The page is displayed successfully.
  @return Other value if the page failed to be diplayed.

**/
EFI_STATUS
SetupBrowser (
  IN OUT UI_MENU_SELECTION    *Selection
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *Link;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  EFI_HANDLE                      NotifyHandle;
  EFI_HII_VALUE                   *HiiValue;
  FORM_BROWSER_STATEMENT          *Statement;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_INPUT_KEY                   Key;
  CHAR16                          YesResponse;
  CHAR16                          NoResponse;

  gMenuRefreshHead = NULL;
  gResetRequired = FALSE;
  gNvUpdateRequired = FALSE;

  UiInitMenuList ();

  //
  // Register notify for Form package update
  //
  Status = mHiiDatabase->RegisterPackageNotify (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_FORM,
                           NULL,
                           FormUpdateNotify,
                           EFI_HII_DATABASE_NOTIFY_REMOVE_PACK,
                           &NotifyHandle
                           );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  do {
    //
    // Displays the Header and Footer borders
    //
    DisplayPageFrame ();

    //
    // Initialize Selection->Form
    //
    if (Selection->FormId == 0) {
      //
      // Zero FormId indicates display the first Form in a FormSet
      //
      Link = GetFirstNode (&Selection->FormSet->FormListHead);

      Selection->Form = FORM_BROWSER_FORM_FROM_LINK (Link);
      Selection->FormId = Selection->Form->FormId;
    } else {
      Selection->Form = IdToForm (Selection->FormSet, Selection->FormId);
    }

    //
    // Load Questions' Value for display
    //
    Status = LoadFormConfig (Selection->FormSet, Selection->Form);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Display form
    //
    Status = DisplayForm (Selection);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Check Selected Statement (if press ESC, Selection->Statement will be NULL)
    //
    Statement = Selection->Statement;
    if (Statement != NULL) {
      if ((Statement->QuestionFlags & EFI_IFR_FLAG_RESET_REQUIRED) == EFI_IFR_FLAG_RESET_REQUIRED) {
        gResetRequired = TRUE;
      }

      //
      // Reset FormPackage update flag
      //
      mHiiPackageListUpdated = FALSE;

      if (((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) == EFI_IFR_FLAG_CALLBACK) && (Statement->Operand != EFI_IFR_PASSWORD_OP)) {
        ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

        HiiValue = &Statement->HiiValue;
        if (HiiValue->Type == EFI_IFR_TYPE_STRING) {
          //
          // Create String in HII database for Configuration Driver to retrieve
          //
          HiiValue->Value.string = NewString ((CHAR16 *) Statement->BufferValue, Selection->FormSet->HiiHandle);
        }

        ConfigAccess = Selection->FormSet->ConfigAccess;
        if (ConfigAccess == NULL) {
          return EFI_UNSUPPORTED;
        }
        Status = ConfigAccess->Callback (
                                 ConfigAccess,
                                 EFI_BROWSER_ACTION_CHANGING,
                                 Statement->QuestionId,
                                 HiiValue->Type,
                                 &HiiValue->Value,
                                 &ActionRequest
                                 );

        if (HiiValue->Type == EFI_IFR_TYPE_STRING) {
          //
          // Clean the String in HII Database
          //
          DeleteString (HiiValue->Value.string, Selection->FormSet->HiiHandle);
        }

        if (!EFI_ERROR (Status)) {
          switch (ActionRequest) {
          case EFI_BROWSER_ACTION_REQUEST_RESET:
            gResetRequired = TRUE;
            break;

          case EFI_BROWSER_ACTION_REQUEST_SUBMIT:
            SubmitForm (Selection->FormSet, Selection->Form);
            break;

          case EFI_BROWSER_ACTION_REQUEST_EXIT:
            Selection->Action = UI_ACTION_EXIT;
            gNvUpdateRequired = FALSE;
            break;

          default:
            break;
          }
        }
      }

      //
      // Check whether Form Package has been updated during Callback
      //
      if (mHiiPackageListUpdated && (Selection->Action == UI_ACTION_REFRESH_FORM)) {
        //
        // Force to reparse IFR binary of target Formset
        //
        Selection->Action = UI_ACTION_REFRESH_FORMSET;

        //
        // Uncommitted data will be lost after IFR binary re-pasing, so confirm on whether to save
        //
        if (gNvUpdateRequired) {
          Status      = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

          YesResponse = gYesResponse[0];
          NoResponse  = gNoResponse[0];

          do {
            CreateDialog (3, TRUE, 0, NULL, &Key, gEmptyString, gSaveChanges, gEmptyString);
          } while
          (
            (Key.ScanCode != SCAN_ESC) &&
            ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (NoResponse | UPPER_LOWER_CASE_OFFSET)) &&
            ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (YesResponse | UPPER_LOWER_CASE_OFFSET))
          );

          if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (YesResponse | UPPER_LOWER_CASE_OFFSET)) {
            //
            // If the user hits the YesResponse key
            //
            SubmitForm (Selection->FormSet, Selection->Form);
          }
        }
      }
    }
  } while (Selection->Action == UI_ACTION_REFRESH_FORM);

  //
  // Unregister notify for Form package update
  //
  Status = mHiiDatabase->UnregisterPackageNotify (
                           mHiiDatabase,
                           NotifyHandle
                           );
  return Status;
}
