/** @file
Utility functions for User Interface functions.

Copyright (c) 2004 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ui.h"
#include "Setup.h"

LIST_ENTRY          Menu;
LIST_ENTRY          gMenuList;
MENU_REFRESH_ENTRY  *gMenuRefreshHead;

//
// Search table for UiDisplayMenu()
//
SCAN_CODE_TO_SCREEN_OPERATION     gScanCodeToOperation[] = {
  {
    SCAN_UP,
    UiUp,
  },
  {
    SCAN_DOWN,
    UiDown,
  },
  {
    SCAN_PAGE_UP,
    UiPageUp,
  },
  {
    SCAN_PAGE_DOWN,
    UiPageDown,
  },
  {
    SCAN_ESC,
    UiReset,
  },
  {
    SCAN_F2,
    UiPrevious,
  },
  {
    SCAN_LEFT,
    UiLeft,
  },
  {
    SCAN_RIGHT,
    UiRight,
  },
  {
    SCAN_F9,
    UiDefault,
  },
  {
    SCAN_F10,
    UiSave
  }
};

SCREEN_OPERATION_T0_CONTROL_FLAG  gScreenOperationToControlFlag[] = {
  {
    UiNoOperation,
    CfUiNoOperation,
  },
  {
    UiDefault,
    CfUiDefault,
  },
  {
    UiSelect,
    CfUiSelect,
  },
  {
    UiUp,
    CfUiUp,
  },
  {
    UiDown,
    CfUiDown,
  },
  {
    UiLeft,
    CfUiLeft,
  },
  {
    UiRight,
    CfUiRight,
  },
  {
    UiReset,
    CfUiReset,
  },
  {
    UiSave,
    CfUiSave,
  },
  {
    UiPrevious,
    CfUiPrevious,
  },
  {
    UiPageUp,
    CfUiPageUp,
  },
  {
    UiPageDown,
    CfUiPageDown
  }
};

BOOLEAN  mInputError;


/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

  @return Value.

**/
VOID
SetUnicodeMem (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN CHAR16 Value
  )
{
  CHAR16  *Ptr;

  Ptr = Buffer;
  while ((Size--)  != 0) {
    *(Ptr++) = Value;
  }
}


/**
  Initialize Menu option list.

**/
VOID
UiInitMenu (
  VOID
  )
{
  InitializeListHead (&Menu);
}


/**
  Initialize Menu option list.

**/
VOID
UiInitMenuList (
  VOID
  )
{
  InitializeListHead (&gMenuList);
}


/**
  Remove a Menu in list, and return FormId/QuestionId for previous Menu.

  @param  Selection              Menu selection.

**/
VOID
UiRemoveMenuListEntry (
  OUT UI_MENU_SELECTION  *Selection
  )
{
  UI_MENU_LIST  *UiMenuList;

  if (!IsListEmpty (&gMenuList)) {
    UiMenuList = CR (gMenuList.ForwardLink, UI_MENU_LIST, MenuLink, UI_MENU_LIST_SIGNATURE);

    Selection->FormId = UiMenuList->FormId;
    Selection->QuestionId = UiMenuList->QuestionId;
    RemoveEntryList (&UiMenuList->MenuLink);
    gBS->FreePool (UiMenuList);
  }
}


/**
  Free Menu option linked list.

**/
VOID
UiFreeMenuList (
  VOID
  )
{
  UI_MENU_LIST  *UiMenuList;

  while (!IsListEmpty (&gMenuList)) {
    UiMenuList = CR (gMenuList.ForwardLink, UI_MENU_LIST, MenuLink, UI_MENU_LIST_SIGNATURE);
    RemoveEntryList (&UiMenuList->MenuLink);
    gBS->FreePool (UiMenuList);
  }
}


/**
  Add one menu entry to the linked lst

  @param  Selection              Menu selection.

**/
VOID
UiAddMenuListEntry (
  IN UI_MENU_SELECTION            *Selection
  )
{
  UI_MENU_LIST  *UiMenuList;

  UiMenuList = AllocateZeroPool (sizeof (UI_MENU_LIST));
  ASSERT (UiMenuList != NULL);

  UiMenuList->Signature = UI_MENU_LIST_SIGNATURE;
  UiMenuList->FormId = Selection->FormId;
  UiMenuList->QuestionId = Selection->QuestionId;

  InsertHeadList (&gMenuList, &UiMenuList->MenuLink);
}


/**
  Free Menu option linked list.

**/
VOID
UiFreeMenu (
  VOID
  )
{
  UI_MENU_OPTION  *MenuOption;

  while (!IsListEmpty (&Menu)) {
    MenuOption = MENU_OPTION_FROM_LINK (Menu.ForwardLink);
    RemoveEntryList (&MenuOption->Link);

    //
    // We allocated space for this description when we did a GetToken, free it here
    //
    if (MenuOption->Skip != 0) {
      //
      // For date/time, MenuOption->Description is shared by three Menu Options
      // Data format :      [01/02/2004]      [11:22:33]
      // Line number :        0  0    1         0  0  1
      //
      gBS->FreePool (MenuOption->Description);
    }
    gBS->FreePool (MenuOption);
  }
}


/**
  Free Menu option linked list.

**/
VOID
UiFreeRefreshList (
  VOID
  )
{
  MENU_REFRESH_ENTRY  *OldMenuRefreshEntry;

  while (gMenuRefreshHead != NULL) {
    OldMenuRefreshEntry = gMenuRefreshHead->Next;
    gBS->FreePool (gMenuRefreshHead);
    gMenuRefreshHead = OldMenuRefreshEntry;
  }

  gMenuRefreshHead = NULL;
}



/**
  Refresh screen.

**/
EFI_STATUS
RefreshForm (
  VOID
  )
{
  CHAR16                          *OptionString;
  MENU_REFRESH_ENTRY              *MenuRefreshEntry;
  UINTN                           Index;
  EFI_STATUS                      Status;
  UI_MENU_SELECTION               *Selection;
  FORM_BROWSER_STATEMENT          *Question;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_HII_VALUE                   *HiiValue;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;

  if (gMenuRefreshHead != NULL) {

    MenuRefreshEntry = gMenuRefreshHead;

    //
    // Reset FormPackage update flag
    //
    mHiiPackageListUpdated = FALSE;

    do {
      gST->ConOut->SetAttribute (gST->ConOut, MenuRefreshEntry->CurrentAttribute);

      Selection = MenuRefreshEntry->Selection;
      Question = MenuRefreshEntry->MenuOption->ThisTag;

      Status = GetQuestionValue (Selection->FormSet, Selection->Form, Question, FALSE);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      OptionString = NULL;
      ProcessOptions (Selection, MenuRefreshEntry->MenuOption, FALSE, &OptionString);

      if (OptionString != NULL) {
        //
        // If leading spaces on OptionString - remove the spaces
        //
        for (Index = 0; OptionString[Index] == L' '; Index++)
          ;

        PrintStringAt (MenuRefreshEntry->CurrentColumn, MenuRefreshEntry->CurrentRow, &OptionString[Index]);
        gBS->FreePool (OptionString);
      }

      //
      // Question value may be changed, need invoke its Callback()
      //
      ConfigAccess = Selection->FormSet->ConfigAccess;
      if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) && (ConfigAccess != NULL)) {
        ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;

        HiiValue = &Question->HiiValue;
        if (HiiValue->Type == EFI_IFR_TYPE_STRING) {
          //
          // Create String in HII database for Configuration Driver to retrieve
          //
          HiiValue->Value.string = NewString ((CHAR16 *) Question->BufferValue, Selection->FormSet->HiiHandle);
        }

        Status = ConfigAccess->Callback (
                                 ConfigAccess,
                                 EFI_BROWSER_ACTION_CHANGING,
                                 Question->QuestionId,
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

      MenuRefreshEntry = MenuRefreshEntry->Next;

    } while (MenuRefreshEntry != NULL);

    if (mHiiPackageListUpdated) {
      //
      // Package list is updated, force to reparse IFR binary of target Formset
      //
      mHiiPackageListUpdated = FALSE;
      Selection->Action = UI_ACTION_REFRESH_FORMSET;
      return EFI_SUCCESS;
    }
  }

  return EFI_TIMEOUT;
}


/**
  Wait for a given event to fire, or for an optional timeout to expire.

  @param  Event                  The event to wait for
  @param  Timeout                An optional timeout value in 100 ns units.
  @param  RefreshInterval        Menu refresh interval (in seconds).

  @retval EFI_SUCCESS            Event fired before Timeout expired.
  @retval EFI_TIME_OUT           Timout expired before Event fired.

**/
EFI_STATUS
UiWaitForSingleEvent (
  IN EFI_EVENT                Event,
  IN UINT64                   Timeout, OPTIONAL
  IN UINT8                    RefreshInterval OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
            TimerEvent,
            TimerRelative,
            Timeout
            );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // Update screen every second
    //
    if (RefreshInterval == 0) {
      Timeout = ONE_SECOND;
    } else {
      Timeout = RefreshInterval * ONE_SECOND;
    }

    do {
      Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);

      //
      // Set the timer event
      //
      gBS->SetTimer (
            TimerEvent,
            TimerRelative,
            Timeout
            );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);

      //
      // If the timer expired, update anything that needs a refresh and keep waiting
      //
      if (!EFI_ERROR (Status) && Index == 1) {
        Status = EFI_TIMEOUT;
        if (RefreshInterval != 0) {
          Status = RefreshForm ();
        }
      }

      gBS->CloseEvent (TimerEvent);
    } while (Status == EFI_TIMEOUT);
  }

  return Status;
}


