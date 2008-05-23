/** @file

Copyright (c) 2004 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  InputHandler.c

Abstract:

  Implementation for handling user input from the User Interface

Revision History


**/

#include "Ui.h"
#include "Setup.h"


/**
  Get string or password input from user.

  @param  MenuOption        Pointer to the current input menu.
  @param  Prompt            The prompt string shown on popup window.
  @param  StringPtr         Destination for use input string.

  @retval EFI_SUCCESS       If string input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
ReadString (
  IN  UI_MENU_OPTION              *MenuOption,
  IN  CHAR16                      *Prompt,
  OUT CHAR16                      *StringPtr
  )
{
  EFI_STATUS              Status;
  EFI_INPUT_KEY           Key;
  CHAR16                  NullCharacter;
  UINTN                   ScreenSize;
  CHAR16                  Space[2];
  CHAR16                  KeyPad[2];
  CHAR16                  *TempString;
  CHAR16                  *BufferedString;
  UINTN                   Index;
  UINTN                   Count;
  UINTN                   Start;
  UINTN                   Top;
  UINTN                   DimensionsWidth;
  UINTN                   DimensionsHeight;
  BOOLEAN                 CursorVisible;
  UINTN                   Minimum;
  UINTN                   Maximum;
  FORM_BROWSER_STATEMENT  *Question;
  BOOLEAN                 IsPassword;

  DimensionsWidth  = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  NullCharacter    = CHAR_NULL;
  ScreenSize       = GetStringWidth (Prompt) / sizeof (CHAR16);
  Space[0]         = L' ';
  Space[1]         = CHAR_NULL;

  Question         = MenuOption->ThisTag;
  Minimum          = (UINTN) Question->Minimum;
  Maximum          = (UINTN) Question->Maximum;

  if (Question->Operand == EFI_IFR_PASSWORD_OP) {
    IsPassword = TRUE;
  } else {
    IsPassword = FALSE;
  }

  TempString = AllocateZeroPool ((Maximum + 1)* sizeof (CHAR16));
  ASSERT (TempString);

  if (ScreenSize < (Maximum + 1)) {
    ScreenSize = Maximum + 1;
  }

  if ((ScreenSize + 2) > DimensionsWidth) {
    ScreenSize = DimensionsWidth - 2;
  }

  BufferedString = AllocateZeroPool (ScreenSize * 2);
  ASSERT (BufferedString);

  Start = (DimensionsWidth - ScreenSize - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  Top   = ((DimensionsHeight - 6) / 2) + gScreenDimensions.TopRow - 1;

  //
  // Display prompt for string
  //
  CreatePopUp (ScreenSize, 4, &NullCharacter, Prompt, Space, &NullCharacter);

  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));

  CursorVisible = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  do {
    Status = WaitForKeyStroke (&Key);
    ASSERT_EFI_ERROR (Status);

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
    switch (Key.UnicodeChar) {
    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
        break;

      case SCAN_RIGHT:
        break;

      case SCAN_ESC:
        gBS->FreePool (TempString);
        gBS->FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;

      default:
        break;
      }

      break;

    case CHAR_CARRIAGE_RETURN:
      if (GetStringWidth (StringPtr) >= ((Minimum + 1) * sizeof (CHAR16))) {

        gBS->FreePool (TempString);
        gBS->FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_SUCCESS;
      } else {
        //
        // Simply create a popup to tell the user that they had typed in too few characters.
        // To save code space, we can then treat this as an error and return back to the menu.
        //
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, &NullCharacter, gMiniString, gPressEnter, &NullCharacter);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        gBS->FreePool (TempString);
        gBS->FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;
      }

      break;

    case CHAR_BACKSPACE:
      if (StringPtr[0] != CHAR_NULL) {
        for (Index = 0; StringPtr[Index] != CHAR_NULL; Index++) {
          TempString[Index] = StringPtr[Index];
        }
        //
        // Effectively truncate string by 1 character
        //
        TempString[Index - 1] = CHAR_NULL;
        StrCpy (StringPtr, TempString);
      }

    default:
      //
      // If it is the beginning of the string, don't worry about checking maximum limits
      //
      if ((StringPtr[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        StrnCpy (StringPtr, &Key.UnicodeChar, 1);
        StrnCpy (TempString, &Key.UnicodeChar, 1);
      } else if ((GetStringWidth (StringPtr) < ((Maximum + 1) * sizeof (CHAR16))) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        KeyPad[0] = Key.UnicodeChar;
        KeyPad[1] = CHAR_NULL;
        StrCat (StringPtr, KeyPad);
        StrCat (TempString, KeyPad);
      }

      //
      // If the width of the input string is now larger than the screen, we nee to
      // adjust the index to start printing portions of the string
      //
      SetUnicodeMem (BufferedString, ScreenSize - 1, L' ');
      PrintStringAt (Start + 1, Top + 3, BufferedString);

      if ((GetStringWidth (StringPtr) / 2) > (DimensionsWidth - 2)) {
        Index = (GetStringWidth (StringPtr) / 2) - DimensionsWidth + 2;
      } else {
        Index = 0;
      }

      if (IsPassword) {
        gST->ConOut->SetCursorPosition (gST->ConOut, Start + 1, Top + 3);
      }

      for (Count = 0; Index + 1 < GetStringWidth (StringPtr) / 2; Index++, Count++) {
        BufferedString[Count] = StringPtr[Index];

        if (IsPassword) {
          PrintChar (L'*');
        }
      }

      if (!IsPassword) {
        PrintStringAt (Start + 1, Top + 3, BufferedString);
      }
      break;
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, Start + GetStringWidth (StringPtr) / 2, Top + 3);
  } while (TRUE);

}


/**
  This routine reads a numeric value from the user input.

  @param  Selection         Pointer to current selection.
  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If numerical input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetNumericInput (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption
  )
{
  EFI_STATUS              Status;
  UINTN                   Column;
  UINTN                   Row;
  CHAR16                  InputText[23];
  CHAR16                  FormattedNumber[22];
  UINT64                  PreviousNumber[20];
  UINTN                   Count;
  UINTN                   Loop;
  BOOLEAN                 ManualInput;
  BOOLEAN                 HexInput;
  BOOLEAN                 DateOrTime;
  UINTN                   InputWidth;
  UINT64                  EditValue;
  UINT64                  Step;
  UINT64                  Minimum;
  UINT64                  Maximum;
  UINTN                   EraseLen;
  UINT8                   Digital;
  EFI_INPUT_KEY           Key;
  EFI_HII_VALUE           *QuestionValue;
  FORM_BROWSER_FORM       *Form;
  FORM_BROWSER_FORMSET    *FormSet;
  FORM_BROWSER_STATEMENT  *Question;

  Column            = MenuOption->OptCol;
  Row               = MenuOption->Row;
  PreviousNumber[0] = 0;
  Count             = 0;
  InputWidth        = 0;
  Digital           = 0;

  FormSet       = Selection->FormSet;
  Form          = Selection->Form;
  Question      = MenuOption->ThisTag;
  QuestionValue = &Question->HiiValue;
  Step          = Question->Step;
  Minimum       = Question->Minimum;
  Maximum       = Question->Maximum;

  if ((Question->Operand == EFI_IFR_DATE_OP) || (Question->Operand == EFI_IFR_TIME_OP)) {
    DateOrTime = TRUE;
  } else {
    DateOrTime = FALSE;
  }

  //
  // Prepare Value to be edit
  //
  EraseLen = 0;
  EditValue = 0;
  if (Question->Operand == EFI_IFR_DATE_OP) {
    Step = 1;
    Minimum = 1;

    switch (MenuOption->Sequence) {
    case 0:
      Maximum = 12;
      EraseLen = 4;
      EditValue = QuestionValue->Value.date.Month;
      break;

    case 1:
      Maximum = 31;
      EraseLen = 3;
      EditValue = QuestionValue->Value.date.Day;
      break;

    case 2:
      Maximum = 0xffff;
      EraseLen = 5;
      EditValue = QuestionValue->Value.date.Year;
      break;

    default:
      break;
    }
  } else if (Question->Operand == EFI_IFR_TIME_OP) {
    Step = 1;
    Minimum = 0;

    switch (MenuOption->Sequence) {
    case 0:
      Maximum = 23;
      EraseLen = 4;
      EditValue = QuestionValue->Value.time.Hour;
      break;

    case 1:
      Maximum = 59;
      EraseLen = 3;
      EditValue = QuestionValue->Value.time.Minute;
      break;

    case 2:
      Maximum = 59;
      EraseLen = 3;
      EditValue = QuestionValue->Value.time.Second;
      break;

    default:
      break;
    }
  } else {
    //
    // Numeric
    //
    EraseLen = gOptionBlockWidth;
    EditValue = QuestionValue->Value.u64;
    if (Maximum == 0) {
      Maximum = (UINT64) -1;
    }
  }

  if (Step == 0) {
    ManualInput = TRUE;
  } else {
    ManualInput = FALSE;
  }

  if ((Question->Operand == EFI_IFR_NUMERIC_OP) &&
      ((Question->Flags & EFI_IFR_DISPLAY) == EFI_IFR_DISPLAY_UINT_HEX)) {
    HexInput = TRUE;
  } else {
    HexInput = FALSE;
  }

  if (ManualInput) {
    if (HexInput) {
      InputWidth = Question->StorageWidth * 2;
    } else {
      switch (Question->StorageWidth) {
      case 1:
        InputWidth = 3;
        break;

      case 2:
        InputWidth = 5;
        break;

      case 4:
        InputWidth = 10;
        break;

      case 8:
        InputWidth = 20;
        break;

      default:
        InputWidth = 0;
        break;
      }
    }

    InputText[0] = LEFT_NUMERIC_DELIMITER;
    SetUnicodeMem (InputText + 1, InputWidth, L' ');
    InputText[InputWidth + 1] = RIGHT_NUMERIC_DELIMITER;
    InputText[InputWidth + 2] = L'\0';

    PrintAt (Column, Row, InputText);
    Column++;
  }

  //
  // First time we enter this handler, we need to check to see if
  // we were passed an increment or decrement directive
  //
  do {
    Key.UnicodeChar = CHAR_NULL;
    if (gDirection != 0) {
      Key.ScanCode  = gDirection;
      gDirection    = 0;
      goto TheKey2;
    }

    Status = WaitForKeyStroke (&Key);

TheKey2:
    switch (Key.UnicodeChar) {

    case '+':
    case '-':
      if (Key.UnicodeChar == '+') {
        Key.ScanCode = SCAN_RIGHT;
      } else {
        Key.ScanCode = SCAN_LEFT;
      }
      Key.UnicodeChar = CHAR_NULL;
      goto TheKey2;

    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
      case SCAN_RIGHT:
        if (DateOrTime) {
          //
          // By setting this value, we will return back to the caller.
          // We need to do this since an auto-refresh will destroy the adjustment
          // based on what the real-time-clock is showing.  So we always commit
          // upon changing the value.
          //
          gDirection = SCAN_DOWN;
        }

        if (!ManualInput) {
          if (Key.ScanCode == SCAN_LEFT) {
            if (EditValue > Step) {
              EditValue = EditValue - Step;
            } else {
              EditValue = Minimum;
            }
          } else if (Key.ScanCode == SCAN_RIGHT) {
            EditValue = EditValue + Step;
            if (EditValue > Maximum) {
              EditValue = Maximum;
            }
          }

          ZeroMem (FormattedNumber, 21 * sizeof (CHAR16));
          if (Question->Operand == EFI_IFR_DATE_OP) {
            if (MenuOption->Sequence == 2) {
              //
              // Year
              //
              UnicodeSPrint (FormattedNumber, 21 * sizeof (CHAR16), L"%04d", EditValue);
            } else {
              //
              // Month/Day
              //
              UnicodeSPrint (FormattedNumber, 21 * sizeof (CHAR16), L"%02d", EditValue);
            }

            if (MenuOption->Sequence == 0) {
              FormattedNumber[EraseLen - 2] = DATE_SEPARATOR;
            } else if (MenuOption->Sequence == 1) {
              FormattedNumber[EraseLen - 1] = DATE_SEPARATOR;
            }
          } else if (Question->Operand == EFI_IFR_TIME_OP) {
            UnicodeSPrint (FormattedNumber, 21 * sizeof (CHAR16), L"%02d", EditValue);

            if (MenuOption->Sequence == 0) {
              FormattedNumber[EraseLen - 2] = TIME_SEPARATOR;
            } else if (MenuOption->Sequence == 1) {
              FormattedNumber[EraseLen - 1] = TIME_SEPARATOR;
            }
          } else {
            QuestionValue->Value.u64 = EditValue;
            PrintFormattedNumber (Question, FormattedNumber, 21 * sizeof (CHAR16));
          }

          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
          for (Loop = 0; Loop < EraseLen; Loop++) {
            PrintAt (MenuOption->OptCol + Loop, MenuOption->Row, L" ");
          }
          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_HIGHLIGHT | FIELD_BACKGROUND_HIGHLIGHT);

          if (MenuOption->Sequence == 0) {
            PrintCharAt (MenuOption->OptCol, Row, LEFT_NUMERIC_DELIMITER);
            Column = MenuOption->OptCol + 1;
          }

          PrintStringAt (Column, Row, FormattedNumber);

          if (!DateOrTime || MenuOption->Sequence == 2) {
            PrintChar (RIGHT_NUMERIC_DELIMITER);
          }
        }
        break;

      case SCAN_UP:
      case SCAN_DOWN:
        goto EnterCarriageReturn;

      case SCAN_ESC:
        return EFI_DEVICE_ERROR;

      default:
        break;
      }

      break;

EnterCarriageReturn:

    case CHAR_CARRIAGE_RETURN:
      //
      // Store Edit value back to Question
      //
      if (Question->Operand == EFI_IFR_DATE_OP) {
        switch (MenuOption->Sequence) {
        case 0:
          QuestionValue->Value.date.Month = (UINT8) EditValue;
          break;

        case 1:
          QuestionValue->Value.date.Day = (UINT8) EditValue;
          break;

        case 2:
          QuestionValue->Value.date.Year = (UINT16) EditValue;
          break;

        default:
          break;
        }
      } else if (Question->Operand == EFI_IFR_TIME_OP) {
        switch (MenuOption->Sequence) {
        case 0:
          QuestionValue->Value.time.Hour = (UINT8) EditValue;
          break;

        case 1:
          QuestionValue->Value.time.Minute = (UINT8) EditValue;
          break;

        case 2:
          QuestionValue->Value.time.Second = (UINT8) EditValue;
          break;

        default:
          break;
        }
      } else {
        //
        // Numeric
        //
        QuestionValue->Value.u64 = EditValue;
      }

      //
      // Check to see if the Value is something reasonable against consistency limitations.
      // If not, let's kick the error specified.
      //
      Status = ValidateQuestion (FormSet, Form, Question, EFI_HII_EXPRESSION_INCONSISTENT_IF);
      if (EFI_ERROR (Status)) {
        //
        // Input value is not valid, restore Question Value
        //
        GetQuestionValue (FormSet, Form, Question, TRUE);
      } else {
        SetQuestionValue (FormSet, Form, Question, TRUE);
        if (!DateOrTime || (Question->Storage != NULL)) {
          //
          // NV flag is unnecessary for RTC type of Date/Time
          //
          UpdateStatusBar (NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);
        }
      }

      return Status;
      break;

    case CHAR_BACKSPACE:
      if (ManualInput) {
        if (Count == 0) {
          break;
        }
        //
        // Remove a character
        //
        EditValue = PreviousNumber[Count - 1];
        UpdateStatusBar (INPUT_ERROR, Question->QuestionFlags, FALSE);
        Count--;
        Column--;
        PrintAt (Column, Row, L" ");
      }
      break;

    default:
      if (ManualInput) {
        if (HexInput) {
          if (!IsHexDigit (&Digital, Key.UnicodeChar)) {
            UpdateStatusBar (INPUT_ERROR, Question->QuestionFlags, TRUE);
            break;
          }
        } else {
          if (Key.UnicodeChar > L'9' || Key.UnicodeChar < L'0') {
            UpdateStatusBar (INPUT_ERROR, Question->QuestionFlags, TRUE);
            break;
          }
        }

        //
        // If Count exceed input width, there is no way more is valid
        //
        if (Count >= InputWidth) {
          break;
        }
        //
        // Someone typed something valid!
        //
        if (Count != 0) {
          if (HexInput) {
            EditValue = LShiftU64 (EditValue, 4) + Digital;
          } else {
            EditValue = MultU64x32 (EditValue, 10) + (Key.UnicodeChar - L'0');
          }
        } else {
          if (HexInput) {
            EditValue = Digital;
          } else {
            EditValue = Key.UnicodeChar - L'0';
          }
        }

        if (EditValue > Maximum) {
          UpdateStatusBar (INPUT_ERROR, Question->QuestionFlags, TRUE);
          EditValue = PreviousNumber[Count];
          break;
        } else {
          UpdateStatusBar (INPUT_ERROR, Question->QuestionFlags, FALSE);
        }

        Count++;
        PreviousNumber[Count] = EditValue;

        PrintCharAt (Column, Row, Key.UnicodeChar);
        Column++;
      }
      break;
    }
  } while (TRUE);

}


/**
  Get selection for OneOf and OrderedList (Left/Right will be ignored).

  @param  Selection         Pointer to current selection.
  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption
  )
{
  EFI_STATUS              Status;
  EFI_INPUT_KEY           Key;
  UINTN                   Index;
  CHAR16                  *StringPtr;
  CHAR16                  *TempStringPtr;
  UINTN                   Index2;
  UINTN                   TopOptionIndex;
  UINTN                   HighlightOptionIndex;
  UINTN                   Start;
  UINTN                   End;
  UINTN                   Top;
  UINTN                   Bottom;
  UINTN                   PopUpMenuLines;
  UINTN                   MenuLinesInView;
  UINTN                   PopUpWidth;
  CHAR16                  Character;
  INT32                   SavedAttribute;
  BOOLEAN                 ShowDownArrow;
  BOOLEAN                 ShowUpArrow;
  UINTN                   DimensionsWidth;
  LIST_ENTRY              *Link;
  BOOLEAN                 OrderedList;
  UINT8                   *ValueArray;
  EFI_HII_VALUE           HiiValue;
  EFI_HII_VALUE           *HiiValueArray;
  UINTN                   OptionCount;
  QUESTION_OPTION         *OneOfOption;
  QUESTION_OPTION         *CurrentOption;
  FORM_BROWSER_STATEMENT  *Question;

  DimensionsWidth   = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;

  ValueArray        = NULL;
  CurrentOption     = NULL;
  ShowDownArrow     = FALSE;
  ShowUpArrow       = FALSE;

  StringPtr = AllocateZeroPool ((gOptionBlockWidth + 1) * 2);
  ASSERT (StringPtr);

  Question = MenuOption->ThisTag;
  if (Question->Operand == EFI_IFR_ORDERED_LIST_OP) {
    ValueArray = Question->BufferValue;
    OrderedList = TRUE;
  } else {
    OrderedList = FALSE;
  }

  //
  // Calculate Option count
  //
  if (OrderedList) {
    for (Index = 0; Index < Question->MaxContainers; Index++) {
      if (ValueArray[Index] == 0) {
        break;
      }
    }

    OptionCount = Index;
  } else {
    OptionCount = 0;
    Link = GetFirstNode (&Question->OptionListHead);
    while (!IsNull (&Question->OptionListHead, Link)) {
      OneOfOption = QUESTION_OPTION_FROM_LINK (Link);

      OptionCount++;

      Link = GetNextNode (&Question->OptionListHead, Link);
    }
  }

  //
  // Prepare HiiValue array
  //
  HiiValueArray = AllocateZeroPool (OptionCount * sizeof (EFI_HII_VALUE));
  ASSERT (HiiValueArray != NULL);
  Link = GetFirstNode (&Question->OptionListHead);
  for (Index = 0; Index < OptionCount; Index++) {
    if (OrderedList) {
      HiiValueArray[Index].Type = EFI_IFR_TYPE_NUM_SIZE_8;
      HiiValueArray[Index].Value.u8 = ValueArray[Index];
    } else {
      OneOfOption = QUESTION_OPTION_FROM_LINK (Link);
      CopyMem (&HiiValueArray[Index], &OneOfOption->Value, sizeof (EFI_HII_VALUE));
      Link = GetNextNode (&Question->OptionListHead, Link);
    }
  }

  //
  // Move Suppressed Option to list tail
  //
  PopUpMenuLines = 0;
  for (Index = 0; Index < OptionCount; Index++) {
    OneOfOption = ValueToOption (Question, &HiiValueArray[OptionCount - Index - 1]);
    if (OneOfOption == NULL) {
      return EFI_NOT_FOUND;
    }

    RemoveEntryList (&OneOfOption->Link);

    if ((OneOfOption->SuppressExpression != NULL) &&
        (OneOfOption->SuppressExpression->Result.Value.b)) {
      //
      // This option is suppressed, insert to tail
      //
      InsertTailList (&Question->OptionListHead, &OneOfOption->Link);
    } else {
      //
      // Insert to head
      //
      InsertHeadList (&Question->OptionListHead, &OneOfOption->Link);

      PopUpMenuLines++;
    }
  }

  //
  // Get the number of one of options present and its size
  //
  PopUpWidth = 0;
  HighlightOptionIndex = 0;
  Link = GetFirstNode (&Question->OptionListHead);
  for (Index = 0; Index < PopUpMenuLines; Index++) {
    OneOfOption = QUESTION_OPTION_FROM_LINK (Link);

    StringPtr = GetToken (OneOfOption->Text, MenuOption->Handle);
    if (StrLen (StringPtr) > PopUpWidth) {
      PopUpWidth = StrLen (StringPtr);
    }
    gBS->FreePool (StringPtr);

    if (!OrderedList && CompareHiiValue (&Question->HiiValue, &OneOfOption->Value, NULL) == 0) {
      //
      // Find current selected Option for OneOf
      //
      HighlightOptionIndex = Index;
    }

    Link = GetNextNode (&Question->OptionListHead, Link);
  }

  //
  // Perform popup menu initialization.
  //
  PopUpWidth = PopUpWidth + POPUP_PAD_SPACE_COUNT;

  SavedAttribute = gST->ConOut->Mode->Attribute;
  gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);

  if ((PopUpWidth + POPUP_FRAME_WIDTH) > DimensionsWidth) {
    PopUpWidth = DimensionsWidth - POPUP_FRAME_WIDTH;
  }

  Start  = (DimensionsWidth - PopUpWidth - POPUP_FRAME_WIDTH) / 2 + gScreenDimensions.LeftColumn;
  End    = Start + PopUpWidth + POPUP_FRAME_WIDTH;
  Top    = gScreenDimensions.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT;
  Bottom = gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT - 1;

  MenuLinesInView = Bottom - Top - 1;
  if (MenuLinesInView >= PopUpMenuLines) {
    Top     = Top + (MenuLinesInView - PopUpMenuLines) / 2;
    Bottom  = Top + PopUpMenuLines + 1;
  } else {
    ShowDownArrow = TRUE;
  }

  if (HighlightOptionIndex > (MenuLinesInView - 1)) {
    TopOptionIndex = HighlightOptionIndex - MenuLinesInView + 1;
  } else {
    TopOptionIndex = 0;
  }

  do {
    //
    // Clear that portion of the screen
    //
    ClearLines (Start, End, Top, Bottom, POPUP_TEXT | POPUP_BACKGROUND);

    //
    // Draw "One of" pop-up menu
    //
    Character = BOXDRAW_DOWN_RIGHT;
    PrintCharAt (Start, Top, Character);
    for (Index = Start; Index + 2 < End; Index++) {
      if ((ShowUpArrow) && ((Index + 1) == (Start + End) / 2)) {
        Character = GEOMETRICSHAPE_UP_TRIANGLE;
      } else {
        Character = BOXDRAW_HORIZONTAL;
      }

      PrintChar (Character);
    }

    Character = BOXDRAW_DOWN_LEFT;
    PrintChar (Character);
    Character = BOXDRAW_VERTICAL;
    for (Index = Top + 1; Index < Bottom; Index++) {
      PrintCharAt (Start, Index, Character);
      PrintCharAt (End - 1, Index, Character);
    }

    //
    // Move to top Option
    //
    Link = GetFirstNode (&Question->OptionListHead);
    for (Index = 0; Index < TopOptionIndex; Index++) {
      Link = GetNextNode (&Question->OptionListHead, Link);
    }

    //
    // Display the One of options
    //
    Index2 = Top + 1;
    for (Index = TopOptionIndex; (Index < PopUpMenuLines) && (Index2 < Bottom); Index++) {
      OneOfOption = QUESTION_OPTION_FROM_LINK (Link);
      Link = GetNextNode (&Question->OptionListHead, Link);

      StringPtr = GetToken (OneOfOption->Text, MenuOption->Handle);
      //
      // If the string occupies multiple lines, truncate it to fit in one line,
      // and append a "..." for indication.
      //
      if (StrLen (StringPtr) > (PopUpWidth - 1)) {
        TempStringPtr = AllocateZeroPool (sizeof (CHAR16) * (PopUpWidth - 1));
        ASSERT ( TempStringPtr != NULL );
        CopyMem (TempStringPtr, StringPtr, (sizeof (CHAR16) * (PopUpWidth - 5)));
        gBS->FreePool (StringPtr);
        StringPtr = TempStringPtr;
        StrCat (StringPtr, L"...");
      }

      if (Index == HighlightOptionIndex) {
          //
          // Highlight the selected one
          //
          CurrentOption = OneOfOption;

          gST->ConOut->SetAttribute (gST->ConOut, PICKLIST_HIGHLIGHT_TEXT | PICKLIST_HIGHLIGHT_BACKGROUND);
          PrintStringAt (Start + 2, Index2, StringPtr);
          gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);
        } else {
          gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);
          PrintStringAt (Start + 2, Index2, StringPtr);
        }

      Index2++;
      gBS->FreePool (StringPtr);
    }

    Character = BOXDRAW_UP_RIGHT;
    PrintCharAt (Start, Bottom, Character);
    for (Index = Start; Index + 2 < End; Index++) {
      if ((ShowDownArrow) && ((Index + 1) == (Start + End) / 2)) {
        Character = GEOMETRICSHAPE_DOWN_TRIANGLE;
      } else {
        Character = BOXDRAW_HORIZONTAL;
      }

      PrintChar (Character);
    }

    Character = BOXDRAW_UP_LEFT;
    PrintChar (Character);

    //
    // Get User selection
    //
    Key.UnicodeChar = CHAR_NULL;
    if ((gDirection == SCAN_UP) || (gDirection == SCAN_DOWN)) {
      Key.ScanCode  = gDirection;
      gDirection    = 0;
      goto TheKey;
    }

    Status = WaitForKeyStroke (&Key);

TheKey:
    switch (Key.UnicodeChar) {
    case '+':
      if (OrderedList) {
        if ((TopOptionIndex > 0) && (TopOptionIndex == HighlightOptionIndex)) {
          //
          // Highlight reaches the top of the popup window, scroll one menu item.
          //
          TopOptionIndex--;
          ShowDownArrow = TRUE;
        }

        if (TopOptionIndex == 0) {
          ShowUpArrow = FALSE;
        }

        if (HighlightOptionIndex > 0) {
          HighlightOptionIndex--;

          SwapListEntries (CurrentOption->Link.BackLink, &CurrentOption->Link);
        }
      }
      break;

    case '-':
      //
      // If an ordered list op-code, we will allow for a popup of +/- keys
      // to create an ordered list of items
      //
      if (OrderedList) {
        if (((TopOptionIndex + MenuLinesInView) < PopUpMenuLines) &&
            (HighlightOptionIndex == (TopOptionIndex + MenuLinesInView - 1))) {
          //
          // Highlight reaches the bottom of the popup window, scroll one menu item.
          //
          TopOptionIndex++;
          ShowUpArrow = TRUE;
        }

        if ((TopOptionIndex + MenuLinesInView) == PopUpMenuLines) {
          ShowDownArrow = FALSE;
        }

        if (HighlightOptionIndex < (PopUpMenuLines - 1)) {
          HighlightOptionIndex++;

          SwapListEntries (&CurrentOption->Link, CurrentOption->Link.ForwardLink);
        }
      }
      break;

    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_UP:
      case SCAN_DOWN:
        if (Key.ScanCode == SCAN_UP) {
          if ((TopOptionIndex > 0) && (TopOptionIndex == HighlightOptionIndex)) {
            //
            // Highlight reaches the top of the popup window, scroll one menu item.
            //
            TopOptionIndex--;
            ShowDownArrow = TRUE;
          }

          if (TopOptionIndex == 0) {
            ShowUpArrow = FALSE;
          }

          if (HighlightOptionIndex > 0) {
            HighlightOptionIndex--;
          }
        } else {
          if (((TopOptionIndex + MenuLinesInView) < PopUpMenuLines) &&
              (HighlightOptionIndex == (TopOptionIndex + MenuLinesInView - 1))) {
            //
            // Highlight reaches the bottom of the popup window, scroll one menu item.
            //
            TopOptionIndex++;
            ShowUpArrow = TRUE;
          }

          if ((TopOptionIndex + MenuLinesInView) == PopUpMenuLines) {
            ShowDownArrow = FALSE;
          }

          if (HighlightOptionIndex < (PopUpMenuLines - 1)) {
            HighlightOptionIndex++;
          }
        }
        break;

      case SCAN_ESC:
        gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);

        //
        // Restore link list order for orderedlist
        //
        if (OrderedList) {
          HiiValue.Type = EFI_IFR_TYPE_NUM_SIZE_8;
          HiiValue.Value.u64 = 0;
          for (Index = 0; Index < Question->MaxContainers; Index++) {
            HiiValue.Value.u8 = ValueArray[Index];
            if (HiiValue.Value.u8) {
              break;
            }

            OneOfOption = ValueToOption (Question, &HiiValue);
            if (OneOfOption == NULL) {
              return EFI_NOT_FOUND;
            }

            RemoveEntryList (&OneOfOption->Link);
            InsertTailList (&Question->OptionListHead, &OneOfOption->Link);
          }
        }

        gBS->FreePool (HiiValueArray);
        return EFI_DEVICE_ERROR;

      default:
        break;
      }

      break;

    case CHAR_CARRIAGE_RETURN:
      //
      // return the current selection
      //
      if (OrderedList) {
        Index = 0;
        Link = GetFirstNode (&Question->OptionListHead);
        while (!IsNull (&Question->OptionListHead, Link)) {
          OneOfOption = QUESTION_OPTION_FROM_LINK (Link);

          Question->BufferValue[Index] = OneOfOption->Value.Value.u8;

          Index++;
          if (Index > Question->MaxContainers) {
            break;
          }

          Link = GetNextNode (&Question->OptionListHead, Link);
        }
      } else {
        CopyMem (&Question->HiiValue, &CurrentOption->Value, sizeof (EFI_HII_VALUE));
      }

      gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);
      gBS->FreePool (HiiValueArray);

      Status = ValidateQuestion (Selection->FormSet, Selection->Form, Question, EFI_HII_EXPRESSION_INCONSISTENT_IF);
      if (EFI_ERROR (Status)) {
        //
        // Input value is not valid, restore Question Value
        //
        GetQuestionValue (Selection->FormSet, Selection->Form, Question, TRUE);
      } else {
        SetQuestionValue (Selection->FormSet, Selection->Form, Question, TRUE);
        UpdateStatusBar (NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);
      }

      return Status;

    default:
      break;
    }
  } while (TRUE);

}

EFI_STATUS
WaitForKeyStroke (
  OUT  EFI_INPUT_KEY           *Key
  )
{
  EFI_STATUS  Status;

  do {
    UiWaitForSingleEvent (gST->ConIn->WaitForKey, 0, 0);
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, Key);
  } while (EFI_ERROR(Status));

  return Status;
}
