/** @file
Implementation for handling user input from the User Interfaces.

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FormDisplay.h"

/**
  Get maximum and minimum info from this opcode.

  @param  OpCode            Pointer to the current input opcode.
  @param  Minimum           The minimum size info for this opcode.
  @param  Maximum           The maximum size info for this opcode.

**/
VOID
GetFieldFromOp (
  IN   EFI_IFR_OP_HEADER       *OpCode,
  OUT  UINTN                   *Minimum,
  OUT  UINTN                   *Maximum
  )
{
  EFI_IFR_STRING    *StringOp;
  EFI_IFR_PASSWORD  *PasswordOp;
  if (OpCode->OpCode == EFI_IFR_STRING_OP) {
    StringOp = (EFI_IFR_STRING *) OpCode;
    *Minimum = StringOp->MinSize;
    *Maximum = StringOp->MaxSize;    
  } else if (OpCode->OpCode == EFI_IFR_PASSWORD_OP) {
    PasswordOp = (EFI_IFR_PASSWORD *) OpCode;
    *Minimum = PasswordOp->MinSize;
    *Maximum = PasswordOp->MaxSize;       
  } else {
    *Minimum = 0;
    *Maximum = 0;       
  }
}

/**
  Get string or password input from user.

  @param  MenuOption        Pointer to the current input menu.
  @param  Prompt            The prompt string shown on popup window.
  @param  StringPtr         Old user input and destination for use input string.

  @retval EFI_SUCCESS       If string input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
ReadString (
  IN     UI_MENU_OPTION              *MenuOption,
  IN     CHAR16                      *Prompt,
  IN OUT CHAR16                      *StringPtr
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
  UINTN                   Index2;
  UINTN                   Count;
  UINTN                   Start;
  UINTN                   Top;
  UINTN                   DimensionsWidth;
  UINTN                   DimensionsHeight;
  UINTN                   CurrentCursor;
  BOOLEAN                 CursorVisible;
  UINTN                   Minimum;
  UINTN                   Maximum;
  FORM_DISPLAY_ENGINE_STATEMENT  *Question;
  BOOLEAN                 IsPassword;
  UINTN                   MaxLen;

  DimensionsWidth  = gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn;
  DimensionsHeight = gStatementDimensions.BottomRow - gStatementDimensions.TopRow;

  NullCharacter    = CHAR_NULL;
  ScreenSize       = GetStringWidth (Prompt) / sizeof (CHAR16);
  Space[0]         = L' ';
  Space[1]         = CHAR_NULL;

  Question         = MenuOption->ThisTag;
  GetFieldFromOp(Question->OpCode, &Minimum, &Maximum);

  if (Question->OpCode->OpCode == EFI_IFR_PASSWORD_OP) {
    IsPassword = TRUE;
  } else {
    IsPassword = FALSE;
  }

  MaxLen = Maximum + 1;
  TempString = AllocateZeroPool (MaxLen * sizeof (CHAR16));
  ASSERT (TempString);

  if (ScreenSize < (Maximum + 1)) {
    ScreenSize = Maximum + 1;
  }

  if ((ScreenSize + 2) > DimensionsWidth) {
    ScreenSize = DimensionsWidth - 2;
  }

  BufferedString = AllocateZeroPool (ScreenSize * 2);
  ASSERT (BufferedString);

  Start = (DimensionsWidth - ScreenSize - 2) / 2 + gStatementDimensions.LeftColumn + 1;
  Top   = ((DimensionsHeight - 6) / 2) + gStatementDimensions.TopRow - 1;

  //
  // Display prompt for string
  //
  // CreateDialog (NULL, "", Prompt, Space, "", NULL);
  CreateMultiStringPopUp (ScreenSize, 4, &NullCharacter, Prompt, Space, &NullCharacter);
  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));

  CursorVisible = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);

  CurrentCursor = GetStringWidth (StringPtr) / 2 - 1;
  if (CurrentCursor != 0) {
    //
    // Show the string which has beed saved before.
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
        PrintCharAt ((UINTN)-1, (UINTN)-1, L'*');
      }
    }

    if (!IsPassword) {
      PrintStringAt (Start + 1, Top + 3, BufferedString);
    }
    
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, Start + GetStringWidth (StringPtr) / 2, Top + 3);
  }
  
  do {
    Status = WaitForKeyStroke (&Key);
    ASSERT_EFI_ERROR (Status);

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_BLACK, EFI_LIGHTGRAY));
    switch (Key.UnicodeChar) {
    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
        if (CurrentCursor > 0) {
          CurrentCursor--;
        }
        break;

      case SCAN_RIGHT:
        if (CurrentCursor < (GetStringWidth (StringPtr) / 2 - 1)) {
          CurrentCursor++;
        }
        break;

      case SCAN_ESC:
        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;

       case SCAN_DELETE:
        for (Index = CurrentCursor; StringPtr[Index] != CHAR_NULL; Index++) {
          StringPtr[Index] = StringPtr[Index + 1];
          PrintCharAt (Start + Index + 1, Top + 3, IsPassword && StringPtr[Index] != CHAR_NULL? L'*' : StringPtr[Index]);
        }
        break;

      default:
        break;
      }

      break;

    case CHAR_CARRIAGE_RETURN:
      if (GetStringWidth (StringPtr) >= ((Minimum + 1) * sizeof (CHAR16))) {

        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_SUCCESS;
      } else {
        //
        // Simply create a popup to tell the user that they had typed in too few characters.
        // To save code space, we can then treat this as an error and return back to the menu.
        //
        do {
          CreateDialog (&Key, &NullCharacter, gMiniString, gPressEnter, &NullCharacter, NULL);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        FreePool (TempString);
        FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
        gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
        return EFI_DEVICE_ERROR;
      }


    case CHAR_BACKSPACE:
      if (StringPtr[0] != CHAR_NULL && CurrentCursor != 0) {
        for (Index = 0; Index < CurrentCursor - 1; Index++) {
          TempString[Index] = StringPtr[Index];
        }
        Count = GetStringWidth (StringPtr) / 2 - 1;
        if (Count >= CurrentCursor) {
          for (Index = CurrentCursor - 1, Index2 = CurrentCursor; Index2 < Count; Index++, Index2++) {
            TempString[Index] = StringPtr[Index2];
          }
          TempString[Index] = CHAR_NULL;
        }
        //
        // Effectively truncate string by 1 character
        //
        StrCpyS (StringPtr, MaxLen, TempString);
        CurrentCursor --;
      }

    default:
      //
      // If it is the beginning of the string, don't worry about checking maximum limits
      //
      if ((StringPtr[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        StrnCpyS (StringPtr, MaxLen, &Key.UnicodeChar, 1);
        CurrentCursor++;
      } else if ((GetStringWidth (StringPtr) < ((Maximum + 1) * sizeof (CHAR16))) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
        KeyPad[0] = Key.UnicodeChar;
        KeyPad[1] = CHAR_NULL;
        Count = GetStringWidth (StringPtr) / 2 - 1;
        if (CurrentCursor < Count) {
          for (Index = 0; Index < CurrentCursor; Index++) {
            TempString[Index] = StringPtr[Index];
          }
		  TempString[Index] = CHAR_NULL;
          StrCatS (TempString, MaxLen, KeyPad);
          StrCatS (TempString, MaxLen, StringPtr + CurrentCursor);
          StrCpyS (StringPtr, MaxLen, TempString);
        } else {
          StrCatS (StringPtr, MaxLen, KeyPad);
        }
        CurrentCursor++;
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
          PrintCharAt ((UINTN)-1, (UINTN)-1, L'*');
        }
      }

      if (!IsPassword) {
        PrintStringAt (Start + 1, Top + 3, BufferedString);
      }
      break;
    }

    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    gST->ConOut->SetCursorPosition (gST->ConOut, Start + CurrentCursor + 1, Top + 3);
  } while (TRUE);

}

/**
  Adjust the value to the correct one. Rules follow the sample:
  like:  Year change:  2012.02.29 -> 2013.02.29 -> 2013.02.01
         Month change: 2013.03.29 -> 2013.02.29 -> 2013.02.28

  @param  QuestionValue     Pointer to current question.
  @param  Sequence          The sequence of the field in the question.
**/
VOID
AdjustQuestionValue (
  IN  EFI_HII_VALUE           *QuestionValue,
  IN  UINT8                   Sequence
  )
{
  UINT8     Month;
  UINT16    Year;
  UINT8     Maximum;
  UINT8     Minimum;

  Month   = QuestionValue->Value.date.Month;
  Year    = QuestionValue->Value.date.Year;
  Minimum = 1;

  switch (Month) {
  case 2:
    if ((Year % 4) == 0 && ((Year % 100) != 0 || (Year % 400) == 0)) {
      Maximum = 29;
    } else {
      Maximum = 28;
    }
    break;
  case 4:
  case 6:
  case 9:
  case 11:
    Maximum = 30;
    break;
  default:
    Maximum = 31;
    break;
  }

  //
  // Change the month area.
  //
  if (Sequence == 0) {
    if (QuestionValue->Value.date.Day > Maximum) {
      QuestionValue->Value.date.Day = Maximum;
    }
  }
  
  //
  // Change the Year area.
  //
  if (Sequence == 2) {
    if (QuestionValue->Value.date.Day > Maximum) {
      QuestionValue->Value.date.Day = Minimum;
    }
  }
}

