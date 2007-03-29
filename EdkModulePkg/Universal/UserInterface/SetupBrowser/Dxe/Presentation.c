/*++
Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  Presentation.c

Abstract:

  Some presentation routines.

Revision History:

--*/

#include "Setup.h"
#include "Ui.h"
#include "Colors.h"

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

  FreePool (Buffer);
  return ;
}

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
  Destination[Length] = (CHAR16) NARROW_CHAR;
  Length++;

  StrCpy (Destination + Length, Source);
}

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

VOID
DisplayPageFrame (
  VOID
  )
{
  UINTN             Index;
  UINT8             Line;
  UINT8             Alignment;
  CHAR16            Character;
  CHAR16            *Buffer;
  CHAR16            *StrFrontPageBanner;
  EFI_SCREEN_DESCRIPTOR LocalScreen;
  UINTN             Row;

  ZeroMem (&LocalScreen, sizeof (EFI_SCREEN_DESCRIPTOR));
  gST->ConOut->QueryMode (gST->ConOut, gST->ConOut->Mode->Mode, &LocalScreen.RightColumn, &LocalScreen.BottomRow);
  ClearLines (0, LocalScreen.RightColumn, 0, LocalScreen.BottomRow, KEYHELP_BACKGROUND);

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  // For now, allocate an arbitrarily long buffer
  //
  Buffer = AllocateZeroPool (0x10000);
  ASSERT (Buffer != NULL);

  Character = (CHAR16) BOXDRAW_HORIZONTAL;

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

        FreePool (StrFrontPageBanner);
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
    Character = (CHAR16) BOXDRAW_DOWN_RIGHT;

    PrintChar (Character);
    PrintString (Buffer);

    Character = (CHAR16) BOXDRAW_DOWN_LEFT;
    PrintChar (Character);

    Character = (CHAR16) BOXDRAW_VERTICAL;
    for (Row = LocalScreen.TopRow + 1; Row <= LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT - 2; Row++) {
      PrintCharAt (LocalScreen.LeftColumn, Row, Character);
      PrintCharAt (LocalScreen.RightColumn - 1, Row, Character);
    }

    Character = (CHAR16) BOXDRAW_UP_RIGHT;
    PrintCharAt (LocalScreen.LeftColumn, LocalScreen.TopRow + NONE_FRONT_PAGE_HEADER_HEIGHT - 1, Character);
    PrintString (Buffer);

    Character = (CHAR16) BOXDRAW_UP_LEFT;
    PrintChar (Character);

    if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
      //
      // Print Bottom border line
      // +------------------------------------------------------------------------------+
      // ?                                                                             ?
      // +------------------------------------------------------------------------------+
      //
      Character = (CHAR16) BOXDRAW_DOWN_RIGHT;
      PrintCharAt (LocalScreen.LeftColumn, LocalScreen.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT, Character);

      PrintString (Buffer);

      Character = (CHAR16) BOXDRAW_DOWN_LEFT;
      PrintChar (Character);
      Character = (CHAR16) BOXDRAW_VERTICAL;
      for (Row = LocalScreen.BottomRow - STATUS_BAR_HEIGHT - FOOTER_HEIGHT + 1;
           Row <= LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 2;
           Row++
          ) {
        PrintCharAt (LocalScreen.LeftColumn, Row, Character);
        PrintCharAt (LocalScreen.RightColumn - 1, Row, Character);
      }

      Character = (CHAR16) BOXDRAW_UP_RIGHT;
      PrintCharAt (LocalScreen.LeftColumn, LocalScreen.BottomRow - STATUS_BAR_HEIGHT - 1, Character);

      PrintString (Buffer);

      Character = (CHAR16) BOXDRAW_UP_LEFT;
      PrintChar (Character);
    }
  }

  FreePool (Buffer);

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
STATIC
UI_MENU_OPTION *
DisplayForm (
  OUT UI_MENU_OPTION              *Selection,
  IN  UINT16                      FormHandle,
  IN  UINT16                      TitleToken,
  IN  EFI_FORM_TAGS               FormTags,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  UINT8                       *CallbackData
  )
{
  CHAR16              *StringPtr;
  UINTN               Index;
  UINTN               Count;
  UINT16              MenuItemCount;
  EFI_HII_HANDLE      Handle;
  UINT16              FormId;
  STRING_REF          String;
  EFI_FILE_FORM_TAGS  *FileFormTags;
  BOOLEAN             SuppressIf;
  BOOLEAN             Suppress;
  BOOLEAN             GrayOut;
  BOOLEAN             Conditional;
  EFI_SCREEN_DESCRIPTOR   LocalScreen;
  UINT16              Width;
  UINTN               ArrayEntry;
  CHAR16              *OutputString;

  Handle        = Selection->Handle;
  FormId        = 0;
  String        = 0;
  MenuItemCount = 0;
  ArrayEntry    = 0;
  OutputString  = NULL;

  CopyMem (&LocalScreen, &gScreenDimensions, sizeof (EFI_SCREEN_DESCRIPTOR));

  //
  // If we hit a F2 (previous) we already nuked the menu and are simply carrying around what information we need
  //
  if (Selection->Previous) {
    Selection->Previous = FALSE;
  } else {
    UiFreeMenu ();
    UiInitMenu ();
  }

  StringPtr = GetToken (TitleToken, Handle);

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
    PrintAt (LocalScreen.LeftColumn + 2, LocalScreen.BottomRow - 3, (CHAR16 *) L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
    PrintStringAt (
      LocalScreen.LeftColumn + (LocalScreen.RightColumn - LocalScreen.LeftColumn) / 3,
      LocalScreen.BottomRow - 3,
      gEscapeString
      );
  }
  //
  // Remove Buffer allocated for StringPtr after it has been used.
  //
  FreePool (StringPtr);

  for (Index = 0; FormTags.Tags[Index].Operand != EFI_IFR_END_FORM_OP; Index++) {
    GrayOut       = FALSE;
    Suppress      = FALSE;
    SuppressIf    = FALSE;
    Conditional   = FALSE;
    FileFormTags  = FileFormTagsHead;

    if (FormTags.Tags[Index].Operand == EFI_IFR_FORM_OP) {
      FormId = FormTags.Tags[Index].Id;
    }
    //
    // This gives us visibility to the FileFormTags->NvRamMap to check things
    // ActiveIfr is a global maintained by the menuing code to ensure that we
    // are pointing to the correct formset's file data.
    //
    for (Count = 0; Count < gActiveIfr; Count++) {
      FileFormTags = FileFormTags->NextFile;
    }
    //
    //  GrayoutIf [SuppressIf]
    //    <BOOLEANS>
    //      OpCode(s)
    //  EndIf
    //
    //  SuppressIf [GrayoutIf]
    //    <BOOLEANS>
    //      OpCode(s)
    //  EndIf
    //
    Count = 0;

    do {
      switch (FormTags.Tags[Index].Operand) {
      case EFI_IFR_SUPPRESS_IF_OP:
        SuppressIf = TRUE;

      case EFI_IFR_GRAYOUT_IF_OP:

        Conditional = TRUE;

        //
        // Advance to the next op-code
        //
        Index++;

        //
        // We are now pointing to the beginning of the consistency checking.  Let's fast forward
        // through the AND/OR/NOT data to come up with some meaningful ID data.
        //
        for (;
             FormTags.Tags[Index].Operand == EFI_IFR_AND_OP   ||
             FormTags.Tags[Index].Operand == EFI_IFR_OR_OP    ||
             FormTags.Tags[Index].Operand == EFI_IFR_GT_OP    ||
             FormTags.Tags[Index].Operand == EFI_IFR_GE_OP    ||
             FormTags.Tags[Index].Operand == EFI_IFR_NOT_OP;
           Index++
            )
          ;

        //
        // We need to walk through the consistency checks until we hit the end of the consistency
        // FALSE means evaluate this single expression
        // The ConsistencyId refers to which expression in the Consistency database to use
        //
        if (SuppressIf) {
          Suppress = ValueIsNotValid (
                      FALSE,
                      FormTags.Tags[Index].ConsistencyId,
                      &FormTags.Tags[Index],
                      FileFormTags,
                      &String
                      );
          SuppressIf = FALSE;
        } else {
          GrayOut = ValueIsNotValid (
                      FALSE,
                      FormTags.Tags[Index].ConsistencyId,
                      &FormTags.Tags[Index],
                      FileFormTags,
                      &String
                      );
        }
        //
        // Advance to the end of the expression (Will land us at a grayoutif/suppressif or the op-code being affected)
        //
        for (;
             FormTags.Tags[Index].Operand == EFI_IFR_EQ_ID_VAL_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_EQ_VAR_VAL_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_EQ_ID_ID_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_EQ_ID_LIST_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_NOT_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_AND_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_OR_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_TRUE_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_FALSE_OP ||
             FormTags.Tags[Index].Operand == EFI_IFR_GT_OP    ||
             FormTags.Tags[Index].Operand == EFI_IFR_GE_OP    ||
             FormTags.Tags[Index].Operand == EFI_IFR_LABEL_OP;
           Index++
            )
          ;
        break;

      default:
        goto GetOut;
      }
      //
      // Do this two times (at most will see a suppress and grayout combination
      //
      Count++;
    } while (Count < 2);

GetOut:
    do {
      if (GrayOut) {
        FormTags.Tags[Index].GrayOut = TRUE;
      } else {
        FormTags.Tags[Index].GrayOut = FALSE;
      }
      if (Suppress && FormTags.Tags[Index].Operand == EFI_IFR_ONE_OF_OPTION_OP) {
        //
        // Only need .Suppress field when the tag is a one_of_option. For other cases, omit them directly.
        //
        FormTags.Tags[Index].Suppress = TRUE;
      } else {
        FormTags.Tags[Index].Suppress = FALSE;
      }

      if ((
            FormTags.Tags[Index].NumberOfLines > 0 ||
            FormTags.Tags[Index].Operand == EFI_IFR_DATE_OP ||
            FormTags.Tags[Index].Operand == EFI_IFR_TIME_OP
          ) &&
          !Suppress
          ) {

        StringPtr = GetToken (FormTags.Tags[Index].Text, Handle);

        Width     = GetWidth (&FormTags.Tags[Index], Handle);

        //
        // This data can be retrieved over and over again.  Therefore, reset to original values
        // before processing otherwise things will start growing linearly
        //
        if (FormTags.Tags[Index].NumberOfLines > 1) {
          FormTags.Tags[Index].NumberOfLines = 1;
        }

        for (Count = 0; GetLineByWidth (StringPtr, Width, &ArrayEntry, &OutputString) != 0x0000;) {
          //
          // If there is more string to process print on the next row and increment the Skip value
          //
          if (StrLen (&StringPtr[ArrayEntry])) {
            FormTags.Tags[Index].NumberOfLines++;
          }

          FreePool (OutputString);
        }

        ArrayEntry = 0;

        //
        // We are NOT!! removing this StringPtr buffer via FreePool since it is being used in the menuoptions, we will do
        // it in UiFreeMenu.
        //
        UiAddSubMenuOption (StringPtr, Handle, FormTags.Tags, Index, FormId, MenuItemCount);
        MenuItemCount++;
      }
      //
      // Keep processing menu entries based on the resultant suppress/grayout results until we hit an end-if
      //
      Index++;
    } while (FormTags.Tags[Index].Operand != EFI_IFR_END_IF_OP && Conditional);

    //
    // We advanced the index for the above conditional, rewind it to keep harmony with the for loop logic
    //
    Index--;
  }

  Selection = UiDisplayMenu (TRUE, FileFormTagsHead, (EFI_IFR_DATA_ARRAY *) CallbackData);

  return Selection;
}

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
  gMoveHighlight        = GetToken (STRING_TOKEN (MOVE_HIGHLIGHT), gHiiHandle);
  gMakeSelection        = GetToken (STRING_TOKEN (MAKE_SELECTION), gHiiHandle);
  gNumericInput         = GetToken (STRING_TOKEN (NUMERIC_INPUT), gHiiHandle);
  gToggleCheckBox       = GetToken (STRING_TOKEN (TOGGLE_CHECK_BOX), gHiiHandle);
  gPromptForPassword    = GetToken (STRING_TOKEN (PROMPT_FOR_PASSWORD), gHiiHandle);
  gPromptForNewPassword = GetToken (STRING_TOKEN (PROMPT_FOR_NEW_PASSWORD), gHiiHandle);
  gConfirmPassword      = GetToken (STRING_TOKEN (CONFIRM_PASSWORD), gHiiHandle);
  gConfirmError         = GetToken (STRING_TOKEN (CONFIRM_ERROR), gHiiHandle);
  gPressEnter           = GetToken (STRING_TOKEN (PRESS_ENTER), gHiiHandle);
  gEmptyString          = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
  gAreYouSure           = GetToken (STRING_TOKEN (ARE_YOU_SURE), gHiiHandle);
  gYesResponse          = GetToken (STRING_TOKEN (ARE_YOU_SURE_YES), gHiiHandle);
  gNoResponse           = GetToken (STRING_TOKEN (ARE_YOU_SURE_NO), gHiiHandle);
  gMiniString           = GetToken (STRING_TOKEN (MINI_STRING), gHiiHandle);
  gPlusString           = GetToken (STRING_TOKEN (PLUS_STRING), gHiiHandle);
  gMinusString          = GetToken (STRING_TOKEN (MINUS_STRING), gHiiHandle);
  gAdjustNumber         = GetToken (STRING_TOKEN (ADJUST_NUMBER), gHiiHandle);
  return ;
}

