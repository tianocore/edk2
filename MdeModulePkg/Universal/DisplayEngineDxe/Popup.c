/** @file
Implementation for Hii Popup Protocol.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FormDisplay.h"

EFI_SCREEN_DESCRIPTOR  gPopupDimensions;
LIST_ENTRY             gUserSelectableOptions;
EFI_STRING             gMessageString;
UINTN                  gMesStrLineNum;
UINTN                  gMaxRowWidth;

/**
  Free the user selectable option structure data.

  @param  OptionList  Point to the selectable option list which need to be freed.

**/
VOID
FreeSelectableOptions (
  LIST_ENTRY  *OptionList
  )
{
  LIST_ENTRY              *Link;
  USER_SELECTABLE_OPTION  *SelectableOption;

  while (!IsListEmpty (OptionList)) {
    Link             = GetFirstNode (OptionList);
    SelectableOption = SELECTABLE_OPTION_FROM_LINK (Link);
    RemoveEntryList (&SelectableOption->Link);
    FreePool (SelectableOption);
  }
}

/**
  Display one selectable option.

  @param  SelectableOption  The selectable option need to be drew.
  @param  Highlight         Whether the option need to be highlighted.

**/
VOID
DisplayOneSelectableOption (
  IN USER_SELECTABLE_OPTION  *SelectableOption,
  IN BOOLEAN                 Highlight
  )
{
  if (Highlight) {
    gST->ConOut->SetAttribute (gST->ConOut, GetHighlightTextColor ());
  }

  PrintStringAt (SelectableOption->OptionCol, SelectableOption->OptionRow, SelectableOption->OptionString);
  gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
}

