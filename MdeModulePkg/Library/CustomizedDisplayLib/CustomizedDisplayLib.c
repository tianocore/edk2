/** @file

  This library class defines a set of interfaces to customize Display module

Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "CustomizedDisplayLibInternal.h"

EFI_GUID  gCustomizedDisplayLibGuid = {
  0x99fdc8fd, 0x849b, 0x4eba, { 0xad, 0x13, 0xfb, 0x96, 0x99, 0xc9, 0xa, 0x4d }
};

EFI_HII_HANDLE  mCDLStringPackHandle;
UINT16          gClassOfVfr;                   // Formset class information
BOOLEAN         gLibIsFirstForm = TRUE;
BANNER_DATA     *gBannerData;

UINTN  gFooterHeight;

/**
+------------------------------------------------------------------------------+
|                                 Setup Page                                   |
+------------------------------------------------------------------------------+

Statement
Statement
Statement





+------------------------------------------------------------------------------+
|                                F9=Reset to Defaults        F10=Save          |
| ^"=Move Highlight          <Spacebar> Toggles Checkbox     Esc=Exit          |
+------------------------------------------------------------------------------+
  StatusBar
**/

/**
  This funtion defines Page Frame and Backgroud.

  Based on the above layout, it will be responsible for HeaderHeight, FooterHeight,
  StatusBarHeight and Backgroud. And, it will reserve Screen for Statement.

  @param[in]  FormData             Form Data to be shown in Page.
  @param[out] ScreenForStatement   Screen to be used for Statement. (Prompt, Value and Help)

  @return Status
**/
EFI_STATUS
EFIAPI
DisplayPageFrame (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT EFI_SCREEN_DESCRIPTOR    *ScreenForStatement
  )
{
  EFI_STATUS  Status;

  ASSERT (FormData != NULL && ScreenForStatement != NULL);
  if ((FormData == NULL) || (ScreenForStatement == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = ScreenDiemensionInfoValidate (FormData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  gClassOfVfr = FORMSET_CLASS_PLATFORM_SETUP;

  ProcessExternedOpcode (FormData);

  //
  // Calculate the ScreenForStatement.
  //
  ScreenForStatement->BottomRow = gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - gFooterHeight;
  if (gClassOfVfr == FORMSET_CLASS_FRONT_PAGE) {
    ScreenForStatement->TopRow = gScreenDimensions.TopRow + FRONT_PAGE_HEADER_HEIGHT;
  } else {
    ScreenForStatement->TopRow = gScreenDimensions.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT;
  }

  ScreenForStatement->LeftColumn  = gScreenDimensions.LeftColumn;
  ScreenForStatement->RightColumn = gScreenDimensions.RightColumn;

  if ((gLibIsFirstForm) || ((FormData->Attribute & HII_DISPLAY_MODAL) != 0)) {
    //
    // Ensure we are in Text mode
    //
    gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
    ClearLines (0, gScreenDimensions.RightColumn, 0, gScreenDimensions.BottomRow, KEYHELP_BACKGROUND);
    gLibIsFirstForm = FALSE;
  }

  //
  // Don't print frame for modal form.
  //
  if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
    return EFI_SUCCESS;
  }

  if (gClassOfVfr == FORMSET_CLASS_FRONT_PAGE) {
    PrintBannerInfo (FormData);
  }

  PrintFramework (FormData);

  UpdateStatusBar (NV_UPDATE_REQUIRED, FormData->SettingChangedFlag);

  return EFI_SUCCESS;
}

/**
  This function updates customized key panel's help information.
  The library will prepare those Strings for the basic key, ESC, Enter, Up/Down/Left/Right, +/-.
  and arrange them in Footer panel.

  @param[in]  FormData       Form Data to be shown in Page. FormData has the highlighted statement.
  @param[in]  Statement      The statement current selected.
  @param[in]  Selected       Whether or not a tag be selected. TRUE means Enter has hit this question.
**/
VOID
EFIAPI
RefreshKeyHelp (
  IN FORM_DISPLAY_ENGINE_FORM       *FormData,
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
  IN  BOOLEAN                       Selected
  )
{
  UINTN            SecCol;
  UINTN            ThdCol;
  UINTN            RightColumnOfHelp;
  UINTN            TopRowOfHelp;
  UINTN            BottomRowOfHelp;
  UINTN            StartColumnOfHelp;
  EFI_IFR_NUMERIC  *NumericOp;
  EFI_IFR_DATE     *DateOp;
  EFI_IFR_TIME     *TimeOp;
  BOOLEAN          HexDisplay;
  UINTN            ColumnWidth1;
  UINTN            ColumnWidth2;
  UINTN            ColumnWidth3;
  CHAR16           *ColumnStr1;
  CHAR16           *ColumnStr2;
  CHAR16           *ColumnStr3;

  ASSERT (FormData != NULL);
  if (FormData == NULL) {
    return;
  }

  gST->ConOut->SetAttribute (gST->ConOut, KEYHELP_TEXT | KEYHELP_BACKGROUND);

  if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
    return;
  }

  SecCol = gScreenDimensions.LeftColumn + (gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn) / 3;
  ThdCol = gScreenDimensions.LeftColumn + (gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn) / 3 * 2;

  //
  // + 2 means leave 1 space before the first hotkey info.
  //
  StartColumnOfHelp = gScreenDimensions.LeftColumn + 2;
  RightColumnOfHelp = gScreenDimensions.RightColumn - 1;
  TopRowOfHelp      = gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - gFooterHeight + 1;
  BottomRowOfHelp   = gScreenDimensions.BottomRow - STATUS_BAR_HEIGHT - 2;

  ColumnWidth1 = SecCol - StartColumnOfHelp;
  ColumnWidth2 = ThdCol - SecCol;
  ColumnWidth3 = RightColumnOfHelp - ThdCol;
  ColumnStr1   = gLibEmptyString;
  ColumnStr2   = gLibEmptyString;
  ColumnStr3   = gLibEmptyString;

  //
  // Clean the space at gScreenDimensions.LeftColumn + 1.
  //
  PrintStringAtWithWidth (StartColumnOfHelp - 1, BottomRowOfHelp, gLibEmptyString, 1);
  PrintStringAtWithWidth (StartColumnOfHelp - 1, TopRowOfHelp, gLibEmptyString, 1);

  if (Statement == NULL) {
    //
    // Print Key for Form without showable statement.
    //
    PrintHotKeyHelpString (FormData, TRUE);
    PrintStringAtWithWidth (StartColumnOfHelp, BottomRowOfHelp, gLibEmptyString, ColumnWidth1);
    PrintStringAtWithWidth (SecCol, BottomRowOfHelp, gLibEmptyString, ColumnWidth2);
    PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, gLibEmptyString, ColumnWidth1);
    if (gClassOfVfr == FORMSET_CLASS_PLATFORM_SETUP) {
      ColumnStr3 = gEscapeString;
    }

    PrintStringAtWithWidth (ThdCol, BottomRowOfHelp, ColumnStr3, ColumnWidth3);

    return;
  }

  HexDisplay = FALSE;
  NumericOp  = NULL;
  DateOp     = NULL;
  TimeOp     = NULL;
  if (Statement->OpCode->OpCode == EFI_IFR_NUMERIC_OP) {
    NumericOp  = (EFI_IFR_NUMERIC *)Statement->OpCode;
    HexDisplay = (NumericOp->Flags & EFI_IFR_DISPLAY_UINT_HEX) == EFI_IFR_DISPLAY_UINT_HEX;
  } else if (Statement->OpCode->OpCode == EFI_IFR_DATE_OP) {
    DateOp     = (EFI_IFR_DATE *)Statement->OpCode;
    HexDisplay = (DateOp->Flags & EFI_IFR_DISPLAY_UINT_HEX) == EFI_IFR_DISPLAY_UINT_HEX;
  } else if (Statement->OpCode->OpCode == EFI_IFR_TIME_OP) {
    TimeOp     = (EFI_IFR_TIME *)Statement->OpCode;
    HexDisplay = (TimeOp->Flags & EFI_IFR_DISPLAY_UINT_HEX) == EFI_IFR_DISPLAY_UINT_HEX;
  }

  switch (Statement->OpCode->OpCode) {
    case EFI_IFR_ORDERED_LIST_OP:
    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_NUMERIC_OP:
    case EFI_IFR_TIME_OP:
    case EFI_IFR_DATE_OP:
      if (!Selected) {
        PrintHotKeyHelpString (FormData, TRUE);

        if (gClassOfVfr == FORMSET_CLASS_PLATFORM_SETUP) {
          ColumnStr3 = gEscapeString;
        }

        PrintStringAtWithWidth (ThdCol, BottomRowOfHelp, ColumnStr3, ColumnWidth3);

        if ((Statement->OpCode->OpCode == EFI_IFR_DATE_OP) ||
            (Statement->OpCode->OpCode == EFI_IFR_TIME_OP))
        {
          PrintAt (
            ColumnWidth1,
            StartColumnOfHelp,
            BottomRowOfHelp,
            L"%c%c%c%c%s",
            ARROW_UP,
            ARROW_DOWN,
            ARROW_RIGHT,
            ARROW_LEFT,
            gMoveHighlight
            );
          PrintStringAtWithWidth (SecCol, BottomRowOfHelp, gEnterString, ColumnWidth2);
          PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, gAdjustNumber, ColumnWidth1);
        } else {
          PrintAt (ColumnWidth1, StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
          if ((Statement->OpCode->OpCode == EFI_IFR_NUMERIC_OP) && (NumericOp != NULL) && (LibGetFieldFromNum (Statement->OpCode) != 0)) {
            ColumnStr1 = gAdjustNumber;
          }

          PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, ColumnStr1, ColumnWidth1);
          PrintStringAtWithWidth (SecCol, BottomRowOfHelp, gEnterString, ColumnWidth2);
        }
      } else {
        PrintHotKeyHelpString (FormData, FALSE);
        PrintStringAtWithWidth (SecCol, BottomRowOfHelp, gEnterCommitString, ColumnWidth2);

        //
        // If it is a selected numeric with manual input, display different message
        //
        if ((Statement->OpCode->OpCode == EFI_IFR_NUMERIC_OP) ||
            (Statement->OpCode->OpCode == EFI_IFR_DATE_OP) ||
            (Statement->OpCode->OpCode == EFI_IFR_TIME_OP))
        {
          ColumnStr2 = HexDisplay ? gHexNumericInput : gDecNumericInput;
          PrintStringAtWithWidth (StartColumnOfHelp, BottomRowOfHelp, gLibEmptyString, ColumnWidth1);
        } else {
          PrintAt (ColumnWidth1, StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
        }

        if (Statement->OpCode->OpCode == EFI_IFR_ORDERED_LIST_OP) {
          ColumnStr1 = gPlusString;
          ColumnStr3 = gMinusString;
        }

        PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, ColumnStr1, ColumnWidth1);
        PrintStringAtWithWidth (ThdCol, TopRowOfHelp, ColumnStr3, ColumnWidth3);
        PrintStringAtWithWidth (SecCol, TopRowOfHelp, ColumnStr2, ColumnWidth2);

        PrintStringAtWithWidth (ThdCol, BottomRowOfHelp, gEnterEscapeString, ColumnWidth3);
      }

      break;

    case EFI_IFR_CHECKBOX_OP:
      PrintHotKeyHelpString (FormData, TRUE);

      if (gClassOfVfr == FORMSET_CLASS_PLATFORM_SETUP) {
        ColumnStr3 = gEscapeString;
      }

      PrintStringAtWithWidth (ThdCol, BottomRowOfHelp, ColumnStr3, ColumnWidth3);

      PrintAt (ColumnWidth1, StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      PrintStringAtWithWidth (SecCol, BottomRowOfHelp, gToggleCheckBox, ColumnWidth2);
      PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, gLibEmptyString, ColumnWidth1);
      break;

    case EFI_IFR_REF_OP:
    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
    case EFI_IFR_TEXT_OP:
    case EFI_IFR_ACTION_OP:
    case EFI_IFR_RESET_BUTTON_OP:
    case EFI_IFR_SUBTITLE_OP:
      if (!Selected) {
        PrintHotKeyHelpString (FormData, TRUE);

        if (gClassOfVfr == FORMSET_CLASS_PLATFORM_SETUP) {
          ColumnStr3 = gEscapeString;
        }

        PrintStringAtWithWidth (ThdCol, BottomRowOfHelp, ColumnStr3, ColumnWidth3);

        PrintAt (ColumnWidth1, StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
        if ((Statement->OpCode->OpCode != EFI_IFR_TEXT_OP) && (Statement->OpCode->OpCode != EFI_IFR_SUBTITLE_OP)) {
          ColumnStr2 = gEnterString;
        }

        PrintStringAtWithWidth (SecCol, BottomRowOfHelp, ColumnStr2, ColumnWidth2);
        PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, ColumnStr1, ColumnWidth1);
      } else {
        PrintHotKeyHelpString (FormData, FALSE);
        if (Statement->OpCode->OpCode != EFI_IFR_REF_OP) {
          ColumnStr2 = gEnterCommitString;
          ColumnStr3 = gEnterEscapeString;
        }

        PrintStringAtWithWidth (StartColumnOfHelp, TopRowOfHelp, ColumnStr1, ColumnWidth1);
        PrintStringAtWithWidth (StartColumnOfHelp, BottomRowOfHelp, ColumnStr1, ColumnWidth1);
        PrintStringAtWithWidth (SecCol, BottomRowOfHelp, ColumnStr2, ColumnWidth2);
        PrintStringAtWithWidth (ThdCol, BottomRowOfHelp, ColumnStr3, ColumnWidth3);
      }

      break;

    default:
      break;
  }
}