VOID
UpdateKeyHelp (
  IN  UI_MENU_OPTION              *Selection,
  IN  BOOLEAN                     Selected
  )
/*++
Routine Description:
  Update key's help imformation

Arguments:
  Selection C The form that current display
  Selected C  Whether or not a tag be selected

Returns:
  None
--*/
{
  UINTN             SecCol;
  UINTN             ThdCol;
  UINTN             LeftColumnOfHelp;
  UINTN             RightColumnOfHelp;
  UINTN             TopRowOfHelp;
  UINTN             BottomRowOfHelp;
  UINTN             StartColumnOfHelp;
  EFI_SCREEN_DESCRIPTOR LocalScreen;

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

  switch (Selection->ThisTag->Operand) {
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

      if ((Selection->ThisTag->Operand == EFI_IFR_DATE_OP) || (Selection->ThisTag->Operand == EFI_IFR_TIME_OP)) {
        PrintAt (
          StartColumnOfHelp,
          BottomRowOfHelp,
          (CHAR16 *) L"%c%c%c%c%s",
          ARROW_UP,
          ARROW_DOWN,
          ARROW_RIGHT,
          ARROW_LEFT,
          gMoveHighlight
          );
        PrintStringAt (SecCol, BottomRowOfHelp, gAdjustNumber);
      } else {
        PrintAt (StartColumnOfHelp, BottomRowOfHelp, (CHAR16 *) L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
        PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
      }
    } else {
      PrintStringAt (SecCol, BottomRowOfHelp, gEnterCommitString);

      //
      // If it is a selected numeric with manual input, display different message
      //
      if ((Selection->ThisTag->Operand == EFI_IFR_NUMERIC_OP) && (Selection->ThisTag->Step == 0)) {
        PrintStringAt (SecCol, TopRowOfHelp, gNumericInput);
      } else if (Selection->ThisTag->Operand != EFI_IFR_ORDERED_LIST_OP) {
        PrintAt (StartColumnOfHelp, BottomRowOfHelp, L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      }

      if (Selection->ThisTag->Operand == EFI_IFR_ORDERED_LIST_OP) {
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

    PrintAt (StartColumnOfHelp, BottomRowOfHelp, (CHAR16 *) L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
    PrintStringAt (SecCol, BottomRowOfHelp, gToggleCheckBox);
    break;

  case EFI_IFR_REF_OP:
  case EFI_IFR_PASSWORD_OP:
  case EFI_IFR_STRING_OP:
    ClearLines (LeftColumnOfHelp, RightColumnOfHelp, TopRowOfHelp, BottomRowOfHelp, KEYHELP_TEXT | KEYHELP_BACKGROUND);

    if (!Selected) {
      if (gClassOfVfr == EFI_SETUP_APPLICATION_SUBCLASS) {
        PrintStringAt (StartColumnOfHelp, TopRowOfHelp, gFunctionOneString);
        PrintStringAt (SecCol, TopRowOfHelp, gFunctionNineString);
        PrintStringAt (ThdCol, TopRowOfHelp, gFunctionTenString);
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }

      PrintAt (StartColumnOfHelp, BottomRowOfHelp, (CHAR16 *) L"%c%c%s", ARROW_UP, ARROW_DOWN, gMoveHighlight);
      PrintStringAt (SecCol, BottomRowOfHelp, gEnterString);
    } else {
      if (Selection->ThisTag->Operand != EFI_IFR_REF_OP) {
        PrintStringAt (
          (LocalScreen.RightColumn - GetStringWidth (gEnterCommitString) / 2) / 2,
          BottomRowOfHelp,
          gEnterCommitString
          );
        PrintStringAt (ThdCol, BottomRowOfHelp, gEscapeString);
      }
    }
    break;
  }

}

STATIC
VOID
ExtractFormHandle (
  IN  UI_MENU_OPTION              *Selection,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  UINTN                       IdValue,
  OUT UINT16                      *FormHandle,
  OUT UINT16                      *TitleToken,
  OUT EFI_FORM_TAGS               *FormTags
  )
{
  UINTN               Index;
  EFI_FILE_FORM_TAGS  *FileFormTags;
  EFI_FORM_TAGS       LocalTags;

  FileFormTags = FileFormTagsHead;

  //
  // Advance FileFormTags to the correct file's tag information.
  // For instance, if Selection->IfrNumber is 3, that means the 4th
  // file (0-based) in the FileFormTags linked-list contains the tag
  // information.
  //
  for (Index = 0; Index < Selection->IfrNumber; Index++) {
    FileFormTags = FileFormTags->NextFile;
  }

  LocalTags = FileFormTags->FormTags;

  if (IdValue == 0) {
    //
    // Advance Index to the first FormOp tag information
    //
    for (Index = 0; FileFormTags->FormTags.Tags[Index].Operand != EFI_IFR_FORM_OP; Index++)
      ;
  } else {
    //
    // Advance Index to the FormOp with the correct ID value
    //
    for (; LocalTags.Next != NULL; LocalTags = *LocalTags.Next) {
      for (Index = 0; LocalTags.Tags[Index].Operand != EFI_IFR_FORM_OP; Index++)
        ;
      if (LocalTags.Tags[Index].Id == IdValue) {
        break;
      }
    }
  }
  //
  // return the Form Id, Text, and the File's FormTags structure
  //
  *FormHandle = LocalTags.Tags[Index].Id;
  *TitleToken = LocalTags.Tags[Index].Text;
  *FormTags   = LocalTags;
  return ;
}

STATIC
EFI_STATUS
UpdateNewTagData (
  IN  UINT8                                     *FormData,
  IN  UINT16                                    ConsistencyId,
  IN  UINT16                                    CurrentVariable,
  IN  EFI_FORM_TAGS                             *FormTags,
  OUT EFI_FILE_FORM_TAGS                        *FileFormTags
  )
{
  EFI_STATUS  Status;
  UINT16      Index;
  UINT16      QuestionIndex;
  UINT16      NumberOfTags;
  INT16       CurrTag;
  UINT8       TagLength;
  UINTN       Count;
  BOOLEAN     Finished;

  //
  // Initialize some Index variable and Status
  //
  Count         = 0;
  QuestionIndex = 0;
  NumberOfTags  = 1;
  Index         = 0;
  Status        = EFI_SUCCESS;
  Finished      = FALSE;

  //
  // Determine the number of tags for the first form
  //
  GetTagCount (&FormData[Index], &NumberOfTags);

  //
  // Allocate memory for our tags on the first form
  //
  FormTags->Tags = AllocateZeroPool (NumberOfTags * sizeof (EFI_TAG));
  ASSERT (FormTags->Tags != NULL);

  for (CurrTag = 0; FormData[Index] != EFI_IFR_END_FORM_SET_OP; CurrTag++) {
    //
    // Operand = IFR OpCode
    //
    FormTags->Tags[CurrTag].Operand = FormData[Index];

    //
    // Assume for now 0 lines occupied by this OpCode
    //
    FormTags->Tags[CurrTag].NumberOfLines = 0;

    //
    // Determine the length of the Tag so we can later skip to the next tag in the form
    //
    //
    // get the length
    //
    TagLength = FormData[Index + 1];
    //
    // Operate on the Found OpCode
    //
    switch (FormData[Index]) {

    case EFI_IFR_FORM_OP:
    case EFI_IFR_SUBTITLE_OP:
    case EFI_IFR_TEXT_OP:
    case EFI_IFR_REF_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      break;

    case EFI_IFR_VARSTORE_SELECT_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      CopyMem (&CurrentVariable, &((EFI_IFR_VARSTORE_SELECT *) &FormData[Index])->VarId, sizeof (UINT16));
      break;

    case EFI_IFR_END_FORM_OP:
      FormTags->Tags[CurrTag].Operand       = FormData[Index];
      FormTags->Tags[CurrTag].NumberOfLines = 0;

      Finished = TRUE;
      break;

    case EFI_IFR_ORDERED_LIST_OP:
    case EFI_IFR_ONE_OF_OP:
      GetQuestionHeader (&FormTags->Tags[CurrTag], FormData, Index, FileFormTags, CurrentVariable);

      //
      // Store away the CurrTag since what follows will be the answer that we
      // need to place into the appropriate location in the tag array
      //
      //
      // record for setting default later
      //
      QuestionIndex = (UINT16) CurrTag;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      FormTags->Tags[QuestionIndex].Key = ((EFI_IFR_ONE_OF_OPTION *) &FormData[Index])->Key;
      FormTags->Tags[QuestionIndex].ResetRequired = (BOOLEAN) (FormTags->Tags[QuestionIndex].Flags & EFI_IFR_FLAG_RESET_REQUIRED);
      break;

    case EFI_IFR_CHECKBOX_OP:
      GetQuestionHeader (&FormTags->Tags[CurrTag], FormData, Index, FileFormTags, CurrentVariable);
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      break;

    case EFI_IFR_NUMERIC_OP:
      GetNumericHeader (&FormTags->Tags[CurrTag], FormData, Index, (UINT16) 1, FileFormTags, CurrentVariable);
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      break;

    case EFI_IFR_DATE_OP:
      //
      // Date elements come in as a Year, Month, Day.  We need to process them as a country-based
      // Order.  It is much easier to do it here than anywhere else.
      //
      // For US standards - we want Month/Day/Year, thus we advance "i" +1, +2, +0 while CurrTag is +0, +1, +2
      //
      GetNumericHeader (
        &FormTags->Tags[CurrTag],
        FormData,
        (UINT16) (Index + TagLength),
        (UINT16) 0,
        FileFormTags,
        CurrentVariable
        );

      //
      // The current language selected + the Date operand
      //
      FormTags->Tags[CurrTag + 1].Operand = FormData[Index];
      GetNumericHeader (
        &FormTags->Tags[CurrTag + 1],
        FormData,
        (UINT16) (Index + TagLength + FormData[Index + TagLength + 1]),
        (UINT16) 0,
        FileFormTags,
        CurrentVariable
        );

      //
      // The current language selected + the Date operand
      //
      FormTags->Tags[CurrTag + 2].Operand = FormData[Index];
      GetNumericHeader (&FormTags->Tags[CurrTag + 2], FormData, Index, (UINT16) 1, FileFormTags, CurrentVariable);

      CurrTag   = (INT16) (CurrTag + 2);

      Index     = (UINT16) (Index + TagLength);
      //
      // get the length
      //
      TagLength = FormData[Index + 1];
      Index     = (UINT16) (Index + TagLength);
      //
      // get the length
      //
      TagLength = FormData[Index + 1];
      break;

    case EFI_IFR_TIME_OP:
      GetNumericHeader (&FormTags->Tags[CurrTag], FormData, Index, (UINT16) 0, FileFormTags, CurrentVariable);

      if (Count == 2) {
        //
        // Override the GetQuestionHeader information - date/time are treated very differently
        //
        FormTags->Tags[CurrTag].NumberOfLines = 1;
        Count = 0;
      } else {
        //
        // The premise is that every date/time op-code have 3 elements, the first 2 have 0 lines
        // associated with them, and the third has 1 line to allow to space beyond the choice.
        //
        Count++;
      }
      break;

    case EFI_IFR_PASSWORD_OP:
    case EFI_IFR_STRING_OP:
      GetQuestionHeader (&FormTags->Tags[CurrTag], FormData, Index, FileFormTags, CurrentVariable);
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      break;

    case EFI_IFR_INCONSISTENT_IF_OP:
    case EFI_IFR_SUPPRESS_IF_OP:
    case EFI_IFR_GRAYOUT_IF_OP:
      ConsistencyId++;
      break;

    case EFI_IFR_EQ_ID_VAL_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      FormTags->Tags[CurrTag].ConsistencyId = ConsistencyId;
      break;

    case EFI_IFR_EQ_VAR_VAL_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      FormTags->Tags[CurrTag].ConsistencyId = ConsistencyId;
      break;

    case EFI_IFR_EQ_ID_ID_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);
      FormTags->Tags[CurrTag].ConsistencyId = ConsistencyId;
      break;

    case EFI_IFR_AND_OP:
    case EFI_IFR_OR_OP:
    case EFI_IFR_NOT_OP:
    case EFI_IFR_TRUE_OP:
    case EFI_IFR_FALSE_OP:
    case EFI_IFR_GT_OP:
    case EFI_IFR_GE_OP:
      FormTags->Tags[CurrTag].ConsistencyId = ConsistencyId;
      break;

    case EFI_IFR_EQ_ID_LIST_OP:
      IfrToFormTag (FormData[Index], &FormTags->Tags[CurrTag], (VOID *) &FormData[Index], NULL);

      FormTags->Tags[CurrTag].ConsistencyId = ConsistencyId;
      break;

    default:
      break;
    }
    //
    // End of switch
    //
    if (Finished) {
      break;
    }
    //
    // Per spec., we ignore ops that we don't know how to deal with.  Skip to next tag
    //
    Index = (UINT16) (Index + TagLength);
  }
  //
  // End of Index
  //
  return Status;
}

STATIC
VOID
ExtractDynamicFormHandle (
  IN  UI_MENU_OPTION              *Selection,
  IN  UINT8                       *CallbackData,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  UINTN                       IdValue,
  OUT UINT16                      *FormHandle,
  OUT UINT16                      *TitleToken,
  OUT EFI_FORM_TAGS               *FormTags
  )
/*++

Routine Description:

  The function does the most of the works when the EFI_TAG that
  user selects on is EFI_IFR_FLAG_INTERACTIVE or EFI_IFR_PASSWORD_OP:
  invoke CallBack, update the new form data.

Arguments:

  Selection         - The current selection of the form.
  CallbackData      - The pointer to host the data passed back by the callback function.
  FileFormTagsHead  - Prompt string token of the one-of box
  IdValue           - The current page number.
  FormHandle        - Output the  the handle of the form.
  TitleToken        - Output the  TitleToken of the new page.
  FormTags          - Output the  FormFags of the new page.

Returns:
  VOID

--*/
{
  UINTN                       Index;
  UINTN                       BackupIndex;
  EFI_FILE_FORM_TAGS          *FileFormTags;
  EFI_FORM_TAGS               *LocalTags;
  EFI_FORM_CALLBACK_PROTOCOL  *FormCallback;
  EFI_STATUS                  Status;
  UINTN                       Length;
  UINT8                       *Buffer;
  EFI_PHYSICAL_ADDRESS        CallbackHandle;
  EFI_GUID                    TagGuid;
  UINT16                      TargetPage;
  EFI_HII_CALLBACK_PACKET     *Packet;
  UINTN                       ScreenSize;
  CHAR16                      NullCharacter;
  EFI_INPUT_KEY               Key;
  UINT16                      ConsistencyId;
  UINT16                      CurrentVariable;
  EFI_VARIABLE_DEFINITION     *VariableDefinition;
  EFI_IFR_DATA_ENTRY          *DataEntry;

  VariableDefinition  = NULL;
  NullCharacter       = CHAR_NULL;

  CurrentVariable     = 0;
  FileFormTags        = FileFormTagsHead;
  Length              = 0;
  CallbackHandle      = 0;
  TargetPage          = (UINT16) IdValue;
  Packet              = NULL;
  ConsistencyId       = 0;

  //
  // Advance FileFormTags to the correct file's tag information.
  // For instance, if Selection->IfrNumber is 3, that means the 4th
  // file (0-based) in the FileFormTags linked-list contains the tag
  // information.
  //
  for (Index = 0; Index < Selection->IfrNumber; Index++) {
    FileFormTags = FileFormTags->NextFile;
  }

  LocalTags = &FileFormTags->FormTags;

  //
  // Advance Index to the FormOp with the correct ID value
  //
  for (; LocalTags->Next != NULL; LocalTags = LocalTags->Next) {
    if ((LocalTags->Tags[0].CallbackHandle != 0) && (CallbackHandle == 0)) {
      CallbackHandle = LocalTags->Tags[0].CallbackHandle;
      CopyMem (&TagGuid, &LocalTags->Tags[0].GuidValue, sizeof (EFI_GUID));
    }

    for (Index = 0; LocalTags->Tags[Index].Operand != EFI_IFR_FORM_OP; Index++)
      ;
    if (LocalTags->Tags[Index].Id == IdValue) {
      break;
    }
  }
  //
  // If we are going to callback on a non-goto opcode, make sure we don't change pages
  //
  if (Selection->ThisTag->Operand != EFI_IFR_REF_OP) {
    TargetPage = Selection->FormId;
  }
  //
  // The first tag below should be the form op-code.  We need to store away the
  // current variable setting to ensure if we have to reload the page, that we
  // can correctly restore the values for the active variable
  //
  CurrentVariable = Selection->Tags[0].VariableNumber;

  //
  // Remember that dynamic pages in an environment where all pages are not
  // dynamic require us to call back to the user to give them an opportunity
  // to register fresh information in the HII database so that we can extract it.
  //
  Status = gBS->HandleProtocol (
                  (VOID *) (UINTN) CallbackHandle,
                  &gEfiFormCallbackProtocolGuid,
                  (VOID **) &FormCallback
                  );

  if (EFI_ERROR (Status)) {
    FreePool (LocalTags->Tags);
    return ;
  }

  ExtractRequestedNvMap (FileFormTags, CurrentVariable, &VariableDefinition);

  if (Selection->ThisTag->Flags & (EFI_IFR_FLAG_INTERACTIVE | EFI_IFR_FLAG_NV_ACCESS)) {
    ((EFI_IFR_DATA_ARRAY *) CallbackData)->NvRamMap = VariableDefinition->NvRamMap;
  } else {
    ((EFI_IFR_DATA_ARRAY *) CallbackData)->NvRamMap = NULL;
  }

  if ((FormCallback != NULL) && (FormCallback->Callback != NULL)) {
    Status = FormCallback->Callback (
                            FormCallback,
                            Selection->ThisTag->Key,
                            (EFI_IFR_DATA_ARRAY *) CallbackData,
                            &Packet
                            );
  }

  if (EFI_ERROR (Status)) {
    //
    // Restore Previous Value
    //
    CopyMem (
      &VariableDefinition->NvRamMap[Selection->ThisTag->StorageStart],
      gPreviousValue,
      Selection->ThisTag->StorageWidth
      );

    if (Packet != NULL) {
      //
      // Upon error, we will likely receive a string to print out
      //
      ScreenSize = GetStringWidth (Packet->String) / 2;

      //
      // Display error popup
      //
      CreatePopUp (ScreenSize, 3, &NullCharacter, Packet->String, &NullCharacter);

      do {
        Status = WaitForKeyStroke (&Key);
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    } else {
      UpdateStatusBar (INPUT_ERROR, (UINT8) 0, TRUE);
    }

  } else {
    if (Packet != NULL) {
      //
      // We need to on a non-error, look in the outbound Packet for information and update the NVRAM
      // location associated with the op-code specified there.  This is used on single op-code instances
      // and not for when a hyperlink sent us a whole page of data.
      //
      DataEntry = (EFI_IFR_DATA_ENTRY *) (&Packet->DataArray + 1);
      if (Packet->DataArray.EntryCount == 1) {
        switch (DataEntry->OpCode) {
        case EFI_IFR_STRING_OP:
        case EFI_IFR_NUMERIC_OP:
        case EFI_IFR_ORDERED_LIST_OP:
        case EFI_IFR_ONE_OF_OP:
        case EFI_IFR_CHECKBOX_OP:
          CopyMem (
            &VariableDefinition->NvRamMap[Selection->ThisTag->StorageStart],
            &DataEntry->Data,
            Selection->ThisTag->StorageWidth
            );
          break;

        case EFI_IFR_NV_ACCESS_COMMAND:
          CopyMem (
            &VariableDefinition->NvRamMap[((EFI_IFR_NV_DATA *) Packet)->QuestionId],
            ((EFI_IFR_NV_DATA *) Packet) + 1,
            ((EFI_IFR_NV_DATA *) Packet)->StorageWidth
            );
          break;

        }

        if (DataEntry->Flags & RESET_REQUIRED) {
          gResetRequired = TRUE;
        }

        if (DataEntry->Flags & EXIT_REQUIRED) {
          gExitRequired = TRUE;
        }

        if (DataEntry->Flags & SAVE_REQUIRED) {
          gSaveRequired = TRUE;
        }

        if (DataEntry->Flags & NV_CHANGED) {
          gNvUpdateRequired = TRUE;
        }

        if (DataEntry->Flags & NV_NOT_CHANGED) {
          gNvUpdateRequired = FALSE;
        }
      }
    }
  }

  if (Packet != NULL) {
    FreePool (Packet);
  }

  for (BackupIndex = 0; LocalTags->Tags[BackupIndex].Operand != EFI_IFR_END_FORM_OP; BackupIndex++) {
    switch (LocalTags->Tags[BackupIndex].Operand) {
    case EFI_IFR_EQ_VAR_VAL_OP:
    case EFI_IFR_EQ_ID_VAL_OP:
    case EFI_IFR_EQ_ID_ID_OP:
    case EFI_IFR_AND_OP:
    case EFI_IFR_OR_OP:
    case EFI_IFR_NOT_OP:
    case EFI_IFR_TRUE_OP:
    case EFI_IFR_FALSE_OP:
    case EFI_IFR_GT_OP:
    case EFI_IFR_GE_OP:
    case EFI_IFR_EQ_ID_LIST_OP:
      //
      // If we encountered a ConsistencyId value, on this page they will be incremental
      // So register the first value we encounter.  We will pass this in when we re-create this page
      //
      if ((LocalTags->Tags[BackupIndex].ConsistencyId != 0) && (ConsistencyId == 0)) {
        ConsistencyId = (UINT16) (LocalTags->Tags[BackupIndex].ConsistencyId - 1);
      }
      break;
    }
  }
  //
  // Delete the buffer associated with previous dynamic page
  // We will re-allocate a buffer....
  //
  FreePool (LocalTags->Tags);

  Length  = 0xF000;
  Buffer  = AllocateZeroPool (Length);
  ASSERT (Buffer != NULL);

  //
  // Get the form that was updated by the callback
  //
  Hii->GetForms (
        Hii,
        Selection->Handle,
        TargetPage,
        &Length,
        Buffer
        );

  //
  // Ok, we have the new page.....now we must purge the old page and re-allocate
  // the tag page with the new data
  //
  UpdateNewTagData (
    Buffer,
    ConsistencyId,
    CurrentVariable,
    LocalTags,
    FileFormTags
    );

  //
  // return the Form Id, Text, and the File's FormTags structure
  //
  *FormHandle                       = LocalTags->Tags[0].Id;
  *TitleToken                       = LocalTags->Tags[0].Text;
  *FormTags                         = *LocalTags;

  FormTags->Tags[0].CallbackHandle  = CallbackHandle;
  CopyMem (&FormTags->Tags[0].GuidValue, &TagGuid, sizeof (EFI_GUID));

  return ;
}

UI_MENU_OPTION *
SetupBrowser (
  IN  UI_MENU_OPTION              *Selection,
  IN  BOOLEAN                     Callback,
  IN  EFI_FILE_FORM_TAGS          *FileFormTagsHead,
  IN  UINT8                       *CallbackData
  )
{
  UINT16        FormHandle;
  UINT16        TitleToken;
  EFI_FORM_TAGS FormTags;

  gEntryNumber  = -1;
  gLastOpr      = FALSE;
  //
  // Displays the Header and Footer borders
  //
  DisplayPageFrame ();

  //
  // Id of 0 yields the getting of the top form whatever the ID is.  Usually the first form in the IFR
  //
  ExtractFormHandle (Selection, FileFormTagsHead, 0, &FormHandle, &TitleToken, &FormTags);

  Selection = DisplayForm (Selection, FormHandle, TitleToken, FormTags, FileFormTagsHead, CallbackData);

  //
  // If selection is null use the former selection
  //
  if (Selection == NULL) {
    return Selection;
  }

  if (Callback) {
    return Selection;
  }

  while (Selection->Tags != NULL) {
    if (Selection->Previous) {
      ExtractFormHandle (Selection, FileFormTagsHead, Selection->FormId, &FormHandle, &TitleToken, &FormTags);
    } else {
      //
      // True if a hyperlink/jump is selected
      //
      if (Selection->ThisTag->Operand == EFI_IFR_REF_OP && Selection->ThisTag->Id != 0x0000) {
        if (Selection->ThisTag->Flags & EFI_IFR_FLAG_INTERACTIVE) {
          ExtractDynamicFormHandle (
            Selection,
            CallbackData,
            FileFormTagsHead,
            Selection->ThisTag->Id,
            &FormHandle,
            &TitleToken,
            &FormTags
            );
          goto DisplayPage;
        } else {
          ExtractFormHandle (Selection, FileFormTagsHead, Selection->ThisTag->Id, &FormHandle, &TitleToken, &FormTags);
          goto DisplayPage;
        }
      }

      if ((Selection->ThisTag->Flags & EFI_IFR_FLAG_INTERACTIVE) &&
          (Selection->ThisTag->Operand != EFI_IFR_PASSWORD_OP)
          ) {
        ExtractDynamicFormHandle (
          Selection,
          CallbackData,
          FileFormTagsHead,
          Selection->FormId,
          &FormHandle,
          &TitleToken,
          &FormTags
          );
      } else {
        ExtractFormHandle (Selection, FileFormTagsHead, Selection->FormId, &FormHandle, &TitleToken, &FormTags);
      }
    }

DisplayPage:
    //
    // Displays the Header and Footer borders
    //
    DisplayPageFrame ();

    Selection = DisplayForm (Selection, FormHandle, TitleToken, FormTags, FileFormTagsHead, CallbackData);

    if (Selection == NULL) {
      break;
    }
  };

  return Selection;
}