/**
  Get field info from numeric opcode.

  @param  OpCode            Pointer to the current input opcode.
  @param  IntInput          Whether question shows with EFI_IFR_DISPLAY_INT_DEC type.
  @param  QuestionValue     Input question value, with EFI_HII_VALUE type.
  @param  Value             Return question value, always return UINT64 type.
  @param  Minimum           The minimum size info for this opcode.
  @param  Maximum           The maximum size info for this opcode.
  @param  Step              The step size info for this opcode.
  @param  StorageWidth      The storage width info for this opcode.

**/
VOID
GetValueFromNum (
  IN  EFI_IFR_OP_HEADER     *OpCode,
  IN  BOOLEAN               IntInput,
  IN  EFI_HII_VALUE         *QuestionValue,
  OUT UINT64                *Value,
  OUT UINT64                *Minimum,
  OUT UINT64                *Maximum,
  OUT UINT64                *Step,
  OUT UINT16                *StorageWidth
)
{
  EFI_IFR_NUMERIC       *NumericOp;

  NumericOp = (EFI_IFR_NUMERIC *) OpCode;
  
  switch (NumericOp->Flags & EFI_IFR_NUMERIC_SIZE) {
  case EFI_IFR_NUMERIC_SIZE_1:
    if (IntInput) {
      *Minimum = (INT64) (INT8) NumericOp->data.u8.MinValue;
      *Maximum = (INT64) (INT8) NumericOp->data.u8.MaxValue;
      *Value   = (INT64) (INT8) QuestionValue->Value.u8;
    } else {
      *Minimum = NumericOp->data.u8.MinValue;
      *Maximum = NumericOp->data.u8.MaxValue;
      *Value   = QuestionValue->Value.u8;
    }
    *Step    = NumericOp->data.u8.Step;
    *StorageWidth = (UINT16) sizeof (UINT8);
    break;
  
  case EFI_IFR_NUMERIC_SIZE_2:
    if (IntInput) {
      *Minimum = (INT64) (INT16) NumericOp->data.u16.MinValue;
      *Maximum = (INT64) (INT16) NumericOp->data.u16.MaxValue;
      *Value   = (INT64) (INT16) QuestionValue->Value.u16;
    } else {
      *Minimum = NumericOp->data.u16.MinValue;
      *Maximum = NumericOp->data.u16.MaxValue;
      *Value   = QuestionValue->Value.u16;
    }
    *Step    = NumericOp->data.u16.Step;
    *StorageWidth = (UINT16) sizeof (UINT16);
    break;
  
  case EFI_IFR_NUMERIC_SIZE_4:
    if (IntInput) {
      *Minimum = (INT64) (INT32) NumericOp->data.u32.MinValue;
      *Maximum = (INT64) (INT32) NumericOp->data.u32.MaxValue;
      *Value   = (INT64) (INT32) QuestionValue->Value.u32;
    } else {
      *Minimum = NumericOp->data.u32.MinValue;
      *Maximum = NumericOp->data.u32.MaxValue;
      *Value   = QuestionValue->Value.u32;
    }
    *Step    = NumericOp->data.u32.Step;
    *StorageWidth = (UINT16) sizeof (UINT32);
    break;
  
  case EFI_IFR_NUMERIC_SIZE_8:
    if (IntInput) {
      *Minimum = (INT64) NumericOp->data.u64.MinValue;
      *Maximum = (INT64) NumericOp->data.u64.MaxValue;
      *Value   = (INT64) QuestionValue->Value.u64;
    } else {
      *Minimum = NumericOp->data.u64.MinValue;
      *Maximum = NumericOp->data.u64.MaxValue;
      *Value   = QuestionValue->Value.u64;
    }
    *Step    = NumericOp->data.u64.Step;
    *StorageWidth = (UINT16) sizeof (UINT64);
    break;
  
  default:
    break;
  }

  if (*Maximum == 0) {
    *Maximum = (UINT64) -1;
  }
}