/**
  Update status bar.

  This function updates the status bar on the bottom of menu screen. It just shows StatusBar.
  Original logic in this function should be splitted out.

  @param[in]  MessageType            The type of message to be shown. InputError or Configuration Changed.
  @param[in]  State                  Show or Clear Message.
**/
VOID
EFIAPI
UpdateStatusBar (
  IN  UINTN    MessageType,
  IN  BOOLEAN  State
  )
{
  UINTN   Index;
  CHAR16  OptionWidth;

  OptionWidth = (CHAR16)((gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn) / 3);

  switch (MessageType) {
    case INPUT_ERROR:
      if (State) {
        gST->ConOut->SetAttribute (gST->ConOut, ERROR_TEXT);
        PrintStringAt (
          gScreenDimensions.LeftColumn + OptionWidth,
          gScreenDimensions.BottomRow - 1,
          gInputErrorMessage
          );
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, KEYHELP_BACKGROUND);
        for (Index = 0; Index < (LibGetStringWidth (gInputErrorMessage) - 2) / 2; Index++) {
          PrintStringAt (gScreenDimensions.LeftColumn + OptionWidth + Index, gScreenDimensions.BottomRow - 1, L"  ");
        }
      }

      break;

    case NV_UPDATE_REQUIRED:
      //
      // Global setting support. Show configuration change on every form.
      //
      if (State) {
        gST->ConOut->SetAttribute (gST->ConOut, INFO_TEXT);
        PrintStringAt (
          gScreenDimensions.LeftColumn + OptionWidth * 2,
          gScreenDimensions.BottomRow - 1,
          gNvUpdateMessage
          );
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, KEYHELP_BACKGROUND);
        for (Index = 0; Index < (LibGetStringWidth (gNvUpdateMessage) - 2) / 2; Index++) {
          PrintStringAt (
            (gScreenDimensions.LeftColumn + OptionWidth * 2 + Index),
            gScreenDimensions.BottomRow - 1,
            L"  "
            );
        }
      }

      break;

    default:
      break;
  }
}