/**
  Add one menu option by specified description and context.

  @param  String                 String description for this option.
  @param  Handle                 Hii handle for the package list.
  @param  Statement              Statement of this Menu Option.
  @param  NumberOfLines          Display lines for this Menu Option.
  @param  MenuItemCount          The index for this Option in the Menu.

**/
VOID
UiAddMenuOption (
  IN CHAR16                  *String,
  IN EFI_HII_HANDLE          Handle,
  IN FORM_BROWSER_STATEMENT  *Statement,
  IN UINT16                  NumberOfLines,
  IN UINT16                  MenuItemCount
  )
{
  UI_MENU_OPTION  *MenuOption;
  UINTN           Index;
  UINTN           Count;

  Count = 1;

  if (Statement->Operand == EFI_IFR_DATE_OP || Statement->Operand == EFI_IFR_TIME_OP) {
    //
    // Add three MenuOptions for Date/Time
    // Data format :      [01/02/2004]      [11:22:33]
    // Line number :        0  0    1         0  0  1
    //
    NumberOfLines = 0;
    Count = 3;

    if (Statement->Storage == NULL) {
      //
      // For RTC type of date/time, set default refresh interval to be 1 second
      //
      if (Statement->RefreshInterval == 0) {
        Statement->RefreshInterval = 1;
      }
    }
  }

  for (Index = 0; Index < Count; Index++) {
    MenuOption = AllocateZeroPool (sizeof (UI_MENU_OPTION));
    ASSERT (MenuOption);

    MenuOption->Signature   = UI_MENU_OPTION_SIGNATURE;
    MenuOption->Description = String;
    MenuOption->Handle      = Handle;
    MenuOption->ThisTag     = Statement;
    MenuOption->EntryNumber = MenuItemCount;

    if (Index == 2) {
      //
      // Override LineNumber for the MenuOption in Date/Time sequence
      //
      MenuOption->Skip = 1;
    } else {
      MenuOption->Skip = NumberOfLines;
    }
    MenuOption->Sequence = Index;

    if (Statement->GrayOutExpression != NULL) {
      MenuOption->GrayOut = Statement->GrayOutExpression->Result.Value.b;
    }

    if ((Statement->ValueExpression != NULL) ||
        ((Statement->QuestionFlags & EFI_IFR_FLAG_READ_ONLY) != 0)) {
      MenuOption->ReadOnly = TRUE;
    }

    InsertTailList (&Menu, &MenuOption->Link);
  }
}


/**
  Routine used to abstract a generic dialog interface and return the selected key or string

  @param  NumberOfLines          The number of lines for the dialog box
  @param  HotKey                 Defines whether a single character is parsed
                                 (TRUE) and returned in KeyValue or a string is
                                 returned in StringBuffer.  Two special characters
                                 are considered when entering a string, a SCAN_ESC
                                 and an CHAR_CARRIAGE_RETURN.  SCAN_ESC terminates
                                 string input and returns
  @param  MaximumStringSize      The maximum size in bytes of a typed in string
                                 (each character is a CHAR16) and the minimum
                                 string returned is two bytes
  @param  StringBuffer           The passed in pointer to the buffer which will
                                 hold the typed in string if HotKey is FALSE
  @param  KeyValue               The EFI_KEY value returned if HotKey is TRUE..
  @param  ...                    A series of (quantity == NumberOfLines) text
                                 strings which will be used to construct the dialog
                                 box

  @retval EFI_SUCCESS            Displayed dialog and received user interaction
  @retval EFI_INVALID_PARAMETER  One of the parameters was invalid (e.g.
                                 (StringBuffer == NULL) && (HotKey == FALSE))
  @retval EFI_DEVICE_ERROR       User typed in an ESC character to exit the routine

**/
EFI_STATUS
CreateDialog (
  IN  UINTN                       NumberOfLines,
  IN  BOOLEAN                     HotKey,
  IN  UINTN                       MaximumStringSize,
  OUT CHAR16                      *StringBuffer,
  OUT EFI_INPUT_KEY               *KeyValue,
  ...
  )
{
  VA_LIST       Marker;
  VA_LIST       MarkerBackup;
  UINTN         Count;
  EFI_INPUT_KEY Key;
  UINTN         LargestString;
  CHAR16        *TempString;
  CHAR16        *BufferedString;
  CHAR16        *StackString;
  CHAR16        KeyPad[2];
  UINTN         Start;
  UINTN         Top;
  UINTN         Index;
  EFI_STATUS    Status;
  BOOLEAN       SelectionComplete;
  UINTN         InputOffset;
  UINTN         CurrentAttribute;
  UINTN         DimensionsWidth;
  UINTN         DimensionsHeight;

  DimensionsWidth   = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight  = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  SelectionComplete = FALSE;
  InputOffset       = 0;
  TempString        = AllocateZeroPool (MaximumStringSize * 2);
  BufferedString    = AllocateZeroPool (MaximumStringSize * 2);
  CurrentAttribute  = gST->ConOut->Mode->Attribute;

  ASSERT (TempString);
  ASSERT (BufferedString);

  VA_START (Marker, KeyValue);
  MarkerBackup = Marker;

  //
  // Zero the outgoing buffer
  //
  ZeroMem (StringBuffer, MaximumStringSize);

  if (HotKey) {
    if (KeyValue == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if (StringBuffer == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  }
  //
  // Disable cursor
  //
  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  LargestString = 0;

  //
  // Determine the largest string in the dialog box
  // Notice we are starting with 1 since String is the first string
  //
  for (Count = 0; Count < NumberOfLines; Count++) {
    StackString = VA_ARG (Marker, CHAR16 *);

    if (StackString[0] == L' ') {
      InputOffset = Count + 1;
    }

    if ((GetStringWidth (StackString) / 2) > LargestString) {
      //
      // Size of the string visually and subtract the width by one for the null-terminator
      //
      LargestString = (GetStringWidth (StackString) / 2);
    }
  }

  Start = (DimensionsWidth - LargestString - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  Top   = ((DimensionsHeight - NumberOfLines - 2) / 2) + gScreenDimensions.TopRow - 1;

  Count = 0;

  //
  // Display the Popup
  //
  CreateSharedPopUp (LargestString, NumberOfLines, MarkerBackup);

  //
  // Take the first key typed and report it back?
  //
  if (HotKey) {
    Status = WaitForKeyStroke (&Key);
    ASSERT_EFI_ERROR (Status);
    CopyMem (KeyValue, &Key, sizeof (EFI_INPUT_KEY));

  } else {
    do {
      Status = WaitForKeyStroke (&Key);

      switch (Key.UnicodeChar) {
      case CHAR_NULL:
        switch (Key.ScanCode) {
        case SCAN_ESC:
          gBS->FreePool (TempString);
          gBS->FreePool (BufferedString);
          gST->ConOut->SetAttribute (gST->ConOut, CurrentAttribute);
          gST->ConOut->EnableCursor (gST->ConOut, TRUE);
          return EFI_DEVICE_ERROR;

        default:
          break;
        }

        break;

      case CHAR_CARRIAGE_RETURN:
        SelectionComplete = TRUE;
        gBS->FreePool (TempString);
        gBS->FreePool (BufferedString);
        gST->ConOut->SetAttribute (gST->ConOut, CurrentAttribute);
        gST->ConOut->EnableCursor (gST->ConOut, TRUE);
        return EFI_SUCCESS;
        break;

      case CHAR_BACKSPACE:
        if (StringBuffer[0] != CHAR_NULL) {
          for (Index = 0; StringBuffer[Index] != CHAR_NULL; Index++) {
            TempString[Index] = StringBuffer[Index];
          }
          //
          // Effectively truncate string by 1 character
          //
          TempString[Index - 1] = CHAR_NULL;
          StrCpy (StringBuffer, TempString);
        }

      default:
        //
        // If it is the beginning of the string, don't worry about checking maximum limits
        //
        if ((StringBuffer[0] == CHAR_NULL) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
          StrnCpy (StringBuffer, &Key.UnicodeChar, 1);
          StrnCpy (TempString, &Key.UnicodeChar, 1);
        } else if ((GetStringWidth (StringBuffer) < MaximumStringSize) && (Key.UnicodeChar != CHAR_BACKSPACE)) {
          KeyPad[0] = Key.UnicodeChar;
          KeyPad[1] = CHAR_NULL;
          StrCat (StringBuffer, KeyPad);
          StrCat (TempString, KeyPad);
        }
        //
        // If the width of the input string is now larger than the screen, we nee to
        // adjust the index to start printing portions of the string
        //
        SetUnicodeMem (BufferedString, LargestString, L' ');

        PrintStringAt (Start + 1, Top + InputOffset, BufferedString);

        if ((GetStringWidth (StringBuffer) / 2) > (DimensionsWidth - 2)) {
          Index = (GetStringWidth (StringBuffer) / 2) - DimensionsWidth + 2;
        } else {
          Index = 0;
        }

        for (Count = 0; Index + 1 < GetStringWidth (StringBuffer) / 2; Index++, Count++) {
          BufferedString[Count] = StringBuffer[Index];
        }

        PrintStringAt (Start + 1, Top + InputOffset, BufferedString);
        break;
      }
    } while (!SelectionComplete);
  }

  gST->ConOut->SetAttribute (gST->ConOut, CurrentAttribute);
  gST->ConOut->EnableCursor (gST->ConOut, TRUE);
  return EFI_SUCCESS;
}

/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param RequestedWidth  The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param Marker          The variable argument list for the list of string to be printed.

**/
VOID
CreateSharedPopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  IN  VA_LIST                     Marker
  )
{
  UINTN   Index;
  UINTN   Count;
  CHAR16  Character;
  UINTN   Start;
  UINTN   End;
  UINTN   Top;
  UINTN   Bottom;
  CHAR16  *String;
  UINTN   DimensionsWidth;
  UINTN   DimensionsHeight;

  DimensionsWidth   = gScreenDimensions.RightColumn - gScreenDimensions.LeftColumn;
  DimensionsHeight  = gScreenDimensions.BottomRow - gScreenDimensions.TopRow;

  gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);

  if ((RequestedWidth + 2) > DimensionsWidth) {
    RequestedWidth = DimensionsWidth - 2;
  }

  //
  // Subtract the PopUp width from total Columns, allow for one space extra on
  // each end plus a border.
  //
  Start     = (DimensionsWidth - RequestedWidth - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  End       = Start + RequestedWidth + 1;

  Top       = ((DimensionsHeight - NumberOfLines - 2) / 2) + gScreenDimensions.TopRow - 1;
  Bottom    = Top + NumberOfLines + 2;

  Character = BOXDRAW_DOWN_RIGHT;
  PrintCharAt (Start, Top, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintChar (Character);
  }

  Character = BOXDRAW_DOWN_LEFT;
  PrintChar (Character);
  Character = BOXDRAW_VERTICAL;

  Count = 0;
  for (Index = Top; Index + 2 < Bottom; Index++, Count++) {
    String = VA_ARG (Marker, CHAR16*);

    //
    // This will clear the background of the line - we never know who might have been
    // here before us.  This differs from the next clear in that it used the non-reverse
    // video for normal printing.
    //
    if (GetStringWidth (String) / 2 > 1) {
      ClearLines (Start, End, Index + 1, Index + 1, POPUP_TEXT | POPUP_BACKGROUND);
    }

    //
    // Passing in a space results in the assumption that this is where typing will occur
    //
    if (String[0] == L' ') {
      ClearLines (Start + 1, End - 1, Index + 1, Index + 1, POPUP_INVERSE_TEXT | POPUP_INVERSE_BACKGROUND);
    }

    //
    // Passing in a NULL results in a blank space
    //
    if (String[0] == CHAR_NULL) {
      ClearLines (Start, End, Index + 1, Index + 1, POPUP_TEXT | POPUP_BACKGROUND);
    }

    PrintStringAt (
      ((DimensionsWidth - GetStringWidth (String) / 2) / 2) + gScreenDimensions.LeftColumn + 1,
      Index + 1,
      String
      );
    gST->ConOut->SetAttribute (gST->ConOut, POPUP_TEXT | POPUP_BACKGROUND);
    PrintCharAt (Start, Index + 1, Character);
    PrintCharAt (End - 1, Index + 1, Character);
  }

  Character = BOXDRAW_UP_RIGHT;
  PrintCharAt (Start, Bottom - 1, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintChar (Character);
  }

  Character = BOXDRAW_UP_LEFT;
  PrintChar (Character);
}

/**
  Draw a pop up windows based on the dimension, number of lines and
  strings specified.

  @param RequestedWidth  The width of the pop-up.
  @param NumberOfLines   The number of lines.
  @param ...             A series of text strings that displayed in the pop-up.

**/
VOID
CreatePopUp (
  IN  UINTN                       RequestedWidth,
  IN  UINTN                       NumberOfLines,
  ...
  )
{
  VA_LIST Marker;

  VA_START (Marker, NumberOfLines);
  
  CreateSharedPopUp (RequestedWidth, NumberOfLines, Marker);

  VA_END (Marker);
}


/**
  Update status bar on the bottom of menu.

  @param  MessageType            The type of message to be shown.
  @param  Flags                  The flags in Question header.
  @param  State                  Set or clear.

**/
VOID
UpdateStatusBar (
  IN  UINTN                       MessageType,
  IN  UINT8                       Flags,
  IN  BOOLEAN                     State
  )
{
  UINTN           Index;
  CHAR16          *NvUpdateMessage;
  CHAR16          *InputErrorMessage;

  NvUpdateMessage   = GetToken (STRING_TOKEN (NV_UPDATE_MESSAGE), gHiiHandle);
  InputErrorMessage = GetToken (STRING_TOKEN (INPUT_ERROR_MESSAGE), gHiiHandle);

  switch (MessageType) {
  case INPUT_ERROR:
    if (State) {
      gST->ConOut->SetAttribute (gST->ConOut, ERROR_TEXT);
      PrintStringAt (
        gScreenDimensions.LeftColumn + gPromptBlockWidth,
        gScreenDimensions.BottomRow - 1,
        InputErrorMessage
        );
      mInputError = TRUE;
    } else {
      gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_HIGHLIGHT);
      for (Index = 0; Index < (GetStringWidth (InputErrorMessage) - 2) / 2; Index++) {
        PrintAt (gScreenDimensions.LeftColumn + gPromptBlockWidth + Index, gScreenDimensions.BottomRow - 1, L"  ");
      }

      mInputError = FALSE;
    }
    break;

  case NV_UPDATE_REQUIRED:
    if (gClassOfVfr != EFI_FRONT_PAGE_SUBCLASS) {
      if (State) {
        gST->ConOut->SetAttribute (gST->ConOut, INFO_TEXT);
        PrintStringAt (
          gScreenDimensions.LeftColumn + gPromptBlockWidth + gOptionBlockWidth,
          gScreenDimensions.BottomRow - 1,
          NvUpdateMessage
          );
        gResetRequired    = (BOOLEAN) (gResetRequired | ((Flags & EFI_IFR_FLAG_RESET_REQUIRED) == EFI_IFR_FLAG_RESET_REQUIRED));

        gNvUpdateRequired = TRUE;
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_HIGHLIGHT);
        for (Index = 0; Index < (GetStringWidth (NvUpdateMessage) - 2) / 2; Index++) {
          PrintAt (
            (gScreenDimensions.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + Index),
            gScreenDimensions.BottomRow - 1,
            L"  "
            );
        }

        gNvUpdateRequired = FALSE;
      }
    }
    break;

  case REFRESH_STATUS_BAR:
    if (mInputError) {
      UpdateStatusBar (INPUT_ERROR, Flags, TRUE);
    }

    if (gNvUpdateRequired) {
      UpdateStatusBar (NV_UPDATE_REQUIRED, Flags, TRUE);
    }
    break;

  default:
    break;
  }

  gBS->FreePool (InputErrorMessage);
  gBS->FreePool (NvUpdateMessage);
  return ;
}