/**
  This routine reads a numeric value from the user input.

  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If numerical input is read successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetNumericInput (
  IN  UI_MENU_OPTION              *MenuOption
  )
{
  UINTN                   Column;
  UINTN                   Row;
  CHAR16                  InputText[MAX_NUMERIC_INPUT_WIDTH];
  CHAR16                  FormattedNumber[MAX_NUMERIC_INPUT_WIDTH - 1];
  UINT64                  PreviousNumber[MAX_NUMERIC_INPUT_WIDTH - 3];
  UINTN                   Count;
  UINTN                   Loop;
  BOOLEAN                 ManualInput;
  BOOLEAN                 HexInput;
  BOOLEAN                 IntInput;
  BOOLEAN                 Negative;
  BOOLEAN                 ValidateFail;
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
  FORM_DISPLAY_ENGINE_STATEMENT  *Question;
  EFI_IFR_NUMERIC                *NumericOp;
  UINT16                         StorageWidth;

  Column            = MenuOption->OptCol;
  Row               = MenuOption->Row;
  PreviousNumber[0] = 0;
  Count             = 0;
  InputWidth        = 0;
  Digital           = 0;
  StorageWidth      = 0;
  Minimum           = 0;
  Maximum           = 0;
  NumericOp         = NULL;
  IntInput          = FALSE;
  HexInput          = FALSE;
  Negative          = FALSE;
  ValidateFail      = FALSE;

  Question      = MenuOption->ThisTag;
  QuestionValue = &Question->CurrentValue;
  ZeroMem (InputText, MAX_NUMERIC_INPUT_WIDTH * sizeof (CHAR16));

  //
  // Only two case, user can enter to this function: Enter and +/- case.
  // In Enter case, gDirection = 0; in +/- case, gDirection = SCAN_LEFT/SCAN_WRIGHT
  //
  ManualInput        = (BOOLEAN)(gDirection == 0 ? TRUE : FALSE);

  if ((Question->OpCode->OpCode == EFI_IFR_DATE_OP) || (Question->OpCode->OpCode == EFI_IFR_TIME_OP)) {
    DateOrTime = TRUE;
  } else {
    DateOrTime = FALSE;
  }

  //
  // Prepare Value to be edit
  //
  EraseLen = 0;
  EditValue = 0;
  if (Question->OpCode->OpCode == EFI_IFR_DATE_OP) {
    Step = 1;
    Minimum = 1;

    switch (MenuOption->Sequence) {
    case 0:
      Maximum = 12;
      EraseLen = 4;
      EditValue = QuestionValue->Value.date.Month;
      break;

    case 1:
      switch (QuestionValue->Value.date.Month) {
      case 2:
        if ((QuestionValue->Value.date.Year % 4) == 0  && 
            ((QuestionValue->Value.date.Year % 100) != 0 || 
            (QuestionValue->Value.date.Year % 400) == 0)) {
          Maximum = 29;
        } else {
          Maximum = 28;
        }
        break;
      case 4:
      case 6:
      case 9:
      case 11:
        Maximum = 30;
        break;
      default:
        Maximum = 31;
        break;
      } 

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
  } else if (Question->OpCode->OpCode == EFI_IFR_TIME_OP) {
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
    ASSERT (Question->OpCode->OpCode == EFI_IFR_NUMERIC_OP);
    NumericOp = (EFI_IFR_NUMERIC *) Question->OpCode;
    GetValueFromNum(Question->OpCode, (NumericOp->Flags & EFI_IFR_DISPLAY) == 0, QuestionValue, &EditValue, &Minimum, &Maximum, &Step, &StorageWidth);
    EraseLen  = gOptionBlockWidth;
  }

  if ((Question->OpCode->OpCode == EFI_IFR_NUMERIC_OP) && (NumericOp != NULL)) {
    if ((NumericOp->Flags & EFI_IFR_DISPLAY) == EFI_IFR_DISPLAY_UINT_HEX){
      HexInput = TRUE;
    } else if ((NumericOp->Flags & EFI_IFR_DISPLAY) == 0){
      //
      // Display with EFI_IFR_DISPLAY_INT_DEC type. Support negative number.
      //
      IntInput = TRUE;
    }
  }

  //
  // Enter from "Enter" input, clear the old word showing.
  //
  if (ManualInput) {
    if (Question->OpCode->OpCode == EFI_IFR_NUMERIC_OP) {
      if (HexInput) {
        InputWidth = StorageWidth * 2;
      } else {
        switch (StorageWidth) {
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

        if (IntInput) {
          //
          // Support an extra '-' for negative number.
          //
          InputWidth += 1;
        }
      }

      InputText[0] = LEFT_NUMERIC_DELIMITER;
      SetUnicodeMem (InputText + 1, InputWidth, L' ');
      ASSERT (InputWidth + 2 < MAX_NUMERIC_INPUT_WIDTH);
      InputText[InputWidth + 1] = RIGHT_NUMERIC_DELIMITER;
      InputText[InputWidth + 2] = L'\0';

      PrintStringAt (Column, Row, InputText);
      Column++;
    }

    if (Question->OpCode->OpCode == EFI_IFR_DATE_OP) {
      if (MenuOption->Sequence == 2) {
        InputWidth = 4;
      } else {
        InputWidth = 2;
      }

      if (MenuOption->Sequence == 0) {
        InputText[0] = LEFT_NUMERIC_DELIMITER;
        SetUnicodeMem (InputText + 1, InputWidth, L' ');
        InputText[InputWidth + 1] = DATE_SEPARATOR;
        InputText[InputWidth + 2] = L'\0';
      } else  if (MenuOption->Sequence == 1){
        SetUnicodeMem (InputText, InputWidth, L' ');
        InputText[InputWidth] = DATE_SEPARATOR;
        InputText[InputWidth + 1] = L'\0';
      } else {
        SetUnicodeMem (InputText, InputWidth, L' ');
        InputText[InputWidth] = RIGHT_NUMERIC_DELIMITER;
        InputText[InputWidth + 1] = L'\0';
      }

      PrintStringAt (Column, Row, InputText);
      if (MenuOption->Sequence == 0) {
        Column++;
      }
    }

    if (Question->OpCode->OpCode == EFI_IFR_TIME_OP) {
      InputWidth = 2;

      if (MenuOption->Sequence == 0) {
        InputText[0] = LEFT_NUMERIC_DELIMITER;
        SetUnicodeMem (InputText + 1, InputWidth, L' ');
        InputText[InputWidth + 1] = TIME_SEPARATOR;
        InputText[InputWidth + 2] = L'\0';
      } else if (MenuOption->Sequence == 1){
        SetUnicodeMem (InputText, InputWidth, L' ');
        InputText[InputWidth] = TIME_SEPARATOR;
        InputText[InputWidth + 1] = L'\0';
      } else {
        SetUnicodeMem (InputText, InputWidth, L' ');
        InputText[InputWidth] = RIGHT_NUMERIC_DELIMITER;
        InputText[InputWidth + 1] = L'\0';
      }

      PrintStringAt (Column, Row, InputText);
      if (MenuOption->Sequence == 0) {
        Column++;
      }
    }
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

    WaitForKeyStroke (&Key);

TheKey2:
    switch (Key.UnicodeChar) {

    case '+':
    case '-':
      if (ManualInput && IntInput) {
        //
        // In Manual input mode, check whether input the negative flag.
        //
        if (Key.UnicodeChar == '-') {
          if (Negative) {
            break;
          }
          Negative = TRUE;
          PrintCharAt (Column++, Row, Key.UnicodeChar);
        }
      } else {
        if (Key.UnicodeChar == '+') {
          Key.ScanCode = SCAN_RIGHT;
        } else {
          Key.ScanCode = SCAN_LEFT;
        }
        Key.UnicodeChar = CHAR_NULL;
        goto TheKey2;
      }
      break;

    case CHAR_NULL:
      switch (Key.ScanCode) {
      case SCAN_LEFT:
      case SCAN_RIGHT:
        if (DateOrTime && !ManualInput) {
          //
          // By setting this value, we will return back to the caller.
          // We need to do this since an auto-refresh will destroy the adjustment
          // based on what the real-time-clock is showing.  So we always commit
          // upon changing the value.
          //
          gDirection = SCAN_DOWN;
        }

        if ((Step != 0) && !ManualInput) {
          if (Key.ScanCode == SCAN_LEFT) {
            if (IntInput) {
              if ((INT64) EditValue >= (INT64) Minimum + (INT64) Step) {
                EditValue = EditValue - Step;
              } else if ((INT64) EditValue > (INT64) Minimum){
                EditValue = Minimum;
              } else {
                EditValue = Maximum;
              }
            } else {
              if (EditValue >= Minimum + Step) {
                EditValue = EditValue - Step;
              } else if (EditValue > Minimum){
                EditValue = Minimum;
              } else {
                EditValue = Maximum;
              }
            }
          } else if (Key.ScanCode == SCAN_RIGHT) {
            if (IntInput) {
              if ((INT64) EditValue + (INT64) Step <= (INT64) Maximum) {
                EditValue = EditValue + Step;
              } else if ((INT64) EditValue < (INT64) Maximum) {
                EditValue = Maximum;
              } else {
                EditValue = Minimum;
              }
            } else {
              if (EditValue + Step <= Maximum) {
                EditValue = EditValue + Step;
              } else if (EditValue < Maximum) {
                EditValue = Maximum;
              } else {
                EditValue = Minimum;
              }
            }
          }

          ZeroMem (FormattedNumber, 21 * sizeof (CHAR16));
          if (Question->OpCode->OpCode == EFI_IFR_DATE_OP) {
            if (MenuOption->Sequence == 2) {
              //
              // Year
              //
              UnicodeSPrint (FormattedNumber, 21 * sizeof (CHAR16), L"%04d", (UINT16) EditValue);
            } else {
              //
              // Month/Day
              //
              UnicodeSPrint (FormattedNumber, 21 * sizeof (CHAR16), L"%02d", (UINT8) EditValue);
            }

            if (MenuOption->Sequence == 0) {
              ASSERT (EraseLen >= 2);
              FormattedNumber[EraseLen - 2] = DATE_SEPARATOR;
            } else if (MenuOption->Sequence == 1) {
              ASSERT (EraseLen >= 1);
              FormattedNumber[EraseLen - 1] = DATE_SEPARATOR;
            }
          } else if (Question->OpCode->OpCode == EFI_IFR_TIME_OP) {
            UnicodeSPrint (FormattedNumber, 21 * sizeof (CHAR16), L"%02d", (UINT8) EditValue);

            if (MenuOption->Sequence == 0) {
              ASSERT (EraseLen >= 2);
              FormattedNumber[EraseLen - 2] = TIME_SEPARATOR;
            } else if (MenuOption->Sequence == 1) {
              ASSERT (EraseLen >= 1);
              FormattedNumber[EraseLen - 1] = TIME_SEPARATOR;
            }
          } else {
            QuestionValue->Value.u64 = EditValue;
            PrintFormattedNumber (Question, FormattedNumber, 21 * sizeof (CHAR16));
          }

          gST->ConOut->SetAttribute (gST->ConOut, GetFieldTextColor ());
          for (Loop = 0; Loop < EraseLen; Loop++) {
            PrintStringAt (MenuOption->OptCol + Loop, MenuOption->Row, L" ");
          }
          gST->ConOut->SetAttribute (gST->ConOut, GetHighlightTextColor ());

          if (MenuOption->Sequence == 0) {
            PrintCharAt (MenuOption->OptCol, Row, LEFT_NUMERIC_DELIMITER);
            Column = MenuOption->OptCol + 1;
          }

          PrintStringAt (Column, Row, FormattedNumber);

          if (!DateOrTime || MenuOption->Sequence == 2) {
            PrintCharAt ((UINTN)-1, (UINTN)-1, RIGHT_NUMERIC_DELIMITER);
          }
        }

        goto EnterCarriageReturn;

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
      // Validate input value with Minimum value.
      //
      ValidateFail = FALSE;
      if (IntInput) {
        //
        // After user input Enter, need to check whether the input value.
        // If input a negative value, should compare with maximum value. 
        // else compare with the minimum value.
        //
        if (Negative) {
          ValidateFail = (INT64) EditValue > (INT64) Maximum ? TRUE : FALSE;
        } else {
          ValidateFail = (INT64) EditValue < (INT64) Minimum ? TRUE : FALSE;
        }

        if (ValidateFail) {
          UpdateStatusBar (INPUT_ERROR, TRUE);
          break;
        }
      } else if (EditValue < Minimum) {
        UpdateStatusBar (INPUT_ERROR, TRUE);
        break;
      }

      UpdateStatusBar (INPUT_ERROR, FALSE);
      CopyMem (&gUserInput->InputValue, &Question->CurrentValue, sizeof (EFI_HII_VALUE));
      QuestionValue = &gUserInput->InputValue;
      //
      // Store Edit value back to Question
      //
      if (Question->OpCode->OpCode == EFI_IFR_DATE_OP) {
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
      } else if (Question->OpCode->OpCode  == EFI_IFR_TIME_OP) {
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
      // Adjust the value to the correct one.
      // Sample like: 2012.02.29 -> 2013.02.29 -> 2013.02.01
      //              2013.03.29 -> 2013.02.29 -> 2013.02.28
      //
      if (Question->OpCode->OpCode  == EFI_IFR_DATE_OP && 
        (MenuOption->Sequence == 0 || MenuOption->Sequence == 2)) {
        AdjustQuestionValue (QuestionValue, (UINT8)MenuOption->Sequence);
      }

      return EFI_SUCCESS;

    case CHAR_BACKSPACE:
      if (ManualInput) {
        if (Count == 0) {
          if (Negative) {
            Negative = FALSE;
            Column--;
            PrintStringAt (Column, Row, L" ");
          }
          break;
        }
        //
        // Remove a character
        //
        EditValue = PreviousNumber[Count - 1];
        UpdateStatusBar (INPUT_ERROR,  FALSE);
        Count--;
        Column--;
        PrintStringAt (Column, Row, L" ");
      }
      break;

    default:
      if (ManualInput) {
        if (HexInput) {
          if ((Key.UnicodeChar >= L'0') && (Key.UnicodeChar <= L'9')) {
            Digital = (UINT8) (Key.UnicodeChar - L'0');
          } else if ((Key.UnicodeChar >= L'A') && (Key.UnicodeChar <= L'F')) {
            Digital = (UINT8) (Key.UnicodeChar - L'A' + 0x0A);
          } else if ((Key.UnicodeChar >= L'a') && (Key.UnicodeChar <= L'f')) {
            Digital = (UINT8) (Key.UnicodeChar - L'a' + 0x0A);
          } else {
            UpdateStatusBar (INPUT_ERROR, TRUE);
            break;
          }
        } else {
          if (Key.UnicodeChar > L'9' || Key.UnicodeChar < L'0') {
            UpdateStatusBar (INPUT_ERROR, TRUE);
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
          } else if (IntInput && Negative) {
            //
            // Save the negative number.
            //
            EditValue = ~(MultU64x32 (~(EditValue - 1), 10) + (Key.UnicodeChar - L'0')) + 1;
          } else {
            EditValue = MultU64x32 (EditValue, 10) + (Key.UnicodeChar - L'0');
          }
        } else {
          if (HexInput) {
            EditValue = Digital;
          } else if (IntInput && Negative) {
            //
            // Save the negative number.
            //
            EditValue = ~(Key.UnicodeChar - L'0') + 1;
          } else {
            EditValue = Key.UnicodeChar - L'0';
          }
        }

        if (IntInput) {
          ValidateFail = FALSE;
          //
          // When user input a new value, should check the current value.
          // If user input a negative value, should compare it with minimum
          // value, else compare it with maximum value.
          //
          if (Negative) {
            ValidateFail = (INT64) EditValue < (INT64) Minimum ? TRUE : FALSE;
          } else {
            ValidateFail = (INT64) EditValue > (INT64) Maximum ? TRUE : FALSE;
          }

          if (ValidateFail) {
            UpdateStatusBar (INPUT_ERROR, TRUE);
            ASSERT (Count < ARRAY_SIZE (PreviousNumber));
            EditValue = PreviousNumber[Count];
            break;
          }
        } else {
          if (EditValue > Maximum) {
            UpdateStatusBar (INPUT_ERROR, TRUE);
            ASSERT (Count < ARRAY_SIZE (PreviousNumber));
            EditValue = PreviousNumber[Count];
            break;
          }
        }

        UpdateStatusBar (INPUT_ERROR, FALSE);

        Count++;
        ASSERT (Count < (ARRAY_SIZE (PreviousNumber)));
        PreviousNumber[Count] = EditValue;

        gST->ConOut->SetAttribute (gST->ConOut, GetHighlightTextColor ());
        PrintCharAt (Column, Row, Key.UnicodeChar);
        Column++;
      }
      break;
    }
  } while (TRUE);
}

/**
  Adjust option order base on the question value.

  @param  Question           Pointer to current question.
  @param  PopUpMenuLines     The line number of the pop up menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
AdjustOptionOrder (
  IN  FORM_DISPLAY_ENGINE_STATEMENT  *Question,
  OUT UINTN                          *PopUpMenuLines
  )
{
  UINTN                   Index;
  EFI_IFR_ORDERED_LIST    *OrderList;
  UINT8                   *ValueArray;
  UINT8                   ValueType;
  LIST_ENTRY              *Link;
  DISPLAY_QUESTION_OPTION *OneOfOption;
  EFI_HII_VALUE           *HiiValueArray;

  Link        = GetFirstNode (&Question->OptionListHead);
  OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
  ValueArray  = Question->CurrentValue.Buffer;
  ValueType   =  OneOfOption->OptionOpCode->Type;
  OrderList   = (EFI_IFR_ORDERED_LIST *) Question->OpCode;

  for (Index = 0; Index < OrderList->MaxContainers; Index++) {
    if (GetArrayData (ValueArray, ValueType, Index) == 0) {
      break;
    }
  }
  
  *PopUpMenuLines = Index;
  
  //
  // Prepare HiiValue array
  //  
  HiiValueArray = AllocateZeroPool (*PopUpMenuLines * sizeof (EFI_HII_VALUE));
  ASSERT (HiiValueArray != NULL);

  for (Index = 0; Index < *PopUpMenuLines; Index++) {
    HiiValueArray[Index].Type = ValueType;
    HiiValueArray[Index].Value.u64 = GetArrayData (ValueArray, ValueType, Index);
  }
  
  for (Index = 0; Index < *PopUpMenuLines; Index++) {
    OneOfOption = ValueToOption (Question, &HiiValueArray[*PopUpMenuLines - Index - 1]);
    if (OneOfOption == NULL) {
      return EFI_NOT_FOUND;
    }
  
    RemoveEntryList (&OneOfOption->Link);
  
    //
    // Insert to head.
    //
    InsertHeadList (&Question->OptionListHead, &OneOfOption->Link);
  }
  
  FreePool (HiiValueArray);

  return EFI_SUCCESS;
}

/**
  Base on the type to compare the value.

  @param  Value1                The first value need to compare.
  @param  Value2                The second value need to compare.
  @param  Type                  The value type for above two values.

  @retval TRUE                  The two value are same.
  @retval FALSE                 The two value are different.

**/
BOOLEAN
IsValuesEqual (
  IN EFI_IFR_TYPE_VALUE *Value1,
  IN EFI_IFR_TYPE_VALUE *Value2,
  IN UINT8              Type
  )
{
  switch (Type) {
  case EFI_IFR_TYPE_BOOLEAN:
  case EFI_IFR_TYPE_NUM_SIZE_8:
    return (BOOLEAN) (Value1->u8 == Value2->u8);
  
  case EFI_IFR_TYPE_NUM_SIZE_16:
    return (BOOLEAN) (Value1->u16 == Value2->u16);
  
  case EFI_IFR_TYPE_NUM_SIZE_32:
    return (BOOLEAN) (Value1->u32 == Value2->u32);
  
  case EFI_IFR_TYPE_NUM_SIZE_64:
    return (BOOLEAN) (Value1->u64 == Value2->u64);

  default:
    ASSERT (FALSE);
    return FALSE;
  }
}