/**
  Create popup window. It will replace CreateDialog().

  This function draws OEM/Vendor specific pop up windows.

  @param[out]  Key    User Input Key
  @param       ...    String to be shown in Popup. The variable argument list is terminated by a NULL.

**/
VOID
EFIAPI
CreateDialog (
  OUT EFI_INPUT_KEY  *Key         OPTIONAL,
  ...
  )
{
  VA_LIST        Marker;
  EFI_INPUT_KEY  KeyValue;
  EFI_STATUS     Status;
  UINTN          LargestString;
  UINTN          LineNum;
  UINTN          Index;
  UINTN          Count;
  CHAR16         Character;
  UINTN          Start;
  UINTN          End;
  UINTN          Top;
  UINTN          Bottom;
  CHAR16         *String;
  UINTN          DimensionsWidth;
  UINTN          DimensionsHeight;
  UINTN          CurrentAttribute;
  BOOLEAN        CursorVisible;

  //
  // If screen dimension info is not ready, get it from console.
  //
  if ((gScreenDimensions.RightColumn == 0) || (gScreenDimensions.BottomRow == 0)) {
    ZeroMem (&gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));
    gST->ConOut->QueryMode (
                   gST->ConOut,
                   gST->ConOut->Mode->Mode,
                   &gScreenDimensions.RightColumn,
                   &gScreenDimensions.BottomRow
                   );
  }

  DimensionsWidth  = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  LargestString = 0;
  LineNum       = 0;
  VA_START (Marker, Key);
  while ((String = VA_ARG (Marker, CHAR16 *)) != NULL) {
    LineNum++;

    if ((LibGetStringWidth (String) / 2) > LargestString) {
      LargestString = (LibGetStringWidth (String) / 2);
    }
  }

  VA_END (Marker);

  if ((LargestString + 2) > DimensionsWidth) {
    LargestString = DimensionsWidth - 2;
  }

  CurrentAttribute = gST->ConOut->Mode->Attribute;
  CursorVisible    = gST->ConOut->Mode->CursorVisible;
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());

  //
  // Subtract the PopUp width from total Columns, allow for one space extra on
  // each end plus a border.
  //
  Start = (DimensionsWidth - LargestString - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  End   = Start + LargestString + 1;

  Top    = ((DimensionsHeight - LineNum - 2) / 2) + gScreenDimensions.TopRow - 1;
  Bottom = Top + LineNum + 2;

  Character = BOXDRAW_DOWN_RIGHT;
  PrintCharAt (Start, Top, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_DOWN_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  Character = BOXDRAW_VERTICAL;

  Count = 0;
  VA_START (Marker, Key);
  for (Index = Top; Index + 2 < Bottom; Index++, Count++) {
    String = VA_ARG (Marker, CHAR16 *);

    if (String[0] == CHAR_NULL) {
      //
      // Passing in a NULL results in a blank space
      //
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
    } else if (String[0] == L' ') {
      //
      // Passing in a space results in the assumption that this is where typing will occur
      //
      ClearLines (Start + 1, End - 1, Index + 1, Index + 1, POPUP_INVERSE_TEXT | POPUP_INVERSE_BACKGROUND);
      PrintStringAt (
        ((DimensionsWidth - LibGetStringWidth (String) / 2) / 2) + gScreenDimensions.LeftColumn + 1,
        Index + 1,
        String + 1
        );
    } else {
      //
      // This will clear the background of the line - we never know who might have been
      // here before us.  This differs from the next clear in that it used the non-reverse
      // video for normal printing.
      //
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
      PrintStringAt (
        ((DimensionsWidth - LibGetStringWidth (String) / 2) / 2) + gScreenDimensions.LeftColumn + 1,
        Index + 1,
        String
        );
    }

    gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
    PrintCharAt (Start, Index + 1, Character);
    PrintCharAt (End - 1, Index + 1, Character);
  }

  VA_END (Marker);

  Character = BOXDRAW_UP_RIGHT;
  PrintCharAt (Start, Bottom - 1, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_UP_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);

  if (Key != NULL) {
    Status = WaitForKeyStroke (&KeyValue);
    ASSERT_EFI_ERROR (Status);
    CopyMem (Key, &KeyValue, sizeof (EFI_INPUT_KEY));
  }

  gST->ConOut->SetAttribute (gST->ConOut, CurrentAttribute);
  gST->ConOut->EnableCursor (gST->ConOut, CursorVisible);
}

/**
  Confirm how to handle the changed data.

  @return Action BROWSER_ACTION_SUBMIT, BROWSER_ACTION_DISCARD or other values.
**/
UINTN
EFIAPI
ConfirmDataChange (
  VOID
  )
{
  CHAR16         YesResponse;
  CHAR16         NoResponse;
  EFI_INPUT_KEY  Key;

  gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

  YesResponse = gYesResponse[0];
  NoResponse  = gNoResponse[0];

  //
  // If NV flag is up, prompt user
  //
  do {
    CreateDialog (&Key, gLibEmptyString, gSaveChanges, gAreYouSure, gLibEmptyString, NULL);
  } while
  (
   (Key.ScanCode != SCAN_ESC) &&
   ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (NoResponse | UPPER_LOWER_CASE_OFFSET)) &&
   ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (YesResponse | UPPER_LOWER_CASE_OFFSET))
  );

  if (Key.ScanCode == SCAN_ESC) {
    return BROWSER_ACTION_NONE;
  } else if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (YesResponse | UPPER_LOWER_CASE_OFFSET)) {
    return BROWSER_ACTION_SUBMIT;
  } else {
    return BROWSER_ACTION_DISCARD;
  }
}

