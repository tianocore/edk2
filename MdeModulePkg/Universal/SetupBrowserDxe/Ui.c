/** @file
Utility functions for User Interface functions.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"

LIST_ENTRY          gMenuOption;
LIST_ENTRY          gMenuList = INITIALIZE_LIST_HEAD_VARIABLE (gMenuList);
MENU_REFRESH_ENTRY  *gMenuRefreshHead;                // Menu list used for refresh timer opcode.
MENU_REFRESH_ENTRY  *gMenuEventGuidRefreshHead;       // Menu list used for refresh event guid opcode.

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
    SCAN_LEFT,
    UiLeft,
  },
  {
    SCAN_RIGHT,
    UiRight,
  }
};

UINTN mScanCodeNumber = sizeof (gScanCodeToOperation) / sizeof (gScanCodeToOperation[0]);

SCREEN_OPERATION_T0_CONTROL_FLAG  gScreenOperationToControlFlag[] = {
  {
    UiNoOperation,
    CfUiNoOperation,
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
    UiPageUp,
    CfUiPageUp,
  },
  {
    UiPageDown,
    CfUiPageDown
  }, 
  {
    UiHotKey,
    CfUiHotKey
  }
};

BOOLEAN  mInputError;
BOOLEAN GetLineByWidthFinished = FALSE;


/**
  Set Buffer to Value for Size bytes.

  @param  Buffer                 Memory to set.
  @param  Size                   Number of bytes to set
  @param  Value                  Value of the set operation.

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
  InitializeListHead (&gMenuOption);
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

  while (!IsListEmpty (&gMenuOption)) {
    MenuOption = MENU_OPTION_FROM_LINK (gMenuOption.ForwardLink);
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
      FreePool (MenuOption->Description);
    }
    FreePool (MenuOption);
  }
}


/**
  Create a menu with specified formset GUID and form ID, and add it as a child
  of the given parent menu.

  @param  Parent                 The parent of menu to be added.
  @param  HiiHandle              Hii handle related to this formset.
  @param  FormSetGuid            The Formset Guid of menu to be added.
  @param  FormId                 The Form ID of menu to be added.

  @return A pointer to the newly added menu or NULL if memory is insufficient.

**/
UI_MENU_LIST *
UiAddMenuList (
  IN OUT UI_MENU_LIST     *Parent,
  IN EFI_HII_HANDLE       HiiHandle,
  IN EFI_GUID             *FormSetGuid,
  IN UINT16               FormId
  )
{
  UI_MENU_LIST  *MenuList;

  MenuList = AllocateZeroPool (sizeof (UI_MENU_LIST));
  if (MenuList == NULL) {
    return NULL;
  }

  MenuList->Signature = UI_MENU_LIST_SIGNATURE;
  InitializeListHead (&MenuList->ChildListHead);

  MenuList->HiiHandle = HiiHandle;
  CopyMem (&MenuList->FormSetGuid, FormSetGuid, sizeof (EFI_GUID));
  MenuList->FormId = FormId;
  MenuList->Parent = Parent;

  if (Parent == NULL) {
    //
    // If parent is not specified, it is the root Form of a Formset
    //
    InsertTailList (&gMenuList, &MenuList->Link);
  } else {
    InsertTailList (&Parent->ChildListHead, &MenuList->Link);
  }

  return MenuList;
}


/**
  Search Menu with given FormId and FormSetGuid in all cached menu list.

  @param  Parent                 The parent of menu to search.
  @param  FormSetGuid            The Formset GUID of the menu to search.  
  @param  FormId                 The Form ID of menu to search.

  @return A pointer to menu found or NULL if not found.

**/
UI_MENU_LIST *
UiFindChildMenuList (
  IN UI_MENU_LIST         *Parent,
  IN EFI_GUID             *FormSetGuid, 
  IN UINT16               FormId
  )
{
  LIST_ENTRY      *Link;
  UI_MENU_LIST    *Child;
  UI_MENU_LIST    *MenuList;

  ASSERT (Parent != NULL);

  if (Parent->FormId == FormId && CompareGuid (FormSetGuid, &Parent->FormSetGuid)) {
    return Parent;
  }

  Link = GetFirstNode (&Parent->ChildListHead);
  while (!IsNull (&Parent->ChildListHead, Link)) {
    Child = UI_MENU_LIST_FROM_LINK (Link);

    MenuList = UiFindChildMenuList (Child, FormSetGuid, FormId);
    if (MenuList != NULL) {
      return MenuList;
    }

    Link = GetNextNode (&Parent->ChildListHead, Link);
  }

  return NULL;
}


/**
  Search Menu with given FormSetGuid and FormId in all cached menu list.

  @param  FormSetGuid            The Formset GUID of the menu to search.
  @param  FormId                 The Form ID of menu to search.

  @return A pointer to menu found or NULL if not found.

**/
UI_MENU_LIST *
UiFindMenuList (
  IN EFI_GUID             *FormSetGuid,
  IN UINT16               FormId
  )
{
  LIST_ENTRY      *Link;
  UI_MENU_LIST    *MenuList;
  UI_MENU_LIST    *Child;

  Link = GetFirstNode (&gMenuList);
  while (!IsNull (&gMenuList, Link)) {
    MenuList = UI_MENU_LIST_FROM_LINK (Link);

    Child = UiFindChildMenuList(MenuList, FormSetGuid, FormId);
    if (Child != NULL) {
      return Child;
    }

    Link = GetNextNode (&gMenuList, Link);
  }

  return NULL;
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
    FreePool (gMenuRefreshHead);
    gMenuRefreshHead = OldMenuRefreshEntry;
  }

  while (gMenuEventGuidRefreshHead != NULL) {
    OldMenuRefreshEntry = gMenuEventGuidRefreshHead->Next;
    if (gMenuEventGuidRefreshHead != NULL) {
      gBS->CloseEvent(gMenuEventGuidRefreshHead->Event);
    }
    FreePool (gMenuEventGuidRefreshHead);
    gMenuEventGuidRefreshHead = OldMenuRefreshEntry;
  }
}


/**
  Process option string for date/time opcode.

  @param  MenuOption              Menu option point to date/time.
  @param  OptionString            Option string input for process.
  @param  AddOptCol               Whether need to update MenuOption->OptCol. 

**/
VOID 
ProcessStringForDateTime (
  UI_MENU_OPTION                  *MenuOption,
  CHAR16                          *OptionString,
  BOOLEAN                         AddOptCol
  )
{
  UINTN Index;
  UINTN Count;
  FORM_BROWSER_STATEMENT          *Statement;

  ASSERT (MenuOption != NULL && OptionString != NULL);
  
  Statement = MenuOption->ThisTag;
  
  //
  // If leading spaces on OptionString - remove the spaces
  //
  for (Index = 0; OptionString[Index] == L' '; Index++) {
    //
    // Base on the blockspace to get the option column info.
    //
    if (AddOptCol) {
      MenuOption->OptCol++;
    }
  }
  
  for (Count = 0; OptionString[Index] != CHAR_NULL; Index++) {
    OptionString[Count] = OptionString[Index];
    Count++;
  }
  OptionString[Count] = CHAR_NULL;
  
  //
  // Enable to suppress field in the opcode base on the flag.
  //
  if (Statement->Operand == EFI_IFR_DATE_OP) {
    //
    // OptionString format is: <**:  **: ****>
    //                        |month|day|year|
    //                          4     3    5
    //
    if ((Statement->Flags & EFI_QF_DATE_MONTH_SUPPRESS) && (MenuOption->Sequence == 0)) {
      //
      // At this point, only "<**:" in the optionstring. 
      // Clean the day's ** field, after clean, the format is "<  :"
      //
      SetUnicodeMem (&OptionString[1], 2, L' ');
    } else if ((Statement->Flags & EFI_QF_DATE_DAY_SUPPRESS) && (MenuOption->Sequence == 1)) {
      //
      // At this point, only "**:" in the optionstring. 
      // Clean the month's "**" field, after clean, the format is "  :"
      //                
      SetUnicodeMem (&OptionString[0], 2, L' ');
    } else if ((Statement->Flags & EFI_QF_DATE_YEAR_SUPPRESS) && (MenuOption->Sequence == 2)) {
      //
      // At this point, only "****>" in the optionstring. 
      // Clean the year's "****" field, after clean, the format is "  >"
      //                
      SetUnicodeMem (&OptionString[0], 4, L' ');
    }
  } else if (Statement->Operand == EFI_IFR_TIME_OP) {
    //
    // OptionString format is: <**:  **:    **>
    //                        |hour|minute|second|
    //                          4     3      3
    //
    if ((Statement->Flags & QF_TIME_HOUR_SUPPRESS) && (MenuOption->Sequence == 0)) {
      //
      // At this point, only "<**:" in the optionstring. 
      // Clean the hour's ** field, after clean, the format is "<  :"
      //
      SetUnicodeMem (&OptionString[1], 2, L' ');
    } else if ((Statement->Flags & QF_TIME_MINUTE_SUPPRESS) && (MenuOption->Sequence == 1)) {
      //
      // At this point, only "**:" in the optionstring. 
      // Clean the minute's "**" field, after clean, the format is "  :"
      //                
      SetUnicodeMem (&OptionString[0], 2, L' ');
    } else if ((Statement->Flags & QF_TIME_SECOND_SUPPRESS) && (MenuOption->Sequence == 2)) {
      //
      // At this point, only "**>" in the optionstring. 
      // Clean the second's "**" field, after clean, the format is "  >"
      //                
      SetUnicodeMem (&OptionString[0], 2, L' ');
    }
  }
}

/**
  Refresh question.

  @param     MenuRefreshEntry    Menu refresh structure which has info about the refresh question.
**/
EFI_STATUS 
RefreshQuestion (
  IN   MENU_REFRESH_ENTRY    *MenuRefreshEntry
  )
{
  CHAR16                          *OptionString;
  EFI_STATUS                      Status;
  UI_MENU_SELECTION               *Selection;
  FORM_BROWSER_STATEMENT          *Question;

  Selection = MenuRefreshEntry->Selection;
  Question = MenuRefreshEntry->MenuOption->ThisTag;

  Status = GetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithHiiDriver);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  OptionString = NULL;
  ProcessOptions (Selection, MenuRefreshEntry->MenuOption, FALSE, &OptionString);

  if (OptionString != NULL) {
    //
    // If old Text is longer than new string, need to clean the old string before paint the newer.
    // This option is no need for time/date opcode, because time/data opcode has fixed string length.
    //
    if ((MenuRefreshEntry->MenuOption->ThisTag->Operand != EFI_IFR_DATE_OP) &&
      (MenuRefreshEntry->MenuOption->ThisTag->Operand != EFI_IFR_TIME_OP)) {
      ClearLines (
        MenuRefreshEntry->CurrentColumn, 
        MenuRefreshEntry->CurrentColumn + gOptionBlockWidth - 1,
        MenuRefreshEntry->CurrentRow,
        MenuRefreshEntry->CurrentRow,
        PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND
        );
    }

    gST->ConOut->SetAttribute (gST->ConOut, MenuRefreshEntry->CurrentAttribute);
    ProcessStringForDateTime(MenuRefreshEntry->MenuOption, OptionString, FALSE);
    PrintStringAt (MenuRefreshEntry->CurrentColumn, MenuRefreshEntry->CurrentRow, OptionString);
    FreePool (OptionString);
  }

  //
  // Question value may be changed, need invoke its Callback()
  //
  Status = ProcessCallBackFunction (Selection, Question, EFI_BROWSER_ACTION_CHANGING, FALSE);

  return Status;
}