/**
  Get the supported width for a particular op-code

  @param  Statement              The FORM_BROWSER_STATEMENT structure passed in.
  @param  Handle                 The handle in the HII database being used

  @return Returns the number of CHAR16 characters that is support.

**/
UINT16
GetWidth (
  IN FORM_BROWSER_STATEMENT        *Statement,
  IN EFI_HII_HANDLE                 Handle
  )
{
  CHAR16  *String;
  UINTN   Size;
  UINT16  Width;

  Size = 0;

  //
  // See if the second text parameter is really NULL
  //
  if ((Statement->Operand == EFI_IFR_TEXT_OP) && (Statement->TextTwo != 0)) {
    String = GetToken (Statement->TextTwo, Handle);
    Size   = StrLen (String);
    gBS->FreePool (String);
  }

  if ((Statement->Operand == EFI_IFR_SUBTITLE_OP) ||
      (Statement->Operand == EFI_IFR_REF_OP) ||
      (Statement->Operand == EFI_IFR_PASSWORD_OP) ||
      (Statement->Operand == EFI_IFR_ACTION_OP) ||
      (Statement->Operand == EFI_IFR_RESET_BUTTON_OP) ||
      //
      // Allow a wide display if text op-code and no secondary text op-code
      //
      ((Statement->Operand == EFI_IFR_TEXT_OP) && (Size == 0))
      ) {
    Width = (UINT16) (gPromptBlockWidth + gOptionBlockWidth);
  } else {
    Width = (UINT16) gPromptBlockWidth;
  }

  if (Statement->InSubtitle) {
    Width -= SUBTITLE_INDENT;
  }

  return Width;
}


BOOLEAN GetLineByWidthFinished = FALSE;

/**
  Will copy LineWidth amount of a string in the OutputString buffer and return the
  number of CHAR16 characters that were copied into the OutputString buffer.

  @param  InputString            String description for this option.
  @param  LineWidth              Width of the desired string to extract in CHAR16
                                 characters
  @param  Index                  Where in InputString to start the copy process
  @param  OutputString           Buffer to copy the string into

  @return Returns the number of CHAR16 characters that were copied into the OutputString buffer.

**/
UINT16
GetLineByWidth (
  IN      CHAR16                      *InputString,
  IN      UINT16                      LineWidth,
  IN OUT  UINTN                       *Index,
  OUT     CHAR16                      **OutputString
  )
{
  UINT16          Count;
  UINT16          Count2;

  if (GetLineByWidthFinished) {
    GetLineByWidthFinished = FALSE;
    return (UINT16) 0;
  }

  Count         = LineWidth;
  Count2        = 0;

  *OutputString = AllocateZeroPool (((UINTN) (LineWidth + 1) * 2));

  //
  // Ensure we have got a valid buffer
  //
  if (*OutputString != NULL) {

    //
    //NARROW_CHAR can not be printed in screen, so if a line only contain  the two CHARs: 'NARROW_CHAR + CHAR_CARRIAGE_RETURN' , it is a empty line  in Screen.
    //To avoid displaying this  empty line in screen,  just skip  the two CHARs here.
    //
   if ((InputString[*Index] == NARROW_CHAR) && (InputString[*Index + 1] == CHAR_CARRIAGE_RETURN)) {
     *Index = *Index + 2;
   }

    //
    // Fast-forward the string and see if there is a carriage-return in the string
    //
    for (; (InputString[*Index + Count2] != CHAR_CARRIAGE_RETURN) && (Count2 != LineWidth); Count2++)
      ;

    //
    // Copy the desired LineWidth of data to the output buffer.
    // Also make sure that we don't copy more than the string.
    // Also make sure that if there are linefeeds, we account for them.
    //
    if ((StrSize (&InputString[*Index]) <= ((UINTN) (LineWidth + 1) * 2)) &&
        (StrSize (&InputString[*Index]) <= ((UINTN) (Count2 + 1) * 2))
        ) {
      //
      // Convert to CHAR16 value and show that we are done with this operation
      //
      LineWidth = (UINT16) ((StrSize (&InputString[*Index]) - 2) / 2);
      if (LineWidth != 0) {
        GetLineByWidthFinished = TRUE;
      }
    } else {
      if (Count2 == LineWidth) {
        //
        // Rewind the string from the maximum size until we see a space to break the line
        //
        for (; (InputString[*Index + LineWidth] != CHAR_SPACE) && (LineWidth != 0); LineWidth--)
          ;
        if (LineWidth == 0) {
          LineWidth = Count;
        }
      } else {
        LineWidth = Count2;
      }
    }

    CopyMem (*OutputString, &InputString[*Index], LineWidth * 2);

    //
    // If currently pointing to a space, increment the index to the first non-space character
    //
    for (;
         (InputString[*Index + LineWidth] == CHAR_SPACE) || (InputString[*Index + LineWidth] == CHAR_CARRIAGE_RETURN);
         (*Index)++
        )
      ;
    *Index = (UINT16) (*Index + LineWidth);
    return LineWidth;
  } else {
    return (UINT16) 0;
  }
}


/**
  Update display lines for a Menu Option.

  @param  Selection              The user's selection.
  @param  MenuOption             The MenuOption to be checked.
  @param  OptionalString         The option string.
  @param  SkipValue              The number of lins to skip.

**/
VOID
UpdateOptionSkipLines (
  IN UI_MENU_SELECTION            *Selection,
  IN UI_MENU_OPTION               *MenuOption,
  OUT CHAR16                       **OptionalString,
  IN UINTN                        SkipValue
  )
{
  UINTN   Index;
  UINT16  Width;
  UINTN   Row;
  UINTN   OriginalRow;
  CHAR16  *OutputString;
  CHAR16  *OptionString;

  Row           = 0;
  OptionString  = *OptionalString;
  OutputString  = NULL;

  ProcessOptions (Selection, MenuOption, FALSE, &OptionString);

  if (OptionString != NULL) {
    Width               = (UINT16) gOptionBlockWidth;

    OriginalRow         = Row;

    for (Index = 0; GetLineByWidth (OptionString, Width, &Index, &OutputString) != 0x0000;) {
      //
      // If there is more string to process print on the next row and increment the Skip value
      //
      if (StrLen (&OptionString[Index])) {
        if (SkipValue == 0) {
          Row++;
          //
          // Since the Number of lines for this menu entry may or may not be reflected accurately
          // since the prompt might be 1 lines and option might be many, and vice versa, we need to do
          // some testing to ensure we are keeping this in-sync.
          //
          // If the difference in rows is greater than or equal to the skip value, increase the skip value
          //
          if ((Row - OriginalRow) >= MenuOption->Skip) {
            MenuOption->Skip++;
          }
        }
      }

      gBS->FreePool (OutputString);
      if (SkipValue != 0) {
        SkipValue--;
      }
    }

    Row = OriginalRow;
  }

  *OptionalString = OptionString;
}