/**
  OEM specifies whether Setup exits Page by ESC key.

  This function customized the behavior that whether Setup exits Page so that
  system able to boot when configuration is not changed.

  @retval  TRUE     Exits FrontPage
  @retval  FALSE    Don't exit FrontPage.
**/
BOOLEAN
EFIAPI
FormExitPolicy (
  VOID
  )
{
  return gClassOfVfr == FORMSET_CLASS_FRONT_PAGE ? FALSE : TRUE;
}

/**
  Set Timeout value for a ceratain Form to get user response.

  This function allows to set timeout value on a ceratain form if necessary.
  If timeout is not zero, the form will exit if user has no response in timeout.

  @param[in]  FormData   Form Data to be shown in Page

  @return 0     No timeout for this form.
  @return > 0   Timeout value in 100 ns units.
**/
UINT64
EFIAPI
FormExitTimeout (
  IN FORM_DISPLAY_ENGINE_FORM  *FormData
  )
{
  return 0;
}

//
// Print Functions
//

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  String     String pointer.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintStringAt (
  IN UINTN   Column,
  IN UINTN   Row,
  IN CHAR16  *String
  )
{
  return PrintAt (0, Column, Row, L"%s", String);
}

/**
  Prints a unicode string to the default console, at
  the supplied cursor position, using L"%s" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  String     String pointer.
  @param  Width      Width for String.

  @return Length of string printed to the console

**/
UINTN
EFIAPI
PrintStringAtWithWidth (
  IN UINTN   Column,
  IN UINTN   Row,
  IN CHAR16  *String,
  IN UINTN   Width
  )
{
  return PrintAt (Width, Column, Row, L"%s", String);
}