/**
  Refresh the question which has refresh guid event attribute.
  
  @param Event    The event which has this function related.     
  @param Context  The input context info related to this event or the status code return to the caller.
**/
VOID
EFIAPI
RefreshQuestionNotify(
  IN      EFI_EVENT Event,
  IN      VOID      *Context
  )
{
  MENU_REFRESH_ENTRY              *MenuRefreshEntry;
  UI_MENU_SELECTION               *Selection;

  //
  // Reset FormPackage update flag
  //
  mHiiPackageListUpdated = FALSE;

  MenuRefreshEntry = (MENU_REFRESH_ENTRY *)Context;
  ASSERT (MenuRefreshEntry != NULL);
  Selection = MenuRefreshEntry->Selection;

  RefreshQuestion (MenuRefreshEntry);
  
  if (mHiiPackageListUpdated) {
    //
    // Package list is updated, force to reparse IFR binary of target Formset
    //
    mHiiPackageListUpdated = FALSE;
    Selection->Action = UI_ACTION_REFRESH_FORMSET;
  } 
}


/**
  Refresh screen.

**/
EFI_STATUS
RefreshForm (
  VOID
  )
{
  MENU_REFRESH_ENTRY              *MenuRefreshEntry;
  EFI_STATUS                      Status;
  UI_MENU_SELECTION               *Selection;

  if (gMenuRefreshHead != NULL) {
    //
    // call from refresh interval process.
    //
    MenuRefreshEntry = gMenuRefreshHead;
    Selection = MenuRefreshEntry->Selection;
    //
    // Reset FormPackage update flag
    //
    mHiiPackageListUpdated = FALSE;

    do {
      Status = RefreshQuestion (MenuRefreshEntry);
      if (EFI_ERROR (Status)) {
        return Status;
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
  @param  Form                   The form this statement belong to.
  @param  Statement              Statement of this Menu Option.
  @param  NumberOfLines          Display lines for this Menu Option.
  @param  MenuItemCount          The index for this Option in the Menu.

  @retval Pointer                Pointer to the added Menu Option.

**/
UI_MENU_OPTION *
UiAddMenuOption (
  IN CHAR16                  *String,
  IN EFI_HII_HANDLE          Handle,
  IN FORM_BROWSER_FORM       *Form,
  IN FORM_BROWSER_STATEMENT  *Statement,
  IN UINT16                  NumberOfLines,
  IN UINT16                  MenuItemCount
  )
{
  UI_MENU_OPTION  *MenuOption;
  UINTN           Index;
  UINTN           Count;

  Count = 1;
  MenuOption = NULL;

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

    if (EvaluateExpressionList(Statement->Expression, FALSE, NULL, NULL) == ExpressGrayOut ) {
      MenuOption->GrayOut = TRUE;
    } else {
      MenuOption->GrayOut = FALSE;
    }

    //
    // If the form or the question has the lock attribute, deal same as grayout.
    //
    if (Form->Locked || Statement->Locked) {
      MenuOption->GrayOut = TRUE;
    }
    
    switch (Statement->Operand) {
    case EFI_IFR_ORDERED_LIST_OP:
    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_NUMERIC_OP:
    case EFI_IFR_TIME_OP:
    case EFI_IFR_DATE_OP:
    case EFI_IFR_CHECKBOX_OP:
    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
      //
      // User could change the value of these items
      //
      MenuOption->IsQuestion = TRUE;
      break;

    case EFI_IFR_TEXT_OP:
      if (FeaturePcdGet (PcdBrowserGrayOutTextStatement)) {
        //
        // Initializing GrayOut option as TRUE for Text setup options 
        // so that those options will be Gray in colour and un selectable.
        //
        MenuOption->GrayOut = TRUE;
      }

    default:
      MenuOption->IsQuestion = FALSE;
      break;
    }

    if ((Statement->ValueExpression != NULL) ||
        ((Statement->QuestionFlags & EFI_IFR_FLAG_READ_ONLY) != 0)) {
      MenuOption->ReadOnly = TRUE;
    }

    InsertTailList (&gMenuOption, &MenuOption->Link);
  }

  return MenuOption;
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
EFIAPI
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

  VA_START (Marker, KeyValue);

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
  VA_END (Marker);

  Start = (DimensionsWidth - LargestString - 2) / 2 + gScreenDimensions.LeftColumn + 1;
  Top   = ((DimensionsHeight - NumberOfLines - 2) / 2) + gScreenDimensions.TopRow - 1;

  Count = 0;

  //
  // Display the Popup
  //
  VA_START (Marker, KeyValue);
  CreateSharedPopUp (LargestString, NumberOfLines, Marker);
  VA_END (Marker);

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
          FreePool (TempString);
          FreePool (BufferedString);
          gST->ConOut->SetAttribute (gST->ConOut, CurrentAttribute);
          gST->ConOut->EnableCursor (gST->ConOut, TRUE);
          return EFI_DEVICE_ERROR;

        default:
          break;
        }

        break;

      case CHAR_CARRIAGE_RETURN:
        SelectionComplete = TRUE;
        FreePool (TempString);
        FreePool (BufferedString);
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
EFIAPI
CreateMultiStringPopUp (
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

  @param  Selection              Current Selction info.
  @param  MessageType            The type of message to be shown.
  @param  Flags                  The flags in Question header.
  @param  State                  Set or clear.

**/
VOID
UpdateStatusBar (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UINTN                       MessageType,
  IN  UINT8                       Flags,
  IN  BOOLEAN                     State
  )
{
  UINTN           Index;
  CHAR16          *NvUpdateMessage;
  CHAR16          *InputErrorMessage;
  LIST_ENTRY              *Link;
  FORM_BROWSER_FORMSET    *LocalFormSet;
  FORM_BROWSER_STATEMENT  *Question;
  
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
      gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextHighlightColor));
      for (Index = 0; Index < (GetStringWidth (InputErrorMessage) - 2) / 2; Index++) {
        PrintAt (gScreenDimensions.LeftColumn + gPromptBlockWidth + Index, gScreenDimensions.BottomRow - 1, L"  ");
      }

      mInputError = FALSE;
    }
    break;

  case NV_UPDATE_REQUIRED:
    //
    // Global setting support. Show configuration change on every form.
    //
    if (State) {
      gResetRequired    = (BOOLEAN) (gResetRequired | ((Flags & EFI_IFR_FLAG_RESET_REQUIRED) == EFI_IFR_FLAG_RESET_REQUIRED));

      if (Selection != NULL && Selection->Statement != NULL) {
        Question = Selection->Statement;
        if (Question->Storage != NULL || Question->Operand == EFI_IFR_DATE_OP || Question->Operand == EFI_IFR_TIME_OP) {
          //
          // Update only for Question value that need to be saved into Storage.
          //
          Selection->Form->NvUpdateRequired = TRUE;
        }
      }
      
      if (Selection == NULL || IsNvUpdateRequired (Selection->FormSet)) {
        gST->ConOut->SetAttribute (gST->ConOut, INFO_TEXT);
        PrintStringAt (
          gScreenDimensions.LeftColumn + gPromptBlockWidth + gOptionBlockWidth,
          gScreenDimensions.BottomRow - 1,
          NvUpdateMessage
          );
      }
    } else {
      gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextHighlightColor));
      for (Index = 0; Index < (GetStringWidth (NvUpdateMessage) - 2) / 2; Index++) {
        PrintAt (
          (gScreenDimensions.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + Index),
          gScreenDimensions.BottomRow - 1,
          L"  "
          );
      }
    }
    break;

  case REFRESH_STATUS_BAR:
    if (mInputError) {
      UpdateStatusBar (Selection, INPUT_ERROR, Flags, TRUE);
    }

    switch (gBrowserSettingScope) {
    case SystemLevel:
      //
      // Check the maintain list to see whether there is any change.
      //
      Link = GetFirstNode (&gBrowserFormSetList);
      while (!IsNull (&gBrowserFormSetList, Link)) {
        LocalFormSet = FORM_BROWSER_FORMSET_FROM_LINK (Link);
        if (IsNvUpdateRequired(LocalFormSet)) {
          UpdateStatusBar (NULL, NV_UPDATE_REQUIRED, Flags, TRUE);
          break;
        }
        Link = GetNextNode (&gBrowserFormSetList, Link);
      }
      break;
    case FormSetLevel:
    case FormLevel:
      UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, Flags, TRUE);
    default:
      break;
    }

    break;

  default:
    break;
  }

  FreePool (InputErrorMessage);
  FreePool (NvUpdateMessage);
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
    FreePool (String);
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

  return (UINT16) (Width - LEFT_SKIPPED_COLUMNS);
}