/**
  Check whether this Menu Option could be highlighted.

  This is an internal function.

  @param  MenuOption             The MenuOption to be checked.

  @retval TRUE                   This Menu Option is selectable.
  @retval FALSE                  This Menu Option could not be selected.

**/
BOOLEAN
IsSelectable (
  UI_MENU_OPTION   *MenuOption
  )
{
  if ((MenuOption->ThisTag->Operand == EFI_IFR_SUBTITLE_OP) ||
      MenuOption->GrayOut || MenuOption->ReadOnly) {
    return FALSE;
  } else {
    return TRUE;
  }
}


/**
  Determine if the menu is the last menu that can be selected.

  This is an internal function.
  
  @param  Direction              The scroll direction. False is down. True is up.
  @param  CurrentPos             The current focus.

  @return FALSE -- the menu isn't the last menu that can be selected.
  @return TRUE  -- the menu is the last menu that can be selected.

**/
BOOLEAN
ValueIsScroll (
  IN  BOOLEAN                     Direction,
  IN  LIST_ENTRY                  *CurrentPos
  )
{
  LIST_ENTRY      *Temp;
  UI_MENU_OPTION  *MenuOption;

  Temp = Direction ? CurrentPos->BackLink : CurrentPos->ForwardLink;

  if (Temp == &Menu) {
    return TRUE;
  }

  for (; Temp != &Menu; Temp = Direction ? Temp->BackLink : Temp->ForwardLink) {
    MenuOption = MENU_OPTION_FROM_LINK (Temp);
    if (IsSelectable (MenuOption)) {
      return FALSE;
    }
  }

  return TRUE;
}


/**
  Move to next selectable statement.
  
  This is an internal function.
  
  @param  GoUp                   The navigation direction. TRUE: up, FALSE: down.
  @param  CurrentPosition        Current position.

  @return The row distance from current MenuOption to next selectable MenuOption.

**/
INTN
MoveToNextStatement (
  IN     BOOLEAN                   GoUp,
  IN OUT LIST_ENTRY                **CurrentPosition
  )
{
  INTN             Distance;
  LIST_ENTRY       *Pos;
  BOOLEAN          HitEnd;
  UI_MENU_OPTION   *NextMenuOption;

  Distance = 0;
  Pos      = *CurrentPosition;
  HitEnd   = FALSE;

  while (TRUE) {
    NextMenuOption = MENU_OPTION_FROM_LINK (Pos);
    if (IsSelectable (NextMenuOption)) {
      break;
    }
    if ((GoUp ? Pos->BackLink : Pos->ForwardLink) == &Menu) {
      HitEnd = TRUE;
      break;
    }
    Distance += NextMenuOption->Skip;
    Pos = (GoUp ? Pos->BackLink : Pos->ForwardLink);
  }

  if (HitEnd) {
    //
    // If we hit end there is still no statement can be focused,
    // we go backwards to find the statement can be focused.
    //
    Distance = 0;
    Pos = *CurrentPosition;

    while (TRUE) {
      NextMenuOption = MENU_OPTION_FROM_LINK (Pos);
      if (IsSelectable (NextMenuOption)) {
        break;
      }
      if ((!GoUp ? Pos->BackLink : Pos->ForwardLink) == &Menu) {
        ASSERT (FALSE);
        break;
      }
      Distance -= NextMenuOption->Skip;
      Pos = (!GoUp ? Pos->BackLink : Pos->ForwardLink);
    }
  }

  *CurrentPosition = &NextMenuOption->Link;
  return Distance;
}


/**
  Adjust Data and Time position accordingly.
  Data format :      [01/02/2004]      [11:22:33]
  Line number :        0  0    1         0  0  1
  
  This is an internal function.

  @param  DirectionUp            the up or down direction. False is down. True is
                                 up.
  @param  CurrentPosition        Current position. On return: Point to the last
                                 Option (Year or Second) if up; Point to the first
                                 Option (Month or Hour) if down.

  @return Return line number to pad. It is possible that we stand on a zero-advance
  @return data or time opcode, so pad one line when we judge if we are going to scroll outside.

**/
UINTN
AdjustDateAndTimePosition (
  IN     BOOLEAN                     DirectionUp,
  IN OUT LIST_ENTRY                  **CurrentPosition
  )
{
  UINTN           Count;
  LIST_ENTRY      *NewPosition;
  UI_MENU_OPTION  *MenuOption;
  UINTN           PadLineNumber;

  PadLineNumber = 0;
  NewPosition   = *CurrentPosition;
  MenuOption    = MENU_OPTION_FROM_LINK (NewPosition);

  if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) ||
      (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)) {
    //
    // Calculate the distance from current position to the last Date/Time MenuOption
    //
    Count = 0;
    while (MenuOption->Skip == 0) {
      Count++;
      NewPosition   = NewPosition->ForwardLink;
      MenuOption    = MENU_OPTION_FROM_LINK (NewPosition);
      PadLineNumber = 1;
    }

    NewPosition = *CurrentPosition;
    if (DirectionUp) {
      //
      // Since the behavior of hitting the up arrow on a Date/Time MenuOption is intended
      // to be one that back to the previous set of MenuOptions, we need to advance to the first
      // Date/Time MenuOption and leave the remaining logic in CfUiUp intact so the appropriate
      // checking can be done.
      //
      while (Count++ < 2) {
        NewPosition = NewPosition->BackLink;
      }
    } else {
      //
      // Since the behavior of hitting the down arrow on a Date/Time MenuOption is intended
      // to be one that progresses to the next set of MenuOptions, we need to advance to the last
      // Date/Time MenuOption and leave the remaining logic in CfUiDown intact so the appropriate
      // checking can be done.
      //
      while (Count-- > 0) {
        NewPosition = NewPosition->ForwardLink;
      }
    }

    *CurrentPosition = NewPosition;
  }

  return PadLineNumber;
}