/**
  Add one selectable option to option list. This is the work function for AddUserSelectableOptions.

  @param  PopupType     The option need to be drew.
  @param  OptionType    The type of this selection option.
  @param  OptionString  Point to the option string that to be shown.
  @param  OptionCol     The column that the option need to be drew at.
  @param  OptionRow     The row that the option need to be drew at.

  @retval  EFI_SUCCESS           This function implement successfully.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available.

**/
EFI_STATUS
AddOneSelectableOption (
  IN EFI_HII_POPUP_TYPE       PopupType,
  IN EFI_HII_POPUP_SELECTION  OptionType,
  IN CHAR16                   *OptionString,
  IN UINTN                    OptionCol,
  IN UINTN                    OptionRow
  )
{
  USER_SELECTABLE_OPTION  *UserSelectableOption;

  UserSelectableOption = AllocateZeroPool (sizeof (USER_SELECTABLE_OPTION));
  if (UserSelectableOption == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the user selectable option based on the PopupType and OptionType.
  // And then add the option to the option list gUserSelectableOptions.
  //
  UserSelectableOption->Signature    = USER_SELECTABLE_OPTION_SIGNATURE;
  UserSelectableOption->OptionString = OptionString;
  UserSelectableOption->OptionType   = OptionType;
  UserSelectableOption->OptionCol    = OptionCol;
  UserSelectableOption->OptionRow    = OptionRow;
  UserSelectableOption->MinSequence  = 0;

  switch (PopupType) {
    case EfiHiiPopupTypeOk:
      UserSelectableOption->MaxSequence = 0;
      UserSelectableOption->Sequence    = 0;
      break;
    case EfiHiiPopupTypeOkCancel:
      UserSelectableOption->MaxSequence = 1;
      if (OptionType == EfiHiiPopupSelectionOk) {
        UserSelectableOption->Sequence = 0;
      } else {
        UserSelectableOption->Sequence = 1;
      }

      break;
    case EfiHiiPopupTypeYesNo:
      UserSelectableOption->MaxSequence = 1;
      if (OptionType == EfiHiiPopupSelectionYes) {
        UserSelectableOption->Sequence = 0;
      } else {
        UserSelectableOption->Sequence = 1;
      }

      break;
    case EfiHiiPopupTypeYesNoCancel:
      UserSelectableOption->MaxSequence = 2;
      if (OptionType == EfiHiiPopupSelectionYes) {
        UserSelectableOption->Sequence = 0;
      } else if (OptionType == EfiHiiPopupSelectionNo) {
        UserSelectableOption->Sequence = 1;
      } else {
        UserSelectableOption->Sequence = 2;
      }

      break;
    default:
      break;
  }

  InsertTailList (&gUserSelectableOptions, &UserSelectableOption->Link);

  return EFI_SUCCESS;
}

/**
  Add user selectable options to option list for different types of Popup.

  @param  PopupType    Type of the popup to display.

  @retval  EFI_SUCCESS           This function implement successfully.
  @retval  EFI_OUT_OF_RESOURCES  There are not enough resources available.

**/
EFI_STATUS
AddUserSelectableOptions (
  IN  EFI_HII_POPUP_TYPE  PopupType
  )
{
  EFI_STATUS  Status;
  UINTN       EndCol;
  UINTN       StartCol;
  UINTN       OptionCol;
  UINTN       OptionRow;
  UINTN       ColDimension;

  Status       = EFI_SUCCESS;
  EndCol       = gPopupDimensions.RightColumn;
  StartCol     = gPopupDimensions.LeftColumn;
  OptionRow    = gPopupDimensions.BottomRow - POPUP_BORDER;
  ColDimension = EndCol - StartCol + 1;

  InitializeListHead (&gUserSelectableOptions);

  switch (PopupType) {
    case EfiHiiPopupTypeOk:
      //
      // Add [Ok] option to the option list.
      //
      OptionCol = StartCol + (ColDimension - USER_SELECTABLE_OPTION_OK_WIDTH) / 2;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionOk, gOkOption, OptionCol, OptionRow);
      break;
    case EfiHiiPopupTypeOkCancel:
      //
      // Add [Ok] and [Cancel] options to the option list.
      //
      OptionCol = StartCol + (ColDimension - USER_SELECTABLE_OPTION_OK_CAL_WIDTH) / 3;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionOk, gOkOption, OptionCol, OptionRow);
      OptionCol = EndCol - (ColDimension - USER_SELECTABLE_OPTION_OK_CAL_WIDTH) / 3 - (GetStringWidth (gCancelOption) -2) / 2 + 1;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionCancel, gCancelOption, OptionCol, OptionRow);
      break;
    case EfiHiiPopupTypeYesNo:
      //
      // Add [Yes] and [No] options to the option list.
      //
      OptionCol = StartCol + (ColDimension - USER_SELECTABLE_OPTION_YES_NO_WIDTH) / 3;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionYes, gYesOption, OptionCol, OptionRow);
      OptionCol = EndCol - (ColDimension - USER_SELECTABLE_OPTION_YES_NO_WIDTH) / 3 - (GetStringWidth (gNoOption)- 2) / 2 + 1;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionNo, gNoOption, OptionCol, OptionRow);
      break;
    case EfiHiiPopupTypeYesNoCancel:
      //
      // Add [Yes], [No] and [Cancel] options to the option list.
      //
      OptionCol = StartCol + (ColDimension - USER_SELECTABLE_OPTION_YES_NO_CAL_WIDTH) / 4;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionYes, gYesOption, OptionCol, OptionRow);
      OptionCol = StartCol + (ColDimension - (GetStringWidth (gNoOption) -2) / 2) / 2;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionNo, gNoOption, OptionCol, OptionRow);
      OptionCol = EndCol - (ColDimension - USER_SELECTABLE_OPTION_YES_NO_CAL_WIDTH) / 4 - (GetStringWidth (gCancelOption) - 2) / 2 + 1;
      Status    = AddOneSelectableOption (PopupType, EfiHiiPopupSelectionCancel, gCancelOption, OptionCol, OptionRow);
      break;
    default:
      break;
  }

  return Status;
}