/**
  Prints a character to the default console, at
  the supplied cursor position, using L"%c" format.

  @param  Column     The cursor position to print the string at. When it is -1, use current Position.
  @param  Row        The cursor position to print the string at. When it is -1, use current Position.
  @param  Character  Character to print.

  @return Length of string printed to the console.

**/
UINTN
EFIAPI
PrintCharAt (
  IN UINTN  Column,
  IN UINTN  Row,
  CHAR16    Character
  )
{
  return PrintAt (0, Column, Row, L"%c", Character);
}

/**
  Clear retangle with specified text attribute.

  @param  LeftColumn     Left column of retangle.
  @param  RightColumn    Right column of retangle.
  @param  TopRow         Start row of retangle.
  @param  BottomRow      End row of retangle.
  @param  TextAttribute  The character foreground and background.

**/
VOID
EFIAPI
ClearLines (
  IN UINTN  LeftColumn,
  IN UINTN  RightColumn,
  IN UINTN  TopRow,
  IN UINTN  BottomRow,
  IN UINTN  TextAttribute
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
  LibSetUnicodeMem (Buffer, RightColumn - LeftColumn, L' ');

  //
  // Clear the desired area with the appropriate foreground/background
  //
  for (Row = TopRow; Row <= BottomRow; Row++) {
    PrintStringAt (LeftColumn, Row, Buffer);
  }

  gST->ConOut->SetCursorPosition (gST->ConOut, LeftColumn, TopRow);

  FreePool (Buffer);
}