/**
  Display menu and wait for user to select one menu option, then return it.
  If AutoBoot is enabled, then if user doesn't select any option,
  after period of time, it will automatically return the first menu option.

  @param  Selection              Menu selection.

  @retval EFI_SUCESSS            This function always return successfully for now.

**/
EFI_STATUS
UiDisplayMenu (
  IN OUT UI_MENU_SELECTION           *Selection
  )
{
  INTN                            SkipValue;
  INTN                            Difference;
  INTN                            OldSkipValue;
  UINTN                           DistanceValue;
  UINTN                           Row;
  UINTN                           Col;
  UINTN                           Temp;
  UINTN                           Temp2;
  UINTN                           TopRow;
  UINTN                           BottomRow;
  UINTN                           OriginalRow;
  UINTN                           Index;
  UINT32                          Count;
  UINT16                          Width;
  CHAR16                          *StringPtr;
  CHAR16                          *OptionString;
  CHAR16                          *OutputString;
  CHAR16                          *FormattedString;
  CHAR16                          YesResponse;
  CHAR16                          NoResponse;
  BOOLEAN                         NewLine;
  BOOLEAN                         Repaint;
  BOOLEAN                         SavedValue;
  EFI_STATUS                      Status;
  EFI_INPUT_KEY                   Key;
  LIST_ENTRY                      *Link;
  LIST_ENTRY                      *NewPos;
  LIST_ENTRY                      *TopOfScreen;
  LIST_ENTRY                      *SavedListEntry;
  UI_MENU_OPTION                  *MenuOption;
  UI_MENU_OPTION                  *NextMenuOption;
  UI_MENU_OPTION                  *SavedMenuOption;
  UI_MENU_OPTION                  *PreviousMenuOption;
  UI_CONTROL_FLAG                 ControlFlag;
  EFI_SCREEN_DESCRIPTOR           LocalScreen;
  MENU_REFRESH_ENTRY              *MenuRefreshEntry;
  UI_SCREEN_OPERATION             ScreenOperation;
  UINT8                           MinRefreshInterval;
  UINTN                           BufferSize;
  UINT16                          DefaultId;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  FORM_BROWSER_STATEMENT          *Statement;

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  Status              = EFI_SUCCESS;
  FormattedString     = NULL;
  OptionString        = NULL;
  ScreenOperation     = UiNoOperation;
  NewLine             = TRUE;
  MinRefreshInterval  = 0;
  DefaultId           = 0;

  OutputString        = NULL;
  gUpArrow            = FALSE;
  gDownArrow          = FALSE;
  SkipValue           = 0;
  OldSkipValue        = 0;
  MenuRefreshEntry    = gMenuRefreshHead;

  NextMenuOption      = NULL;
  PreviousMenuOption  = NULL;
  SavedMenuOption     = NULL;

  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  if (gClassOfVfr == EFI_FRONT_PAGE_SUBCLASS) {
    TopRow  = LocalScreen.TopRow + FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
    Row     = LocalScreen.TopRow + FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
  } else {
    TopRow  = LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
    Row     = LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
  }

  Col = LocalScreen.LeftColumn;
  BottomRow = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT - SCROLL_ARROW_HEIGHT - 1;

  Selection->TopRow = TopRow;
  Selection->BottomRow = BottomRow;
  Selection->PromptCol = Col;
  Selection->OptionCol = gPromptBlockWidth + 1 + LocalScreen.LeftColumn;
  Selection->Statement = NULL;

  TopOfScreen = Menu.ForwardLink;
  Repaint     = TRUE;
  MenuOption  = NULL;

  //
  // Get user's selection
  //
  NewPos = Menu.ForwardLink;

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  UpdateStatusBar (REFRESH_STATUS_BAR, (UINT8) 0, TRUE);

  ControlFlag = CfInitialization;
  Selection->Action = UI_ACTION_NONE;
  while (TRUE) {
    switch (ControlFlag) {
    case CfInitialization:
      if (IsListEmpty (&Menu)) {
        ControlFlag = CfReadKey;
      } else {
        ControlFlag = CfCheckSelection;
      }
      break;

    case CfCheckSelection:
      if (Selection->Action != UI_ACTION_NONE) {
        ControlFlag = CfExit;
      } else {
        ControlFlag = CfRepaint;
      }
      break;

    case CfRepaint:
      ControlFlag = CfRefreshHighLight;

      if (Repaint) {
        //
        // Display menu
        //
        gDownArrow      = FALSE;
        gUpArrow        = FALSE;
        Row             = TopRow;

        Temp            = SkipValue;
        Temp2           = SkipValue;

        ClearLines (
          LocalScreen.LeftColumn,
          LocalScreen.RightColumn,
          TopRow - SCROLL_ARROW_HEIGHT,
          BottomRow + SCROLL_ARROW_HEIGHT,
          FIELD_TEXT | FIELD_BACKGROUND
          );

        UiFreeRefreshList ();
        MinRefreshInterval = 0;

        for (Link = TopOfScreen; Link != &Menu; Link = Link->ForwardLink) {
          MenuOption          = MENU_OPTION_FROM_LINK (Link);
          MenuOption->Row     = Row;
          MenuOption->Col     = Col;
          MenuOption->OptCol  = gPromptBlockWidth + 1 + LocalScreen.LeftColumn;

          Statement = MenuOption->ThisTag;
          if (Statement->InSubtitle) {
            MenuOption->Col += SUBTITLE_INDENT;
          }

          if (MenuOption->GrayOut) {
            gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_GRAYED | FIELD_BACKGROUND);
          } else {
            if (Statement->Operand == EFI_IFR_SUBTITLE_OP) {
              gST->ConOut->SetAttribute (gST->ConOut, SUBTITLE_TEXT | FIELD_BACKGROUND);
            }
          }

          Width       = GetWidth (Statement, MenuOption->Handle);
          OriginalRow = Row;

          for (Index = 0; GetLineByWidth (MenuOption->Description, Width, &Index, &OutputString) != 0x0000;) {
            if ((Temp == 0) && (Row <= BottomRow)) {
              PrintStringAt (MenuOption->Col, Row, OutputString);
            }
            //
            // If there is more string to process print on the next row and increment the Skip value
            //
            if (StrLen (&MenuOption->Description[Index])) {
              if (Temp == 0) {
                Row++;
              }
            }

            gBS->FreePool (OutputString);
            if (Temp != 0) {
              Temp--;
            }
          }

          Temp  = 0;
          Row   = OriginalRow;

          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
          Status = ProcessOptions (Selection, MenuOption, FALSE, &OptionString);
          if (EFI_ERROR (Status)) {
            //
            // Repaint to clear possible error prompt pop-up
            //
            Repaint = TRUE;
            NewLine = TRUE;
            ControlFlag = CfRepaint;
            break;
          }

          if (OptionString != NULL) {
            if (Statement->Operand == EFI_IFR_DATE_OP || Statement->Operand == EFI_IFR_TIME_OP) {
              //
              // If leading spaces on OptionString - remove the spaces
              //
              for (Index = 0; OptionString[Index] == L' '; Index++) {
                MenuOption->OptCol++;
              }

              for (Count = 0; OptionString[Index] != CHAR_NULL; Index++) {
                OptionString[Count] = OptionString[Index];
                Count++;
              }

              OptionString[Count] = CHAR_NULL;
            }

            //
            // If Question request refresh, register the op-code
            //
            if (Statement->RefreshInterval != 0) {
              //
              // Menu will be refreshed at minimal interval of all Questions
              // which have refresh request
              //
              if (MinRefreshInterval == 0 || Statement->RefreshInterval < MinRefreshInterval) {
                MinRefreshInterval = Statement->RefreshInterval;
              }

              if (gMenuRefreshHead == NULL) {
                MenuRefreshEntry = AllocateZeroPool (sizeof (MENU_REFRESH_ENTRY));
                ASSERT (MenuRefreshEntry != NULL);
                MenuRefreshEntry->MenuOption        = MenuOption;
                MenuRefreshEntry->Selection         = Selection;
                MenuRefreshEntry->CurrentColumn     = MenuOption->OptCol;
                MenuRefreshEntry->CurrentRow        = MenuOption->Row;
                MenuRefreshEntry->CurrentAttribute  = FIELD_TEXT | FIELD_BACKGROUND;
                gMenuRefreshHead                    = MenuRefreshEntry;
              } else {
                //
                // Advance to the last entry
                //
                for (MenuRefreshEntry = gMenuRefreshHead;
                     MenuRefreshEntry->Next != NULL;
                     MenuRefreshEntry = MenuRefreshEntry->Next
                    )
                  ;
                MenuRefreshEntry->Next = AllocateZeroPool (sizeof (MENU_REFRESH_ENTRY));
                ASSERT (MenuRefreshEntry->Next != NULL);
                MenuRefreshEntry                    = MenuRefreshEntry->Next;
                MenuRefreshEntry->MenuOption        = MenuOption;
                MenuRefreshEntry->Selection         = Selection;
                MenuRefreshEntry->CurrentColumn     = MenuOption->OptCol;
                MenuRefreshEntry->CurrentRow        = MenuOption->Row;
                MenuRefreshEntry->CurrentAttribute  = FIELD_TEXT | FIELD_BACKGROUND;
              }
            }

            Width       = (UINT16) gOptionBlockWidth;
            OriginalRow = Row;

            for (Index = 0; GetLineByWidth (OptionString, Width, &Index, &OutputString) != 0x0000;) {
              if ((Temp2 == 0) && (Row <= BottomRow)) {
                PrintStringAt (MenuOption->OptCol, Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&OptionString[Index])) {
                if (Temp2 == 0) {
                  Row++;
                  //
                  // Since the Number of lines for this menu entry may or may not be reflected accurately
                  // since the prompt might be 1 lines and option might be many, and vice versa, we need to do
                  // some testing to ensure we are keeping this in-sync.
                  //
                  // If the difference in rows is greater than or equal to the skip value, increase the skip value
                  //
                  if ((Row - OriginalRow) >= MenuOption->Skip) {
                    MenuOption->Skip++;
                  }
                }
              }

              gBS->FreePool (OutputString);
              if (Temp2 != 0) {
                Temp2--;
              }
            }

            Temp2 = 0;
            Row   = OriginalRow;

            gBS->FreePool (OptionString);
          }
          //
          // If this is a text op with secondary text information
          //
          if ((Statement->Operand == EFI_IFR_TEXT_OP) && (Statement->TextTwo != 0)) {
            StringPtr   = GetToken (Statement->TextTwo, MenuOption->Handle);

            Width       = (UINT16) gOptionBlockWidth;
            OriginalRow = Row;

            for (Index = 0; GetLineByWidth (StringPtr, Width, &Index, &OutputString) != 0x0000;) {
              if ((Temp == 0) && (Row <= BottomRow)) {
                PrintStringAt (MenuOption->OptCol, Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&StringPtr[Index])) {
                if (Temp2 == 0) {
                  Row++;
                  //
                  // Since the Number of lines for this menu entry may or may not be reflected accurately
                  // since the prompt might be 1 lines and option might be many, and vice versa, we need to do
                  // some testing to ensure we are keeping this in-sync.
                  //
                  // If the difference in rows is greater than or equal to the skip value, increase the skip value
                  //
                  if ((Row - OriginalRow) >= MenuOption->Skip) {
                    MenuOption->Skip++;
                  }
                }
              }

              gBS->FreePool (OutputString);
              if (Temp2 != 0) {
                Temp2--;
              }
            }

            Row = OriginalRow;
            gBS->FreePool (StringPtr);
          }

          //
          // Need to handle the bottom of the display
          //
          if (MenuOption->Skip > 1) {
            Row += MenuOption->Skip - SkipValue;
            SkipValue = 0;
          } else {
            Row += MenuOption->Skip;
          }

          if (Row > BottomRow) {
            if (!ValueIsScroll (FALSE, Link)) {
              gDownArrow = TRUE;
            }

            Row = BottomRow + 1;
            break;
          }
        }

        if (!ValueIsScroll (TRUE, TopOfScreen)) {
          gUpArrow = TRUE;
        }

        if (gUpArrow) {
          gST->ConOut->SetAttribute (gST->ConOut, ARROW_TEXT | ARROW_BACKGROUND);
          PrintAt (
            LocalScreen.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + 1,
            TopRow - SCROLL_ARROW_HEIGHT,
            L"%c",
            ARROW_UP
            );
          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
        }

        if (gDownArrow) {
          gST->ConOut->SetAttribute (gST->ConOut, ARROW_TEXT | ARROW_BACKGROUND);
          PrintAt (
            LocalScreen.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + 1,
            BottomRow + SCROLL_ARROW_HEIGHT,
            L"%c",
            ARROW_DOWN
            );
          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
        }

        MenuOption = NULL;
      }
      break;

    case CfRefreshHighLight:
      //
      // MenuOption: Last menu option that need to remove hilight
      //             MenuOption is set to NULL in Repaint
      // NewPos:     Current menu option that need to hilight
      //
      ControlFlag = CfUpdateHelpString;

      //
      // Repaint flag is normally reset when finish processing CfUpdateHelpString. Temporarily
      // reset Repaint flag because we may break halfway and skip CfUpdateHelpString processing.
      //
      SavedValue  = Repaint;
      Repaint     = FALSE;

      if (Selection->QuestionId != 0) {
        NewPos = Menu.ForwardLink;
        SavedMenuOption = MENU_OPTION_FROM_LINK (NewPos);

        while (SavedMenuOption->ThisTag->QuestionId != Selection->QuestionId && NewPos->ForwardLink != &Menu) {
          NewPos     = NewPos->ForwardLink;
          SavedMenuOption = MENU_OPTION_FROM_LINK (NewPos);
        }
        if (SavedMenuOption->ThisTag->QuestionId == Selection->QuestionId) {
          //
          // Target Question found, find its MenuOption
          //
          Link = TopOfScreen;

          for (Index = TopRow; Index <= BottomRow && Link != NewPos;) {
            SavedMenuOption = MENU_OPTION_FROM_LINK (Link);
            Index += SavedMenuOption->Skip;
            Link = Link->ForwardLink;
          }

          if (Link != NewPos || Index > BottomRow) {
            //
            // NewPos is not in the current page, simply scroll page so that NewPos is in the end of the page
            //
            Link    = NewPos;
            for (Index = TopRow; Index <= BottomRow; ) {
              Link = Link->BackLink;
              SavedMenuOption = MENU_OPTION_FROM_LINK (Link);
              Index     += SavedMenuOption->Skip;
            }
            TopOfScreen     = Link->ForwardLink;

            Repaint = TRUE;
            NewLine = TRUE;
            ControlFlag = CfRepaint;
            break;
          }
        } else {
          //
          // Target Question not found, highlight the default menu option
          //
          NewPos = TopOfScreen;
        }

        Selection->QuestionId = 0;
      }

      if (NewPos != NULL && (MenuOption == NULL || NewPos != &MenuOption->Link)) {
        if (MenuOption != NULL) {
          //
          // Remove highlight on last Menu Option
          //
          gST->ConOut->SetCursorPosition (gST->ConOut, MenuOption->Col, MenuOption->Row);
          ProcessOptions (Selection, MenuOption, FALSE, &OptionString);
          gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
          if (OptionString != NULL) {
            if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) ||
                (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)
               ) {
              //
              // If leading spaces on OptionString - remove the spaces
              //
              for (Index = 0; OptionString[Index] == L' '; Index++)
                ;

              for (Count = 0; OptionString[Index] != CHAR_NULL; Index++) {
                OptionString[Count] = OptionString[Index];
                Count++;
              }

              OptionString[Count] = CHAR_NULL;
            }

            Width               = (UINT16) gOptionBlockWidth;
            OriginalRow         = MenuOption->Row;

            for (Index = 0; GetLineByWidth (OptionString, Width, &Index, &OutputString) != 0x0000;) {
              if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
                PrintStringAt (MenuOption->OptCol, MenuOption->Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&OptionString[Index])) {
                MenuOption->Row++;
              }

              gBS->FreePool (OutputString);
            }

            MenuOption->Row = OriginalRow;

            gBS->FreePool (OptionString);
          } else {
            if (NewLine) {
              if (MenuOption->GrayOut) {
                gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_GRAYED | FIELD_BACKGROUND);
              } else if (MenuOption->ThisTag->Operand == EFI_IFR_SUBTITLE_OP) {
                gST->ConOut->SetAttribute (gST->ConOut, SUBTITLE_TEXT | FIELD_BACKGROUND);
              }

              OriginalRow = MenuOption->Row;
              Width       = GetWidth (MenuOption->ThisTag, MenuOption->Handle);

              for (Index = 0; GetLineByWidth (MenuOption->Description, Width, &Index, &OutputString) != 0x0000;) {
                if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
                  PrintStringAt (MenuOption->Col, MenuOption->Row, OutputString);
                }
                //
                // If there is more string to process print on the next row and increment the Skip value
                //
                if (StrLen (&MenuOption->Description[Index])) {
                  MenuOption->Row++;
                }

                gBS->FreePool (OutputString);
              }

              MenuOption->Row = OriginalRow;
              gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
            }
          }
        }

        //
        // This is only possible if we entered this page and the first menu option is
        // a "non-menu" item.  In that case, force it UiDown
        //
        MenuOption = MENU_OPTION_FROM_LINK (NewPos);
        if (!IsSelectable (MenuOption)) {
          ASSERT (ScreenOperation == UiNoOperation);
          ScreenOperation = UiDown;
          ControlFlag     = CfScreenOperation;
          break;
        }

        //
        // This is the current selected statement
        //
        Statement = MenuOption->ThisTag;
        Selection->Statement = Statement;

        //
        // Set reverse attribute
        //
        gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_HIGHLIGHT | FIELD_BACKGROUND_HIGHLIGHT);
        gST->ConOut->SetCursorPosition (gST->ConOut, MenuOption->Col, MenuOption->Row);

        //
        // Assuming that we have a refresh linked-list created, lets annotate the
        // appropriate entry that we are highlighting with its new attribute.  Just prior to this
        // lets reset all of the entries' attribute so we do not get multiple highlights in he refresh
        //
        if (gMenuRefreshHead != NULL) {
          for (MenuRefreshEntry = gMenuRefreshHead; MenuRefreshEntry != NULL; MenuRefreshEntry = MenuRefreshEntry->Next) {
            MenuRefreshEntry->CurrentAttribute = FIELD_TEXT | FIELD_BACKGROUND;
            if (MenuRefreshEntry->MenuOption == MenuOption) {
              MenuRefreshEntry->CurrentAttribute = FIELD_TEXT_HIGHLIGHT | FIELD_BACKGROUND_HIGHLIGHT;
            }
          }
        }

        ProcessOptions (Selection, MenuOption, FALSE, &OptionString);
        if (OptionString != NULL) {
          if (Statement->Operand == EFI_IFR_DATE_OP || Statement->Operand == EFI_IFR_TIME_OP) {
            //
            // If leading spaces on OptionString - remove the spaces
            //
            for (Index = 0; OptionString[Index] == L' '; Index++)
              ;

            for (Count = 0; OptionString[Index] != CHAR_NULL; Index++) {
              OptionString[Count] = OptionString[Index];
              Count++;
            }

            OptionString[Count] = CHAR_NULL;
          }
          Width               = (UINT16) gOptionBlockWidth;

          OriginalRow         = MenuOption->Row;

          for (Index = 0; GetLineByWidth (OptionString, Width, &Index, &OutputString) != 0x0000;) {
            if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
              PrintStringAt (MenuOption->OptCol, MenuOption->Row, OutputString);
            }
            //
            // If there is more string to process print on the next row and increment the Skip value
            //
            if (StrLen (&OptionString[Index])) {
              MenuOption->Row++;
            }

            gBS->FreePool (OutputString);
          }

          MenuOption->Row = OriginalRow;

          gBS->FreePool (OptionString);
        } else {
          if (NewLine) {
            OriginalRow = MenuOption->Row;

            Width       = GetWidth (Statement, MenuOption->Handle);

            for (Index = 0; GetLineByWidth (MenuOption->Description, Width, &Index, &OutputString) != 0x0000;) {
              if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
                PrintStringAt (MenuOption->Col, MenuOption->Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&MenuOption->Description[Index])) {
                MenuOption->Row++;
              }

              gBS->FreePool (OutputString);
            }

            MenuOption->Row = OriginalRow;

          }
        }

        UpdateKeyHelp (MenuOption, FALSE);

        //
        // Clear reverse attribute
        //
        gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT | FIELD_BACKGROUND);
      }
      //
      // Repaint flag will be used when process CfUpdateHelpString, so restore its value
      // if we didn't break halfway when process CfRefreshHighLight.
      //
      Repaint = SavedValue;
      break;

    case CfUpdateHelpString:
      ControlFlag = CfPrepareToReadKey;

        if ((Repaint || NewLine) && (gClassOfVfr != EFI_GENERAL_APPLICATION_SUBCLASS)) {
        //
        // Don't print anything if it is a NULL help token
        //
        if (MenuOption->ThisTag->Help == 0) {
          StringPtr = L"\0";
        } else {
          StringPtr = GetToken (MenuOption->ThisTag->Help, MenuOption->Handle);
        }

        ProcessHelpString (StringPtr, &FormattedString, BottomRow - TopRow);

        gST->ConOut->SetAttribute (gST->ConOut, HELP_TEXT | FIELD_BACKGROUND);

        for (Index = 0; Index < BottomRow - TopRow; Index++) {
          //
          // Pad String with spaces to simulate a clearing of the previous line
          //
          for (; GetStringWidth (&FormattedString[Index * gHelpBlockWidth * 2]) / 2 < gHelpBlockWidth;) {
            StrCat (&FormattedString[Index * gHelpBlockWidth * 2], L" ");
          }

          PrintStringAt (
            LocalScreen.RightColumn - gHelpBlockWidth,
            Index + TopRow,
            &FormattedString[Index * gHelpBlockWidth * 2]
            );
        }
      }
      //
      // Reset this flag every time we finish using it.
      //
      Repaint = FALSE;
      NewLine = FALSE;
      break;

    case CfPrepareToReadKey:
      ControlFlag = CfReadKey;
      ScreenOperation = UiNoOperation;
      break;

    case CfReadKey:
      ControlFlag = CfScreenOperation;

      //
      // Wait for user's selection
      //
      do {
        Status = UiWaitForSingleEvent (gST->ConIn->WaitForKey, 0, MinRefreshInterval);
      } while (Status == EFI_TIMEOUT);

      if (Selection->Action == UI_ACTION_REFRESH_FORMSET) {
        //
        // IFR is updated in Callback of refresh opcode, re-parse it
        //
        ControlFlag = CfUiReset;
        Selection->Statement = NULL;
        break;
      }

      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      //
      // if we encounter error, continue to read another key in.
      //
      if (EFI_ERROR (Status)) {
        ControlFlag = CfReadKey;
        break;
      }

      if (IsListEmpty (&Menu) && Key.UnicodeChar != CHAR_NULL) {
        //
        // If the screen has no menu items, and the user didn't select UiPrevious, or UiReset
        //
        break;
      }

      switch (Key.UnicodeChar) {
      case CHAR_CARRIAGE_RETURN:
        ScreenOperation = UiSelect;
        gDirection      = 0;
        break;

      //
      // We will push the adjustment of these numeric values directly to the input handler
      //  NOTE: we won't handle manual input numeric
      //
      case '+':
      case '-':
        Statement = MenuOption->ThisTag;
        if ((Statement->Operand == EFI_IFR_DATE_OP)
          || (Statement->Operand == EFI_IFR_TIME_OP)
          || ((Statement->Operand == EFI_IFR_NUMERIC_OP) && (Statement->Step != 0))
        ){
          if (Key.UnicodeChar == '+') {
            gDirection = SCAN_RIGHT;
          } else {
            gDirection = SCAN_LEFT;
          }
          Status = ProcessOptions (Selection, MenuOption, TRUE, &OptionString);
          if (EFI_ERROR (Status)) {
            //
            // Repaint to clear possible error prompt pop-up
            //
            Repaint = TRUE;
            NewLine = TRUE;
          }
          if (OptionString != NULL) {
            FreePool (OptionString);
          }
        }
        break;

      case '^':
        ScreenOperation = UiUp;
        break;

      case 'V':
      case 'v':
        ScreenOperation = UiDown;
        break;

      case ' ':
        if (gClassOfVfr != EFI_FRONT_PAGE_SUBCLASS) {
          if (MenuOption->ThisTag->Operand == EFI_IFR_CHECKBOX_OP && !MenuOption->GrayOut) {
            ScreenOperation = UiSelect;
          }
        }
        break;

      case CHAR_NULL:
        if (((Key.ScanCode == SCAN_F1) && ((gFunctionKeySetting & FUNCTION_ONE) != FUNCTION_ONE)) ||
            ((Key.ScanCode == SCAN_F2) && ((gFunctionKeySetting & FUNCTION_TWO) != FUNCTION_TWO)) ||
            ((Key.ScanCode == SCAN_F9) && ((gFunctionKeySetting & FUNCTION_NINE) != FUNCTION_NINE)) ||
            ((Key.ScanCode == SCAN_F10) && ((gFunctionKeySetting & FUNCTION_TEN) != FUNCTION_TEN))
            ) {
          //
          // If the function key has been disabled, just ignore the key.
          //
        } else {
          for (Index = 0; Index < sizeof (gScanCodeToOperation) / sizeof (gScanCodeToOperation[0]); Index++) {
            if (Key.ScanCode == gScanCodeToOperation[Index].ScanCode) {
              if (Key.ScanCode == SCAN_F9) {
                //
                // Reset to standard default
                //
                DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
              }
              ScreenOperation = gScanCodeToOperation[Index].ScreenOperation;
              break;
            }
          }
        }
        break;
      }
      break;

    case CfScreenOperation:
      if (ScreenOperation != UiPrevious && ScreenOperation != UiReset) {
        //
        // If the screen has no menu items, and the user didn't select UiPrevious, or UiReset
        // ignore the selection and go back to reading keys.
        //
        if (IsListEmpty (&Menu)) {
          ControlFlag = CfReadKey;
          break;
        }
        //
        // if there is nothing logical to place a cursor on, just move on to wait for a key.
        //
        for (Link = Menu.ForwardLink; Link != &Menu; Link = Link->ForwardLink) {
          NextMenuOption = MENU_OPTION_FROM_LINK (Link);
          if (IsSelectable (NextMenuOption)) {
            break;
          }
        }

        if (Link == &Menu) {
          ControlFlag = CfPrepareToReadKey;
          break;
        }
      } else if (ScreenOperation == UiReset) {
        //
        // Press ESC to exit FormSet
        //
        Selection->Action = UI_ACTION_EXIT;
        Selection->Statement = NULL;
      }

      for (Index = 0;
           Index < sizeof (gScreenOperationToControlFlag) / sizeof (gScreenOperationToControlFlag[0]);
           Index++
          ) {
        if (ScreenOperation == gScreenOperationToControlFlag[Index].ScreenOperation) {
          ControlFlag = gScreenOperationToControlFlag[Index].ControlFlag;
          break;
        }
      }
      break;

    case CfUiPrevious:
      ControlFlag = CfCheckSelection;

      if (IsListEmpty (&gMenuList)) {
        Selection->Action = UI_ACTION_NONE;
        if (IsListEmpty (&Menu)) {
          ControlFlag = CfReadKey;
        }
        break;
      }

      //
      // Remove the Cached page entry
      //
      UiRemoveMenuListEntry (Selection);

      Selection->Action = UI_ACTION_REFRESH_FORM;
      Selection->Statement = NULL;
      break;

    case CfUiSelect:
      ControlFlag = CfCheckSelection;

      Statement = MenuOption->ThisTag;
      if ((Statement->Operand == EFI_IFR_TEXT_OP) ||
          (Statement->Operand == EFI_IFR_DATE_OP) ||
          (Statement->Operand == EFI_IFR_TIME_OP) ||
          (Statement->Operand == EFI_IFR_NUMERIC_OP && Statement->Step != 0)) {
        break;
      }

      //
      // Keep highlight on current MenuOption
      //
      Selection->QuestionId = Statement->QuestionId;

      switch (Statement->Operand) {
      case EFI_IFR_REF_OP:
        if (Statement->RefDevicePath != 0) {
          //
          // Goto another Hii Package list
          //
          ControlFlag = CfUiReset;
          Selection->Action = UI_ACTION_REFRESH_FORMSET;

          StringPtr = GetToken (Statement->RefDevicePath, Selection->FormSet->HiiHandle);
          if (StringPtr == NULL) {
            //
            // No device path string not found, exit
            //
            Selection->Action = UI_ACTION_EXIT;
            Selection->Statement = NULL;
            break;
          }
          BufferSize = StrLen (StringPtr) / 2;
          DevicePath = AllocatePool (BufferSize);

          HexStringToBufInReverseOrder ((UINT8 *) DevicePath, &BufferSize, StringPtr);
          Selection->Handle = HiiLibDevicePathToHiiHandle (DevicePath);
          if (Selection->Handle == NULL) {
            //
            // If target Hii Handle not found, exit
            //
            Selection->Action = UI_ACTION_EXIT;
            Selection->Statement = NULL;
            break;
          }

          gBS->FreePool (StringPtr);
          gBS->FreePool (DevicePath);

          CopyMem (&Selection->FormSetGuid, &Statement->RefFormSetId, sizeof (EFI_GUID));
          Selection->FormId = Statement->RefFormId;
          Selection->QuestionId = Statement->RefQuestionId;
        } else if (!CompareGuid (&Statement->RefFormSetId, &gZeroGuid)) {
          //
          // Goto another Formset, check for uncommitted data
          //
          ControlFlag = CfUiReset;
          Selection->Action = UI_ACTION_REFRESH_FORMSET;

          CopyMem (&Selection->FormSetGuid, &Statement->RefFormSetId, sizeof (EFI_GUID));
          Selection->FormId = Statement->RefFormId;
          Selection->QuestionId = Statement->RefQuestionId;
        } else if (Statement->RefFormId != 0) {
          //
          // Goto another form inside this formset,
          //
          Selection->Action = UI_ACTION_REFRESH_FORM;

          //
          // Link current form so that we can always go back when someone hits the UiPrevious
          //
          UiAddMenuListEntry (Selection);

          Selection->FormId = Statement->RefFormId;
          Selection->QuestionId = Statement->RefQuestionId;
        } else if (Statement->RefQuestionId != 0) {
          //
          // Goto another Question
          //
          Selection->QuestionId = Statement->RefQuestionId;

          if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
            Selection->Action = UI_ACTION_REFRESH_FORM;
          } else {
            Repaint = TRUE;
            NewLine = TRUE;
            break;
          }
        }
        break;

      case EFI_IFR_ACTION_OP:
        //
        // Process the Config string <ConfigResp>
        //
        Status = ProcessQuestionConfig (Selection, Statement);

        if (EFI_ERROR (Status)) {
          break;
        }

        //
        // The action button may change some Question value, so refresh the form
        //
        Selection->Action = UI_ACTION_REFRESH_FORM;
        break;

      case EFI_IFR_RESET_BUTTON_OP:
        //
        // Reset Question to default value specified by DefaultId
        //
        ControlFlag = CfUiDefault;
        DefaultId = Statement->DefaultId;
        break;

      default:
        //
        // Editable Questions: oneof, ordered list, checkbox, numeric, string, password
        //
        UpdateKeyHelp (MenuOption, TRUE);
        Status = ProcessOptions (Selection, MenuOption, TRUE, &OptionString);

        if (EFI_ERROR (Status)) {
          Repaint = TRUE;
          NewLine = TRUE;
            UpdateKeyHelp (MenuOption, FALSE);
          } else {
            Selection->Action = UI_ACTION_REFRESH_FORM;
          }

          if (OptionString != NULL) {
            FreePool (OptionString);
          }
        break;
      }
      break;

    case CfUiReset:
      //
      // We are going to leave current FormSet, so check uncommited data in this FormSet
      //
      ControlFlag = CfCheckSelection;

      if (gClassOfVfr == EFI_FRONT_PAGE_SUBCLASS) {
        //
        // There is no parent menu for FrontPage
        //
        Selection->Action = UI_ACTION_NONE;
        Selection->Statement = MenuOption->ThisTag;
        break;
      }

      //
      // If NV flag is up, prompt user
      //
      if (gNvUpdateRequired) {
        Status      = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

        YesResponse = gYesResponse[0];
        NoResponse  = gNoResponse[0];

        do {
          CreateDialog (3, TRUE, 0, NULL, &Key, gEmptyString, gAreYouSure, gEmptyString);
        } while
        (
          (Key.ScanCode != SCAN_ESC) &&
          ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (NoResponse | UPPER_LOWER_CASE_OFFSET)) &&
          ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (YesResponse | UPPER_LOWER_CASE_OFFSET))
        );

        //
        // If the user hits the YesResponse key
        //
        if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (YesResponse | UPPER_LOWER_CASE_OFFSET)) {
        } else {
          Repaint = TRUE;
          NewLine = TRUE;

          Selection->Action = UI_ACTION_NONE;
          break;
        }
      }

      gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
      gST->ConOut->EnableCursor (gST->ConOut, TRUE);

      UiFreeMenuList ();
      gST->ConOut->ClearScreen (gST->ConOut);
      return EFI_SUCCESS;

    case CfUiLeft:
      ControlFlag = CfCheckSelection;
      if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) || (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)) {
        if (MenuOption->Sequence != 0) {
          //
          // In the middle or tail of the Date/Time op-code set, go left.
          //
          NewPos = NewPos->BackLink;
        }
      }
      break;

    case CfUiRight:
      ControlFlag = CfCheckSelection;
      if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) || (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)) {
        if (MenuOption->Sequence != 2) {
          //
          // In the middle or tail of the Date/Time op-code set, go left.
          //
          NewPos = NewPos->ForwardLink;
        }
      }
      break;

    case CfUiUp:
      ControlFlag = CfCheckSelection;

      SavedListEntry = TopOfScreen;

      if (NewPos->BackLink != &Menu) {
        NewLine = TRUE;
        //
        // Adjust Date/Time position before we advance forward.
        //
        AdjustDateAndTimePosition (TRUE, &NewPos);

        //
        // Caution that we have already rewind to the top, don't go backward in this situation.
        //
        if (NewPos->BackLink != &Menu) {
          NewPos = NewPos->BackLink;
        }

        PreviousMenuOption = MENU_OPTION_FROM_LINK (NewPos);
        DistanceValue = PreviousMenuOption->Skip;

        //
        // Since the behavior of hitting the up arrow on a Date/Time op-code is intended
        // to be one that back to the previous set of op-codes, we need to advance to the sencond
        // Date/Time op-code and leave the remaining logic in UiDown intact so the appropriate
        // checking can be done.
        //
        DistanceValue += AdjustDateAndTimePosition (TRUE, &NewPos);

        //
        // Check the previous menu entry to see if it was a zero-length advance.  If it was,
        // don't worry about a redraw.
        //
        if ((INTN) MenuOption->Row - (INTN) DistanceValue < (INTN) TopRow) {
          Repaint     = TRUE;
          TopOfScreen = NewPos;
        }

        Difference = MoveToNextStatement (TRUE, &NewPos);
        if ((INTN) MenuOption->Row - (INTN) DistanceValue < (INTN) TopRow) {
          if (Difference > 0) {
            //
            // Previous focus MenuOption is above the TopOfScreen, so we need to scroll
            //
            TopOfScreen = NewPos;
            Repaint     = TRUE;
          }
        }
        if (Difference < 0) {
          //
          // We want to goto previous MenuOption, but finally we go down.
          // it means that we hit the begining MenuOption that can be focused
          // so we simply scroll to the top
          //
          if (SavedListEntry != Menu.ForwardLink) {
            TopOfScreen = Menu.ForwardLink;
            Repaint     = TRUE;
          }
        }

        //
        // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
        //
        AdjustDateAndTimePosition (TRUE, &TopOfScreen);

        UpdateStatusBar (INPUT_ERROR, MenuOption->ThisTag->QuestionFlags, FALSE);
      } else {
        SavedMenuOption = MenuOption;
        MenuOption      = MENU_OPTION_FROM_LINK (NewPos);
        if (!IsSelectable (MenuOption)) {
          //
          // If we are at the end of the list and sitting on a text op, we need to more forward
          //
          ScreenOperation = UiDown;
          ControlFlag     = CfScreenOperation;
          break;
        }

        MenuOption = SavedMenuOption;
      }
      break;

    case CfUiPageUp:
      ControlFlag     = CfCheckSelection;

      if (NewPos->BackLink == &Menu) {
        NewLine = FALSE;
        Repaint = FALSE;
        break;
      }

      NewLine   = TRUE;
      Repaint   = TRUE;
      Link      = TopOfScreen;
      PreviousMenuOption = MENU_OPTION_FROM_LINK (Link);
      Index = BottomRow;
      while ((Index >= TopRow) && (Link->BackLink != &Menu)) {
        Index = Index - PreviousMenuOption->Skip;
        Link = Link->BackLink;
        PreviousMenuOption = MENU_OPTION_FROM_LINK (Link);
      }

      TopOfScreen = Link;
      Difference = MoveToNextStatement (TRUE, &Link);
      if (Difference > 0) {
        //
        // The focus MenuOption is above the TopOfScreen
        //
        TopOfScreen = Link;
      } else if (Difference < 0) {
        //
        // This happens when there is no MenuOption can be focused from
        // Current MenuOption to the first MenuOption
        //
        TopOfScreen = Menu.ForwardLink;
      }
      Index += Difference;
      if (Index < TopRow) {
        MenuOption = NULL;
      }

      if (NewPos == Link) {
        Repaint = FALSE;
        NewLine = FALSE;
      } else {
        NewPos = Link;
      }

      //
      // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
      // Don't do this when we are already in the first page.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);
      break;

    case CfUiPageDown:
      ControlFlag     = CfCheckSelection;

      if (NewPos->ForwardLink == &Menu) {
        NewLine = FALSE;
        Repaint = FALSE;
        break;
      }

      NewLine = TRUE;
      Repaint = TRUE;
      Link    = TopOfScreen;
      NextMenuOption = MENU_OPTION_FROM_LINK (Link);
      Index = TopRow;
      while ((Index <= BottomRow) && (Link->ForwardLink != &Menu)) {
        Index = Index + NextMenuOption->Skip;
        Link           = Link->ForwardLink;
        NextMenuOption = MENU_OPTION_FROM_LINK (Link);
      }

      Index += MoveToNextStatement (FALSE, &Link);
      if (Index > BottomRow) {
        //
        // There are more MenuOption needing scrolling
        //
        TopOfScreen = Link;
        MenuOption = NULL;
      }
      if (NewPos == Link && Index <= BottomRow) {
        //
        // Finally we know that NewPos is the last MenuOption can be focused.
        //
        NewLine = FALSE;
        Repaint = FALSE;
      } else {
        NewPos  = Link;
      }

      //
      // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
      // Don't do this when we are already in the last page.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);
      break;

    case CfUiDown:
      ControlFlag = CfCheckSelection;
      //
      // Since the behavior of hitting the down arrow on a Date/Time op-code is intended
      // to be one that progresses to the next set of op-codes, we need to advance to the last
      // Date/Time op-code and leave the remaining logic in UiDown intact so the appropriate
      // checking can be done.  The only other logic we need to introduce is that if a Date/Time
      // op-code is the last entry in the menu, we need to rewind back to the first op-code of
      // the Date/Time op-code.
      //
      SavedListEntry = NewPos;
      DistanceValue  = AdjustDateAndTimePosition (FALSE, &NewPos);

      if (NewPos->ForwardLink != &Menu) {
        MenuOption      = MENU_OPTION_FROM_LINK (NewPos);
        NewLine         = TRUE;
        NewPos          = NewPos->ForwardLink;
        NextMenuOption  = MENU_OPTION_FROM_LINK (NewPos);

        DistanceValue  += NextMenuOption->Skip;
        DistanceValue  += MoveToNextStatement (FALSE, &NewPos);
        //
        // An option might be multi-line, so we need to reflect that data in the overall skip value
        //
        UpdateOptionSkipLines (Selection, NextMenuOption, &OptionString, SkipValue);

        Temp = MenuOption->Row + MenuOption->Skip + DistanceValue - 1;
        if ((MenuOption->Row + MenuOption->Skip == BottomRow + 1) &&
            (NextMenuOption->ThisTag->Operand == EFI_IFR_DATE_OP ||
             NextMenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)
            ) {
          Temp ++;
        }

        //
        // If we are going to scroll, update TopOfScreen
        //
        if (Temp > BottomRow) {
          do {
            //
            // Is the current top of screen a zero-advance op-code?
            // If so, keep moving forward till we hit a >0 advance op-code
            //
            SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);

            //
            // If bottom op-code is more than one line or top op-code is more than one line
            //
            if ((DistanceValue > 1) || (MenuOption->Skip > 1)) {
              //
              // Is the bottom op-code greater than or equal in size to the top op-code?
              //
              if ((Temp - BottomRow) >= (SavedMenuOption->Skip - OldSkipValue)) {
                //
                // Skip the top op-code
                //
                TopOfScreen     = TopOfScreen->ForwardLink;
                Difference      = (Temp - BottomRow) - (SavedMenuOption->Skip - OldSkipValue);

                OldSkipValue    = Difference;

                SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);

                //
                // If we have a remainder, skip that many more op-codes until we drain the remainder
                //
                for (;
                     Difference >= (INTN) SavedMenuOption->Skip;
                     Difference = Difference - (INTN) SavedMenuOption->Skip
                    ) {
                  //
                  // Since the Difference is greater than or equal to this op-code's skip value, skip it
                  //
                  TopOfScreen     = TopOfScreen->ForwardLink;
                  SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);
                  if (Difference < (INTN) SavedMenuOption->Skip) {
                    Difference = SavedMenuOption->Skip - Difference - 1;
                    break;
                  } else {
                    if (Difference == (INTN) SavedMenuOption->Skip) {
                      TopOfScreen     = TopOfScreen->ForwardLink;
                      SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);
                      Difference      = SavedMenuOption->Skip - Difference;
                      break;
                    }
                  }
                }
                //
                // Since we will act on this op-code in the next routine, and increment the
                // SkipValue, set the skips to one less than what is required.
                //
                SkipValue = Difference - 1;

              } else {
                //
                // Since we will act on this op-code in the next routine, and increment the
                // SkipValue, set the skips to one less than what is required.
                //
                SkipValue = OldSkipValue + (Temp - BottomRow) - 1;
              }
            } else {
              if ((OldSkipValue + 1) == (INTN) SavedMenuOption->Skip) {
                TopOfScreen = TopOfScreen->ForwardLink;
                break;
              } else {
                SkipValue = OldSkipValue;
              }
            }
            //
            // If the op-code at the top of the screen is more than one line, let's not skip it yet
            // Let's set a skip flag to smoothly scroll the top of the screen.
            //
            if (SavedMenuOption->Skip > 1) {
              if (SavedMenuOption == NextMenuOption) {
                SkipValue = 0;
              } else {
                SkipValue++;
              }
            } else {
              SkipValue   = 0;
              TopOfScreen = TopOfScreen->ForwardLink;
            }
          } while (SavedMenuOption->Skip == 0);

          Repaint       = TRUE;
          OldSkipValue  = SkipValue;
        }

        MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);

        UpdateStatusBar (INPUT_ERROR, MenuOption->ThisTag->QuestionFlags, FALSE);

      } else {
        SavedMenuOption = MenuOption;
        MenuOption      = MENU_OPTION_FROM_LINK (NewPos);
        if (!IsSelectable (MenuOption)) {
          //
          // If we are at the end of the list and sitting on a text op, we need to more forward
          //
          ScreenOperation = UiUp;
          ControlFlag     = CfScreenOperation;
          break;
        }

        MenuOption = SavedMenuOption;
        //
        // If we are at the end of the list and sitting on a Date/Time op, rewind to the head.
        //
        AdjustDateAndTimePosition (TRUE, &NewPos);
      }
      break;

    case CfUiSave:
      ControlFlag = CfCheckSelection;

      //
      // Submit the form
      //
      Status = SubmitForm (Selection->FormSet, Selection->Form);

      if (!EFI_ERROR (Status)) {
        UpdateStatusBar (INPUT_ERROR, MenuOption->ThisTag->QuestionFlags, FALSE);
        UpdateStatusBar (NV_UPDATE_REQUIRED, MenuOption->ThisTag->QuestionFlags, FALSE);
      } else {
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gSaveFailed, gPressEnter, gEmptyString);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        Repaint = TRUE;
        NewLine = TRUE;
      }
      break;

    case CfUiDefault:
      ControlFlag = CfCheckSelection;

      Status = ExtractFormDefault (Selection->FormSet, Selection->Form, DefaultId);

      if (!EFI_ERROR (Status)) {
        Selection->Action = UI_ACTION_REFRESH_FORM;
        Selection->Statement = NULL;

        //
        // Show NV update flag on status bar
        //
        gNvUpdateRequired = TRUE;
      }
      break;

    case CfUiNoOperation:
      ControlFlag = CfCheckSelection;
      break;

    case CfExit:
      UiFreeRefreshList ();

      gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
      gST->ConOut->SetCursorPosition (gST->ConOut, 0, Row + 4);
      gST->ConOut->EnableCursor (gST->ConOut, TRUE);
      gST->ConOut->OutputString (gST->ConOut, L"\n");

      return EFI_SUCCESS;

    default:
      break;
    }
  }
}