/**
  Show selectable options to user and get the one that user select.

  @param  PopupType      Type of the popup to display.
  @param  UserSelection  User selection.

**/
VOID
GetUserSelection (
  IN  EFI_HII_POPUP_TYPE       PopupType,
  OUT EFI_HII_POPUP_SELECTION  *UserSelection
  )
{
  LIST_ENTRY              *HighlightPos;
  LIST_ENTRY              *Link;
  USER_SELECTABLE_OPTION  *SelectableOption;
  USER_SELECTABLE_OPTION  *HighlightOption;
  EFI_INPUT_KEY           KeyValue;
  EFI_STATUS              Status;

  //
  // Display user selectable options in gUserSelectableOptions and get the option which user selects.
  //
  HighlightPos = gUserSelectableOptions.ForwardLink;
  do {
    for (Link = gUserSelectableOptions.ForwardLink; Link != &gUserSelectableOptions; Link = Link->ForwardLink) {
      SelectableOption = SELECTABLE_OPTION_FROM_LINK (Link);
      DisplayOneSelectableOption (SelectableOption, (BOOLEAN)(Link == HighlightPos));
    }

    //
    // If UserSelection is NULL, there is no need to handle the key user input, just return.
    //
    if (UserSelection == NULL) {
      return;
    }

    Status = WaitForKeyStroke (&KeyValue);
    ASSERT_EFI_ERROR (Status);

    HighlightOption = SELECTABLE_OPTION_FROM_LINK (HighlightPos);
    switch (KeyValue.UnicodeChar) {
      case CHAR_NULL:
        switch (KeyValue.ScanCode) {
          case SCAN_RIGHT:
            if (HighlightOption->Sequence < HighlightOption->MaxSequence) {
              HighlightPos = HighlightPos->ForwardLink;
            } else {
              HighlightPos = gUserSelectableOptions.ForwardLink;
            }

            break;
          case SCAN_LEFT:
            if (HighlightOption->Sequence > HighlightOption->MinSequence) {
              HighlightPos = HighlightPos->BackLink;
            } else {
              HighlightPos = gUserSelectableOptions.BackLink;
            }

            break;
          default:
            break;
        }

        break;

      case CHAR_CARRIAGE_RETURN:
        *UserSelection = HighlightOption->OptionType;
        return;
      default:
        if (((KeyValue.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (*gConfirmOptYes | UPPER_LOWER_CASE_OFFSET)) &&
            ((PopupType == EfiHiiPopupTypeYesNo) || (PopupType == EfiHiiPopupTypeYesNoCancel)))
        {
          *UserSelection = EfiHiiPopupSelectionYes;
          return;
        } else if (((KeyValue.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (*gConfirmOptNo| UPPER_LOWER_CASE_OFFSET)) &&
                   ((PopupType == EfiHiiPopupTypeYesNo) || (PopupType == EfiHiiPopupTypeYesNoCancel)))
        {
          *UserSelection = EfiHiiPopupSelectionNo;
          return;
        } else if (((KeyValue.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (*gConfirmOptOk | UPPER_LOWER_CASE_OFFSET)) &&
                   ((PopupType == EfiHiiPopupTypeOk) || (PopupType == EfiHiiPopupTypeOkCancel)))
        {
          *UserSelection = EfiHiiPopupSelectionOk;
          return;
        } else if (((KeyValue.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (*gConfirmOptCancel| UPPER_LOWER_CASE_OFFSET)) &&
                   ((PopupType == EfiHiiPopupTypeOkCancel) || (PopupType == EfiHiiPopupTypeYesNoCancel)))
        {
          *UserSelection = EfiHiiPopupSelectionCancel;
          return;
        }

        break;
    }
  } while (TRUE);
}

/**
  Get the offset in the input string when the width reaches to a fixed one.

  The input string may contain NARROW_CHAR and WIDE_CHAR.
  Notice: the input string doesn't contain line break characters.

  @param  String      The input string to be counted.
  @param  MaxWidth    The max length this function supported.
  @param  Offset      The max index of the string can be show out. If string's width less than MaxWidth, offset will point to the "\0" of the string.

**/
VOID
GetStringOffsetWithWidth (
  IN  CHAR16  *String,
  IN  UINTN   MaxWidth,
  OUT UINTN   *Offset
  )
{
  UINTN  StringWidth;
  UINTN  CharWidth;
  UINTN  StrOffset;

  StringWidth = 0;
  CharWidth   = 1;

  for (StrOffset = 0; String[StrOffset] != CHAR_NULL; StrOffset++) {
    switch (String[StrOffset]) {
      case NARROW_CHAR:
        CharWidth = 1;
        break;
      case WIDE_CHAR:
        CharWidth = 2;
        break;
      default:
        StringWidth += CharWidth;
        if (StringWidth >= MaxWidth) {
          *Offset = StrOffset;
          return;
        }
    }
  }

  *Offset = StrOffset;
}

/**
  Parse the message to check if it contains line break characters.
  For once call, caller can get the string for one line and the width of the string.
  This function call be called recursively to parse the whole InputString.

  (Notice: current implementation, it only checks \r, \n characters, it deals \r,\n,\n\r same as \r\n.)

  @param  InputString       String description for this option.
  @param  OutputString      Buffer to copy the string into, caller is responsible for freeing the buffer.
  @param  OutputStrWidth    The width of OutputString.
  @param  Index             Where in InputString to start the copy process

  @return Returns the number of CHAR16 characters that were copied into the OutputString buffer, include the '\0' info.

**/
UINTN
ParseMessageString (
  IN     CHAR16  *InputString,
  OUT    CHAR16  **OutputString,
  OUT    UINTN   *OutputStrWidth,
  IN OUT UINTN   *Index
  )
{
  UINTN  StrOffset;

  if ((InputString == NULL) || (Index == NULL) || (OutputString == NULL)) {
    return 0;
  }

  *OutputStrWidth = 0;

  //
  // Check the string to see if there are line break characters in the string
  //
  for (StrOffset = 0;
       InputString[*Index + StrOffset] != CHAR_CARRIAGE_RETURN && InputString[*Index + StrOffset] != CHAR_LINEFEED && InputString[*Index + StrOffset] != CHAR_NULL;
       StrOffset++
       )
  {
  }

  //
  // The CHAR_NULL has process last time, this time just return 0 to stand for finishing parsing the InputString.
  //
  if ((StrOffset == 0) && (InputString[*Index + StrOffset] == CHAR_NULL)) {
    return 0;
  }

  //
  // Copy the string to OutputString buffer and calculate the width of OutputString.
  //
  *OutputString = AllocateZeroPool ((StrOffset + 1) * sizeof (CHAR16));
  if (*OutputString == NULL) {
    return 0;
  }

  CopyMem ((*OutputString), &InputString[*Index], StrOffset * sizeof (CHAR16));
  *OutputStrWidth = (GetStringWidth (*OutputString) -2) / 2;

  //
  // Update the value of Index, can be used for marking where to check the input string for next call.
  //
  if (InputString[*Index + StrOffset] == CHAR_LINEFEED) {
    //
    // Skip the /n or /n/r info.
    //
    if (InputString[*Index + StrOffset + 1] == CHAR_CARRIAGE_RETURN) {
      *Index = (*Index + StrOffset + 2);
    } else {
      *Index = (*Index + StrOffset + 1);
    }
  } else if (InputString[*Index + StrOffset] == CHAR_CARRIAGE_RETURN) {
    //
    // Skip the /r or /r/n info.
    //
    if (InputString[*Index + StrOffset + 1] == CHAR_LINEFEED) {
      *Index = (*Index + StrOffset + 2);
    } else {
      *Index = (*Index + StrOffset + 1);
    }
  } else {
    *Index = (*Index + StrOffset);
  }

  return StrOffset + 1;
}

/**
  Calculate the position of the popup.

  @param  PopupType       Type of the popup to display.
  @param  ScreenForPopup  The screen dimensions for the popup.

**/
VOID
CalculatePopupPosition (
  IN  EFI_HII_POPUP_TYPE     PopupType,
  OUT EFI_SCREEN_DESCRIPTOR  *ScreenForPopup
  )
{
  CHAR16  *OutputString;
  UINTN   StringIndex;
  UINTN   OutputStrWidth;
  UINTN   OptionRowWidth;
  UINTN   Columns;
  UINTN   Rows;

  OptionRowWidth = 0;

  //
  // Calculate the row number which is needed to show the message string and the max width of the string in one row.
  //
  for (StringIndex = 0; ParseMessageString (gMessageString, &OutputString, &OutputStrWidth, &StringIndex) != 0;) {
    gMesStrLineNum++;
    if (gMaxRowWidth < OutputStrWidth) {
      gMaxRowWidth = OutputStrWidth;
    }

    FreePool (OutputString);
  }

  //
  // Calculate the row width for the selectable options.(OptionRowWidth = Number * SkipWidth + OptionWidth)
  //
  if (PopupType == EfiHiiPopupTypeOk) {
    OptionRowWidth = USER_SELECTABLE_OPTION_SKIP_WIDTH *2 + USER_SELECTABLE_OPTION_OK_WIDTH;
  } else if (PopupType == EfiHiiPopupTypeOkCancel) {
    OptionRowWidth = USER_SELECTABLE_OPTION_SKIP_WIDTH *3 + USER_SELECTABLE_OPTION_OK_CAL_WIDTH;
  } else if (PopupType == EfiHiiPopupTypeYesNo) {
    OptionRowWidth = USER_SELECTABLE_OPTION_SKIP_WIDTH *3 + USER_SELECTABLE_OPTION_YES_NO_WIDTH;
  } else if (PopupType == EfiHiiPopupTypeYesNoCancel) {
    OptionRowWidth = USER_SELECTABLE_OPTION_SKIP_WIDTH *4 + USER_SELECTABLE_OPTION_YES_NO_CAL_WIDTH;
  }

  if (OptionRowWidth > gMaxRowWidth) {
    gMaxRowWidth = OptionRowWidth;
  }

  //
  // Avialble row width for message string = screen width - left popup border width - right popup border width.
  // Avialble line number for message string = screen height - 1 - popup header height -  popup footer height.
  // (Notice: screen height - 1 because in current UI page, the bottom row of srceen is usded to show Status Bar,not for form itself.
  // So we don't use the bottom row for popup either. If macro STATUS_BAR_HEIGHT changed, we also need to update the height here.)
  //
  // Select the smaller one between actual dimension of message string and the avialble dimension for message string.
  //
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &Columns, &Rows);
  gMaxRowWidth   = MIN (gMaxRowWidth, Columns - 2 * POPUP_BORDER);
  gMesStrLineNum = MIN (gMesStrLineNum, Rows -1 - POPUP_FOOTER_HEIGHT - POPUP_HEADER_HEIGHT);

  //
  // Calculate the start column, end column, top row and bottom row for the popup.
  //
  ScreenForPopup->LeftColumn  = (Columns -2 * POPUP_BORDER - gMaxRowWidth) / 2;
  ScreenForPopup->RightColumn = ScreenForPopup->LeftColumn + gMaxRowWidth + 2 * POPUP_BORDER - 1;
  ScreenForPopup->TopRow      = (Rows - 1 - POPUP_FOOTER_HEIGHT - POPUP_HEADER_HEIGHT - gMesStrLineNum) / 2;
  ScreenForPopup->BottomRow   = ScreenForPopup->TopRow + gMesStrLineNum + POPUP_FOOTER_HEIGHT + POPUP_HEADER_HEIGHT - 1;
}

/**
  Draw the Message box.
  +-------------------------------------------+
  |            ERROR/WARNING/INFO             |
  |-------------------------------------------|
  |              popup messages               |
  |                                           |
  |          user selectable options          |
  +-------------------------------------------+

  @param  PopupStyle   Popup style to use.

**/
EFI_STATUS
DrawMessageBox (
  IN  EFI_HII_POPUP_STYLE  PopupStyle
  )
{
  UINTN   Index;
  UINTN   Length;
  UINTN   EndCol;
  UINTN   TopRow;
  UINTN   StartCol;
  UINTN   BottomRow;
  CHAR16  Character;
  UINTN   DisplayRow;
  UINTN   StringIndex;
  CHAR16  *TempString;
  CHAR16  *OutputString;
  UINTN   ColDimension;
  UINTN   OutputStrWidth;
  UINTN   DrawMesStrRowNum;

  EndCol           = gPopupDimensions.RightColumn;
  TopRow           = gPopupDimensions.TopRow;
  StartCol         = gPopupDimensions.LeftColumn;
  BottomRow        = gPopupDimensions.BottomRow;
  ColDimension     = EndCol - StartCol + 1;
  DrawMesStrRowNum = 0;

  //
  // 1. Draw the top of the message box.
  //
  Character = BOXDRAW_DOWN_RIGHT;
  PrintCharAt (StartCol, TopRow, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = StartCol; Index + 1 < EndCol; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_DOWN_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);

  //
  // 2. Draw the prompt string for different popup styles.
  //
  Character  = BOXDRAW_VERTICAL;
  DisplayRow = TopRow + POPUP_BORDER;
  ClearLines (StartCol, EndCol, DisplayRow, DisplayRow, GetPopupColor ());
  PrintCharAt (StartCol, DisplayRow, Character);
  PrintCharAt (EndCol, DisplayRow, Character);
  if (PopupStyle == EfiHiiPopupStyleError) {
    PrintStringAt ((ColDimension - (GetStringWidth (gErrorPopup) - 2) / 2) / 2 + StartCol, DisplayRow, gErrorPopup);
  } else if (PopupStyle == EfiHiiPopupStyleWarning) {
    PrintStringAt ((ColDimension - (GetStringWidth (gWarningPopup) - 2) / 2) / 2 + StartCol, DisplayRow, gWarningPopup);
  } else {
    PrintStringAt ((ColDimension - (GetStringWidth (gInfoPopup) - 2) / 2) / 2 + StartCol, DisplayRow, gInfoPopup);
  }

  //
  // 3. Draw the horizontal line below the prompt string for different popup styles.
  //
  DisplayRow = TopRow + POPUP_BORDER + POPUP_STYLE_STRING_HEIGHT;
  ClearLines (StartCol, EndCol, DisplayRow, DisplayRow, GetPopupColor ());
  Character = BOXDRAW_HORIZONTAL;
  for (Index = StartCol + 1; Index < EndCol; Index++) {
    PrintCharAt (Index, DisplayRow, Character);
  }

  Character = BOXDRAW_VERTICAL;
  PrintCharAt (StartCol, DisplayRow, Character);
  PrintCharAt (EndCol, DisplayRow, Character);

  //
  // 4. Draw the mesage string.
  //
  DisplayRow = TopRow + POPUP_HEADER_HEIGHT;
  for (Index = DisplayRow, StringIndex = 0; ParseMessageString (gMessageString, &OutputString, &OutputStrWidth, &StringIndex) != 0 && DrawMesStrRowNum < gMesStrLineNum;) {
    ClearLines (StartCol, EndCol, Index, Index, GetPopupColor ());
    PrintCharAt (StartCol, Index, Character);
    PrintCharAt (EndCol, Index, Character);
    if (OutputStrWidth > gMaxRowWidth) {
      //
      // OutputStrWidth > MaxMesStrWidth, cut off the string and print print ... instead.
      //
      GetStringOffsetWithWidth (OutputString, gMaxRowWidth, &Length);
      TempString = AllocateZeroPool ((Length + 1) * sizeof (CHAR16));
      if (TempString == NULL) {
        FreePool (OutputString);
        return EFI_OUT_OF_RESOURCES;
      }

      StrnCpyS (TempString, Length + 1, OutputString, Length - 3);
      StrCatS (TempString, Length + 1, L"...");
      PrintStringAt ((ColDimension - gMaxRowWidth) / 2 + StartCol, Index, TempString);
      FreePool (TempString);
    } else {
      PrintStringAt ((ColDimension - OutputStrWidth) / 2 + StartCol, Index, OutputString);
    }

    Index++;
    DrawMesStrRowNum++;
    FreePool (OutputString);
  }

  //
  // 5. Draw an empty line after message string.
  //
  ClearLines (StartCol, EndCol, Index, Index, GetPopupColor ());
  PrintCharAt (StartCol, Index, Character);
  PrintCharAt (EndCol, Index, Character);
  //
  // Check whether the actual string row number beyond the MesStrRowNum, if yes, print the ...... in the row.
  //
  if ((OutputStrWidth > 0) && (DrawMesStrRowNum >= gMesStrLineNum)) {
    PrintStringAt ((ColDimension - StrLen (L"......")) / 2 + StartCol, Index, L"......");
  }

  //
  // 6. Draw an empty line which is used to show user selectable options, will draw concrete option strings in function GetUserSelection().
  //
  Character  = BOXDRAW_VERTICAL;
  DisplayRow = BottomRow - POPUP_BORDER;
  ClearLines (StartCol, EndCol, DisplayRow, DisplayRow, GetPopupColor ());
  PrintCharAt (StartCol, DisplayRow, Character);
  PrintCharAt (EndCol, DisplayRow, Character);

  //
  // 7. Draw the bottom of the message box.
  //
  Character = BOXDRAW_UP_RIGHT;
  PrintCharAt (StartCol, BottomRow, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = StartCol; Index + 1 < EndCol; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_UP_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);

  return EFI_SUCCESS;
}

/**
  Displays a popup window.

  @param  This           A pointer to the EFI_HII_POPUP_PROTOCOL instance.
  @param  PopupStyle     Popup style to use.
  @param  PopupType      Type of the popup to display.
  @param  HiiHandle      HII handle of the string pack containing Message
  @param  Message        A message to display in the popup box.
  @param  UserSelection  User selection.

  @retval EFI_SUCCESS            The popup box was successfully displayed.
  @retval EFI_INVALID_PARAMETER  HiiHandle and Message do not define a valid HII string.
  @retval EFI_INVALID_PARAMETER  PopupType is not one of the values defined by this specification.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to display the popup box.

**/
EFI_STATUS
EFIAPI
CreatePopup (
  IN  EFI_HII_POPUP_PROTOCOL   *This,
  IN  EFI_HII_POPUP_STYLE      PopupStyle,
  IN  EFI_HII_POPUP_TYPE       PopupType,
  IN  EFI_HII_HANDLE           HiiHandle,
  IN  EFI_STRING_ID            Message,
  OUT EFI_HII_POPUP_SELECTION  *UserSelection OPTIONAL
  )
{
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;
  EFI_SIMPLE_TEXT_OUTPUT_MODE      SavedConsoleMode;
  EFI_STATUS                       Status;

  if ((PopupType < EfiHiiPopupTypeOk) || (PopupType > EfiHiiPopupTypeYesNoCancel)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((HiiHandle == NULL) || (Message == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  gMessageString = HiiGetString (HiiHandle, Message, NULL);
  if (gMessageString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ConOut         = gST->ConOut;
  gMaxRowWidth   = 0;
  gMesStrLineNum = 0;

  CopyMem (&SavedConsoleMode, ConOut->Mode, sizeof (SavedConsoleMode));
  ConOut->EnableCursor (ConOut, FALSE);
  ConOut->SetAttribute (ConOut, GetPopupColor ());

  CalculatePopupPosition (PopupType, &gPopupDimensions);

  Status = DrawMessageBox (PopupStyle);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // Add user selectable options to option list: gUserSelectableOptions
  //
  Status = AddUserSelectableOptions (PopupType);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  GetUserSelection (PopupType, UserSelection);

Done:
  //
  // Restore Conout attributes and free the resources allocate before.
  //
  ConOut->EnableCursor (ConOut, SavedConsoleMode.CursorVisible);
  ConOut->SetCursorPosition (ConOut, SavedConsoleMode.CursorColumn, SavedConsoleMode.CursorRow);
  ConOut->SetAttribute (ConOut, SavedConsoleMode.Attribute);
  FreeSelectableOptions (&gUserSelectableOptions);
  FreePool (gMessageString);

  return Status;
}