//
// Color Setting Functions
//

/**
  Get OEM/Vendor specific popup attribute colors.

  @retval  Byte code color setting for popup color.
**/
UINT8
EFIAPI
GetPopupColor (
  VOID
  )
{
  return POPUP_TEXT | POPUP_BACKGROUND;
}

/**
  Get OEM/Vendor specific popup attribute colors.

  @retval  Byte code color setting for popup inverse color.
**/
UINT8
EFIAPI
GetPopupInverseColor (
  VOID
  )
{
  return POPUP_INVERSE_TEXT | POPUP_INVERSE_BACKGROUND;
}

/**
  Get OEM/Vendor specific PickList color attribute.

  @retval  Byte code color setting for pick list color.
**/
UINT8
EFIAPI
GetPickListColor (
  VOID
  )
{
  return PICKLIST_HIGHLIGHT_TEXT | PICKLIST_HIGHLIGHT_BACKGROUND;
}

/**
  Get OEM/Vendor specific arrow color attribute.

  @retval  Byte code color setting for arrow color.
**/
UINT8
EFIAPI
GetArrowColor (
  VOID
  )
{
  return ARROW_TEXT | ARROW_BACKGROUND;
}

/**
  Get OEM/Vendor specific info text color attribute.

  @retval  Byte code color setting for info text color.
**/
UINT8
EFIAPI
GetInfoTextColor (
  VOID
  )
{
  return INFO_TEXT | FIELD_BACKGROUND;
}