/**
  Will copy LineWidth amount of a string in the OutputString buffer and return the
  number of CHAR16 characters that were copied into the OutputString buffer.
  The output string format is:
    Glyph Info + String info + '\0'.

  In the code, it deals \r,\n,\r\n same as \n\r, also it not process the \r or \g.

  @param  InputString            String description for this option.
  @param  LineWidth              Width of the desired string to extract in CHAR16
                                 characters
  @param  GlyphWidth             The glyph width of the begin of the char in the string.
  @param  Index                  Where in InputString to start the copy process
  @param  OutputString           Buffer to copy the string into

  @return Returns the number of CHAR16 characters that were copied into the OutputString 
  buffer, include extra glyph info and '\0' info.

**/
UINT16
GetLineByWidth (
  IN      CHAR16                      *InputString,
  IN      UINT16                      LineWidth,
  IN OUT  UINT16                      *GlyphWidth,
  IN OUT  UINTN                       *Index,
  OUT     CHAR16                      **OutputString
  )
{
  UINT16          StrOffset;
  UINT16          GlyphOffset;
  UINT16          OriginalGlyphWidth;
  BOOLEAN         ReturnFlag;
  UINT16          LastSpaceOffset;
  UINT16          LastGlyphWidth;

  if (InputString == NULL || Index == NULL || OutputString == NULL) {
    return 0;
  }

  if (LineWidth == 0 || *GlyphWidth == 0) {
    return 0;
  }

  //
  // Save original glyph width.
  //
  OriginalGlyphWidth = *GlyphWidth;
  LastGlyphWidth     = OriginalGlyphWidth;
  ReturnFlag         = FALSE;
  LastSpaceOffset    = 0;

  //
  // NARROW_CHAR can not be printed in screen, so if a line only contain  the two CHARs: 'NARROW_CHAR + CHAR_CARRIAGE_RETURN' , it is a empty line  in Screen.
  // To avoid displaying this  empty line in screen,  just skip  the two CHARs here.
  //
  if ((InputString[*Index] == NARROW_CHAR) && (InputString[*Index + 1] == CHAR_CARRIAGE_RETURN)) {
    *Index = *Index + 2;
  }

  //
  // Fast-forward the string and see if there is a carriage-return in the string
  //
  for (StrOffset = 0, GlyphOffset = 0; GlyphOffset <= LineWidth; StrOffset++) {
    switch (InputString[*Index + StrOffset]) {
      case NARROW_CHAR:
        *GlyphWidth = 1;
        break;

      case WIDE_CHAR:
        *GlyphWidth = 2;
        break;

      case CHAR_CARRIAGE_RETURN:
      case CHAR_LINEFEED:
      case CHAR_NULL:
        ReturnFlag = TRUE;
        break;

      default:
        GlyphOffset = GlyphOffset + *GlyphWidth;

        //
        // Record the last space info in this line. Will be used in rewind.
        //
        if ((InputString[*Index + StrOffset] == CHAR_SPACE) && (GlyphOffset <= LineWidth)) {
          LastSpaceOffset = StrOffset;
          LastGlyphWidth  = *GlyphWidth;
        }
        break;
    }

    if (ReturnFlag) {
      break;
    }
  } 

  //
  // Rewind the string from the maximum size until we see a space to break the line
  //
  if (GlyphOffset > LineWidth) {
    //
    // Rewind the string to last space char in this line.
    //
    if (LastSpaceOffset != 0) {
      StrOffset   = LastSpaceOffset;
      *GlyphWidth = LastGlyphWidth;
    } else {
      //
      // Roll back to last char in the line width.
      //
      StrOffset--;
    }
  }

  //
  // The CHAR_NULL has process last time, this time just return 0 to stand for the end.
  //
  if (StrOffset == 0 && (InputString[*Index + StrOffset] == CHAR_NULL)) {
    return 0;
  }

  //
  // Need extra glyph info and '\0' info, so +2.
  //
  *OutputString = AllocateZeroPool (((UINTN) (StrOffset + 2) * sizeof(CHAR16)));
  if (*OutputString == NULL) {
    return 0;
  }

  //
  // Save the glyph info at the begin of the string, will used by Print function.
  //
  if (OriginalGlyphWidth == 1) {
    *(*OutputString) = NARROW_CHAR;
  } else  {
    *(*OutputString) = WIDE_CHAR;
  }

  CopyMem ((*OutputString) + 1, &InputString[*Index], StrOffset * sizeof(CHAR16));

  if (InputString[*Index + StrOffset] == CHAR_SPACE) {
    //
    // Skip the space info at the begin of next line.
    //  
    *Index = (UINT16) (*Index + StrOffset + 1);
  } else if ((InputString[*Index + StrOffset] == CHAR_LINEFEED)) {
    //
    // Skip the /n or /n/r info.
    //
    if (InputString[*Index + StrOffset + 1] == CHAR_CARRIAGE_RETURN) {
      *Index = (UINT16) (*Index + StrOffset + 2);
    } else {
      *Index = (UINT16) (*Index + StrOffset + 1);
    }
  } else if ((InputString[*Index + StrOffset] == CHAR_CARRIAGE_RETURN)) {
    //
    // Skip the /r or /r/n info.
    //  
    if (InputString[*Index + StrOffset + 1] == CHAR_LINEFEED) {
      *Index = (UINT16) (*Index + StrOffset + 2);
    } else {
      *Index = (UINT16) (*Index + StrOffset + 1);
    }
  } else {
    *Index = (UINT16) (*Index + StrOffset);
  }

  //
  // Include extra glyph info and '\0' info, so +2.
  //
  return StrOffset + 2;
}


/**
  Update display lines for a Menu Option.

  @param  Selection              The user's selection.
  @param  MenuOption             The MenuOption to be checked.

**/
VOID
UpdateOptionSkipLines (
  IN UI_MENU_SELECTION            *Selection,
  IN UI_MENU_OPTION               *MenuOption
  )
{
  UINTN   Index;
  UINT16  Width;
  UINTN   Row;
  UINTN   OriginalRow;
  CHAR16  *OutputString;
  CHAR16  *OptionString;
  UINT16  GlyphWidth;

  Row           = 0;
  OptionString  = NULL;
  ProcessOptions (Selection, MenuOption, FALSE, &OptionString);

  if (OptionString != NULL) {
    Width               = (UINT16) gOptionBlockWidth;

    OriginalRow         = Row;
    GlyphWidth          = 1;

    for (Index = 0; GetLineByWidth (OptionString, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
      //
      // If there is more string to process print on the next row and increment the Skip value
      //
      if (StrLen (&OptionString[Index]) != 0) {
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

      FreePool (OutputString);
    }

    Row = OriginalRow;
  }

  if (OptionString != NULL) {
    FreePool (OptionString);
  }
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

  Temp = Direction ? CurrentPos->BackLink : CurrentPos->ForwardLink;

  if (Temp == &gMenuOption) {
    return TRUE;
  }

  return FALSE;
}


/**
  Move to next selectable statement.

  This is an internal function.

  @param  Selection              Menu selection.
  @param  GoUp                   The navigation direction. TRUE: up, FALSE: down.
  @param  CurrentPosition        Current position.
  @param  GapToTop               Gap position to top or bottom.

  @return The row distance from current MenuOption to next selectable MenuOption.

**/
INTN
MoveToNextStatement (
  IN     UI_MENU_SELECTION         *Selection,
  IN     BOOLEAN                   GoUp,
  IN OUT LIST_ENTRY                **CurrentPosition,
  IN     UINTN                     GapToTop
  )
{
  INTN             Distance;
  LIST_ENTRY       *Pos;
  UI_MENU_OPTION   *NextMenuOption;
  UI_MENU_OPTION   *PreMenuOption;

  Distance      = 0;
  Pos           = *CurrentPosition;
  PreMenuOption = MENU_OPTION_FROM_LINK (Pos);

  while (TRUE) {
    NextMenuOption = MENU_OPTION_FROM_LINK (Pos);
    if (NextMenuOption->Row == 0) {
      UpdateOptionSkipLines (Selection, NextMenuOption);
    }
    
    if (GoUp && (PreMenuOption != NextMenuOption)) {
      //
      // Current Position doesn't need to be caculated when go up.
      // Caculate distanct at first when go up
      //
      if ((UINTN) Distance + NextMenuOption->Skip > GapToTop) {
        NextMenuOption = PreMenuOption;
        break;
      }
      Distance += NextMenuOption->Skip;
    }
    if (IsSelectable (NextMenuOption)) {
      break;
    }
    if ((GoUp ? Pos->BackLink : Pos->ForwardLink) == &gMenuOption) {
      //
      // Arrive at top.
      //
      Distance = -1;
      break;
    }
    if (!GoUp) {
      //
      // Caculate distanct at later when go down
      //
      if ((UINTN) Distance + NextMenuOption->Skip > GapToTop) {
        NextMenuOption = PreMenuOption;
        break;
      }
      Distance += NextMenuOption->Skip;
    }
    PreMenuOption = NextMenuOption;
    Pos = (GoUp ? Pos->BackLink : Pos->ForwardLink);
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
  Find HII Handle in the HII database associated with given Device Path.

  If DevicePath is NULL, then ASSERT.

  @param  DevicePath             Device Path associated with the HII package list
                                 handle.

  @retval Handle                 HII package list Handle associated with the Device
                                        Path.
  @retval NULL                   Hii Package list handle is not found.

**/
EFI_HII_HANDLE
EFIAPI
DevicePathToHiiHandle (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *TmpDevicePath;
  UINTN                       BufferSize;
  UINTN                       HandleCount;
  UINTN                       Index;
  EFI_HANDLE                  Handle;
  EFI_HANDLE                  DriverHandle;
  EFI_HII_HANDLE              *HiiHandles;
  EFI_HII_HANDLE              HiiHandle;

  ASSERT (DevicePath != NULL);

  TmpDevicePath = DevicePath;
  //
  // Locate Device Path Protocol handle buffer
  //
  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TmpDevicePath,
                  &DriverHandle
                  );
  if (EFI_ERROR (Status) || !IsDevicePathEnd (TmpDevicePath)) {
    return NULL;
  }

  //
  // Retrieve all HII Handles from HII database
  //
  BufferSize = 0x1000;
  HiiHandles = AllocatePool (BufferSize);
  ASSERT (HiiHandles != NULL);
  Status = mHiiDatabase->ListPackageLists (
                           mHiiDatabase,
                           EFI_HII_PACKAGE_TYPE_ALL,
                           NULL,
                           &BufferSize,
                           HiiHandles
                           );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FreePool (HiiHandles);
    HiiHandles = AllocatePool (BufferSize);
    ASSERT (HiiHandles != NULL);

    Status = mHiiDatabase->ListPackageLists (
                             mHiiDatabase,
                             EFI_HII_PACKAGE_TYPE_ALL,
                             NULL,
                             &BufferSize,
                             HiiHandles
                             );
  }

  if (EFI_ERROR (Status)) {
    FreePool (HiiHandles);
    return NULL;
  }

  //
  // Search Hii Handle by Driver Handle
  //
  HiiHandle = NULL;
  HandleCount = BufferSize / sizeof (EFI_HII_HANDLE);
  for (Index = 0; Index < HandleCount; Index++) {
    Status = mHiiDatabase->GetPackageListHandle (
                             mHiiDatabase,
                             HiiHandles[Index],
                             &Handle
                             );
    if (!EFI_ERROR (Status) && (Handle == DriverHandle)) {
      HiiHandle = HiiHandles[Index];
      break;
    }
  }

  FreePool (HiiHandles);
  return HiiHandle;
}

/**
  Find HII Handle in the HII database associated with given form set guid.

  If FormSetGuid is NULL, then ASSERT.

  @param  ComparingGuid          FormSet Guid associated with the HII package list
                                 handle.

  @retval Handle                 HII package list Handle associated with the Device
                                        Path.
  @retval NULL                   Hii Package list handle is not found.

**/
EFI_HII_HANDLE
FormSetGuidToHiiHandle (
  EFI_GUID     *ComparingGuid
  )
{
  EFI_HII_HANDLE               *HiiHandles;
  UINTN                        Index;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  EFI_STATUS                   Status;
  EFI_HII_HANDLE               HiiHandle;

  ASSERT (ComparingGuid != NULL);

  HiiHandle  = NULL;
  //
  // Get all the Hii handles
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  ASSERT (HiiHandles != NULL);

  //
  // Search for formset of each class type
  //
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    BufferSize = 0;
    HiiPackageList = NULL;
    Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, HiiHandles[Index], &BufferSize, HiiPackageList);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      HiiPackageList = AllocatePool (BufferSize);
      ASSERT (HiiPackageList != NULL);

      Status = mHiiDatabase->ExportPackageLists (mHiiDatabase, HiiHandles[Index], &BufferSize, HiiPackageList);
    }
    if (EFI_ERROR (Status) || HiiPackageList == NULL) {
      return NULL;
    }

    //
    // Get Form package from this HII package List
    //
    Offset = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
    Offset2 = 0;
    CopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32)); 

    while (Offset < PackageListLength) {
      Package = ((UINT8 *) HiiPackageList) + Offset;
      CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

      if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
        //
        // Search FormSet in this Form Package
        //
        Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
        while (Offset2 < PackageHeader.Length) {
          OpCodeData = Package + Offset2;

          if (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
            //
            // Try to compare against formset GUID
            //
            if (CompareGuid (ComparingGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
              HiiHandle = HiiHandles[Index];
              break;
            }
          }

          Offset2 += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
        }
      }
      if (HiiHandle != NULL) {
        break;
      }
      Offset += PackageHeader.Length;
    }
    
    FreePool (HiiPackageList);
  	if (HiiHandle != NULL) {
  		break;
  	}
  }

  FreePool (HiiHandles);

  return HiiHandle;
}