/**
  Base on the type to set the value.

  @param  Dest                  The dest value.
  @param  Source                The source value.
  @param  Type                  The value type for above two values.

**/
VOID
SetValuesByType (
  OUT EFI_IFR_TYPE_VALUE *Dest,
  IN  EFI_IFR_TYPE_VALUE *Source,
  IN  UINT8              Type
  )
{
  switch (Type) {
  case EFI_IFR_TYPE_BOOLEAN:
    Dest->b = Source->b;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_8:
    Dest->u8 = Source->u8;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    Dest->u16 = Source->u16;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    Dest->u32 = Source->u32;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    Dest->u64 = Source->u64;
    break;

  default:
    ASSERT (FALSE);
    break;
  }
}

/**
  Get selection for OneOf and OrderedList (Left/Right will be ignored).

  @param  MenuOption        Pointer to the current input menu.

  @retval EFI_SUCCESS       If Option input is processed successfully
  @retval EFI_DEVICE_ERROR  If operation fails

**/
EFI_STATUS
GetSelectionInputPopUp (
  IN  UI_MENU_OPTION              *MenuOption
  )
{
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
  UINT8                   *ReturnValue;
  UINT8                   ValueType;
  EFI_HII_VALUE           HiiValue;
  DISPLAY_QUESTION_OPTION         *OneOfOption;
  DISPLAY_QUESTION_OPTION         *CurrentOption;
  FORM_DISPLAY_ENGINE_STATEMENT  *Question;
  INTN                    Result;
  EFI_IFR_ORDERED_LIST    *OrderList;

  DimensionsWidth   = gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn;

  ValueArray        = NULL;
  ValueType         = 0;
  CurrentOption     = NULL;
  ShowDownArrow     = FALSE;
  ShowUpArrow       = FALSE;

  ZeroMem (&HiiValue, sizeof (EFI_HII_VALUE));

  Question = MenuOption->ThisTag;
  if (Question->OpCode->OpCode == EFI_IFR_ORDERED_LIST_OP) {
    Link = GetFirstNode (&Question->OptionListHead);
    OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
    ValueArray = Question->CurrentValue.Buffer;
    ValueType =  OneOfOption->OptionOpCode->Type;
    OrderedList = TRUE;
    OrderList = (EFI_IFR_ORDERED_LIST *) Question->OpCode;
  } else {
    OrderedList = FALSE;
    OrderList = NULL;
  }

  //
  // Calculate Option count
  //
  PopUpMenuLines = 0;
  if (OrderedList) {
    AdjustOptionOrder(Question, &PopUpMenuLines);
  } else {
    Link = GetFirstNode (&Question->OptionListHead);
    while (!IsNull (&Question->OptionListHead, Link)) {
      OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
      PopUpMenuLines++;
      Link = GetNextNode (&Question->OptionListHead, Link);
    }
  }

  //
  // Get the number of one of options present and its size
  //
  PopUpWidth = 0;
  HighlightOptionIndex = 0;
  Link = GetFirstNode (&Question->OptionListHead);
  for (Index = 0; Index < PopUpMenuLines; Index++) {
    OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);

    StringPtr = GetToken (OneOfOption->OptionOpCode->Option, gFormData->HiiHandle);
    if (StrLen (StringPtr) > PopUpWidth) {
      PopUpWidth = StrLen (StringPtr);
    }
    FreePool (StringPtr);
    HiiValue.Type = OneOfOption->OptionOpCode->Type;
    SetValuesByType (&HiiValue.Value, &OneOfOption->OptionOpCode->Value, HiiValue.Type);
    if (!OrderedList && (CompareHiiValue (&Question->CurrentValue, &HiiValue, &Result, NULL) == EFI_SUCCESS) && (Result == 0)) {
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
  gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());

  if ((PopUpWidth + POPUP_FRAME_WIDTH) > DimensionsWidth) {
    PopUpWidth = DimensionsWidth - POPUP_FRAME_WIDTH;
  }

  Start  = (DimensionsWidth - PopUpWidth - POPUP_FRAME_WIDTH) / 2 + gStatementDimensions.LeftColumn;
  End    = Start + PopUpWidth + POPUP_FRAME_WIDTH;
  Top    = gStatementDimensions.TopRow;
  Bottom = gStatementDimensions.BottomRow - 1;

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
    ClearLines (Start, End, Top, Bottom, GetPopupColor ());

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

      PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
    }

    Character = BOXDRAW_DOWN_LEFT;
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
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
      OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
      Link = GetNextNode (&Question->OptionListHead, Link);

      StringPtr = GetToken (OneOfOption->OptionOpCode->Option, gFormData->HiiHandle);
      ASSERT (StringPtr != NULL);
      //
      // If the string occupies multiple lines, truncate it to fit in one line,
      // and append a "..." for indication.
      //
      if (StrLen (StringPtr) > (PopUpWidth - 1)) {
        TempStringPtr = AllocateZeroPool (sizeof (CHAR16) * (PopUpWidth - 1));
        ASSERT ( TempStringPtr != NULL );
        CopyMem (TempStringPtr, StringPtr, (sizeof (CHAR16) * (PopUpWidth - 5)));
        FreePool (StringPtr);
        StringPtr = TempStringPtr;
        StrCatS (StringPtr, PopUpWidth - 1, L"...");
      }

      if (Index == HighlightOptionIndex) {
          //
          // Highlight the selected one
          //
          CurrentOption = OneOfOption;

          gST->ConOut->SetAttribute (gST->ConOut, GetPickListColor ());
          PrintStringAt (Start + 2, Index2, StringPtr);
          gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
        } else {
          gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
          PrintStringAt (Start + 2, Index2, StringPtr);
        }

      Index2++;
      FreePool (StringPtr);
    }

    Character = BOXDRAW_UP_RIGHT;
    PrintCharAt (Start, Bottom, Character);
    for (Index = Start; Index + 2 < End; Index++) {
      if ((ShowDownArrow) && ((Index + 1) == (Start + End) / 2)) {
        Character = GEOMETRICSHAPE_DOWN_TRIANGLE;
      } else {
        Character = BOXDRAW_HORIZONTAL;
      }

      PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
    }

    Character = BOXDRAW_UP_LEFT;
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);

    //
    // Get User selection
    //
    Key.UnicodeChar = CHAR_NULL;
    if ((gDirection == SCAN_UP) || (gDirection == SCAN_DOWN)) {
      Key.ScanCode  = gDirection;
      gDirection    = 0;
      goto TheKey;
    }

    WaitForKeyStroke (&Key);

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

          ASSERT (CurrentOption != NULL);
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

          ASSERT (CurrentOption != NULL);
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
          HiiValue.Type = ValueType;
          HiiValue.Value.u64 = 0;
          for (Index = 0; Index < OrderList->MaxContainers; Index++) {
            HiiValue.Value.u64 = GetArrayData (ValueArray, ValueType, Index);
            if (HiiValue.Value.u64 == 0) {
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
        ReturnValue = AllocateZeroPool (Question->CurrentValue.BufferLen);
        ASSERT (ReturnValue != NULL);
        Index = 0;
        Link = GetFirstNode (&Question->OptionListHead);
        while (!IsNull (&Question->OptionListHead, Link)) {
          OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
          Link = GetNextNode (&Question->OptionListHead, Link);

          SetArrayData (ReturnValue, ValueType, Index, OneOfOption->OptionOpCode->Value.u64);

          Index++;
          if (Index > OrderList->MaxContainers) {
            break;
          }
        }
        if (CompareMem (ReturnValue, ValueArray, Question->CurrentValue.BufferLen) == 0) {
          FreePool (ReturnValue);
          return EFI_DEVICE_ERROR;
        } else {
          gUserInput->InputValue.Buffer = ReturnValue;
          gUserInput->InputValue.BufferLen = Question->CurrentValue.BufferLen;
        }
      } else {
        ASSERT (CurrentOption != NULL);
        gUserInput->InputValue.Type = CurrentOption->OptionOpCode->Type;
        if (IsValuesEqual (&Question->CurrentValue.Value, &CurrentOption->OptionOpCode->Value, gUserInput->InputValue.Type)) {
          return EFI_DEVICE_ERROR;
        } else {
          SetValuesByType (&gUserInput->InputValue.Value, &CurrentOption->OptionOpCode->Value, gUserInput->InputValue.Type);
        }
      }

      gST->ConOut->SetAttribute (gST->ConOut, SavedAttribute);

      return EFI_SUCCESS;
      
    default:
      break;
    }
  } while (TRUE);

}