/**
  Get OEM/Vendor specific help text color attribute.

  @retval  Byte code color setting for help text color.
**/
UINT8
EFIAPI
GetHelpTextColor (
  VOID
  )
{
  return HELP_TEXT | FIELD_BACKGROUND;
}

/**
  Get OEM/Vendor specific grayed out text color attribute.

  @retval  Byte code color setting for grayed out text color.
**/
UINT8
EFIAPI
GetGrayedTextColor (
  VOID
  )
{
  return FIELD_TEXT_GRAYED | FIELD_BACKGROUND;
}

/**
  Get OEM/Vendor specific highlighted text color attribute.

  @retval  Byte code color setting for highlight text color.
**/
UINT8
EFIAPI
GetHighlightTextColor (
  VOID
  )
{
  return PcdGet8 (PcdBrowserFieldTextHighlightColor) | PcdGet8 (PcdBrowserFieldBackgroundHighlightColor);
}

/**
  Get OEM/Vendor specific field text color attribute.

  @retval  Byte code color setting for field text color.
**/
UINT8
EFIAPI
GetFieldTextColor (
  VOID
  )
{
  return PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND;
}

/**
  Get OEM/Vendor specific subtitle text color attribute.

  @retval  Byte code color setting for subtitle text color.
**/
UINT8
EFIAPI
GetSubTitleTextColor (
  VOID
  )
{
  return PcdGet8 (PcdBrowserSubtitleTextColor) | FIELD_BACKGROUND;
}

/**
  Clear Screen to the initial state.
**/
VOID
EFIAPI
ClearDisplayPage (
  VOID
  )
{
  gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
  gST->ConOut->ClearScreen (gST->ConOut);
  gLibIsFirstForm = TRUE;
}

/**
  Constructor of Customized Display Library Instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
CustomizedDisplayLibConstructor (
  IN      EFI_HANDLE        ImageHandle,
  IN      EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mCDLStringPackHandle = HiiAddPackages (&gCustomizedDisplayLibGuid, ImageHandle, CustomizedDisplayLibStrings, NULL);
  ASSERT (mCDLStringPackHandle != NULL);

  InitializeLibStrings ();

  return EFI_SUCCESS;
}

/**
  Destructor of Customized Display Library Instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
CustomizedDisplayLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  HiiRemovePackages (mCDLStringPackHandle);

  FreeLibStrings ();

  return EFI_SUCCESS;
}