/**
  Process the goto op code, update the info in the selection structure.

  @param Statement    The statement belong to goto op code.
  @param Selection    The selection info.
  @param Repaint      Whether need to repaint the menu.
  @param NewLine      Whether need to create new line.

  @retval EFI_SUCCESS    The menu process successfully.
  @return Other value if the process failed.
**/
EFI_STATUS
ProcessGotoOpCode (
  IN OUT   FORM_BROWSER_STATEMENT      *Statement,
  IN OUT   UI_MENU_SELECTION           *Selection,
  OUT      BOOLEAN                     *Repaint,
  OUT      BOOLEAN                     *NewLine
  )
{
  CHAR16                          *StringPtr;
  UINTN                           StringLen;
  UINTN                           BufferSize;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
  CHAR16                          TemStr[2];
  UINT8                           *DevicePathBuffer;
  UINTN                           Index;
  UINT8                           DigitUint8;
  FORM_BROWSER_FORM               *RefForm;
  EFI_INPUT_KEY                   Key;
  EFI_STATUS                      Status;
  UI_MENU_LIST                    *MenuList;
  BOOLEAN                         UpdateFormInfo;
  
  Status = EFI_SUCCESS;
  UpdateFormInfo = TRUE;
  StringPtr = NULL;
  StringLen = 0;

  //
  // Prepare the device path check, get the device path info first.
  //
  if (Statement->HiiValue.Value.ref.DevicePath != 0) {
    StringPtr = GetToken (Statement->HiiValue.Value.ref.DevicePath, Selection->FormSet->HiiHandle);
    if (StringPtr != NULL) {
      StringLen = StrLen (StringPtr);
    }
  }

  //
  // Check whether the device path string is a valid string.
  //
  if (Statement->HiiValue.Value.ref.DevicePath != 0 && StringPtr != NULL && StringLen != 0) {
    if (Selection->Form->ModalForm) {
      return Status;
    }
    //
    // Goto another Hii Package list
    //
    Selection->Action = UI_ACTION_REFRESH_FORMSET;
    BufferSize = StrLen (StringPtr) / 2;
    DevicePath = AllocatePool (BufferSize);
    ASSERT (DevicePath != NULL);

    //
    // Convert from Device Path String to DevicePath Buffer in the reverse order.
    //
    DevicePathBuffer = (UINT8 *) DevicePath;
    for (Index = 0; StringPtr[Index] != L'\0'; Index ++) {
      TemStr[0] = StringPtr[Index];
      DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
      if (DigitUint8 == 0 && TemStr[0] != L'0') {
        //
        // Invalid Hex Char as the tail.
        //
        break;
      }
      if ((Index & 1) == 0) {
        DevicePathBuffer [Index/2] = DigitUint8;
      } else {
        DevicePathBuffer [Index/2] = (UINT8) ((DevicePathBuffer [Index/2] << 4) + DigitUint8);
      }
    }
    FreePool (StringPtr);

    Selection->Handle = DevicePathToHiiHandle (DevicePath);
    FreePool (DevicePath);

    if (Selection->Handle == NULL) {
      //
      // If target Hii Handle not found, exit
      //
      Selection->Action = UI_ACTION_EXIT;
      Selection->Statement = NULL;
      return Status;
    }

    CopyMem (&Selection->FormSetGuid,&Statement->HiiValue.Value.ref.FormSetGuid, sizeof (EFI_GUID));
    Selection->FormId = Statement->HiiValue.Value.ref.FormId;
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  } else if (!CompareGuid (&Statement->HiiValue.Value.ref.FormSetGuid, &gZeroGuid)) {
    if (Selection->Form->ModalForm) {
      return Status;
    }  
    //
    // Goto another Formset, check for uncommitted data
    //
    Selection->Action = UI_ACTION_REFRESH_FORMSET;
    
    Selection->Handle = FormSetGuidToHiiHandle(&Statement->HiiValue.Value.ref.FormSetGuid);
    if (Selection->Handle == NULL) {
      //
      // If target Hii Handle not found, exit
      //
      Selection->Action = UI_ACTION_EXIT;
      Selection->Statement = NULL;
      return Status;
    } 

    CopyMem (&Selection->FormSetGuid, &Statement->HiiValue.Value.ref.FormSetGuid, sizeof (EFI_GUID));
    Selection->FormId = Statement->HiiValue.Value.ref.FormId;
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  } else if (Statement->HiiValue.Value.ref.FormId != 0) {
    //
    // Check whether target From is suppressed.
    //
    RefForm = IdToForm (Selection->FormSet, Statement->HiiValue.Value.ref.FormId);

    if ((RefForm != NULL) && (RefForm->SuppressExpression != NULL)) {
      if (EvaluateExpressionList(RefForm->SuppressExpression, TRUE, Selection->FormSet, RefForm) != ExpressFalse) {
        //
        // Form is suppressed. 
        //
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gFormSuppress, gPressEnter, gEmptyString);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
        if (Repaint != NULL) {
          *Repaint = TRUE;
        }
        return Status;
      }
    }

    //
    // Goto another form inside this formset,
    //
    Selection->Action = UI_ACTION_REFRESH_FORM;

    Selection->FormId = Statement->HiiValue.Value.ref.FormId;
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;
  } else if (Statement->HiiValue.Value.ref.QuestionId != 0) {
    //
    // Goto another Question
    //
    Selection->QuestionId = Statement->HiiValue.Value.ref.QuestionId;

    if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
      Selection->Action = UI_ACTION_REFRESH_FORM;
    } else {
      if (Repaint != NULL) {
        *Repaint = TRUE;
      }
      if (NewLine != NULL) {
        *NewLine = TRUE;
      }
    }
    UpdateFormInfo = FALSE;
  } else {
    if ((Statement->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
      Selection->Action = UI_ACTION_REFRESH_FORM;
    }
    UpdateFormInfo = FALSE;
  }

  if (UpdateFormInfo) {
    //
    // Link current form so that we can always go back when someone hits the ESC
    //
    MenuList = UiFindMenuList (&Selection->FormSetGuid, Selection->FormId);
    if (MenuList == NULL && Selection->CurrentMenu != NULL) {
      MenuList = UiAddMenuList (Selection->CurrentMenu, Selection->Handle, &Selection->FormSetGuid, Selection->FormId);
    }
  }

  return Status;
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
  UINT16                          Width;
  CHAR16                          *StringPtr;
  CHAR16                          *OptionString;
  CHAR16                          *OutputString;
  CHAR16                          *HelpString;
  CHAR16                          *HelpHeaderString;
  CHAR16                          *HelpBottomString;
  BOOLEAN                         NewLine;
  BOOLEAN                         Repaint;
  BOOLEAN                         SavedValue;
  BOOLEAN                         UpArrow;
  BOOLEAN                         DownArrow;
  BOOLEAN                         InitializedFlag;
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
  MENU_REFRESH_ENTRY              *MenuUpdateEntry;  
  UI_SCREEN_OPERATION             ScreenOperation;
  UINT8                           MinRefreshInterval;
  UINT16                          DefaultId;
  FORM_BROWSER_STATEMENT          *Statement;
  UI_MENU_LIST                    *CurrentMenu;
  UINTN                           ModalSkipColumn;
  BROWSER_HOT_KEY                 *HotKey;
  UINTN                           HelpPageIndex;
  UINTN                           HelpPageCount;
  UINTN                           RowCount;
  UINTN                           HelpLine;
  UINTN                           HelpHeaderLine;
  UINTN                           HelpBottomLine;
  BOOLEAN                         MultiHelpPage;
  UINT16                          GlyphWidth;
  UINT16                          EachLineWidth;
  UINT16                          HeaderLineWidth;
  UINT16                          BottomLineWidth;

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  Status              = EFI_SUCCESS;
  HelpString          = NULL;
  HelpHeaderString    = NULL;
  HelpBottomString    = NULL;
  OptionString        = NULL;
  ScreenOperation     = UiNoOperation;
  NewLine             = TRUE;
  MinRefreshInterval  = 0;
  DefaultId           = 0;
  HelpPageCount       = 0;
  HelpLine            = 0;
  RowCount            = 0;
  HelpBottomLine      = 0;
  HelpHeaderLine      = 0;
  HelpPageIndex       = 0;
  MultiHelpPage       = FALSE;
  EachLineWidth       = 0;
  HeaderLineWidth     = 0;
  BottomLineWidth     = 0;
  OutputString        = NULL;
  UpArrow             = FALSE;
  DownArrow           = FALSE;
  SkipValue           = 0;
  OldSkipValue        = 0;
  MenuRefreshEntry    = gMenuRefreshHead;

  NextMenuOption      = NULL;
  PreviousMenuOption  = NULL;
  SavedMenuOption     = NULL;
  HotKey              = NULL;
  ModalSkipColumn     = (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 6;

  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  if ((gClassOfVfr & FORMSET_CLASS_FRONT_PAGE) == FORMSET_CLASS_FRONT_PAGE){
    TopRow  = LocalScreen.TopRow + FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
    Row     = LocalScreen.TopRow + FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
  } else {
    TopRow  = LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
    Row     = LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT + SCROLL_ARROW_HEIGHT;
  }

  if (Selection->Form->ModalForm) {
    Col = LocalScreen.LeftColumn + LEFT_SKIPPED_COLUMNS + ModalSkipColumn;
  } else {
    Col = LocalScreen.LeftColumn + LEFT_SKIPPED_COLUMNS;
  }

  BottomRow = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - gFooterHeight - SCROLL_ARROW_HEIGHT - 1;

  Selection->TopRow = TopRow;
  Selection->BottomRow = BottomRow;
  Selection->PromptCol = Col;
  Selection->OptionCol = gPromptBlockWidth + 1 + LocalScreen.LeftColumn;
  Selection->Statement = NULL;

  TopOfScreen = gMenuOption.ForwardLink;
  Repaint     = TRUE;
  MenuOption  = NULL;

  //
  // Find current Menu
  //
  CurrentMenu = UiFindMenuList (&Selection->FormSetGuid, Selection->FormId);
  if (CurrentMenu == NULL) {
    //
    // Current menu not found, add it to the menu tree
    //
    CurrentMenu = UiAddMenuList (NULL, Selection->Handle, &Selection->FormSetGuid, Selection->FormId);
  }
  ASSERT (CurrentMenu != NULL);
  Selection->CurrentMenu = CurrentMenu;

  if (Selection->QuestionId == 0) {
    //
    // Highlight not specified, fetch it from cached menu
    //
    Selection->QuestionId = CurrentMenu->QuestionId;
    Selection->Sequence   = CurrentMenu->Sequence;
  }

  //
  // Init option as the current user's selection
  //
  InitializedFlag = TRUE;
  NewPos = gMenuOption.ForwardLink;

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);
  UpdateStatusBar (Selection, REFRESH_STATUS_BAR, (UINT8) 0, TRUE);

  ControlFlag = CfInitialization;
  Selection->Action = UI_ACTION_NONE;
  while (TRUE) {
    switch (ControlFlag) {
    case CfInitialization:
      if (IsListEmpty (&gMenuOption)) {
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
        DownArrow       = FALSE;
        UpArrow         = FALSE;
        Row             = TopRow;

        Temp            = (UINTN) SkipValue;
        Temp2           = (UINTN) SkipValue;

        if (Selection->Form->ModalForm) {
          ClearLines (
            LocalScreen.LeftColumn + ModalSkipColumn,
            LocalScreen.LeftColumn + ModalSkipColumn + gPromptBlockWidth + gOptionBlockWidth,
            TopRow - SCROLL_ARROW_HEIGHT,
            BottomRow + SCROLL_ARROW_HEIGHT,
            PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND
            );   
        } else {
          ClearLines (
            LocalScreen.LeftColumn,
            LocalScreen.RightColumn,
            TopRow - SCROLL_ARROW_HEIGHT,
            BottomRow + SCROLL_ARROW_HEIGHT,
            PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND
            );
        }
        UiFreeRefreshList ();
        MinRefreshInterval = 0;

        for (Link = TopOfScreen; Link != &gMenuOption; Link = Link->ForwardLink) {
          MenuOption          = MENU_OPTION_FROM_LINK (Link);
          MenuOption->Row     = Row;
          MenuOption->Col     = Col;
          if (Selection->Form->ModalForm) {
            MenuOption->OptCol  = gPromptBlockWidth + 1 + LocalScreen.LeftColumn + ModalSkipColumn;
          } else {
            MenuOption->OptCol  = gPromptBlockWidth + 1 + LocalScreen.LeftColumn;
          }

          Statement = MenuOption->ThisTag;
          if (Statement->InSubtitle) {
            MenuOption->Col += SUBTITLE_INDENT;
          }

          if (MenuOption->GrayOut) {
            gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_GRAYED | FIELD_BACKGROUND);
          } else {
            if (Statement->Operand == EFI_IFR_SUBTITLE_OP) {
              gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserSubtitleTextColor) | FIELD_BACKGROUND);
            }
          }

          Width       = GetWidth (Statement, MenuOption->Handle);
          OriginalRow = Row;
          GlyphWidth  = 1;

          if (Statement->Operand == EFI_IFR_REF_OP && MenuOption->Col >= 2) {
            //
            // Print Arrow for Goto button.
            //
            PrintAt (
              MenuOption->Col - 2,
              Row,
              L"%c",
              GEOMETRICSHAPE_RIGHT_TRIANGLE
              );
          }

          for (Index = 0; GetLineByWidth (MenuOption->Description, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
            if ((Temp == 0) && (Row <= BottomRow)) {
              PrintStringAt (MenuOption->Col, Row, OutputString);
            }
            //
            // If there is more string to process print on the next row and increment the Skip value
            //
            if (StrLen (&MenuOption->Description[Index]) != 0) {
              if (Temp == 0) {
                Row++;
              }
            }

            FreePool (OutputString);
            if (Temp != 0) {
              Temp--;
            }
          }

          Temp  = 0;
          Row   = OriginalRow;

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
              ProcessStringForDateTime(MenuOption, OptionString, TRUE);
            }

            Width       = (UINT16) gOptionBlockWidth;
            OriginalRow = Row;
            GlyphWidth  = 1;

            for (Index = 0; GetLineByWidth (OptionString, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
              if ((Temp2 == 0) && (Row <= BottomRow)) {
                PrintStringAt (MenuOption->OptCol, Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&OptionString[Index]) != 0) {
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

              FreePool (OutputString);
              if (Temp2 != 0) {
                Temp2--;
              }
            }

            Temp2 = 0;
            Row   = OriginalRow;

            FreePool (OptionString);
          }

          //
          // If Question has refresh guid, register the op-code.
          //
          if (!CompareGuid (&Statement->RefreshGuid, &gZeroGuid)) {
            if (gMenuEventGuidRefreshHead == NULL) {
              MenuUpdateEntry = AllocateZeroPool (sizeof (MENU_REFRESH_ENTRY));
              gMenuEventGuidRefreshHead = MenuUpdateEntry;
            } else {
              MenuUpdateEntry = gMenuEventGuidRefreshHead;
              while (MenuUpdateEntry->Next != NULL) {
                MenuUpdateEntry = MenuUpdateEntry->Next; 
              }
              MenuUpdateEntry->Next = AllocateZeroPool (sizeof (MENU_REFRESH_ENTRY));
              MenuUpdateEntry = MenuUpdateEntry->Next; 
            }
            ASSERT (MenuUpdateEntry != NULL);
            Status = gBS->CreateEventEx (EVT_NOTIFY_SIGNAL, TPL_NOTIFY, RefreshQuestionNotify, MenuUpdateEntry, &Statement->RefreshGuid, &MenuUpdateEntry->Event);
            ASSERT (!EFI_ERROR (Status));
            MenuUpdateEntry->MenuOption        = MenuOption;
            MenuUpdateEntry->Selection         = Selection;
            MenuUpdateEntry->CurrentColumn     = MenuOption->OptCol;
            MenuUpdateEntry->CurrentRow        = MenuOption->Row;
            if (MenuOption->GrayOut) {
              MenuUpdateEntry->CurrentAttribute = FIELD_TEXT_GRAYED | FIELD_BACKGROUND;
            } else {
              MenuUpdateEntry->CurrentAttribute = PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND;
            }
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
              gMenuRefreshHead = MenuRefreshEntry;
            } else {
              MenuRefreshEntry = gMenuRefreshHead;
              while (MenuRefreshEntry->Next != NULL) {
                MenuRefreshEntry = MenuRefreshEntry->Next; 
              }
              MenuRefreshEntry->Next = AllocateZeroPool (sizeof (MENU_REFRESH_ENTRY));
              MenuRefreshEntry = MenuRefreshEntry->Next;
            }
            ASSERT (MenuRefreshEntry != NULL);            
            MenuRefreshEntry->MenuOption        = MenuOption;
            MenuRefreshEntry->Selection         = Selection;
            MenuRefreshEntry->CurrentColumn     = MenuOption->OptCol;
            MenuRefreshEntry->CurrentRow        = MenuOption->Row;
            if (MenuOption->GrayOut) {
              MenuRefreshEntry->CurrentAttribute = FIELD_TEXT_GRAYED | FIELD_BACKGROUND;
            } else {               
              MenuRefreshEntry->CurrentAttribute = PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND;
            }
          }
          
          //
          // If this is a text op with secondary text information
          //
          if ((Statement->Operand == EFI_IFR_TEXT_OP) && (Statement->TextTwo != 0)) {
            StringPtr   = GetToken (Statement->TextTwo, MenuOption->Handle);

            Width       = (UINT16) gOptionBlockWidth;
            OriginalRow = Row;
            GlyphWidth = 1;

            for (Index = 0; GetLineByWidth (StringPtr, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
              if ((Temp == 0) && (Row <= BottomRow)) {
                PrintStringAt (MenuOption->OptCol, Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&StringPtr[Index]) != 0) {
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

              FreePool (OutputString);
              if (Temp2 != 0) {
                Temp2--;
              }
            }

            Row = OriginalRow;
            FreePool (StringPtr);
          }
          gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);

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
              DownArrow = TRUE;
            }

            Row = BottomRow + 1;
            break;
          }
        }

        if (!ValueIsScroll (TRUE, TopOfScreen)) {
          UpArrow = TRUE;
        }

        if (UpArrow) {
          gST->ConOut->SetAttribute (gST->ConOut, ARROW_TEXT | ARROW_BACKGROUND);
          PrintAt (
            LocalScreen.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + 1,
            TopRow - SCROLL_ARROW_HEIGHT,
            L"%c",
            ARROW_UP
            );
          gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);
        }

        if (DownArrow) {
          gST->ConOut->SetAttribute (gST->ConOut, ARROW_TEXT | ARROW_BACKGROUND);
          PrintAt (
            LocalScreen.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + 1,
            BottomRow + SCROLL_ARROW_HEIGHT,
            L"%c",
            ARROW_DOWN
            );
          gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);
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
      if (InitializedFlag) {
        InitializedFlag = FALSE;
        MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - TopRow);
      }

      //
      // Repaint flag is normally reset when finish processing CfUpdateHelpString. Temporarily
      // reset Repaint flag because we may break halfway and skip CfUpdateHelpString processing.
      //
      SavedValue  = Repaint;
      Repaint     = FALSE;

      if (Selection->QuestionId != 0) {
        NewPos = gMenuOption.ForwardLink;
        SavedMenuOption = MENU_OPTION_FROM_LINK (NewPos);

        while ((SavedMenuOption->ThisTag->QuestionId != Selection->QuestionId ||
                SavedMenuOption->Sequence != Selection->Sequence) &&
                NewPos->ForwardLink != &gMenuOption) {
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
            if (Link == TopOfScreen) {
              Index -= OldSkipValue;
            }
            Link = Link->ForwardLink;
          }
          if (NewPos == Link) {
            SavedMenuOption = MENU_OPTION_FROM_LINK (Link);
          }

          //
          // Not find the selected menu in current show page.
          // Have two case to enter this if:
          // 1. Not find the menu at current page.
          // 2. Find the menu in current page, but the menu shows at the bottom and not all info shows.
          //    For case 2, has an exception: The menu can show more than one pages and now only this menu shows.
          //
          // Base on the selected menu will show at the bottom of the page,
          // select the menu which will show at the top of the page.
          //
          if (Link != NewPos || Index > BottomRow || 
              (Link == NewPos && (SavedMenuOption->Row + SavedMenuOption->Skip - 1 > BottomRow) && (Link != TopOfScreen))) {
            //
            // Find the MenuOption which has the skip value for Date/Time opcode. 
            //
            AdjustDateAndTimePosition(FALSE, &NewPos);
            //
            // NewPos is not in the current page, simply scroll page so that NewPos is in the end of the page
            //
            SavedMenuOption = MENU_OPTION_FROM_LINK (NewPos);
            //
            // SavedMenuOption->Row == 0 means the menu not show yet.
            //
            if (SavedMenuOption->Row == 0) {
              UpdateOptionSkipLines (Selection, SavedMenuOption);
            }

            //
            // Base on the selected menu will show at the bottome of next page, 
            // select the menu show at the top of the next page. 
            //
            Link    = NewPos;
            for (Index = TopRow + SavedMenuOption->Skip; Index <= BottomRow + 1; ) {            
              Link = Link->BackLink;
              SavedMenuOption = MENU_OPTION_FROM_LINK (Link);
              if (SavedMenuOption->Row == 0) {
                UpdateOptionSkipLines (Selection, SavedMenuOption);
              }
              Index += SavedMenuOption->Skip;
            }

            //
            // Found the menu which will show at the top of the page.
            //
            if (Link == NewPos) {
              //
              // The menu can show more than one pages, just show the menu at the top of the page.
              //
              SkipValue    = 0;
              TopOfScreen  = Link;
              OldSkipValue = SkipValue;
            } else {
              //
              // Check whether need to skip some line for menu shows at the top of the page.
              //
              SkipValue = Index - BottomRow - 1;
              if (SkipValue > 0 && SkipValue < (INTN) SavedMenuOption->Skip) {
                TopOfScreen     = Link;
                OldSkipValue    = SkipValue;
              } else {
                SkipValue       = 0;
                TopOfScreen     = Link->ForwardLink;
              }
            }

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
          gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);
          if (OptionString != NULL) {
            if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) ||
                (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)
               ) {
              ProcessStringForDateTime(MenuOption, OptionString, FALSE);
            }

            Width               = (UINT16) gOptionBlockWidth;
            OriginalRow         = MenuOption->Row;
            GlyphWidth          = 1;

            for (Index = 0; GetLineByWidth (OptionString, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
              if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
                PrintStringAt (MenuOption->OptCol, MenuOption->Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&OptionString[Index]) != 0) {
                MenuOption->Row++;
              }

              FreePool (OutputString);
            }

            MenuOption->Row = OriginalRow;

            FreePool (OptionString);
          } else {
            if (NewLine) {
              if (MenuOption->GrayOut) {
                gST->ConOut->SetAttribute (gST->ConOut, FIELD_TEXT_GRAYED | FIELD_BACKGROUND);
              } else if (MenuOption->ThisTag->Operand == EFI_IFR_SUBTITLE_OP) {
                gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserSubtitleTextColor) | FIELD_BACKGROUND);
              }

              OriginalRow = MenuOption->Row;
              Width       = GetWidth (MenuOption->ThisTag, MenuOption->Handle);
              GlyphWidth  = 1;

              for (Index = 0; GetLineByWidth (MenuOption->Description, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
                if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
                  PrintStringAt (MenuOption->Col, MenuOption->Row, OutputString);
                }
                //
                // If there is more string to process print on the next row and increment the Skip value
                //
                if (StrLen (&MenuOption->Description[Index]) != 0) {
                  MenuOption->Row++;
                }

                FreePool (OutputString);
              }

              MenuOption->Row = OriginalRow;
              gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);
            }
          }
        }

        //
        // This is the current selected statement
        //
        MenuOption = MENU_OPTION_FROM_LINK (NewPos);
        Statement = MenuOption->ThisTag;
        Selection->Statement = Statement;
        if (!IsSelectable (MenuOption)) {
          Repaint = SavedValue;
          UpdateKeyHelp (Selection, MenuOption, FALSE);
          break;
        }

        //
        // Record highlight for current menu
        //
        CurrentMenu->QuestionId = Statement->QuestionId;
        CurrentMenu->Sequence   = MenuOption->Sequence;

        //
        // Set reverse attribute
        //
        gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextHighlightColor) | PcdGet8 (PcdBrowserFieldBackgroundHighlightColor));
        gST->ConOut->SetCursorPosition (gST->ConOut, MenuOption->Col, MenuOption->Row);

        //
        // Assuming that we have a refresh linked-list created, lets annotate the
        // appropriate entry that we are highlighting with its new attribute.  Just prior to this
        // lets reset all of the entries' attribute so we do not get multiple highlights in he refresh
        //
        if (gMenuRefreshHead != NULL) {
          for (MenuRefreshEntry = gMenuRefreshHead; MenuRefreshEntry != NULL; MenuRefreshEntry = MenuRefreshEntry->Next) {
            if (MenuRefreshEntry->MenuOption->GrayOut) {
              MenuRefreshEntry->CurrentAttribute = FIELD_TEXT_GRAYED | FIELD_BACKGROUND;
            } else {               
              MenuRefreshEntry->CurrentAttribute = PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND;
            }
            if (MenuRefreshEntry->MenuOption == MenuOption) {
              MenuRefreshEntry->CurrentAttribute = PcdGet8 (PcdBrowserFieldTextHighlightColor) | PcdGet8 (PcdBrowserFieldBackgroundHighlightColor);
            }
          }
        }

        ProcessOptions (Selection, MenuOption, FALSE, &OptionString);
        if (OptionString != NULL) {
          if (Statement->Operand == EFI_IFR_DATE_OP || Statement->Operand == EFI_IFR_TIME_OP) {
            ProcessStringForDateTime(MenuOption, OptionString, FALSE);
          }
          Width               = (UINT16) gOptionBlockWidth;

          OriginalRow         = MenuOption->Row;
          GlyphWidth          = 1;

          for (Index = 0; GetLineByWidth (OptionString, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
            if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
              PrintStringAt (MenuOption->OptCol, MenuOption->Row, OutputString);
            }
            //
            // If there is more string to process print on the next row and increment the Skip value
            //
            if (StrLen (&OptionString[Index]) != 0) {
              MenuOption->Row++;
            }

            FreePool (OutputString);
          }

          MenuOption->Row = OriginalRow;

          FreePool (OptionString);
        } else {
          if (NewLine) {
            OriginalRow = MenuOption->Row;

            Width       = GetWidth (Statement, MenuOption->Handle);
            GlyphWidth          = 1;

            for (Index = 0; GetLineByWidth (MenuOption->Description, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
              if (MenuOption->Row >= TopRow && MenuOption->Row <= BottomRow) {
                PrintStringAt (MenuOption->Col, MenuOption->Row, OutputString);
              }
              //
              // If there is more string to process print on the next row and increment the Skip value
              //
              if (StrLen (&MenuOption->Description[Index]) != 0) {
                MenuOption->Row++;
              }

              FreePool (OutputString);
            }

            MenuOption->Row = OriginalRow;

          }
        }

        UpdateKeyHelp (Selection, MenuOption, FALSE);

        //
        // Clear reverse attribute
        //
        gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);
      }
      //
      // Repaint flag will be used when process CfUpdateHelpString, so restore its value
      // if we didn't break halfway when process CfRefreshHighLight.
      //
      Repaint = SavedValue;
      break;

    case CfUpdateHelpString:
      ControlFlag = CfPrepareToReadKey;
      if (Selection->Form->ModalForm) {
        break;
      }

      if (Repaint || NewLine) {
        //
        // Don't print anything if it is a NULL help token
        //
        ASSERT(MenuOption != NULL);
        if (MenuOption->ThisTag->Help == 0 || !IsSelectable (MenuOption)) {
          StringPtr = L"\0";
        } else {
          StringPtr = GetToken (MenuOption->ThisTag->Help, MenuOption->Handle);
        }

        RowCount      = BottomRow - TopRow;
        HelpPageIndex = 0;
        //
        // 1.Calculate how many line the help string need to print.
        //
        if (HelpString != NULL) {
          FreePool (HelpString);
        }
        HelpLine = ProcessHelpString (StringPtr, &HelpString, &EachLineWidth, RowCount);
        if (HelpLine > RowCount) {
          MultiHelpPage   = TRUE;
          StringPtr       = GetToken (STRING_TOKEN(ADJUST_HELP_PAGE_UP), gHiiHandle);
          if (HelpHeaderString != NULL) {
            FreePool (HelpHeaderString);
          }
          HelpHeaderLine  = ProcessHelpString (StringPtr, &HelpHeaderString, &HeaderLineWidth, RowCount);
          StringPtr       = GetToken (STRING_TOKEN(ADJUST_HELP_PAGE_DOWN), gHiiHandle);
          if (HelpBottomString != NULL) {
            FreePool (HelpBottomString);
          }
          HelpBottomLine  = ProcessHelpString (StringPtr, &HelpBottomString, &BottomLineWidth, RowCount);
          //
          // Calculate the help page count.
          //
          if (HelpLine > 2 * RowCount - 2) {
            HelpPageCount = (HelpLine - RowCount + 1) / (RowCount - 2) + 1;
            if ((HelpLine - RowCount + 1) % (RowCount - 2) > 1) {
              HelpPageCount += 1;
            }
          } else {
            HelpPageCount = 2;
          }
        } else {
          MultiHelpPage = FALSE;
        }
      }

      //
      // Clean the help field first.
      //
      ClearLines (
        LocalScreen.RightColumn - gHelpBlockWidth,
        LocalScreen.RightColumn,
        TopRow,
        BottomRow,
        PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND
        );

      //
      // Check whether need to show the 'More(U/u)' at the begin.
      // Base on current direct info, here shows aligned to the right side of the column.
      // If the direction is multi line and aligned to right side may have problem, so 
      // add ASSERT code here.
      //
      if (HelpPageIndex > 0) {
        gST->ConOut->SetAttribute (gST->ConOut, INFO_TEXT | FIELD_BACKGROUND);
        for (Index = 0; Index < HelpHeaderLine; Index++) {
          ASSERT (HelpHeaderLine == 1);
          ASSERT (GetStringWidth (HelpHeaderString) / 2 < (UINTN) (gHelpBlockWidth - 1));
          PrintStringAt (
            LocalScreen.RightColumn - GetStringWidth (HelpHeaderString) / 2 - 1,
            Index + TopRow,
            &HelpHeaderString[Index * HeaderLineWidth]
            );
        }
      }

      gST->ConOut->SetAttribute (gST->ConOut, HELP_TEXT | FIELD_BACKGROUND);
      //
      // Print the help string info.
      //
      if (!MultiHelpPage) {
        for (Index = 0; Index < HelpLine; Index++) {
          PrintStringAt (
            LocalScreen.RightColumn - gHelpBlockWidth,
            Index + TopRow,
            &HelpString[Index * EachLineWidth]
            );
        }
        gST->ConOut->SetCursorPosition(gST->ConOut, LocalScreen.RightColumn-1, BottomRow);
      } else  {
        if (HelpPageIndex == 0) {
          for (Index = 0; Index < RowCount - HelpBottomLine; Index++) {
            PrintStringAt (
              LocalScreen.RightColumn - gHelpBlockWidth,
              Index + TopRow,
              &HelpString[Index * EachLineWidth]
              );
          }
        } else {
          for (Index = 0; (Index < RowCount - HelpBottomLine - HelpHeaderLine) && 
              (Index + HelpPageIndex * (RowCount - 2) + 1 < HelpLine); Index++) {
            PrintStringAt (
              LocalScreen.RightColumn - gHelpBlockWidth,
              Index + TopRow + HelpHeaderLine,
              &HelpString[(Index + HelpPageIndex * (RowCount - 2) + 1)* EachLineWidth]
              );
          }
          if (HelpPageIndex == HelpPageCount - 1) {
            gST->ConOut->SetCursorPosition(gST->ConOut, LocalScreen.RightColumn-1, BottomRow);
          }
        } 
      }

      //
      // Check whether need to print the 'More(D/d)' at the bottom.
      // Base on current direct info, here shows aligned to the right side of the column.
      // If the direction is multi line and aligned to right side may have problem, so 
      // add ASSERT code here.
      //
      if (HelpPageIndex < HelpPageCount - 1 && MultiHelpPage) {
        gST->ConOut->SetAttribute (gST->ConOut, INFO_TEXT | FIELD_BACKGROUND);
        for (Index = 0; Index < HelpBottomLine; Index++) {
          ASSERT (HelpBottomLine == 1);
          ASSERT (GetStringWidth (HelpBottomString) / 2 < (UINTN) (gHelpBlockWidth - 1)); 
          PrintStringAt (
            LocalScreen.RightColumn - GetStringWidth (HelpBottomString) / 2 - 1,
            Index + BottomRow - HelpBottomLine,
            &HelpBottomString[Index * BottomLineWidth]
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
        ControlFlag = CfCheckSelection;
        Selection->Statement = NULL;
        break;
      }

      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
      //
      // If we encounter error, continue to read another key in.
      //
      if (EFI_ERROR (Status)) {
        ControlFlag = CfReadKey;
        break;
      }

      switch (Key.UnicodeChar) {
      case CHAR_CARRIAGE_RETURN:
        if(MenuOption->GrayOut || MenuOption->ReadOnly) {
          ControlFlag = CfReadKey;
          break;
        }

        ScreenOperation = UiSelect;
        gDirection      = 0;
        break;

      //
      // We will push the adjustment of these numeric values directly to the input handler
      //  NOTE: we won't handle manual input numeric
      //
      case '+':
      case '-':
        //
        // If the screen has no menu items, and the user didn't select UiReset
        // ignore the selection and go back to reading keys.
        //
        if(IsListEmpty (&gMenuOption) || MenuOption->GrayOut || MenuOption->ReadOnly) {
          ControlFlag = CfReadKey;
          break;
        }

        ASSERT(MenuOption != NULL);
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
          } else {
            Selection->Action = UI_ACTION_REFRESH_FORM;
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
        if ((gClassOfVfr & FORMSET_CLASS_FRONT_PAGE) != FORMSET_CLASS_FRONT_PAGE) {
          //
          // If the screen has no menu items, and the user didn't select UiReset
          // ignore the selection and go back to reading keys.
          //
          if(IsListEmpty (&gMenuOption)) {
            ControlFlag = CfReadKey;
            break;
          }
          
          ASSERT(MenuOption != NULL);
          if (MenuOption->ThisTag->Operand == EFI_IFR_CHECKBOX_OP && !MenuOption->GrayOut && !MenuOption->ReadOnly) {
            ScreenOperation = UiSelect;
          }
        }
        break;

      case 'D':
      case 'd':
        if (!MultiHelpPage) {
          ControlFlag = CfReadKey;
          break;
        }
        ControlFlag    = CfUpdateHelpString;
        HelpPageIndex  = HelpPageIndex < HelpPageCount - 1 ? HelpPageIndex + 1 : HelpPageCount - 1;
        break;

      case 'U':
      case 'u':
        if (!MultiHelpPage) {
          ControlFlag = CfReadKey;
          break;
        }
        ControlFlag    = CfUpdateHelpString;
        HelpPageIndex  = HelpPageIndex > 0 ? HelpPageIndex - 1 : 0;
        break;

      case CHAR_NULL:
        for (Index = 0; Index < mScanCodeNumber; Index++) {
          if (Key.ScanCode == gScanCodeToOperation[Index].ScanCode) {
            ScreenOperation = gScanCodeToOperation[Index].ScreenOperation;
            break;
          }
        }
        
        if (Selection->Form->ModalForm && (Key.ScanCode == SCAN_ESC || Index == mScanCodeNumber)) {
          //
          // ModalForm has no ESC key and Hot Key.
          //
          ControlFlag = CfReadKey;
        } else if (Index == mScanCodeNumber) {
          //
          // Check whether Key matches the registered hot key.
          //
          HotKey = NULL;
          if ((gBrowserSettingScope == SystemLevel) || (gFunctionKeySetting != NONE_FUNCTION_KEY_SETTING)) {
            HotKey = GetHotKeyFromRegisterList (&Key);
          }
          if (HotKey != NULL) {
            ScreenOperation = UiHotKey;
          }
        }
        break;
      }
      break;

    case CfScreenOperation:
      if (ScreenOperation != UiReset) {
        //
        // If the screen has no menu items, and the user didn't select UiReset
        // ignore the selection and go back to reading keys.
        //
        if (IsListEmpty (&gMenuOption)) {
          ControlFlag = CfReadKey;
          break;
        }
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

    case CfUiSelect:
      ControlFlag = CfCheckSelection;

      ASSERT(MenuOption != NULL);
      Statement = MenuOption->ThisTag;
      if (Statement->Operand == EFI_IFR_TEXT_OP) {
        break;
      }

      //
      // Keep highlight on current MenuOption
      //
      Selection->QuestionId = Statement->QuestionId;

      switch (Statement->Operand) {
      case EFI_IFR_REF_OP:
        ProcessGotoOpCode(Statement, Selection, &Repaint, &NewLine);
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
        UpdateKeyHelp (Selection, MenuOption, TRUE);
        Status = ProcessOptions (Selection, MenuOption, TRUE, &OptionString);

        if (EFI_ERROR (Status)) {
          Repaint = TRUE;
          NewLine = TRUE;
          UpdateKeyHelp (Selection, MenuOption, FALSE);
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
      // We come here when someone press ESC
      //
      ControlFlag = CfCheckSelection;
      FindNextMenu (Selection, &Repaint, &NewLine);
      break;

    case CfUiLeft:
      ControlFlag = CfCheckSelection;
      ASSERT(MenuOption != NULL);
      if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) || (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)) {
        if (MenuOption->Sequence != 0) {
          //
          // In the middle or tail of the Date/Time op-code set, go left.
          //
          ASSERT(NewPos != NULL);
          NewPos = NewPos->BackLink;
        }
      }
      break;

    case CfUiRight:
      ControlFlag = CfCheckSelection;
      ASSERT(MenuOption != NULL);
      if ((MenuOption->ThisTag->Operand == EFI_IFR_DATE_OP) || (MenuOption->ThisTag->Operand == EFI_IFR_TIME_OP)) {
        if (MenuOption->Sequence != 2) {
          //
          // In the middle or tail of the Date/Time op-code set, go left.
          //
          ASSERT(NewPos != NULL);
          NewPos = NewPos->ForwardLink;
        }
      }
      break;

    case CfUiUp:
      ControlFlag = CfCheckSelection;

      SavedListEntry = NewPos;

      ASSERT(NewPos != NULL);
      //
      // Adjust Date/Time position before we advance forward.
      //
      AdjustDateAndTimePosition (TRUE, &NewPos);
      if (NewPos->BackLink != &gMenuOption) {
        MenuOption = MENU_OPTION_FROM_LINK (NewPos);
        ASSERT (MenuOption != NULL);
        NewLine    = TRUE;
        NewPos     = NewPos->BackLink;

        PreviousMenuOption = MENU_OPTION_FROM_LINK (NewPos);
        if (PreviousMenuOption->Row == 0) {
          UpdateOptionSkipLines (Selection, PreviousMenuOption);
        }
        DistanceValue = PreviousMenuOption->Skip;
        Difference    = 0;
        if (MenuOption->Row >= DistanceValue + TopRow) {
          Difference = MoveToNextStatement (Selection, TRUE, &NewPos, MenuOption->Row - TopRow - DistanceValue);
        }
        NextMenuOption = MENU_OPTION_FROM_LINK (NewPos);
       
        if (Difference < 0) {
          //
          // We hit the begining MenuOption that can be focused
          // so we simply scroll to the top.
          //
          if (TopOfScreen != gMenuOption.ForwardLink) {
            TopOfScreen = gMenuOption.ForwardLink;
            Repaint     = TRUE;
          } else {
            //
            // Scroll up to the last page when we have arrived at top page.
            //
            NewPos          = &gMenuOption;
            TopOfScreen     = &gMenuOption;
            MenuOption      = MENU_OPTION_FROM_LINK (SavedListEntry);
            ScreenOperation = UiPageUp;
            ControlFlag     = CfScreenOperation;
            break;
          }
        } else if (MenuOption->Row < TopRow + DistanceValue + Difference) {
          //
          // Previous focus MenuOption is above the TopOfScreen, so we need to scroll
          //
          TopOfScreen = NewPos;
          Repaint     = TRUE;
          SkipValue = 0;
          OldSkipValue = 0;
        } else if (!IsSelectable (NextMenuOption)) {
          //
          // Continue to go up until scroll to next page or the selectable option is found.
          //
          ScreenOperation = UiUp;
          ControlFlag     = CfScreenOperation;
        }

        //
        // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
        //
        AdjustDateAndTimePosition (TRUE, &TopOfScreen);
        AdjustDateAndTimePosition (TRUE, &NewPos);
        MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);
        UpdateStatusBar (Selection, INPUT_ERROR, MenuOption->ThisTag->QuestionFlags, FALSE);
      } else {
        //
        // Scroll up to the last page.
        //
        NewPos          = &gMenuOption;
        TopOfScreen     = &gMenuOption;
        MenuOption      = MENU_OPTION_FROM_LINK (SavedListEntry);
        ScreenOperation = UiPageUp;
        ControlFlag     = CfScreenOperation;
      }
      break;

    case CfUiPageUp:
      ControlFlag     = CfCheckSelection;

      ASSERT(NewPos != NULL);
      if (NewPos->BackLink == &gMenuOption) {
        NewLine = FALSE;
        Repaint = FALSE;
        break;
      }

      NewLine   = TRUE;
      Repaint   = TRUE;
      Link      = TopOfScreen;
      Index     = BottomRow;
      while ((Index >= TopRow) && (Link->BackLink != &gMenuOption)) {
        Link = Link->BackLink;
        PreviousMenuOption = MENU_OPTION_FROM_LINK (Link);
        if (PreviousMenuOption->Row == 0) {
          UpdateOptionSkipLines (Selection, PreviousMenuOption);
        }        
        if (Index < PreviousMenuOption->Skip) {
          Index = 0;
          break;
        }
        Index = Index - PreviousMenuOption->Skip;
      }
      
      if ((Link->BackLink == &gMenuOption) && (Index >= TopRow)) {
        if (TopOfScreen == &gMenuOption) {
          TopOfScreen = gMenuOption.ForwardLink;
          NewPos      = gMenuOption.BackLink;
          MoveToNextStatement (Selection, TRUE, &NewPos, BottomRow - TopRow);
          Repaint = FALSE;
        } else if (TopOfScreen != Link) {
          TopOfScreen = Link;
          NewPos      = Link;
          MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - TopRow);
        } else {
          //
          // Finally we know that NewPos is the last MenuOption can be focused.
          //
          Repaint = FALSE;
          NewPos  = Link;
          MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - TopRow);
        }
      } else {
        if (Index + 1 < TopRow) {
          //
          // Back up the previous option.
          //
          Link = Link->ForwardLink;
        }

        //
        // Move to the option in Next page.
        //
        if (TopOfScreen == &gMenuOption) {
          NewPos = gMenuOption.BackLink;
          MoveToNextStatement (Selection, TRUE, &NewPos, BottomRow - TopRow);
        } else {
          NewPos = Link;
          MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - TopRow);
        }

        //
        // There are more MenuOption needing scrolling up.
        //
        TopOfScreen = Link;
        MenuOption  = NULL;
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

      ASSERT (NewPos != NULL);
      if (NewPos->ForwardLink == &gMenuOption) {
        NewLine = FALSE;
        Repaint = FALSE;
        break;
      }

      NewLine = TRUE;
      Repaint = TRUE;
      Link    = TopOfScreen;
      NextMenuOption = MENU_OPTION_FROM_LINK (Link);
      Index = TopRow;
      while ((Index <= BottomRow) && (Link->ForwardLink != &gMenuOption)) {
        Index = Index + NextMenuOption->Skip;
        Link           = Link->ForwardLink;
        NextMenuOption = MENU_OPTION_FROM_LINK (Link);
      }

      if ((Link->ForwardLink == &gMenuOption) && (Index <= BottomRow)) {
        //
        // Finally we know that NewPos is the last MenuOption can be focused.
        //
        Repaint = FALSE;
        MoveToNextStatement (Selection, TRUE, &Link, Index - TopRow);
      } else {
        if (Index - 1 > BottomRow) {
          //
          // Back up the previous option.
          //
          Link = Link->BackLink;
        }
        //
        // There are more MenuOption needing scrolling down.
        //
        TopOfScreen = Link;
        MenuOption = NULL;
        //
        // Move to the option in Next page.
        //
        MoveToNextStatement (Selection, FALSE, &Link, BottomRow - TopRow);
      }

      //
      // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
      // Don't do this when we are already in the last page.
      //
      NewPos  = Link;
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
      AdjustDateAndTimePosition (FALSE, &NewPos);

      if (NewPos->ForwardLink != &gMenuOption) {
        MenuOption      = MENU_OPTION_FROM_LINK (NewPos);
        NewLine         = TRUE;
        NewPos          = NewPos->ForwardLink;

        Difference      = 0;
        if (BottomRow >= MenuOption->Row + MenuOption->Skip) {
          Difference    = MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - MenuOption->Row - MenuOption->Skip);
          //
          // We hit the end of MenuOption that can be focused
          // so we simply scroll to the first page.
          //
          if (Difference < 0) {
            //
            // Scroll to the first page.
            //
            if (TopOfScreen != gMenuOption.ForwardLink) {
              TopOfScreen = gMenuOption.ForwardLink;
              Repaint     = TRUE;
              MenuOption  = NULL;
            } else {
              MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);
            }
            NewPos        = gMenuOption.ForwardLink;
            MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - TopRow);
    
            //
            // If we are at the end of the list and sitting on a Date/Time op, rewind to the head.
            //
            AdjustDateAndTimePosition (TRUE, &TopOfScreen);
            AdjustDateAndTimePosition (TRUE, &NewPos);
            break;
          }
        }
        NextMenuOption  = MENU_OPTION_FROM_LINK (NewPos);

        //
        // An option might be multi-line, so we need to reflect that data in the overall skip value
        //
        UpdateOptionSkipLines (Selection, NextMenuOption);
        DistanceValue  = Difference + NextMenuOption->Skip;

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
                while (Difference >= (INTN) SavedMenuOption->Skip) {
                  //
                  // Since the Difference is greater than or equal to this op-code's skip value, skip it
                  //
                  Difference      = Difference - (INTN) SavedMenuOption->Skip;
                  TopOfScreen     = TopOfScreen->ForwardLink;
                  SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);
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
            } else if (SavedMenuOption->Skip == 1) {
              SkipValue   = 0;
            } else {
              SkipValue   = 0;
              TopOfScreen = TopOfScreen->ForwardLink;
            }
          } while (SavedMenuOption->Skip == 0);

          Repaint       = TRUE;
          OldSkipValue  = SkipValue;
        } else if (!IsSelectable (NextMenuOption)) {
          //
          // Continue to go down until scroll to next page or the selectable option is found.
          //
          ScreenOperation = UiDown;
          ControlFlag     = CfScreenOperation;
        }

        MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);

        UpdateStatusBar (Selection, INPUT_ERROR, MenuOption->ThisTag->QuestionFlags, FALSE);

      } else {
        //
        // Scroll to the first page.
        //
        if (TopOfScreen != gMenuOption.ForwardLink) {
          TopOfScreen = gMenuOption.ForwardLink;
          Repaint     = TRUE;
          MenuOption  = NULL;
        } else {
          MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);
        }
        NewLine       = TRUE;
        NewPos        = gMenuOption.ForwardLink;
        MoveToNextStatement (Selection, FALSE, &NewPos, BottomRow - TopRow);
      }

      //
      // If we are at the end of the list and sitting on a Date/Time op, rewind to the head.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);
      break;

    case CfUiHotKey:
      ControlFlag = CfCheckSelection;
      
      Status = EFI_SUCCESS;
      //
      // Discard changes. After it, no NV flag is showed.
      //
      if ((HotKey->Action & BROWSER_ACTION_DISCARD) == BROWSER_ACTION_DISCARD) {
        Status = DiscardForm (Selection->FormSet, Selection->Form, gBrowserSettingScope);
        if (!EFI_ERROR (Status)) {
          Selection->Action = UI_ACTION_REFRESH_FORM;
          Selection->Statement = NULL;
          gResetRequired = FALSE;
        } else {
          do {
            CreateDialog (4, TRUE, 0, NULL, &Key, HotKey->HelpString, gDiscardFailed, gPressEnter, gEmptyString);
          } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
          //
          // Still show current page.
          //
          Selection->Action = UI_ACTION_NONE;
          Repaint = TRUE;
          NewLine = TRUE;
          break;
        }
      }

      //
      // Reterieve default setting. After it. NV flag will be showed.
      //
      if ((HotKey->Action & BROWSER_ACTION_DEFAULT) == BROWSER_ACTION_DEFAULT) {
        Status = ExtractDefault (Selection->FormSet, Selection->Form, HotKey->DefaultId, gBrowserSettingScope, GetDefaultForAll, NULL, FALSE);
        if (!EFI_ERROR (Status)) {
          Selection->Action = UI_ACTION_REFRESH_FORM;
          Selection->Statement = NULL;
          gResetRequired = TRUE;
        } else {
          do {
            CreateDialog (4, TRUE, 0, NULL, &Key, HotKey->HelpString, gDefaultFailed, gPressEnter, gEmptyString);
          } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
          //
          // Still show current page.
          //
          Selection->Action = UI_ACTION_NONE;
          Repaint = TRUE;
          NewLine = TRUE;
          break;
        }
      }

      //
      // Save changes. After it, no NV flag is showed.
      //
      if ((HotKey->Action & BROWSER_ACTION_SUBMIT) == BROWSER_ACTION_SUBMIT) {
        Status = SubmitForm (Selection->FormSet, Selection->Form, gBrowserSettingScope);
        if (!EFI_ERROR (Status)) {
          ASSERT(MenuOption != NULL);
          UpdateStatusBar (Selection, INPUT_ERROR, MenuOption->ThisTag->QuestionFlags, FALSE);
          UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, MenuOption->ThisTag->QuestionFlags, FALSE);
        } else {
          do {
            CreateDialog (4, TRUE, 0, NULL, &Key, HotKey->HelpString, gSaveFailed, gPressEnter, gEmptyString);
          } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
          //
          // Still show current page.
          //
          Selection->Action = UI_ACTION_NONE;
          Repaint = TRUE;
          NewLine = TRUE;
          break;
        }
      }
      
      //
      // Set Reset required Flag
      //
      if ((HotKey->Action & BROWSER_ACTION_RESET) == BROWSER_ACTION_RESET) {
        gResetRequired = TRUE;
      }
      
      //
      // Exit Action
      //
      if ((HotKey->Action & BROWSER_ACTION_EXIT) == BROWSER_ACTION_EXIT) {
        //
        // Form Exit without saving, Similar to ESC Key.
        // FormSet Exit without saving, Exit SendForm.
        // System Exit without saving, CallExitHandler and Exit SendForm.
        //
        DiscardForm (Selection->FormSet, Selection->Form, gBrowserSettingScope);
        if (gBrowserSettingScope == FormLevel) {
          ControlFlag = CfUiReset;
        } else if (gBrowserSettingScope == FormSetLevel) {
          Selection->Action = UI_ACTION_EXIT;
        } else if (gBrowserSettingScope == SystemLevel) {
          if (ExitHandlerFunction != NULL) {
            ExitHandlerFunction ();
          }
          Selection->Action = UI_ACTION_EXIT;
        }
        Selection->Statement = NULL;
      }
      break;

    case CfUiDefault:
      ControlFlag = CfCheckSelection;
      //
      // Reset to default value for all forms in the whole system.
      //
      Status = ExtractDefault (Selection->FormSet, NULL, DefaultId, FormSetLevel, GetDefaultForAll, NULL, FALSE);

      if (!EFI_ERROR (Status)) {
        Selection->Action = UI_ACTION_REFRESH_FORM;
        Selection->Statement = NULL;
        gResetRequired = TRUE;
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
      if (HelpString != NULL) {
        FreePool (HelpString);
      }
      if (HelpHeaderString != NULL) {
        FreePool (HelpHeaderString);
      }
      if (HelpBottomString != NULL) {
        FreePool (HelpBottomString);
      }

      return EFI_SUCCESS;

    default:
      break;
    }
  }
}
