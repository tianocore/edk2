/** @file
Entry and initialization module for the browser.

Copyright (c) 2007 - 2017, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "FormDisplay.h"

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

UINTN mScanCodeNumber = ARRAY_SIZE (gScanCodeToOperation);

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

EFI_GUID  gDisplayEngineGuid = {
  0xE38C1029, 0xE38F, 0x45b9, {0x8F, 0x0D, 0xE2, 0xE6, 0x0B, 0xC9, 0xB2, 0x62}
};

BOOLEAN                       gMisMatch;
EFI_SCREEN_DESCRIPTOR         gStatementDimensions;
BOOLEAN                       mStatementLayoutIsChanged = TRUE;
USER_INPUT                    *gUserInput;
FORM_DISPLAY_ENGINE_FORM      *gFormData;
EFI_HII_HANDLE                gHiiHandle;
UINT16                        gDirection;
LIST_ENTRY                    gMenuOption;
DISPLAY_HIGHLIGHT_MENU_INFO   gHighligthMenuInfo = {0};
BOOLEAN                       mIsFirstForm = TRUE;
FORM_ENTRY_INFO               gOldFormEntry = {0};

//
// Browser Global Strings
//
CHAR16            *gReconnectConfirmChanges;
CHAR16            *gReconnectFail;
CHAR16            *gReconnectRequired;
CHAR16            *gChangesOpt;
CHAR16            *gFormNotFound;
CHAR16            *gNoSubmitIf;
CHAR16            *gBrowserError;
CHAR16            *gSaveFailed;
CHAR16            *gNoSubmitIfFailed;
CHAR16            *gSaveProcess;
CHAR16            *gSaveNoSubmitProcess;
CHAR16            *gDiscardChange;
CHAR16            *gJumpToFormSet;
CHAR16            *gCheckError;
CHAR16            *gPromptForData;
CHAR16            *gPromptForPassword;
CHAR16            *gPromptForNewPassword;
CHAR16            *gConfirmPassword;
CHAR16            *gConfirmError;
CHAR16            *gPassowordInvalid;
CHAR16            *gPressEnter;
CHAR16            *gEmptyString;
CHAR16            *gMiniString;
CHAR16            *gOptionMismatch;
CHAR16            *gFormSuppress;
CHAR16            *gProtocolNotFound;
CHAR16            *gConfirmDefaultMsg;
CHAR16            *gConfirmSubmitMsg;
CHAR16            *gConfirmDiscardMsg;
CHAR16            *gConfirmResetMsg;
CHAR16            *gConfirmExitMsg;
CHAR16            *gConfirmSubmitMsg2nd;
CHAR16            *gConfirmDefaultMsg2nd;
CHAR16            *gConfirmResetMsg2nd;
CHAR16            *gConfirmExitMsg2nd;
CHAR16            *gConfirmOpt;
CHAR16            *gConfirmOptYes;
CHAR16            *gConfirmOptNo;
CHAR16            *gConfirmOptOk;
CHAR16            *gConfirmOptCancel;
CHAR16            *gYesOption;
CHAR16            *gNoOption;
CHAR16            *gOkOption;
CHAR16            *gCancelOption;
CHAR16            *gErrorPopup;
CHAR16            *gWarningPopup;
CHAR16            *gInfoPopup;
CHAR16            *gConfirmMsgConnect;
CHAR16            *gConfirmMsgEnd;
CHAR16            *gPasswordUnsupported;
CHAR16            gModalSkipColumn;
CHAR16            gPromptBlockWidth;
CHAR16            gOptionBlockWidth;
CHAR16            gHelpBlockWidth;
CHAR16            *mUnknownString;

FORM_DISPLAY_DRIVER_PRIVATE_DATA  mPrivateData = {
  FORM_DISPLAY_DRIVER_SIGNATURE,
  NULL,
  {
    FormDisplay,
    DriverClearDisplayPage,
    ConfirmDataChange
  },
  {
    EFI_HII_POPUP_PROTOCOL_REVISION,
    CreatePopup
  }
};


/**
  Get the string based on the StringId and HII Package List Handle.

  @param  Token                  The String's ID.
  @param  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetToken (
  IN  EFI_STRING_ID                Token,
  IN  EFI_HII_HANDLE               HiiHandle
  )
{
  EFI_STRING  String;

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (StrSize (mUnknownString), mUnknownString);
    ASSERT (String != NULL);
  }

  return (CHAR16 *) String;
}


/**
  Initialize the HII String Token to the correct values.

**/
VOID
InitializeDisplayStrings (
  VOID
  )
{
  gReconnectConfirmChanges = GetToken (STRING_TOKEN (RECONNECT_CONFIRM_CHANGES), gHiiHandle);
  mUnknownString        = GetToken (STRING_TOKEN (UNKNOWN_STRING), gHiiHandle);
  gSaveFailed           = GetToken (STRING_TOKEN (SAVE_FAILED), gHiiHandle);
  gNoSubmitIfFailed     = GetToken (STRING_TOKEN (NO_SUBMIT_IF_CHECK_FAILED), gHiiHandle);
  gReconnectFail        = GetToken (STRING_TOKEN (RECONNECT_FAILED), gHiiHandle);
  gReconnectRequired    = GetToken (STRING_TOKEN (RECONNECT_REQUIRED), gHiiHandle);
  gChangesOpt           = GetToken (STRING_TOKEN (RECONNECT_CHANGES_OPTIONS), gHiiHandle);
  gSaveProcess          = GetToken (STRING_TOKEN (DISCARD_OR_JUMP), gHiiHandle);
  gSaveNoSubmitProcess  = GetToken (STRING_TOKEN (DISCARD_OR_CHECK), gHiiHandle);
  gDiscardChange        = GetToken (STRING_TOKEN (DISCARD_OR_JUMP_DISCARD), gHiiHandle);
  gJumpToFormSet        = GetToken (STRING_TOKEN (DISCARD_OR_JUMP_JUMP), gHiiHandle);
  gCheckError           = GetToken (STRING_TOKEN (DISCARD_OR_CHECK_CHECK), gHiiHandle);
  gPromptForData        = GetToken (STRING_TOKEN (PROMPT_FOR_DATA), gHiiHandle);
  gPromptForPassword    = GetToken (STRING_TOKEN (PROMPT_FOR_PASSWORD), gHiiHandle);
  gPromptForNewPassword = GetToken (STRING_TOKEN (PROMPT_FOR_NEW_PASSWORD), gHiiHandle);
  gConfirmPassword      = GetToken (STRING_TOKEN (CONFIRM_PASSWORD), gHiiHandle);
  gConfirmError         = GetToken (STRING_TOKEN (CONFIRM_ERROR), gHiiHandle);
  gPassowordInvalid     = GetToken (STRING_TOKEN (PASSWORD_INVALID), gHiiHandle);
  gPressEnter           = GetToken (STRING_TOKEN (PRESS_ENTER), gHiiHandle);
  gEmptyString          = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
  gMiniString           = GetToken (STRING_TOKEN (MINI_STRING), gHiiHandle);
  gOptionMismatch       = GetToken (STRING_TOKEN (OPTION_MISMATCH), gHiiHandle);
  gFormSuppress         = GetToken (STRING_TOKEN (FORM_SUPPRESSED), gHiiHandle);
  gProtocolNotFound     = GetToken (STRING_TOKEN (PROTOCOL_NOT_FOUND), gHiiHandle);
  gFormNotFound         = GetToken (STRING_TOKEN (STATUS_BROWSER_FORM_NOT_FOUND), gHiiHandle);
  gNoSubmitIf           = GetToken (STRING_TOKEN (STATUS_BROWSER_NO_SUBMIT_IF), gHiiHandle);
  gBrowserError         = GetToken (STRING_TOKEN (STATUS_BROWSER_ERROR), gHiiHandle);
  gConfirmDefaultMsg    = GetToken (STRING_TOKEN (CONFIRM_DEFAULT_MESSAGE), gHiiHandle);
  gConfirmDiscardMsg    = GetToken (STRING_TOKEN (CONFIRM_DISCARD_MESSAGE), gHiiHandle);
  gConfirmSubmitMsg     = GetToken (STRING_TOKEN (CONFIRM_SUBMIT_MESSAGE), gHiiHandle);
  gConfirmResetMsg      = GetToken (STRING_TOKEN (CONFIRM_RESET_MESSAGE), gHiiHandle);
  gConfirmExitMsg       = GetToken (STRING_TOKEN (CONFIRM_EXIT_MESSAGE), gHiiHandle);
  gConfirmDefaultMsg2nd = GetToken (STRING_TOKEN (CONFIRM_DEFAULT_MESSAGE_2ND), gHiiHandle);
  gConfirmSubmitMsg2nd  = GetToken (STRING_TOKEN (CONFIRM_SUBMIT_MESSAGE_2ND), gHiiHandle);
  gConfirmResetMsg2nd   = GetToken (STRING_TOKEN (CONFIRM_RESET_MESSAGE_2ND), gHiiHandle);
  gConfirmExitMsg2nd    = GetToken (STRING_TOKEN (CONFIRM_EXIT_MESSAGE_2ND), gHiiHandle);
  gConfirmOpt           = GetToken (STRING_TOKEN (CONFIRM_OPTION), gHiiHandle);
  gConfirmOptYes        = GetToken (STRING_TOKEN (CONFIRM_OPTION_YES), gHiiHandle);
  gConfirmOptNo         = GetToken (STRING_TOKEN (CONFIRM_OPTION_NO), gHiiHandle);
  gConfirmOptOk         = GetToken (STRING_TOKEN (CONFIRM_OPTION_OK), gHiiHandle);
  gConfirmOptCancel     = GetToken (STRING_TOKEN (CONFIRM_OPTION_CANCEL), gHiiHandle);
  gYesOption            = GetToken (STRING_TOKEN (YES_SELECTABLE_OPTION), gHiiHandle);
  gNoOption             = GetToken (STRING_TOKEN (NO_SELECTABLE_OPTION), gHiiHandle);
  gOkOption             = GetToken (STRING_TOKEN (OK_SELECTABLE_OPTION), gHiiHandle);
  gCancelOption         = GetToken (STRING_TOKEN (CANCEL_SELECTABLE_OPTION), gHiiHandle);
  gErrorPopup           = GetToken (STRING_TOKEN (ERROR_POPUP_STRING), gHiiHandle);
  gWarningPopup         = GetToken (STRING_TOKEN (WARNING_POPUP_STRING), gHiiHandle);
  gInfoPopup            = GetToken (STRING_TOKEN (INFO_POPUP_STRING), gHiiHandle);
  gConfirmMsgConnect    = GetToken (STRING_TOKEN (CONFIRM_OPTION_CONNECT), gHiiHandle);
  gConfirmMsgEnd        = GetToken (STRING_TOKEN (CONFIRM_OPTION_END), gHiiHandle);
  gPasswordUnsupported  = GetToken (STRING_TOKEN (PASSWORD_NOT_SUPPORTED ), gHiiHandle);
}

/**
  Free up the resource allocated for all strings required
  by Setup Browser.

**/
VOID
FreeDisplayStrings (
  VOID
  )
{
  FreePool (mUnknownString);
  FreePool (gEmptyString);
  FreePool (gSaveFailed);
  FreePool (gNoSubmitIfFailed);
  FreePool (gReconnectFail);
  FreePool (gReconnectRequired);
  FreePool (gChangesOpt);
  FreePool (gReconnectConfirmChanges);
  FreePool (gSaveProcess);
  FreePool (gSaveNoSubmitProcess);
  FreePool (gDiscardChange);
  FreePool (gJumpToFormSet);
  FreePool (gCheckError);
  FreePool (gPromptForData);
  FreePool (gPromptForPassword);
  FreePool (gPromptForNewPassword);
  FreePool (gConfirmPassword);
  FreePool (gConfirmError);
  FreePool (gPassowordInvalid);
  FreePool (gPressEnter);
  FreePool (gMiniString);
  FreePool (gOptionMismatch);
  FreePool (gFormSuppress);
  FreePool (gProtocolNotFound);
  FreePool (gBrowserError);
  FreePool (gNoSubmitIf);
  FreePool (gFormNotFound);
  FreePool (gConfirmDefaultMsg);
  FreePool (gConfirmSubmitMsg);
  FreePool (gConfirmDiscardMsg);
  FreePool (gConfirmResetMsg);
  FreePool (gConfirmExitMsg);
  FreePool (gConfirmDefaultMsg2nd);
  FreePool (gConfirmSubmitMsg2nd);
  FreePool (gConfirmResetMsg2nd);
  FreePool (gConfirmExitMsg2nd);
  FreePool (gConfirmOpt);
  FreePool (gConfirmOptYes);
  FreePool (gConfirmOptNo);
  FreePool (gConfirmOptOk);
  FreePool (gConfirmOptCancel);
  FreePool (gYesOption);
  FreePool (gNoOption);
  FreePool (gOkOption);
  FreePool (gCancelOption);
  FreePool (gErrorPopup);
  FreePool (gWarningPopup);
  FreePool (gInfoPopup);
  FreePool (gConfirmMsgConnect);
  FreePool (gConfirmMsgEnd);
  FreePool (gPasswordUnsupported);
}

/**
  Get prompt string id from the opcode data buffer.

  @param  OpCode                 The input opcode buffer.

  @return The prompt string id.

**/
EFI_STRING_ID
GetPrompt (
  IN EFI_IFR_OP_HEADER     *OpCode
  )
{
  EFI_IFR_STATEMENT_HEADER  *Header;

  if (OpCode->Length <= sizeof (EFI_IFR_OP_HEADER)) {
    return 0;
  }

  Header = (EFI_IFR_STATEMENT_HEADER  *) (OpCode + 1);

  return Header->Prompt;
}

/**
  Get the supported width for a particular op-code

  @param  MenuOption             The menu option.
  @param  AdjustWidth            The width which is saved for the space.

  @return Returns the number of CHAR16 characters that is support.

**/
UINT16
GetWidth (
  IN  UI_MENU_OPTION     *MenuOption,
  OUT UINT16             *AdjustWidth
  )
{
  CHAR16                        *String;
  UINTN                         Size;
  EFI_IFR_TEXT                  *TestOp;
  UINT16                        ReturnWidth;
  FORM_DISPLAY_ENGINE_STATEMENT *Statement;

  Statement = MenuOption->ThisTag;

  //
  // For modal form, clean the entire row.
  //
  if ((gFormData->Attribute & HII_DISPLAY_MODAL) != 0) {
    if (AdjustWidth  != NULL) {
      *AdjustWidth = LEFT_SKIPPED_COLUMNS;
    }
    return (UINT16)(gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn - 2 * (gModalSkipColumn + LEFT_SKIPPED_COLUMNS));
  }

  Size = 0;

  //
  // See if the second text parameter is really NULL
  //
  if (Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) {
    TestOp = (EFI_IFR_TEXT *) Statement->OpCode;
    if (TestOp->TextTwo != 0) {
      String = GetToken (TestOp->TextTwo, gFormData->HiiHandle);
      Size   = StrLen (String);
      FreePool (String);
    }
  }

  if ((Statement->OpCode->OpCode == EFI_IFR_SUBTITLE_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_REF_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_PASSWORD_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_ACTION_OP) ||
      (Statement->OpCode->OpCode == EFI_IFR_RESET_BUTTON_OP) ||
      //
      // Allow a wide display if text op-code and no secondary text op-code
      //
      ((Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) && (Size == 0))
      ) {
    
    //
    // Return the space width.
    // 
    if (AdjustWidth != NULL) {
      *AdjustWidth = 2;
    }
    //
    // Keep consistent with current behavior.
    //
    ReturnWidth = (UINT16) (gPromptBlockWidth + gOptionBlockWidth - 2);
  } else {
    if (AdjustWidth != NULL) {
      *AdjustWidth = 1;
    }

    ReturnWidth =  (UINT16) (gPromptBlockWidth - 1);
  }

  //
  // For nest in statement, should the subtitle indent.
  //
  if (MenuOption->NestInStatement) {
    ReturnWidth -= SUBTITLE_INDENT;
  }

  return ReturnWidth;
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
  *OutputString = AllocateZeroPool ((StrOffset + 2) * sizeof(CHAR16));
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
  } else if (InputString[*Index + StrOffset] == CHAR_LINEFEED) {
    //
    // Skip the /n or /n/r info.
    //
    if (InputString[*Index + StrOffset + 1] == CHAR_CARRIAGE_RETURN) {
      *Index = (UINT16) (*Index + StrOffset + 2);
    } else {
      *Index = (UINT16) (*Index + StrOffset + 1);
    }
  } else if (InputString[*Index + StrOffset] == CHAR_CARRIAGE_RETURN) {
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
  Add one menu option by specified description and context.

  @param  Statement              Statement of this Menu Option.
  @param  MenuItemCount          The index for this Option in the Menu.
  @param  NestIn                 Whether this statement is nest in another statement.

**/
VOID
UiAddMenuOption (
  IN FORM_DISPLAY_ENGINE_STATEMENT   *Statement,
  IN UINT16                          *MenuItemCount,
  IN BOOLEAN                         NestIn
  )
{
  UI_MENU_OPTION   *MenuOption;
  UINTN            Index;
  UINTN            Count;
  UINT16           NumberOfLines;
  UINT16           GlyphWidth;
  UINT16           Width;
  UINTN            ArrayEntry;
  CHAR16           *OutputString;
  EFI_STRING_ID    PromptId;

  NumberOfLines = 1;
  ArrayEntry    = 0;
  GlyphWidth    = 1;
  Count         = 1;
  MenuOption    = NULL;

  PromptId = GetPrompt (Statement->OpCode);
  ASSERT (PromptId != 0);

  if (Statement->OpCode->OpCode == EFI_IFR_DATE_OP || Statement->OpCode->OpCode == EFI_IFR_TIME_OP) {
    Count = 3;
  }

  for (Index = 0; Index < Count; Index++) {
    MenuOption = AllocateZeroPool (sizeof (UI_MENU_OPTION));
    ASSERT (MenuOption);

    MenuOption->Signature   = UI_MENU_OPTION_SIGNATURE;
    MenuOption->Description = GetToken (PromptId, gFormData->HiiHandle);
    MenuOption->Handle      = gFormData->HiiHandle;
    MenuOption->ThisTag     = Statement;
    MenuOption->NestInStatement = NestIn;
    MenuOption->EntryNumber = *MenuItemCount;

    MenuOption->Sequence = Index;

    if ((Statement->Attribute & HII_DISPLAY_GRAYOUT) != 0) {
      MenuOption->GrayOut = TRUE;
    } else {
      MenuOption->GrayOut = FALSE;
    }

    if ((Statement->Attribute & HII_DISPLAY_LOCK) != 0 || (gFormData->Attribute & HII_DISPLAY_LOCK) != 0) {
      MenuOption->GrayOut = TRUE;
    }

    //
    // If the form or the question has the lock attribute, deal same as grayout.
    //
    if ((gFormData->Attribute & HII_DISPLAY_LOCK) != 0 || (Statement->Attribute & HII_DISPLAY_LOCK) != 0) {
      MenuOption->GrayOut = TRUE;
    }

    switch (Statement->OpCode->OpCode) {
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
      break;
    default:
      MenuOption->IsQuestion = FALSE;
      break;
    }

    if ((Statement->Attribute & HII_DISPLAY_READONLY) != 0) {
      MenuOption->ReadOnly = TRUE;
      if (FeaturePcdGet (PcdBrowerGrayOutReadOnlyMenu)) {
        MenuOption->GrayOut = TRUE;
      }
    }

    if (Index == 0 && 
      (Statement->OpCode->OpCode != EFI_IFR_DATE_OP) && 
      (Statement->OpCode->OpCode != EFI_IFR_TIME_OP)) {
      Width  = GetWidth (MenuOption, NULL);
      for (; GetLineByWidth (MenuOption->Description, Width, &GlyphWidth,&ArrayEntry, &OutputString) != 0x0000;) {
        //
        // If there is more string to process print on the next row and increment the Skip value
        //
        if (StrLen (&MenuOption->Description[ArrayEntry]) != 0) {
          NumberOfLines++;
        }
        FreePool (OutputString);
      }
    } else {
      //
      // Add three MenuOptions for Date/Time
      // Data format :      [01/02/2004]      [11:22:33]
      // Line number :        0  0    1         0  0  1
      //    
      NumberOfLines = 0;
    }

    if (Index == 2) {
      //
      // Override LineNumber for the MenuOption in Date/Time sequence
      //
      MenuOption->Skip = 1;
    } else {
      MenuOption->Skip = NumberOfLines;
    }

    InsertTailList (&gMenuOption, &MenuOption->Link);
  }

  (*MenuItemCount)++;
}

/**
  Create the menu list base on the form data info.

**/
VOID
ConvertStatementToMenu (
  VOID
  )
{
  UINT16                        MenuItemCount;
  LIST_ENTRY                    *Link;
  LIST_ENTRY                    *NestLink;
  FORM_DISPLAY_ENGINE_STATEMENT *Statement;
  FORM_DISPLAY_ENGINE_STATEMENT *NestStatement;

  MenuItemCount = 0;
  InitializeListHead (&gMenuOption);

  Link = GetFirstNode (&gFormData->StatementListHead);
  while (!IsNull (&gFormData->StatementListHead, Link)) {
    Statement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (Link);
    Link = GetNextNode (&gFormData->StatementListHead, Link);

    //
    // Skip the opcode not recognized by Display core.
    //
    if (Statement->OpCode->OpCode == EFI_IFR_GUID_OP) {
      continue;
    }

    UiAddMenuOption (Statement, &MenuItemCount, FALSE);

    //
    // Check the statement nest in this host statement.
    //
    NestLink = GetFirstNode (&Statement->NestStatementList);
    while (!IsNull (&Statement->NestStatementList, NestLink)) {
      NestStatement = FORM_DISPLAY_ENGINE_STATEMENT_FROM_LINK (NestLink);
      NestLink = GetNextNode (&Statement->NestStatementList, NestLink);

      //
      // Skip the opcode not recognized by Display core.
      //
      if (NestStatement->OpCode->OpCode == EFI_IFR_GUID_OP) {
        continue;
      }

      UiAddMenuOption (NestStatement, &MenuItemCount, TRUE);
    }
  }
}

/**
  Count the storage space of a Unicode string.

  This function handles the Unicode string with NARROW_CHAR
  and WIDE_CHAR control characters. NARROW_HCAR and WIDE_CHAR
  does not count in the resultant output. If a WIDE_CHAR is
  hit, then 2 Unicode character will consume an output storage
  space with size of CHAR16 till a NARROW_CHAR is hit.

  If String is NULL, then ASSERT ().

  @param String          The input string to be counted.

  @return Storage space for the input string.

**/
UINTN
GetStringWidth (
  IN CHAR16               *String
  )
{
  UINTN Index;
  UINTN Count;
  UINTN IncrementValue;

  ASSERT (String != NULL);
  if (String == NULL) {
    return 0;
  }

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
  Base on the input option string to update the skip value for a menu option.

  @param  MenuOption             The MenuOption to be checked.
  @param  OptionString           The input option string.

**/
VOID
UpdateSkipInfoForMenu (
  IN UI_MENU_OPTION               *MenuOption,
  IN CHAR16                       *OptionString
  )
{
  UINTN   Index;
  UINT16  Width;
  UINTN   Row;
  CHAR16  *OutputString;
  UINT16  GlyphWidth;

  Width         = (UINT16) gOptionBlockWidth - 1;
  GlyphWidth    = 1;
  Row           = 1;

  for (Index = 0; GetLineByWidth (OptionString, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
    if (StrLen (&OptionString[Index]) != 0) {
      Row++;
    }

    FreePool (OutputString);
  }

  if ((Row > MenuOption->Skip) && 
      (MenuOption->ThisTag->OpCode->OpCode != EFI_IFR_DATE_OP) && 
      (MenuOption->ThisTag->OpCode->OpCode != EFI_IFR_TIME_OP)) {
    MenuOption->Skip = Row;
  }
}

/**
  Update display lines for a Menu Option.

  @param  MenuOption             The MenuOption to be checked.

**/
VOID
UpdateOptionSkipLines (
  IN UI_MENU_OPTION               *MenuOption
  )
{
  CHAR16  *OptionString;

  OptionString  = NULL;

  ProcessOptions (MenuOption, FALSE, &OptionString, TRUE);
  if (OptionString != NULL) {
    UpdateSkipInfoForMenu (MenuOption, OptionString);

    FreePool (OptionString);
  }

  if ((MenuOption->ThisTag->OpCode->OpCode  == EFI_IFR_TEXT_OP) && (((EFI_IFR_TEXT*)MenuOption->ThisTag->OpCode)->TextTwo != 0)) {
    OptionString   = GetToken (((EFI_IFR_TEXT*)MenuOption->ThisTag->OpCode)->TextTwo, gFormData->HiiHandle);

    if (OptionString != NULL) {
      UpdateSkipInfoForMenu (MenuOption, OptionString);

      FreePool (OptionString);
    }
  }
}

/**
  Check whether this Menu Option could be print.

  Check Prompt string, option string or text two string not NULL.

  This is an internal function.

  @param  MenuOption             The MenuOption to be checked.

  @retval TRUE                   This Menu Option is printable.
  @retval FALSE                  This Menu Option could not be printable.

**/
BOOLEAN
PrintableMenu (
  UI_MENU_OPTION   *MenuOption
  )
{
  EFI_STATUS    Status;
  EFI_STRING    OptionString;

  OptionString = NULL;

  if (MenuOption->Description[0] != '\0') {
    return TRUE;
  }

  Status = ProcessOptions (MenuOption, FALSE, &OptionString, FALSE);
  if (EFI_ERROR (Status)) {
    return FALSE;
  }
  if (OptionString != NULL && OptionString[0] != '\0') {
    FreePool (OptionString);
    return TRUE;
  }

  if ((MenuOption->ThisTag->OpCode->OpCode  == EFI_IFR_TEXT_OP) && (((EFI_IFR_TEXT*)MenuOption->ThisTag->OpCode)->TextTwo != 0)) {
    OptionString   = GetToken (((EFI_IFR_TEXT*)MenuOption->ThisTag->OpCode)->TextTwo, gFormData->HiiHandle);
    ASSERT (OptionString != NULL);
    if (OptionString[0] != '\0'){
      FreePool (OptionString);
      return TRUE;
    }
  }

  return FALSE;
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
  if ((MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_SUBTITLE_OP) ||
      MenuOption->GrayOut || MenuOption->ReadOnly || !PrintableMenu (MenuOption)) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/**
  Move to next selectable statement.

  This is an internal function.

  @param  GoUp                   The navigation direction. TRUE: up, FALSE: down.
  @param  CurrentPosition        Current position.
  @param  GapToTop               Gap position to top or bottom.
  @param  FindInForm             Whether find menu in current form or beyond.

  @return The row distance from current MenuOption to next selectable MenuOption.

  @retval -1       Reach the begin of the menu, still can't find the selectable menu.
  @retval Value    Find the selectable menu, maybe the truly selectable, maybe the 
                   first menu showing beyond current form or last menu showing in 
                   current form.
                   The value is the line number between the new selected menu and the
                   current select menu, not include the new selected menu.

**/
INTN
MoveToNextStatement (
  IN     BOOLEAN                   GoUp,
  IN OUT LIST_ENTRY                **CurrentPosition,
  IN     UINTN                     GapToTop,
  IN     BOOLEAN                   FindInForm
  )
{
  INTN             Distance;
  LIST_ENTRY       *Pos;
  UI_MENU_OPTION   *NextMenuOption;
  UI_MENU_OPTION   *PreMenuOption;

  Distance      = 0;
  Pos           = *CurrentPosition;

  if (Pos == &gMenuOption) {
    return -1;
  }

  PreMenuOption = MENU_OPTION_FROM_LINK (Pos);

  while (TRUE) {
    NextMenuOption = MENU_OPTION_FROM_LINK (Pos);
    //
    // NextMenuOption->Row == 0 means this menu has not calculate
    // the NextMenuOption->Skip value yet, just calculate here.
    //
    if (NextMenuOption->Row == 0) {
      UpdateOptionSkipLines (NextMenuOption);
    }

    //
    // Check whether the menu is beyond current showing form,
    // return the first one beyond the showing form.
    //
    if ((UINTN) Distance + NextMenuOption->Skip > GapToTop) {
      if (FindInForm) {
        NextMenuOption = PreMenuOption;
      }
      break;
    }

    //
    // return the selectable menu in the showing form.
    //
    if (IsSelectable (NextMenuOption)) {
      break;
    }

    Distance += NextMenuOption->Skip;

    //
    // Arrive at begin of the menu list.
    //
    if ((GoUp ? Pos->BackLink : Pos->ForwardLink) == &gMenuOption) {
      Distance = -1;
      break;
    }

    Pos = (GoUp ? Pos->BackLink : Pos->ForwardLink);
    PreMenuOption = NextMenuOption;
  }

  *CurrentPosition = &NextMenuOption->Link;
  return Distance;
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
  FORM_DISPLAY_ENGINE_STATEMENT          *Statement;
  EFI_IFR_DATE                           *Date;
  EFI_IFR_TIME                           *Time;

  ASSERT (MenuOption != NULL && OptionString != NULL);
  
  Statement = MenuOption->ThisTag;
  Date      = NULL;
  Time      = NULL;
  if (Statement->OpCode->OpCode == EFI_IFR_DATE_OP) {
    Date = (EFI_IFR_DATE *) Statement->OpCode;
  } else if (Statement->OpCode->OpCode == EFI_IFR_TIME_OP) {
    Time = (EFI_IFR_TIME *) Statement->OpCode;
  }
  
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
  if (Statement->OpCode->OpCode == EFI_IFR_DATE_OP) {
    //
    // OptionString format is: <**:  **: ****>
    //                        |month|day|year|
    //                          4     3    5
    //
    if ((Date->Flags & EFI_QF_DATE_MONTH_SUPPRESS) && (MenuOption->Sequence == 0)) {
      //
      // At this point, only "<**:" in the optionstring. 
      // Clean the day's ** field, after clean, the format is "<  :"
      //
      SetUnicodeMem (&OptionString[1], 2, L' ');
    } else if ((Date->Flags & EFI_QF_DATE_DAY_SUPPRESS) && (MenuOption->Sequence == 1)) {
      //
      // At this point, only "**:" in the optionstring. 
      // Clean the month's "**" field, after clean, the format is "  :"
      //                
      SetUnicodeMem (&OptionString[0], 2, L' ');
    } else if ((Date->Flags & EFI_QF_DATE_YEAR_SUPPRESS) && (MenuOption->Sequence == 2)) {
      //
      // At this point, only "****>" in the optionstring. 
      // Clean the year's "****" field, after clean, the format is "  >"
      //                
      SetUnicodeMem (&OptionString[0], 4, L' ');
    }
  } else if (Statement->OpCode->OpCode == EFI_IFR_TIME_OP) {
    //
    // OptionString format is: <**:  **:    **>
    //                        |hour|minute|second|
    //                          4     3      3
    //
    if ((Time->Flags & QF_TIME_HOUR_SUPPRESS) && (MenuOption->Sequence == 0)) {
      //
      // At this point, only "<**:" in the optionstring. 
      // Clean the hour's ** field, after clean, the format is "<  :"
      //
      SetUnicodeMem (&OptionString[1], 2, L' ');
    } else if ((Time->Flags & QF_TIME_MINUTE_SUPPRESS) && (MenuOption->Sequence == 1)) {
      //
      // At this point, only "**:" in the optionstring. 
      // Clean the minute's "**" field, after clean, the format is "  :"
      //                
      SetUnicodeMem (&OptionString[0], 2, L' ');
    } else if ((Time->Flags & QF_TIME_SECOND_SUPPRESS) && (MenuOption->Sequence == 2)) {
      //
      // At this point, only "**>" in the optionstring. 
      // Clean the second's "**" field, after clean, the format is "  >"
      //                
      SetUnicodeMem (&OptionString[0], 2, L' ');
    }
  }
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

  if ((MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_DATE_OP) ||
      (MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_TIME_OP)) {
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
  Get step info from numeric opcode.
  
  @param[in] OpCode     The input numeric op code.

  @return step info for this opcode.
**/
UINT64
GetFieldFromNum (
  IN  EFI_IFR_OP_HEADER     *OpCode
  )
{
  EFI_IFR_NUMERIC       *NumericOp;
  UINT64                Step;

  NumericOp = (EFI_IFR_NUMERIC *) OpCode;
  
  switch (NumericOp->Flags & EFI_IFR_NUMERIC_SIZE) {
  case EFI_IFR_NUMERIC_SIZE_1:
    Step    = NumericOp->data.u8.Step;
    break;
  
  case EFI_IFR_NUMERIC_SIZE_2:
    Step    = NumericOp->data.u16.Step;
    break;
  
  case EFI_IFR_NUMERIC_SIZE_4:
    Step    = NumericOp->data.u32.Step;
    break;
  
  case EFI_IFR_NUMERIC_SIZE_8:
    Step    = NumericOp->data.u64.Step;
    break;
  
  default:
    Step = 0;
    break;
  }

  return Step;
}

/**
  Find the registered HotKey based on KeyData.
  
  @param[in] KeyData     A pointer to a buffer that describes the keystroke
                         information for the hot key.

  @return The registered HotKey context. If no found, NULL will return.
**/
BROWSER_HOT_KEY *
GetHotKeyFromRegisterList (
  IN EFI_INPUT_KEY *KeyData
  )
{
  LIST_ENTRY       *Link;
  BROWSER_HOT_KEY  *HotKey;

  Link = GetFirstNode (&gFormData->HotKeyListHead);
  while (!IsNull (&gFormData->HotKeyListHead, Link)) {
    HotKey = BROWSER_HOT_KEY_FROM_LINK (Link);
    
    if (HotKey->KeyData->ScanCode == KeyData->ScanCode) {
      return HotKey;
    }

    Link = GetNextNode (&gFormData->HotKeyListHead, Link);
  }
  
  return NULL;
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
  Wait for a given event to fire, or for an optional timeout to expire.

  @param  Event                  The event to wait for

  @retval UI_EVENT_TYPE          The type of the event which is trigged.

**/
UI_EVENT_TYPE
UiWaitForEvent (
  IN EFI_EVENT                Event
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  UINTN       EventNum;
  UINT64      Timeout;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[3];
  UI_EVENT_TYPE  EventType;

  TimerEvent = NULL;
  Timeout    = FormExitTimeout(gFormData);

  if (Timeout != 0) {
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);

    //
    // Set the timer event
    //
    gBS->SetTimer (
          TimerEvent,
          TimerRelative,
          Timeout
          );
  }
  
  WaitList[0] = Event;
  EventNum    = 1;
  if (gFormData->FormRefreshEvent != NULL) {
    WaitList[EventNum] = gFormData->FormRefreshEvent;
    EventNum ++;
  } 

  if (Timeout != 0) {
    WaitList[EventNum] = TimerEvent;
    EventNum ++;
  }

  Status = gBS->WaitForEvent (EventNum, WaitList, &Index);
  ASSERT_EFI_ERROR (Status);

  switch (Index) {
  case 0:
   EventType = UIEventKey;
   break;

  case 1:
    if (gFormData->FormRefreshEvent != NULL) {
      EventType = UIEventDriver;
    } else {
      ASSERT (Timeout != 0 && EventNum == 2);
      EventType = UIEventTimeOut;
    }
    break;

  default:
    ASSERT (Index == 2 && EventNum == 3);
    EventType = UIEventTimeOut;
    break;
  }

  if (Timeout != 0) {
    gBS->CloseEvent (TimerEvent);
  }
  
  return EventType;
}

/**
  Get question id info from the input opcode header.

  @param  OpCode                 The input opcode header pointer.

  @retval                        The question id for this opcode.

**/
EFI_QUESTION_ID
GetQuestionIdInfo (
  IN   EFI_IFR_OP_HEADER     *OpCode
  )
{
  EFI_IFR_QUESTION_HEADER   *QuestionHeader;

  if (OpCode->Length < sizeof (EFI_IFR_OP_HEADER) + sizeof (EFI_IFR_QUESTION_HEADER)) {
    return 0;
  }

  QuestionHeader = (EFI_IFR_QUESTION_HEADER *)((UINT8 *) OpCode + sizeof(EFI_IFR_OP_HEADER));

  return QuestionHeader->QuestionId;
}


/**
  Find the top of screen menu base on the current menu.

  @param  CurPos                 Current input menu.
  @param  Rows                   Totol screen rows.
  @param  SkipValue              SkipValue for this new form.

  @retval TopOfScreen            Top of screen menu for the new form.

**/
LIST_ENTRY *
FindTopOfScreenMenu (
  IN  LIST_ENTRY                      *CurPos,
  IN  UINTN                           Rows,
  OUT UINTN                           *SkipValue
  )
{
  LIST_ENTRY        *Link;
  LIST_ENTRY        *TopOfScreen;
  UI_MENU_OPTION    *PreviousMenuOption;

  Link = CurPos;
  PreviousMenuOption = NULL;

  while (Link->BackLink != &gMenuOption) {
    Link = Link->BackLink;
    PreviousMenuOption = MENU_OPTION_FROM_LINK (Link);
    if (PreviousMenuOption->Row == 0) {
      UpdateOptionSkipLines (PreviousMenuOption);
    }
    if (Rows <= PreviousMenuOption->Skip) {
      break;
    }
    Rows = Rows - PreviousMenuOption->Skip;
  }

  if (Link->BackLink == &gMenuOption) {
    TopOfScreen = gMenuOption.ForwardLink;
    if (PreviousMenuOption != NULL && Rows < PreviousMenuOption->Skip) {
      *SkipValue = PreviousMenuOption->Skip - Rows;
    } else {
      *SkipValue = 0;
    }
  } else {
    TopOfScreen = Link;
    *SkipValue = PreviousMenuOption->Skip - Rows;
  }

  return TopOfScreen;
}

/**
  Get the index info for this opcode.

  @param  OpCode      The input opcode for the statement.

  @retval  The index of this statement.

**/
UINTN
GetIndexInfoForOpcode (
  IN EFI_IFR_OP_HEADER  *OpCode
  )
{
  LIST_ENTRY                      *NewPos;
  UI_MENU_OPTION                  *MenuOption;
  UINTN                           Index;

  NewPos = gMenuOption.ForwardLink;
  Index  = 0;

  for (NewPos = gMenuOption.ForwardLink; NewPos != &gMenuOption; NewPos = NewPos->ForwardLink){
    MenuOption = MENU_OPTION_FROM_LINK (NewPos);

    if (CompareMem (MenuOption->ThisTag->OpCode, OpCode, OpCode->Length) == 0) {
      if (MenuOption->ThisTag->OpCode == OpCode) {
        return Index;
      }

      Index ++;
    }
  }

  return Index;
}

/**
  Is this the saved highlight statement.

  @param  HighLightedStatement      The input highlight statement.

  @retval  TRUE   This is the highlight statement.
  @retval  FALSE  This is not the highlight statement.

**/
BOOLEAN 
IsSavedHighlightStatement (
  IN FORM_DISPLAY_ENGINE_STATEMENT  *HighLightedStatement
  )
{
  if ((gFormData->HiiHandle == gHighligthMenuInfo.HiiHandle) &&
      (gFormData->FormId == gHighligthMenuInfo.FormId)) {
    if (gHighligthMenuInfo.HLTQuestionId != 0) {
      return (BOOLEAN) (gHighligthMenuInfo.HLTQuestionId == GetQuestionIdInfo (HighLightedStatement->OpCode));
    } else {
      if (CompareMem (gHighligthMenuInfo.HLTOpCode, HighLightedStatement->OpCode, gHighligthMenuInfo.HLTOpCode->Length) == 0) {
        if (gHighligthMenuInfo.HLTIndex == 0 || gHighligthMenuInfo.HLTIndex == GetIndexInfoForOpcode(HighLightedStatement->OpCode)) {
          return TRUE;
        } else {
          return FALSE;
        }
      }
    }
  }

  return FALSE;
}

/**
  Is this the highlight menu.

  @param  MenuOption      The input Menu option.

  @retval  TRUE   This is the highlight menu option.
  @retval  FALSE  This is not the highlight menu option.

**/
BOOLEAN
IsHighLightMenuOption (
  IN UI_MENU_OPTION     *MenuOption
  )
{
  if (gHighligthMenuInfo.HLTQuestionId != 0) {
    if (GetQuestionIdInfo(MenuOption->ThisTag->OpCode) == gHighligthMenuInfo.HLTQuestionId) {
      return (BOOLEAN) (MenuOption->Sequence == gHighligthMenuInfo.HLTSequence);
    }
  } else {
    if(CompareMem (gHighligthMenuInfo.HLTOpCode, MenuOption->ThisTag->OpCode, gHighligthMenuInfo.HLTOpCode->Length) == 0) {
      if (gHighligthMenuInfo.HLTIndex == 0 || gHighligthMenuInfo.HLTIndex == GetIndexInfoForOpcode(MenuOption->ThisTag->OpCode)) {
        return (BOOLEAN) (MenuOption->Sequence == gHighligthMenuInfo.HLTSequence);
      } else {
        return FALSE;
      }
    }
  }

  return FALSE;
}

/**
  Find the highlight menu.

  If the input is NULL, base on the record highlight info in
  gHighligthMenuInfo to find the last highlight menu.

  @param  HighLightedStatement      The input highlight statement.

  @retval  The highlight menu index.

**/
LIST_ENTRY *
FindHighLightMenuOption (
 IN FORM_DISPLAY_ENGINE_STATEMENT  *HighLightedStatement
 )
{
  LIST_ENTRY                      *NewPos;
  UI_MENU_OPTION                  *MenuOption;

  NewPos = gMenuOption.ForwardLink;
  MenuOption = MENU_OPTION_FROM_LINK (NewPos);

  if (HighLightedStatement != NULL) {
    while (MenuOption->ThisTag != HighLightedStatement) {
      NewPos     = NewPos->ForwardLink;
      if (NewPos == &gMenuOption) {
        //
        // Not Found it, break
        //
        break;
      }
      MenuOption = MENU_OPTION_FROM_LINK (NewPos);
    }

    //
    // Must find the highlight statement.
    //
    ASSERT (NewPos != &gMenuOption);

  } else {
    while (!IsHighLightMenuOption (MenuOption)) {
      NewPos     = NewPos->ForwardLink;
      if (NewPos == &gMenuOption) {
        //
        // Not Found it, break
        //
        break;
      }
      MenuOption = MENU_OPTION_FROM_LINK (NewPos);
    }

    //
    // Highlight statement has disappear (suppressed/disableed)
    //
    if (NewPos == &gMenuOption) {
      NewPos = NULL;
    }
  }

  return NewPos;
}

/**
  Is this the Top of screen menu.

  @param  MenuOption      The input Menu option.

  @retval  TRUE   This is the Top of screen menu option.
  @retval  FALSE  This is not the Top of screen menu option.

**/
BOOLEAN
IsTopOfScreeMenuOption (
  IN UI_MENU_OPTION     *MenuOption
  )
{
  if (gHighligthMenuInfo.TOSQuestionId != 0) {
    return (BOOLEAN) (GetQuestionIdInfo(MenuOption->ThisTag->OpCode) == gHighligthMenuInfo.TOSQuestionId);
  } 

  if(CompareMem (gHighligthMenuInfo.TOSOpCode, MenuOption->ThisTag->OpCode, gHighligthMenuInfo.TOSOpCode->Length) == 0) {
    if (gHighligthMenuInfo.TOSIndex == 0 || gHighligthMenuInfo.TOSIndex == GetIndexInfoForOpcode(MenuOption->ThisTag->OpCode)) {
      return TRUE;
    } else {
      return FALSE;
    }
  }

  return FALSE;
}

/**
  Calculate the distance between two menus and include the skip value of StartMenu.

  @param  StartMenu             The link_entry pointer to start menu.
  @param  EndMenu               The link_entry pointer to end menu.

**/
UINTN
GetDistanceBetweenMenus(
  IN LIST_ENTRY  *StartMenu,
  IN LIST_ENTRY  *EndMenu
)
{
  LIST_ENTRY                 *Link;
  UI_MENU_OPTION             *MenuOption;
  UINTN                      Distance;

  Distance = 0;

  Link = StartMenu;
  while (Link != EndMenu) {
    MenuOption = MENU_OPTION_FROM_LINK (Link);
    if (MenuOption->Row == 0) {
      UpdateOptionSkipLines (MenuOption);
    }
    Distance += MenuOption->Skip;
    Link = Link->BackLink;
  }
  return Distance;
}

/**
  Find the top of screen menu base on the previous record menu info.

  @param  HighLightMenu      The link_entry pointer to highlight menu.

  @retval  Return the the link_entry pointer top of screen menu.

**/
LIST_ENTRY *
FindTopOfScreenMenuOption (
  IN LIST_ENTRY                   *HighLightMenu
  )
{
  LIST_ENTRY                      *NewPos;
  UI_MENU_OPTION                  *MenuOption;
  UINTN                           TopRow;
  UINTN                           BottomRow;

  TopRow    = gStatementDimensions.TopRow    + SCROLL_ARROW_HEIGHT;
  BottomRow = gStatementDimensions.BottomRow - SCROLL_ARROW_HEIGHT;

  NewPos = gMenuOption.ForwardLink;
  MenuOption = MENU_OPTION_FROM_LINK (NewPos);

  while (!IsTopOfScreeMenuOption(MenuOption)) {
    NewPos     = NewPos->ForwardLink;
    if (NewPos == &gMenuOption) {
      //
      // Not Found it, break
      //
      break;
    }
    MenuOption = MENU_OPTION_FROM_LINK (NewPos);
  }

  //
  // Last time top of screen menu has disappeared.
  //
  if (NewPos == &gMenuOption) {
    return NULL;
  }
  //
  // Check whether highlight menu and top of screen menu can be shown within one page,
  // if can't, return NULL to re-calcaulate the top of scrren menu. Because some new menus
  // may be dynamically inserted between highlightmenu and previous top of screen menu,
  // So previous record top of screen menu is not appropriate for current display.
  //
  if (GetDistanceBetweenMenus (HighLightMenu, NewPos) + 1 > BottomRow - TopRow) {
    return NULL;
  }

  return NewPos;
}

/**
  Find the first menu which will be show at the top.

  @param  FormData               The data info for this form.
  @param  TopOfScreen            The link_entry pointer to top menu.
  @param  HighlightMenu          The menu which will be highlight.
  @param  SkipValue              The skip value for the top menu.

**/
VOID
FindTopMenu (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT LIST_ENTRY                **TopOfScreen,
  OUT LIST_ENTRY                **HighlightMenu,
  OUT UINTN                     *SkipValue
  )
{
  UINTN                           TopRow;
  UINTN                           BottomRow;
  UI_MENU_OPTION                  *MenuOption;
  UINTN                           TmpValue;

  TopRow    = gStatementDimensions.TopRow    + SCROLL_ARROW_HEIGHT;
  BottomRow = gStatementDimensions.BottomRow - SCROLL_ARROW_HEIGHT;
  //
  // When option mismatch happens,there exist two cases,one is reenter the form, just like the if case below,
  // and the other is exit current form and enter last form, it can be covered by the else case.
  //
  if (gMisMatch && gFormData->HiiHandle == gHighligthMenuInfo.HiiHandle && gFormData->FormId == gHighligthMenuInfo.FormId) {
    //
    // Reenter caused by option mismatch or auto exit caused by refresh form(refresh interval/guid), 
    // base on the record highlight info to find the highlight menu.
    //

    *HighlightMenu = FindHighLightMenuOption(NULL);
    if (*HighlightMenu != NULL) {
      //
      // Update skip info for this highlight menu.
      //
      MenuOption = MENU_OPTION_FROM_LINK (*HighlightMenu);
      UpdateOptionSkipLines (MenuOption);

      //
      // Found the last time highlight menu.
      //
      *TopOfScreen = FindTopOfScreenMenuOption(*HighlightMenu);
      if (*TopOfScreen != NULL) {
        //
        // Found the last time selectable top of screen menu.
        //
        AdjustDateAndTimePosition(TRUE, TopOfScreen);
        MenuOption = MENU_OPTION_FROM_LINK (*TopOfScreen);
        UpdateOptionSkipLines (MenuOption);

        *SkipValue = gHighligthMenuInfo.SkipValue;
      } else {
        //
        // Not found last time top of screen menu, so base on current highlight menu
        // to find the new top of screen menu.
        // Make the current highlight menu at the bottom of the form to calculate the
        // top of screen menu.
        //
        if (MenuOption->Skip >= BottomRow - TopRow) {
          *TopOfScreen = *HighlightMenu;
          TmpValue     = 0;
        } else {
          *TopOfScreen = FindTopOfScreenMenu(*HighlightMenu, BottomRow - TopRow - MenuOption->Skip, &TmpValue);
        }

        *SkipValue   = TmpValue;
      }
    } else {
      //
      // Last time highlight menu has disappear, find the first highlightable menu as the default one.
      //
      *HighlightMenu = gMenuOption.ForwardLink;
      if (!IsListEmpty (&gMenuOption)) {
        MoveToNextStatement (FALSE, HighlightMenu, BottomRow - TopRow, TRUE);
      }
      *TopOfScreen   = gMenuOption.ForwardLink;
      *SkipValue = 0;
    }

  } else if (FormData->HighLightedStatement != NULL) {
    if (IsSavedHighlightStatement (FormData->HighLightedStatement)) {
      //
      // Input highlight menu is same as last time highlight menu.
      // Base on last time highlight menu to set the top of screen menu and highlight menu.
      //
      *HighlightMenu = FindHighLightMenuOption(NULL);
      ASSERT (*HighlightMenu != NULL);

      //
      // Update skip info for this highlight menu.
      //
      MenuOption = MENU_OPTION_FROM_LINK (*HighlightMenu);
      UpdateOptionSkipLines (MenuOption);
      
      *TopOfScreen = FindTopOfScreenMenuOption(*HighlightMenu);
      if (*TopOfScreen == NULL) {
        //
        // Not found last time top of screen menu, so base on current highlight menu
        // to find the new top of screen menu.
        // Make the current highlight menu at the bottom of the form to calculate the
        // top of screen menu.
        //
        if (MenuOption->Skip >= BottomRow - TopRow) {
          *TopOfScreen = *HighlightMenu;
          TmpValue     = 0;
        } else {
          *TopOfScreen = FindTopOfScreenMenu(*HighlightMenu, BottomRow - TopRow - MenuOption->Skip, &TmpValue);
        }

        *SkipValue   = TmpValue;
      } else {
        AdjustDateAndTimePosition(TRUE, TopOfScreen);
        MenuOption = MENU_OPTION_FROM_LINK (*TopOfScreen);
        UpdateOptionSkipLines (MenuOption);

        *SkipValue = gHighligthMenuInfo.SkipValue;
      }
      AdjustDateAndTimePosition(TRUE, TopOfScreen);
    } else {
      //
      // Input highlight menu is not save as last time highlight menu.
      //
      *HighlightMenu = FindHighLightMenuOption(FormData->HighLightedStatement);
      MenuOption = MENU_OPTION_FROM_LINK (*HighlightMenu);
      UpdateOptionSkipLines (MenuOption);

      //
      // Make the current highlight menu at the bottom of the form to calculate the
      // top of screen menu.
      //
      if (MenuOption->Skip >= BottomRow - TopRow) {
        *TopOfScreen = *HighlightMenu;
        TmpValue     = 0;
      } else {
        *TopOfScreen = FindTopOfScreenMenu(*HighlightMenu, BottomRow - TopRow - MenuOption->Skip, &TmpValue);
      }

      *SkipValue   = TmpValue;
    }
    AdjustDateAndTimePosition(TRUE, TopOfScreen);
  } else {
    //
    // If not has input highlight statement, just return the first one in this form.
    //
    *TopOfScreen   = gMenuOption.ForwardLink;
    *HighlightMenu = gMenuOption.ForwardLink;
    if (!IsListEmpty (&gMenuOption)) {
      MoveToNextStatement (FALSE, HighlightMenu, BottomRow - TopRow, TRUE);
    }
    *SkipValue     = 0;
  }

  gMisMatch = FALSE;

  //
  // First enter to show the menu, update highlight info.
  //
  UpdateHighlightMenuInfo (*HighlightMenu, *TopOfScreen, *SkipValue);
}

/**
  Record the highlight menu and top of screen menu info.

  @param  Highlight               The menu opton which is highlight.
  @param  TopOfScreen             The menu opton which is at the top of the form.
  @param  SkipValue               The skip line info for the top of screen menu.

**/
VOID
UpdateHighlightMenuInfo (
  IN  LIST_ENTRY                      *Highlight,
  IN  LIST_ENTRY                      *TopOfScreen,
  IN  UINTN                           SkipValue
  )
{
  UI_MENU_OPTION                  *MenuOption;
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement;

  gHighligthMenuInfo.HiiHandle  = gFormData->HiiHandle;
  gHighligthMenuInfo.FormId     = gFormData->FormId;
  gHighligthMenuInfo.SkipValue  = (UINT16)SkipValue;

  if (!IsListEmpty (&gMenuOption)) {
    MenuOption = MENU_OPTION_FROM_LINK (Highlight);
    Statement  = MenuOption->ThisTag;

    gUserInput->SelectedStatement = Statement;

    gHighligthMenuInfo.HLTSequence   = MenuOption->Sequence;
    gHighligthMenuInfo.HLTQuestionId = GetQuestionIdInfo(Statement->OpCode);
    if (gHighligthMenuInfo.HLTQuestionId == 0) {
      //
      // if question id == 0, save the opcode buffer..
      //
      if (gHighligthMenuInfo.HLTOpCode != NULL) {
        FreePool (gHighligthMenuInfo.HLTOpCode);
      }
      gHighligthMenuInfo.HLTOpCode = AllocateCopyPool (Statement->OpCode->Length, Statement->OpCode);
      ASSERT (gHighligthMenuInfo.HLTOpCode != NULL);

      gHighligthMenuInfo.HLTIndex = GetIndexInfoForOpcode(Statement->OpCode);
    }

    MenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);
    Statement  = MenuOption->ThisTag;

    gHighligthMenuInfo.TOSQuestionId = GetQuestionIdInfo(Statement->OpCode);
    if (gHighligthMenuInfo.TOSQuestionId == 0) {
      //
      // if question id == 0, save the opcode buffer..
      //
      if (gHighligthMenuInfo.TOSOpCode != NULL) {
        FreePool (gHighligthMenuInfo.TOSOpCode);
      }
      gHighligthMenuInfo.TOSOpCode = AllocateCopyPool (Statement->OpCode->Length, Statement->OpCode);
      ASSERT (gHighligthMenuInfo.TOSOpCode != NULL);

      gHighligthMenuInfo.TOSIndex = GetIndexInfoForOpcode(Statement->OpCode);
    }
  } else {
    gUserInput->SelectedStatement    = NULL;

    gHighligthMenuInfo.HLTSequence   = 0;
    gHighligthMenuInfo.HLTQuestionId = 0;
    if (gHighligthMenuInfo.HLTOpCode != NULL) {
      FreePool (gHighligthMenuInfo.HLTOpCode);
    }
    gHighligthMenuInfo.HLTOpCode     = NULL;
    gHighligthMenuInfo.HLTIndex      = 0;

    gHighligthMenuInfo.TOSQuestionId = 0;
    if (gHighligthMenuInfo.TOSOpCode != NULL) {
      FreePool (gHighligthMenuInfo.TOSOpCode);
    }
    gHighligthMenuInfo.TOSOpCode     = NULL;
    gHighligthMenuInfo.TOSIndex      = 0;
  }
}

/**
  Update attribut for this menu.

  @param  MenuOption               The menu opton which this attribut used to.
  @param  Highlight                Whether this menu will be highlight.

**/
VOID
SetDisplayAttribute (
  IN UI_MENU_OPTION                  *MenuOption,
  IN BOOLEAN                         Highlight
  )
{
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement;
  
  Statement = MenuOption->ThisTag;

  if (Highlight) {
    gST->ConOut->SetAttribute (gST->ConOut, GetHighlightTextColor ());
    return;
  }

  if (MenuOption->GrayOut) {
    gST->ConOut->SetAttribute (gST->ConOut, GetGrayedTextColor ());
  } else {
    if (Statement->OpCode->OpCode == EFI_IFR_SUBTITLE_OP) {
      gST->ConOut->SetAttribute (gST->ConOut, GetSubTitleTextColor ());
    } else {
      gST->ConOut->SetAttribute (gST->ConOut, GetFieldTextColor ());
    }
  }
}

/**
  Print string for this menu option.

  @param  MenuOption               The menu opton which this attribut used to.
  @param  Col                      The column that this string will be print at.
  @param  Row                      The row that this string will be print at.
  @param  String                   The string which need to print.
  @param  Width                    The width need to print, if string is less than the
                                   width, the block space will be used.
  @param  Highlight                Whether this menu will be highlight.

**/
VOID
DisplayMenuString (
  IN UI_MENU_OPTION         *MenuOption,
  IN UINTN                  Col,
  IN UINTN                  Row,
  IN CHAR16                 *String,
  IN UINTN                  Width,
  IN BOOLEAN                Highlight
  )
{
  UINTN            Length;

  //
  // Print string with normal color.
  //
  if (!Highlight) {
    PrintStringAtWithWidth (Col, Row, String, Width);
    return;
  }
  
  //
  // Print the highlight menu string.
  // First print the highlight string.
  // 
  SetDisplayAttribute(MenuOption, TRUE);
  Length = PrintStringAt (Col, Row, String);

  //
  // Second, clean the empty after the string.
  //
  SetDisplayAttribute(MenuOption, FALSE);
  PrintStringAtWithWidth (Col + Length, Row, L"", Width - Length);
}

/**
  Check whether this menu can has option string.

  @param  MenuOption               The menu opton which this attribut used to.

  @retval TRUE                     This menu option can have option string.
  @retval FALSE                    This menu option can't have option string.

**/
BOOLEAN 
HasOptionString (
  IN UI_MENU_OPTION                  *MenuOption
  )
{
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement;
  CHAR16                          *String;
  UINTN                           Size;
  EFI_IFR_TEXT                    *TestOp;

  Size = 0;
  Statement = MenuOption->ThisTag;

  //
  // See if the second text parameter is really NULL
  //
  if (Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) {
    TestOp = (EFI_IFR_TEXT *) Statement->OpCode;
    if (TestOp->TextTwo != 0) {
      String = GetToken (TestOp->TextTwo, gFormData->HiiHandle);
      Size   = StrLen (String);
      FreePool (String);
    }
  }

  if ((Statement->OpCode->OpCode == EFI_IFR_SUBTITLE_OP) ||
    (Statement->OpCode->OpCode == EFI_IFR_REF_OP) ||
    (Statement->OpCode->OpCode == EFI_IFR_PASSWORD_OP) ||
    (Statement->OpCode->OpCode == EFI_IFR_ACTION_OP) ||
    (Statement->OpCode->OpCode == EFI_IFR_RESET_BUTTON_OP) ||
    //
    // Allow a wide display if text op-code and no secondary text op-code
    //
    ((Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) && (Size == 0))
    ) {

    return FALSE;
  }

  return TRUE;
}

/**
  Double confirm with user about the action.

  @param  Action               The user input action.

  @retval TRUE                 User confirm with the input or not need user confirm.
  @retval FALSE                User want ignore this input.

**/
BOOLEAN
FxConfirmPopup (
  IN UINT32   Action
  )
{
  EFI_INPUT_KEY                   Key;
  CHAR16                          *CfmStr;
  UINTN                           CfmStrLen;
  UINT32                          CheckFlags;
  BOOLEAN                         RetVal;
  UINTN                           CatLen;
  UINTN                           MaxLen;

  CfmStrLen = 0;
  CatLen    = StrLen (gConfirmMsgConnect);

  //
  // Below action need extra popup dialog to confirm.
  // 
  CheckFlags = BROWSER_ACTION_DISCARD | 
               BROWSER_ACTION_DEFAULT |
               BROWSER_ACTION_SUBMIT |
               BROWSER_ACTION_RESET |
               BROWSER_ACTION_EXIT;

  //
  // Not need to confirm with user, just return TRUE.
  //
  if ((Action & CheckFlags) == 0) {
    return TRUE;
  }

  if ((Action & BROWSER_ACTION_DISCARD) == BROWSER_ACTION_DISCARD) {
    CfmStrLen += StrLen (gConfirmDiscardMsg);
  } 

  if ((Action & BROWSER_ACTION_DEFAULT) == BROWSER_ACTION_DEFAULT) {
    if (CfmStrLen != 0) {
      CfmStrLen += CatLen;
    } 

    CfmStrLen += StrLen (gConfirmDefaultMsg);
  }

  if ((Action & BROWSER_ACTION_SUBMIT)  == BROWSER_ACTION_SUBMIT) {
    if (CfmStrLen != 0) {
      CfmStrLen += CatLen;
    } 

    CfmStrLen += StrLen (gConfirmSubmitMsg);
  }

  if ((Action & BROWSER_ACTION_RESET)  == BROWSER_ACTION_RESET) {
    if (CfmStrLen != 0) {
      CfmStrLen += CatLen;
    } 

    CfmStrLen += StrLen (gConfirmResetMsg);
  }

  if ((Action & BROWSER_ACTION_EXIT)  == BROWSER_ACTION_EXIT) {
    if (CfmStrLen != 0) {
      CfmStrLen += CatLen;
    } 

    CfmStrLen += StrLen (gConfirmExitMsg);
  }

  //
  // Allocate buffer to save the string.
  // String + "?" + "\0"
  //
  MaxLen = CfmStrLen + 1 + 1;
  CfmStr = AllocateZeroPool (MaxLen * sizeof (CHAR16));
  ASSERT (CfmStr != NULL);

  if ((Action & BROWSER_ACTION_DISCARD) == BROWSER_ACTION_DISCARD) {
    StrCpyS (CfmStr, MaxLen, gConfirmDiscardMsg);
  }

  if ((Action & BROWSER_ACTION_DEFAULT) == BROWSER_ACTION_DEFAULT) {
    if (CfmStr[0] != 0) {
      StrCatS (CfmStr, MaxLen, gConfirmMsgConnect);
      StrCatS (CfmStr, MaxLen, gConfirmDefaultMsg2nd);
    } else {
      StrCpyS (CfmStr, MaxLen, gConfirmDefaultMsg);
    }
  }

  if ((Action & BROWSER_ACTION_SUBMIT)  == BROWSER_ACTION_SUBMIT) {
    if (CfmStr[0] != 0) {
      StrCatS (CfmStr, MaxLen, gConfirmMsgConnect);
      StrCatS (CfmStr, MaxLen, gConfirmSubmitMsg2nd);
    } else {
      StrCpyS (CfmStr, MaxLen, gConfirmSubmitMsg);
    }
  }

  if ((Action & BROWSER_ACTION_RESET)  == BROWSER_ACTION_RESET) {
    if (CfmStr[0] != 0) {
      StrCatS (CfmStr, MaxLen, gConfirmMsgConnect);
      StrCatS (CfmStr, MaxLen, gConfirmResetMsg2nd);
    } else {
      StrCpyS (CfmStr, MaxLen, gConfirmResetMsg);
    }
  }

  if ((Action & BROWSER_ACTION_EXIT)  == BROWSER_ACTION_EXIT) {
    if (CfmStr[0] != 0) {
      StrCatS (CfmStr, MaxLen, gConfirmMsgConnect);
      StrCatS (CfmStr, MaxLen, gConfirmExitMsg2nd);
    } else {
      StrCpyS (CfmStr, MaxLen, gConfirmExitMsg);
    }
  }

  StrCatS (CfmStr, MaxLen, gConfirmMsgEnd);

  do {
    CreateDialog (&Key, gEmptyString, CfmStr, gConfirmOpt, gEmptyString, NULL);
  } while (((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (gConfirmOptYes[0] | UPPER_LOWER_CASE_OFFSET)) &&
           ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (gConfirmOptNo[0] | UPPER_LOWER_CASE_OFFSET)) &&
           (Key.ScanCode != SCAN_ESC));

  if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (gConfirmOptYes[0] | UPPER_LOWER_CASE_OFFSET)) {
    RetVal = TRUE;
  } else {
    RetVal = FALSE;
  }

  FreePool (CfmStr);

  return RetVal;
}

/**
  Print string for this menu option.

  @param  MenuOption               The menu opton which this attribut used to.
  @param  SkipWidth                The skip width between the left to the start of the prompt.
  @param  BeginCol                 The begin column for one menu.
  @param  SkipLine                 The skip line for this menu. 
  @param  BottomRow                The bottom row for this form.
  @param  Highlight                Whether this menu will be highlight.
  @param  UpdateCol                Whether need to update the column info for Date/Time.

  @retval EFI_SUCESSS              Process the user selection success.

**/
EFI_STATUS
DisplayOneMenu (
  IN UI_MENU_OPTION                  *MenuOption,
  IN UINTN                           SkipWidth,
  IN UINTN                           BeginCol,
  IN UINTN                           SkipLine,
  IN UINTN                           BottomRow,
  IN BOOLEAN                         Highlight,
  IN BOOLEAN                         UpdateCol
  )
{
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement;
  UINTN                           Index;
  UINT16                          Width;
  UINT16                          PromptWidth;
  CHAR16                          *StringPtr;
  CHAR16                          *OptionString;
  CHAR16                          *OutputString;
  UINT16                          GlyphWidth;
  UINTN                           Temp;
  UINTN                           Temp2;
  UINTN                           Temp3;
  EFI_STATUS                      Status;
  UINTN                           Row;
  BOOLEAN                         IsProcessingFirstRow;
  UINTN                           Col;
  UINTN                           PromptLineNum;
  UINTN                           OptionLineNum;
  CHAR16                          AdjustValue;
  UINTN                           MaxRow;

  Statement = MenuOption->ThisTag;
  Temp      = SkipLine;
  Temp2     = SkipLine;
  Temp3     = SkipLine;
  AdjustValue   = 0;
  PromptLineNum = 0;
  OptionLineNum = 0;
  MaxRow        = 0;
  IsProcessingFirstRow = TRUE;

  //
  // Set default color.
  //
  SetDisplayAttribute (MenuOption, FALSE);

  //
  // 1. Paint the option string.
  //
  Status = ProcessOptions (MenuOption, FALSE, &OptionString, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (OptionString != NULL) {
    if (Statement->OpCode->OpCode == EFI_IFR_DATE_OP || Statement->OpCode->OpCode == EFI_IFR_TIME_OP) {
      //
      // Adjust option string for date/time opcode.
      //
      ProcessStringForDateTime(MenuOption, OptionString, UpdateCol);
    }
  
    Width       = (UINT16) gOptionBlockWidth - 1;
    Row         = MenuOption->Row;
    GlyphWidth  = 1;
    OptionLineNum = 0;
  
    for (Index = 0; GetLineByWidth (OptionString, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) {
      if (((Temp2 == 0)) && (Row <= BottomRow)) {
        if (Statement->OpCode->OpCode == EFI_IFR_DATE_OP || Statement->OpCode->OpCode == EFI_IFR_TIME_OP) {
          //
          // For date/time question, it has three menu options for this qustion.
          // The first/second menu options with the skip value is 0. the last one
          // with skip value is 1.
          //
          if (MenuOption->Skip != 0) {
            //
            // For date/ time, print the last past (year for date and second for time)
            // - 7 means skip [##/##/ for date and [##:##: for time.
            //
            DisplayMenuString (MenuOption,MenuOption->OptCol, Row, OutputString, Width + 1 - 7, Highlight);
          } else {
            //
            // For date/ time, print the first and second past (year for date and second for time)
            // The OutputString has a NARROW_CHAR or WIDE_CHAR at the begin of the string, 
            // so need to - 1 to remove it, otherwise, it will clean 1 extr char follow it.
            DisplayMenuString (MenuOption, MenuOption->OptCol, Row, OutputString, StrLen (OutputString) - 1, Highlight);
          }
        } else {
          DisplayMenuString (MenuOption, MenuOption->OptCol, Row, OutputString, Width + 1, Highlight);
        }
        OptionLineNum++;
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
          if ((Row - MenuOption->Row) >= MenuOption->Skip) {
            MenuOption->Skip++;
          }
        }
      }
  
      FreePool (OutputString);
      if (Temp2 != 0) {
        Temp2--;
      }
    }
  
    Highlight = FALSE;

    FreePool (OptionString);
  }

  //
  // 2. Paint the description.
  //
  PromptWidth   = GetWidth (MenuOption, &AdjustValue);
  Row           = MenuOption->Row;
  GlyphWidth    = 1;
  PromptLineNum = 0;

  if (MenuOption->Description == NULL || MenuOption->Description[0] == '\0') {
    PrintStringAtWithWidth (BeginCol, Row, L"", PromptWidth + AdjustValue + SkipWidth);
    PromptLineNum++;
  } else {
    for (Index = 0; GetLineByWidth (MenuOption->Description, PromptWidth, &GlyphWidth, &Index, &OutputString) != 0x0000;) {      
      if ((Temp == 0) && (Row <= BottomRow)) { 
        //
        // 1.Clean the start LEFT_SKIPPED_COLUMNS 
        //
        PrintStringAtWithWidth (BeginCol, Row, L"", SkipWidth);
        
        if (Statement->OpCode->OpCode == EFI_IFR_REF_OP && MenuOption->Col >= 2 && IsProcessingFirstRow) {
          //
          // Print Arrow for Goto button.
          //
          PrintCharAt (
            MenuOption->Col - 2,
            Row,
            GEOMETRICSHAPE_RIGHT_TRIANGLE
            );
          IsProcessingFirstRow = FALSE;
        }
        DisplayMenuString (MenuOption, MenuOption->Col, Row, OutputString, PromptWidth + AdjustValue, Highlight);
        PromptLineNum ++;
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

    Highlight = FALSE;
  }


  //
  // 3. If this is a text op with secondary text information
  //
  if ((Statement->OpCode->OpCode  == EFI_IFR_TEXT_OP) && (((EFI_IFR_TEXT*)Statement->OpCode)->TextTwo != 0)) {
    StringPtr   = GetToken (((EFI_IFR_TEXT*)Statement->OpCode)->TextTwo, gFormData->HiiHandle);
  
    Width       = (UINT16) gOptionBlockWidth - 1;
    Row         = MenuOption->Row;
    GlyphWidth  = 1;
    OptionLineNum = 0;

    for (Index = 0; GetLineByWidth (StringPtr, Width, &GlyphWidth, &Index, &OutputString) != 0x0000;) { 
      if ((Temp3 == 0) && (Row <= BottomRow)) {
        DisplayMenuString (MenuOption, MenuOption->OptCol, Row, OutputString, Width + 1, Highlight);
        OptionLineNum++;
      }
      //
      // If there is more string to process print on the next row and increment the Skip value
      //
      if (StrLen (&StringPtr[Index]) != 0) {
        if (Temp3 == 0) {
          Row++;
          //
          // If the rows for text two is greater than or equal to the skip value, increase the skip value
          //
          if ((Row - MenuOption->Row) >= MenuOption->Skip) {
            MenuOption->Skip++;
          }
        }
      }
  
      FreePool (OutputString);
      if (Temp3 != 0) {
        Temp3--;
      }
    }

    FreePool (StringPtr);
  }

  //
  // 4.Line number for Option string and prompt string are not equal.
  //  Clean the column whose line number is less.
  //
  if (HasOptionString(MenuOption) && (OptionLineNum != PromptLineNum)) {
    Col    =  OptionLineNum < PromptLineNum ? MenuOption->OptCol : BeginCol;
    Row    = (OptionLineNum < PromptLineNum ? OptionLineNum : PromptLineNum) + MenuOption->Row;
    Width  = (UINT16) (OptionLineNum < PromptLineNum ? gOptionBlockWidth : PromptWidth + AdjustValue + SkipWidth);
    MaxRow = (OptionLineNum < PromptLineNum ? PromptLineNum : OptionLineNum) + MenuOption->Row - 1;
    
    while (Row <= MaxRow) {
      DisplayMenuString (MenuOption, Col, Row++, L"", Width, FALSE);
    }
  }

  return EFI_SUCCESS;
}

/**
  Display menu and wait for user to select one menu option, then return it.
  If AutoBoot is enabled, then if user doesn't select any option,
  after period of time, it will automatically return the first menu option.

  @param  FormData               The current form data info.

  @retval EFI_SUCESSS            Process the user selection success.
  @retval EFI_NOT_FOUND          Process option string for orderedlist/Oneof fail.

**/
EFI_STATUS
UiDisplayMenu (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData
  )
{
  UINTN                           SkipValue;
  INTN                            Difference;
  UINTN                           DistanceValue;
  UINTN                           Row;
  UINTN                           Col;
  UINTN                           Temp;
  UINTN                           Temp2;
  UINTN                           TopRow;
  UINTN                           BottomRow;
  UINTN                           Index;
  CHAR16                          *StringPtr;
  CHAR16                          *StringRightPtr;
  CHAR16                          *StringErrorPtr;
  CHAR16                          *OptionString;
  CHAR16                          *HelpString;
  CHAR16                          *HelpHeaderString;
  CHAR16                          *HelpBottomString;
  BOOLEAN                         NewLine;
  BOOLEAN                         Repaint;
  BOOLEAN                         UpArrow;
  BOOLEAN                         DownArrow;
  EFI_STATUS                      Status;
  EFI_INPUT_KEY                   Key;
  LIST_ENTRY                      *Link;
  LIST_ENTRY                      *NewPos;
  LIST_ENTRY                      *TopOfScreen;
  LIST_ENTRY                      *SavedListEntry;
  UI_MENU_OPTION                  *MenuOption;
  UI_MENU_OPTION                  *NextMenuOption;
  UI_MENU_OPTION                  *SavedMenuOption;
  UI_CONTROL_FLAG                 ControlFlag;
  UI_SCREEN_OPERATION             ScreenOperation;
  FORM_DISPLAY_ENGINE_STATEMENT   *Statement;
  BROWSER_HOT_KEY                 *HotKey;
  UINTN                           HelpPageIndex;
  UINTN                           HelpPageCount;
  UINTN                           RowCount;
  UINTN                           HelpLine;
  UINTN                           HelpHeaderLine;
  UINTN                           HelpBottomLine;
  BOOLEAN                         MultiHelpPage;
  UINT16                          EachLineWidth;
  UINT16                          HeaderLineWidth;
  UINT16                          BottomLineWidth;
  EFI_STRING_ID                   HelpInfo;
  UI_EVENT_TYPE                   EventType;
  BOOLEAN                         SkipHighLight;
  EFI_HII_VALUE                   *StatementValue;

  EventType           = UIEventNone;
  Status              = EFI_SUCCESS;
  HelpString          = NULL;
  HelpHeaderString    = NULL;
  HelpBottomString    = NULL;
  OptionString        = NULL;
  ScreenOperation     = UiNoOperation;
  NewLine             = TRUE;
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
  UpArrow             = FALSE;
  DownArrow           = FALSE;
  SkipValue           = 0;
  SkipHighLight       = FALSE;

  NextMenuOption      = NULL;
  SavedMenuOption     = NULL;
  HotKey              = NULL;
  Repaint             = TRUE;
  MenuOption          = NULL;
  gModalSkipColumn    = (CHAR16) (gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn) / 6;

  ZeroMem (&Key, sizeof (EFI_INPUT_KEY));

  TopRow    = gStatementDimensions.TopRow    + SCROLL_ARROW_HEIGHT;
  BottomRow = gStatementDimensions.BottomRow - SCROLL_ARROW_HEIGHT - 1;

  Row = TopRow;
  if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
    Col = gStatementDimensions.LeftColumn + LEFT_SKIPPED_COLUMNS + gModalSkipColumn;
  } else {
    Col = gStatementDimensions.LeftColumn + LEFT_SKIPPED_COLUMNS;
  }

  FindTopMenu(FormData, &TopOfScreen, &NewPos, &SkipValue);
  if (!IsListEmpty (&gMenuOption)) {
    NextMenuOption = MENU_OPTION_FROM_LINK (NewPos);
    gUserInput->SelectedStatement = NextMenuOption->ThisTag;
  }

  gST->ConOut->EnableCursor (gST->ConOut, FALSE);

  ControlFlag = CfInitialization;
  while (TRUE) {
    switch (ControlFlag) {
    case CfInitialization:
      if ((gOldFormEntry.HiiHandle != FormData->HiiHandle) || 
          (!CompareGuid (&gOldFormEntry.FormSetGuid, &FormData->FormSetGuid))) {
        //
        // Clear Statement range if different formset is painted.
        //
        ClearLines (
          gStatementDimensions.LeftColumn,
          gStatementDimensions.RightColumn,
          TopRow - SCROLL_ARROW_HEIGHT,
          BottomRow + SCROLL_ARROW_HEIGHT,
          GetFieldTextColor ()
          );

      }
      ControlFlag = CfRepaint;
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

        gST->ConOut->SetAttribute (gST->ConOut, GetFieldTextColor ());
        
        //
        // 1. Check whether need to print the arrow up.
        //
        if (!ValueIsScroll (TRUE, TopOfScreen)) {
          UpArrow = TRUE;
        }

        if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
          PrintStringAtWithWidth(gStatementDimensions.LeftColumn + gModalSkipColumn, TopRow - 1, L"", gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn - 2 * gModalSkipColumn);
        } else {
          PrintStringAtWithWidth(gStatementDimensions.LeftColumn, TopRow - 1, L"", gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn);
        }
        if (UpArrow) {
          gST->ConOut->SetAttribute (gST->ConOut, GetArrowColor ());
          PrintCharAt (
            gStatementDimensions.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + 1,
            TopRow - SCROLL_ARROW_HEIGHT,
            ARROW_UP
            );
          gST->ConOut->SetAttribute (gST->ConOut, GetFieldTextColor ());
        }

        //
        // 2.Paint the menu.
        //
        for (Link = TopOfScreen; Link != &gMenuOption; Link = Link->ForwardLink) {
          MenuOption          = MENU_OPTION_FROM_LINK (Link);
          MenuOption->Row     = Row;
          MenuOption->Col     = Col;
          if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
            MenuOption->OptCol  = gStatementDimensions.LeftColumn + LEFT_SKIPPED_COLUMNS + gPromptBlockWidth + gModalSkipColumn;
          } else {
            MenuOption->OptCol  = gStatementDimensions.LeftColumn + LEFT_SKIPPED_COLUMNS + gPromptBlockWidth;
          }

          if (MenuOption->NestInStatement) {
            MenuOption->Col += SUBTITLE_INDENT;
          }

          //
          // Save the highlight menu, will be used in CfRefreshHighLight case.
          //
          if (Link == NewPos) {
            SavedMenuOption = MenuOption;
            SkipHighLight   = TRUE;
          }

          if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
            Status = DisplayOneMenu (MenuOption, 
                            MenuOption->Col - gStatementDimensions.LeftColumn,
                            gStatementDimensions.LeftColumn + gModalSkipColumn, 
                            Link == TopOfScreen ? SkipValue : 0, 
                            BottomRow,
                            (BOOLEAN) ((Link == NewPos) && IsSelectable(MenuOption)),
                            TRUE
                            );
          } else {
            Status = DisplayOneMenu (MenuOption, 
                            MenuOption->Col - gStatementDimensions.LeftColumn,
                            gStatementDimensions.LeftColumn, 
                            Link == TopOfScreen ? SkipValue : 0, 
                            BottomRow,
                            (BOOLEAN) ((Link == NewPos) && IsSelectable(MenuOption)),
                            TRUE
                            );
          }

          if (EFI_ERROR (Status)) {
            if (gMisMatch) {
              return EFI_SUCCESS;
            } else {
              return Status;
            }
          }
          //
          // 3. Update the row info which will be used by next menu.
          //
          if (Link == TopOfScreen) {
            Row += MenuOption->Skip - SkipValue;
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

        //
        // 3. Menus in this form may not cover all form, clean the remain field.
        //
        while (Row <= BottomRow) {
          if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
            PrintStringAtWithWidth(gStatementDimensions.LeftColumn + gModalSkipColumn, Row++, L"", gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn - 2 * gModalSkipColumn);
          } else {
            PrintStringAtWithWidth(gStatementDimensions.LeftColumn, Row++, L"", gStatementDimensions.RightColumn - gHelpBlockWidth - gStatementDimensions.LeftColumn);
          }
        }

        //
        // 4. Print the down arrow row.
        //
        if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
          PrintStringAtWithWidth(gStatementDimensions.LeftColumn + gModalSkipColumn, BottomRow + 1, L"", gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn - 2 *  + gModalSkipColumn);
        } else {
          PrintStringAtWithWidth(gStatementDimensions.LeftColumn, BottomRow + 1, L"", gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn);
        }
        if (DownArrow) {
          gST->ConOut->SetAttribute (gST->ConOut, GetArrowColor ());
          PrintCharAt (
            gStatementDimensions.LeftColumn + gPromptBlockWidth + gOptionBlockWidth + 1,
            BottomRow + SCROLL_ARROW_HEIGHT,
            ARROW_DOWN
            );
          gST->ConOut->SetAttribute (gST->ConOut, GetFieldTextColor ());
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

      UpdateHighlightMenuInfo(NewPos, TopOfScreen, SkipValue);

      if (SkipHighLight) {
        SkipHighLight = FALSE;
        MenuOption    = SavedMenuOption;
        RefreshKeyHelp(gFormData, SavedMenuOption->ThisTag, FALSE);
        break;
      }

      if (IsListEmpty (&gMenuOption)) {
        //
        // No menu option, just update the hotkey filed.
        //
        RefreshKeyHelp(gFormData, NULL, FALSE);
        break;
      }

      if (MenuOption != NULL && TopOfScreen == &MenuOption->Link) {
        Temp = SkipValue;
      } else {
        Temp = 0;
      }
      if (NewPos == TopOfScreen) {
        Temp2 = SkipValue;
      } else {
        Temp2 = 0;
      }

      if (NewPos != NULL && (MenuOption == NULL || NewPos != &MenuOption->Link)) {
        if (MenuOption != NULL) {
          //
          // Remove the old highlight menu.
          //
          Status = DisplayOneMenu (MenuOption, 
                          MenuOption->Col - gStatementDimensions.LeftColumn,
                          gStatementDimensions.LeftColumn, 
                          Temp, 
                          BottomRow,
                          FALSE,
                          FALSE
                          );
        }

        //
        // This is the current selected statement
        //
        MenuOption = MENU_OPTION_FROM_LINK (NewPos);
        RefreshKeyHelp(gFormData, MenuOption->ThisTag, FALSE);

        if (!IsSelectable (MenuOption)) {
          break;
        }

        Status = DisplayOneMenu (MenuOption, 
                        MenuOption->Col - gStatementDimensions.LeftColumn,
                        gStatementDimensions.LeftColumn, 
                        Temp2, 
                        BottomRow,
                        TRUE,
                        FALSE
                        );
      }
      break;

    case CfUpdateHelpString:
      ControlFlag = CfPrepareToReadKey;
      if ((FormData->Attribute & HII_DISPLAY_MODAL) != 0) {
        break;
      }

      //
      // NewLine means only update highlight menu (remove old highlight and highlith
      // the new one), not need to full repain the form.
      //
      if (Repaint || NewLine) {
        if (IsListEmpty (&gMenuOption)) {
          //
          // Don't print anything if no mwnu option.
          //
          StringPtr = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
        } else {
          //
          // Don't print anything if it is a NULL help token
          //
          ASSERT(MenuOption != NULL);
          HelpInfo = ((EFI_IFR_STATEMENT_HEADER *) ((CHAR8 *)MenuOption->ThisTag->OpCode + sizeof (EFI_IFR_OP_HEADER)))->Help;
          Statement = MenuOption->ThisTag;
          StatementValue = &Statement->CurrentValue;
          if (HelpInfo == 0 || !IsSelectable (MenuOption)) {
            if ((Statement->OpCode->OpCode == EFI_IFR_DATE_OP && StatementValue->Value.date.Month== 0xff)||(Statement->OpCode->OpCode == EFI_IFR_TIME_OP && StatementValue->Value.time.Hour == 0xff)){
              StringPtr = GetToken (STRING_TOKEN (GET_TIME_FAIL), gHiiHandle);
            } else {
              StringPtr = GetToken (STRING_TOKEN (EMPTY_STRING), gHiiHandle);
            }
          } else {
            if ((Statement->OpCode->OpCode == EFI_IFR_DATE_OP && StatementValue->Value.date.Month== 0xff)||(Statement->OpCode->OpCode == EFI_IFR_TIME_OP && StatementValue->Value.time.Hour == 0xff)){
              StringRightPtr = GetToken (HelpInfo, gFormData->HiiHandle);
              StringErrorPtr = GetToken (STRING_TOKEN (GET_TIME_FAIL), gHiiHandle);
              StringPtr = AllocateZeroPool ((StrLen (StringRightPtr) + StrLen (StringErrorPtr)+ 1 ) * sizeof (CHAR16));
              StrCpyS (StringPtr, StrLen (StringRightPtr) + StrLen (StringErrorPtr) + 1, StringRightPtr);
              StrCatS (StringPtr, StrLen (StringRightPtr) + StrLen (StringErrorPtr) + 1, StringErrorPtr);
              FreePool (StringRightPtr);
              FreePool (StringErrorPtr);
            } else {
              StringPtr = GetToken (HelpInfo, gFormData->HiiHandle);
            }
          }
        }

        RowCount      = BottomRow - TopRow + 1;
        HelpPageIndex = 0;
        //
        // 1.Calculate how many line the help string need to print.
        //
        if (HelpString != NULL) {
          FreePool (HelpString);
          HelpString = NULL;
        }
        HelpLine = ProcessHelpString (StringPtr, &HelpString, &EachLineWidth, RowCount);
        FreePool (StringPtr);

        if (HelpLine > RowCount) {
          MultiHelpPage   = TRUE;
          StringPtr       = GetToken (STRING_TOKEN(ADJUST_HELP_PAGE_UP), gHiiHandle);
          if (HelpHeaderString != NULL) {
            FreePool (HelpHeaderString);
            HelpHeaderString = NULL;
          }
          HelpHeaderLine  = ProcessHelpString (StringPtr, &HelpHeaderString, &HeaderLineWidth, 0);
          FreePool (StringPtr);
          StringPtr       = GetToken (STRING_TOKEN(ADJUST_HELP_PAGE_DOWN), gHiiHandle);
          if (HelpBottomString != NULL) {
            FreePool (HelpBottomString);
            HelpBottomString = NULL;
          }
          HelpBottomLine  = ProcessHelpString (StringPtr, &HelpBottomString, &BottomLineWidth, 0);
          FreePool (StringPtr);
          //
          // Calculate the help page count.
          //
          if (HelpLine > 2 * RowCount - 2) {
            HelpPageCount = (HelpLine - RowCount + 1) / (RowCount - 2) + 1;
            if ((HelpLine - RowCount + 1) % (RowCount - 2) != 0) {
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
      // Check whether need to show the 'More(U/u)' at the begin.
      // Base on current direct info, here shows aligned to the right side of the column.
      // If the direction is multi line and aligned to right side may have problem, so 
      // add ASSERT code here.
      //
      if (HelpPageIndex > 0) {
        gST->ConOut->SetAttribute (gST->ConOut, GetInfoTextColor ());
        for (Index = 0; Index < HelpHeaderLine; Index++) {
          ASSERT (HelpHeaderLine == 1);
          ASSERT (GetStringWidth (HelpHeaderString) / 2 < ((UINT32) gHelpBlockWidth - 1));
          PrintStringAtWithWidth (
            gStatementDimensions.RightColumn - gHelpBlockWidth,
            Index + TopRow,
            gEmptyString,
            gHelpBlockWidth
            );
          PrintStringAt (
            gStatementDimensions.RightColumn - GetStringWidth (HelpHeaderString) / 2 - 1,
            Index + TopRow,
            &HelpHeaderString[Index * HeaderLineWidth]
            );
        }
      }

      gST->ConOut->SetAttribute (gST->ConOut, GetHelpTextColor ());
      //
      // Print the help string info.
      //
      if (!MultiHelpPage) {
        for (Index = 0; Index < HelpLine; Index++) {
          PrintStringAtWithWidth (
            gStatementDimensions.RightColumn - gHelpBlockWidth,
            Index + TopRow,
            &HelpString[Index * EachLineWidth],
            gHelpBlockWidth
            );
        }
        for (; Index < RowCount; Index ++) {
          PrintStringAtWithWidth (
            gStatementDimensions.RightColumn - gHelpBlockWidth,
            Index + TopRow,
            gEmptyString,
            gHelpBlockWidth
            );
        }
        gST->ConOut->SetCursorPosition(gST->ConOut, gStatementDimensions.RightColumn-1, BottomRow);
      } else  {
        if (HelpPageIndex == 0) {
          for (Index = 0; Index < RowCount - HelpBottomLine; Index++) {
            PrintStringAtWithWidth (
              gStatementDimensions.RightColumn - gHelpBlockWidth,
              Index + TopRow,
              &HelpString[Index * EachLineWidth],
              gHelpBlockWidth
              );
          }
        } else {
          for (Index = 0; (Index < RowCount - HelpBottomLine - HelpHeaderLine) && 
              (Index + HelpPageIndex * (RowCount - 2) + 1 < HelpLine); Index++) {
            PrintStringAtWithWidth (
              gStatementDimensions.RightColumn - gHelpBlockWidth,
              Index + TopRow + HelpHeaderLine,
              &HelpString[(Index + HelpPageIndex * (RowCount - 2) + 1)* EachLineWidth],
              gHelpBlockWidth
              );
          }
          if (HelpPageIndex == HelpPageCount - 1) {
            for (; Index < RowCount - HelpHeaderLine; Index ++) {
              PrintStringAtWithWidth (
                gStatementDimensions.RightColumn - gHelpBlockWidth,
                Index + TopRow + HelpHeaderLine,
                gEmptyString,
                gHelpBlockWidth
                );
            }
            gST->ConOut->SetCursorPosition(gST->ConOut, gStatementDimensions.RightColumn-1, BottomRow);
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
        gST->ConOut->SetAttribute (gST->ConOut, GetInfoTextColor ());
        for (Index = 0; Index < HelpBottomLine; Index++) {
          ASSERT (HelpBottomLine == 1);
          ASSERT (GetStringWidth (HelpBottomString) / 2 < ((UINT32) gHelpBlockWidth - 1));
          PrintStringAtWithWidth (
            gStatementDimensions.RightColumn - gHelpBlockWidth,
            BottomRow + Index - HelpBottomLine + 1,
            gEmptyString,
            gHelpBlockWidth
            );
          PrintStringAt (
            gStatementDimensions.RightColumn - GetStringWidth (HelpBottomString) / 2 - 1,
            BottomRow + Index - HelpBottomLine + 1,
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
      while (TRUE) {
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        if (!EFI_ERROR (Status)) {
          EventType = UIEventKey;
          break;
        }

        //
        // If we encounter error, continue to read another key in.
        //
        if (Status != EFI_NOT_READY) {
          continue;
        }
        
        EventType = UiWaitForEvent(gST->ConIn->WaitForKey);
        if (EventType == UIEventKey) {
          gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        }
        break;
      }

      if (EventType == UIEventDriver) {
        gMisMatch = TRUE;
        gUserInput->Action = BROWSER_ACTION_NONE;
        ControlFlag = CfExit;
        break;
      }
      
      if (EventType == UIEventTimeOut) {
        gUserInput->Action = BROWSER_ACTION_FORM_EXIT;
        ControlFlag = CfExit;
        break;
      }

      switch (Key.UnicodeChar) {
      case CHAR_CARRIAGE_RETURN:
        if(MenuOption == NULL || MenuOption->GrayOut || MenuOption->ReadOnly) {
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
        ASSERT(MenuOption != NULL);
        if(IsListEmpty (&gMenuOption) || MenuOption->GrayOut || MenuOption->ReadOnly) {
          ControlFlag = CfReadKey;
          break;
        }

        Statement = MenuOption->ThisTag;
        if ((Statement->OpCode->OpCode == EFI_IFR_DATE_OP)
          || (Statement->OpCode->OpCode == EFI_IFR_TIME_OP)
          || ((Statement->OpCode->OpCode == EFI_IFR_NUMERIC_OP) && (GetFieldFromNum(Statement->OpCode) != 0))
        ){
          if (Key.UnicodeChar == '+') {
            gDirection = SCAN_RIGHT;
          } else {
            gDirection = SCAN_LEFT;
          }
          
          Status = ProcessOptions (MenuOption, TRUE, &OptionString, TRUE);
          if (OptionString != NULL) {
            FreePool (OptionString);
          }
          if (EFI_ERROR (Status)) {
            //
            // Repaint to clear possible error prompt pop-up
            //
            Repaint = TRUE;
            NewLine = TRUE;
          } else {
            ControlFlag = CfExit;
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
        if(IsListEmpty (&gMenuOption)) {
          ControlFlag = CfReadKey;
          break;
        }
        
        ASSERT(MenuOption != NULL);
        if (MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_CHECKBOX_OP && !MenuOption->GrayOut && !MenuOption->ReadOnly) {
          ScreenOperation = UiSelect;
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
        
        if (((FormData->Attribute & HII_DISPLAY_MODAL) != 0) && (Key.ScanCode == SCAN_ESC || Index == mScanCodeNumber)) {
          //
          // ModalForm has no ESC key and Hot Key.
          //
          ControlFlag = CfReadKey;
        } else if (Index == mScanCodeNumber) {
          //
          // Check whether Key matches the registered hot key.
          //
          HotKey = NULL;
          HotKey = GetHotKeyFromRegisterList (&Key);
          if (HotKey != NULL) {
            ScreenOperation = UiHotKey;
          }
        }
        break;
      }
      break;

    case CfScreenOperation:
      if ((ScreenOperation != UiReset) && (ScreenOperation != UiHotKey)) {
        //
        // If the screen has no menu items, and the user didn't select UiReset or UiHotKey
        // ignore the selection and go back to reading keys.
        //
        if (IsListEmpty (&gMenuOption)) {
          ControlFlag = CfReadKey;
          break;
        }
      }

      for (Index = 0;
           Index < ARRAY_SIZE (gScreenOperationToControlFlag);
           Index++
          ) {
        if (ScreenOperation == gScreenOperationToControlFlag[Index].ScreenOperation) {
          ControlFlag = gScreenOperationToControlFlag[Index].ControlFlag;
          break;
        }
      }
      break;

    case CfUiSelect:
      ControlFlag = CfRepaint;

      ASSERT(MenuOption != NULL);
      Statement = MenuOption->ThisTag;
      if (Statement->OpCode->OpCode == EFI_IFR_TEXT_OP) {
        break;
      }

      switch (Statement->OpCode->OpCode) {
      case EFI_IFR_REF_OP:
      case EFI_IFR_ACTION_OP:
      case EFI_IFR_RESET_BUTTON_OP:
        ControlFlag = CfExit;
        break;

      default:
        //
        // Editable Questions: oneof, ordered list, checkbox, numeric, string, password
        //
        RefreshKeyHelp (gFormData, Statement, TRUE);
        Status = ProcessOptions (MenuOption, TRUE, &OptionString, TRUE);
        
        if (OptionString != NULL) {
          FreePool (OptionString);
        }
        
        if (EFI_ERROR (Status)) {
          Repaint = TRUE;
          NewLine = TRUE;
          RefreshKeyHelp (gFormData, Statement, FALSE);
          break;
        } else {
          ControlFlag = CfExit;
          break;
        }
      }
      break;

    case CfUiReset:
      //
      // We come here when someone press ESC
      // If the policy is not exit front page when user press ESC, process here.
      //
      if (!FormExitPolicy()) {
        Repaint     = TRUE;
        NewLine     = TRUE;
        ControlFlag = CfRepaint;
        break;
      }

      gUserInput->Action = BROWSER_ACTION_FORM_EXIT;
      ControlFlag = CfExit;
      break;

    case CfUiHotKey:
      ControlFlag = CfRepaint;

      ASSERT (HotKey != NULL);

      if (FxConfirmPopup(HotKey->Action)) {
        gUserInput->Action = HotKey->Action;
        if ((HotKey->Action & BROWSER_ACTION_DEFAULT) == BROWSER_ACTION_DEFAULT) {
          gUserInput->DefaultId = HotKey->DefaultId;
        }
        ControlFlag = CfExit;
      } else {
        Repaint     = TRUE;
        NewLine     = TRUE;
        ControlFlag = CfRepaint;
      }

      break;

    case CfUiLeft:
      ControlFlag = CfRepaint;
      ASSERT(MenuOption != NULL);
      if ((MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_DATE_OP) || (MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_TIME_OP)) {
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
      ControlFlag = CfRepaint;
      ASSERT(MenuOption != NULL);
      if ((MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_DATE_OP) || (MenuOption->ThisTag->OpCode->OpCode == EFI_IFR_TIME_OP)) {
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
      ControlFlag = CfRepaint;
      NewLine     = TRUE;

      SavedListEntry = NewPos;
      ASSERT(NewPos != NULL);

      MenuOption = MENU_OPTION_FROM_LINK (NewPos);
      ASSERT (MenuOption != NULL);

      //
      // Adjust Date/Time position before we advance forward.
      //
      AdjustDateAndTimePosition (TRUE, &NewPos);

      NewPos     = NewPos->BackLink;
      //
      // Find next selectable menu or the first menu beyond current form.
      //
      Difference = MoveToNextStatement (TRUE, &NewPos, MenuOption->Row - TopRow, FALSE);
      if (Difference < 0) {
        //
        // We hit the begining MenuOption that can be focused
        // so we simply scroll to the top.
        //
        Repaint     = TRUE;
        if (TopOfScreen != gMenuOption.ForwardLink || SkipValue != 0) {
          TopOfScreen = gMenuOption.ForwardLink;
          NewPos      = SavedListEntry;
          SkipValue = 0;
        } else {
          //
          // Scroll up to the last page when we have arrived at top page.
          //
          TopOfScreen = FindTopOfScreenMenu (gMenuOption.BackLink, BottomRow - TopRow, &SkipValue);
          NewPos = gMenuOption.BackLink;
          MoveToNextStatement (TRUE, &NewPos, BottomRow - TopRow, TRUE);
        }
      } else {
        NextMenuOption = MENU_OPTION_FROM_LINK (NewPos);

        if (MenuOption->Row < TopRow + Difference + NextMenuOption->Skip) {
          //
          // Previous focus MenuOption is above the TopOfScreen, so we need to scroll
          //
          TopOfScreen = NewPos;
          Repaint     = TRUE;
          SkipValue   = 0;
        }

        //
        // Check whether new highlight menu is selectable, if not, keep highlight on the old one.
        //
        // BottomRow - TopRow + 1 means the total rows current forms supported.
        // Difference + NextMenuOption->Skip + 1 means the distance between last highlight menu 
        // and new top menu. New top menu will all shows in next form, but last highlight menu 
        // may only shows 1 line. + 1 at right part means at least need to keep 1 line for the 
        // last highlight menu.
        // 
        if (!IsSelectable(NextMenuOption) && IsSelectable(MenuOption) && 
            (BottomRow - TopRow + 1 >= Difference + NextMenuOption->Skip + 1)) {
          NewPos = SavedListEntry;
        }
      }

      UpdateStatusBar (INPUT_ERROR, FALSE);

      //
      // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);

      UpdateHighlightMenuInfo(NewPos, TopOfScreen, SkipValue);
      break;

    case CfUiPageUp:
      //
      // SkipValue means lines is skipped when show the top menu option.
      //
      ControlFlag = CfRepaint;
      NewLine     = TRUE;
      Repaint     = TRUE;

      Link      = TopOfScreen;
      //
      // First minus the menu of the top screen, it's value is SkipValue.
      //
      if (SkipValue >= BottomRow - TopRow + 1) {
        //
        // SkipValue > (BottomRow - TopRow + 1) means current menu has more than one
        // form of options to be show, so just update the SkipValue to show the next
        // parts of options.
        //
        SkipValue -= BottomRow - TopRow + 1;
        NewPos     = TopOfScreen;
        break;
      } else {
        Index     = (BottomRow + 1) - SkipValue - TopRow;
      }
      
      TopOfScreen = FindTopOfScreenMenu(TopOfScreen, Index, &SkipValue);
      NewPos = TopOfScreen;
      MoveToNextStatement (FALSE, &NewPos, BottomRow - TopRow, FALSE);
      
      UpdateStatusBar (INPUT_ERROR, FALSE);

      //
      // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
      // Don't do this when we are already in the first page.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);

      UpdateHighlightMenuInfo(NewPos, TopOfScreen, SkipValue);
      break;

    case CfUiPageDown:
      //
      // SkipValue means lines is skipped when show the top menu option.
      //
      ControlFlag = CfRepaint;
      NewLine     = TRUE;
      Repaint     = TRUE;

      Link    = TopOfScreen;
      NextMenuOption = MENU_OPTION_FROM_LINK (Link);
      Index = TopRow + NextMenuOption->Skip - SkipValue;
      //
      // Count to the menu option which will show at the top of the next form.
      //
      while ((Index <= BottomRow + 1) && (Link->ForwardLink != &gMenuOption)) {
        Link           = Link->ForwardLink;
        NextMenuOption = MENU_OPTION_FROM_LINK (Link);
        Index = Index + NextMenuOption->Skip;
      }

      if ((Link->ForwardLink == &gMenuOption) && (Index <= BottomRow + 1)) {
        //
        // Highlight on the last menu which can be highlight.
        //
        Repaint = FALSE;
        MoveToNextStatement (TRUE, &Link, Index - TopRow, TRUE);
      } else {
        //
        // Calculate the skip line for top of screen menu.
        //
        if (Link == TopOfScreen) {
          //
          // The top of screen menu option occupies the entire form.
          //
          SkipValue += BottomRow - TopRow + 1;
        } else {
          SkipValue = NextMenuOption->Skip - (Index - (BottomRow + 1));
        }
        TopOfScreen = Link;
        MenuOption = NULL;
        //
        // Move to the Next selectable menu.
        //
        MoveToNextStatement (FALSE, &Link, BottomRow - TopRow, TRUE);
      }

      //
      // Save the menu as the next highlight menu.
      //
      NewPos  = Link;

      UpdateStatusBar (INPUT_ERROR, FALSE);

      //
      // If we encounter a Date/Time op-code set, rewind to the first op-code of the set.
      // Don't do this when we are already in the last page.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);

      UpdateHighlightMenuInfo(NewPos, TopOfScreen, SkipValue);
      break;

    case CfUiDown:
      //
      // SkipValue means lines is skipped when show the top menu option.
      // NewPos  points to the menu which is highlighted now.
      //
      ControlFlag = CfRepaint;
      NewLine     = TRUE;

      if (NewPos == TopOfScreen) {
        Temp2 = SkipValue;
      } else {
        Temp2 = 0;
      }

      SavedListEntry = NewPos;
      //
      // Since the behavior of hitting the down arrow on a Date/Time op-code is intended
      // to be one that progresses to the next set of op-codes, we need to advance to the last
      // Date/Time op-code and leave the remaining logic in UiDown intact so the appropriate
      // checking can be done.  The only other logic we need to introduce is that if a Date/Time
      // op-code is the last entry in the menu, we need to rewind back to the first op-code of
      // the Date/Time op-code.
      //
      AdjustDateAndTimePosition (FALSE, &NewPos);

      MenuOption = MENU_OPTION_FROM_LINK (NewPos);
      NewPos     = NewPos->ForwardLink;
      //
      // Find the next selectable menu.
      //
      if (MenuOption->Row + MenuOption->Skip - Temp2 > BottomRow + 1) {
        if (gMenuOption.ForwardLink == NewPos || &gMenuOption == NewPos) {
          Difference = -1;
        } else {
          Difference = 0;
        }
      } else {
        Difference = MoveToNextStatement (FALSE, &NewPos, BottomRow + 1 - (MenuOption->Row + MenuOption->Skip - Temp2), FALSE);
      }
      if (Difference < 0) {
        //
        // Scroll to the first page.
        //
        if (TopOfScreen != gMenuOption.ForwardLink || SkipValue != 0) { 
          TopOfScreen = gMenuOption.ForwardLink;
          Repaint     = TRUE;
          MenuOption  = NULL;
        } else {
          MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);
        }
        NewPos        = gMenuOption.ForwardLink;
        MoveToNextStatement (FALSE, &NewPos, BottomRow - TopRow, TRUE);

        SkipValue = 0;
        //
        // If we are at the end of the list and sitting on a Date/Time op, rewind to the head.
        //
        AdjustDateAndTimePosition (TRUE, &TopOfScreen);
        AdjustDateAndTimePosition (TRUE, &NewPos);

        UpdateHighlightMenuInfo(NewPos, TopOfScreen, SkipValue);
        break;
      }        

      //
      // Get next selected menu info.
      //
      AdjustDateAndTimePosition (FALSE, &NewPos);
      NextMenuOption  = MENU_OPTION_FROM_LINK (NewPos);
      if (NextMenuOption->Row == 0) {
        UpdateOptionSkipLines (NextMenuOption);
      }

      //
      // Calculate new highlight menu end row.
      //
      Temp = (MenuOption->Row + MenuOption->Skip - Temp2) + Difference + NextMenuOption->Skip - 1;
      if (Temp > BottomRow) {
        //
        // Get the top screen menu info.
        //
        AdjustDateAndTimePosition (FALSE, &TopOfScreen);
        SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);

        //
        // Current Top screen menu occupy (SavedMenuOption->Skip - SkipValue) rows.
        // Full shows the new selected menu need to skip (Temp - BottomRow - 1) rows.
        //
        if ((Temp - BottomRow) >= (SavedMenuOption->Skip - SkipValue)) {
          //
          // Skip the top op-code
          //
          TopOfScreen   = TopOfScreen->ForwardLink;
          DistanceValue = (Temp - BottomRow) - (SavedMenuOption->Skip - SkipValue);

          SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);

          //
          // If we have a remainder, skip that many more op-codes until we drain the remainder
          // Special case is the selected highlight menu has more than one form of menus.
          //
          while (DistanceValue >= SavedMenuOption->Skip && TopOfScreen != NewPos) {
            //
            // Since the Difference is greater than or equal to this op-code's skip value, skip it
            //
            DistanceValue   = DistanceValue - (INTN) SavedMenuOption->Skip;
            TopOfScreen     = TopOfScreen->ForwardLink;
            SavedMenuOption = MENU_OPTION_FROM_LINK (TopOfScreen);
          }
          //
          // Since we will act on this op-code in the next routine, and increment the
          // SkipValue, set the skips to one less than what is required.
          //
          if (TopOfScreen != NewPos) {
            SkipValue = DistanceValue;
          } else {
            SkipValue = 0;
          }
        } else {
          //
          // Since we will act on this op-code in the next routine, and increment the
          // SkipValue, set the skips to one less than what is required.
          //
          SkipValue += Temp - BottomRow;
        }
        Repaint       = TRUE;
      } else if (!IsSelectable (NextMenuOption)) {
        //
        // Continue to go down until scroll to next page or the selectable option is found.
        //
        ScreenOperation = UiDown;
        ControlFlag     = CfScreenOperation;
        break;
      }

      MenuOption = MENU_OPTION_FROM_LINK (SavedListEntry);

      //
      // Check whether new highlight menu is selectable, if not, keep highlight on the old one.
      //
      // BottomRow - TopRow + 1 means the total rows current forms supported.
      // Difference + NextMenuOption->Skip + 1 means the distance between last highlight menu 
      // and new top menu. New top menu will all shows in next form, but last highlight menu 
      // may only shows 1 line. + 1 at right part means at least need to keep 1 line for the 
      // last highlight menu.
      // 
      if (!IsSelectable (NextMenuOption) && IsSelectable (MenuOption) && 
         (BottomRow - TopRow + 1 >= Difference + NextMenuOption->Skip + 1)) {
        NewPos = SavedListEntry;
      }

      UpdateStatusBar (INPUT_ERROR, FALSE);

      //
      // If we are at the end of the list and sitting on a Date/Time op, rewind to the head.
      //
      AdjustDateAndTimePosition (TRUE, &TopOfScreen);
      AdjustDateAndTimePosition (TRUE, &NewPos);

      UpdateHighlightMenuInfo(NewPos, TopOfScreen, SkipValue);
      break;

    case CfUiNoOperation:
      ControlFlag = CfRepaint;
      break;

    case CfExit:
      gST->ConOut->SetAttribute (gST->ConOut, EFI_TEXT_ATTR (EFI_LIGHTGRAY, EFI_BLACK));
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

/**
  Free the UI Menu Option structure data.

  @param   MenuOptionList         Point to the menu option list which need to be free.

**/
VOID
FreeMenuOptionData(
  LIST_ENTRY           *MenuOptionList
  )
{
  LIST_ENTRY           *Link;
  UI_MENU_OPTION       *Option;

  //
  // Free menu option list
  //
  while (!IsListEmpty (MenuOptionList)) {
    Link = GetFirstNode (MenuOptionList);
    Option = MENU_OPTION_FROM_LINK (Link);
    if (Option->Description != NULL){
      FreePool(Option->Description);
    }
    RemoveEntryList (&Option->Link);
    FreePool (Option);
  }
}

/**

  Base on the browser status info to show an pop up message.

**/
VOID
BrowserStatusProcess (
  VOID
  )
{
  CHAR16             *ErrorInfo;
  EFI_INPUT_KEY      Key;
  EFI_EVENT          WaitList[2];
  EFI_EVENT          RefreshIntervalEvent;
  EFI_EVENT          TimeOutEvent;
  UINT8              TimeOut;
  EFI_STATUS         Status;
  UINTN              Index;
  WARNING_IF_CONTEXT EventContext;
  EFI_IFR_OP_HEADER  *OpCodeBuf;
  EFI_STRING_ID      StringToken;
  CHAR16             DiscardChange;
  CHAR16             JumpToFormSet;
  CHAR16             *PrintString;

  if (gFormData->BrowserStatus == BROWSER_SUCCESS) {
    return;
  }

  StringToken          = 0;
  TimeOutEvent         = NULL;
  RefreshIntervalEvent = NULL;
  OpCodeBuf            = NULL;
  if (gFormData->HighLightedStatement != NULL) {
    OpCodeBuf = gFormData->HighLightedStatement->OpCode;
  }

  if (gFormData->BrowserStatus == (BROWSER_WARNING_IF)) {
    ASSERT (OpCodeBuf != NULL && OpCodeBuf->OpCode == EFI_IFR_WARNING_IF_OP);

    TimeOut     = ((EFI_IFR_WARNING_IF *) OpCodeBuf)->TimeOut;
    StringToken = ((EFI_IFR_WARNING_IF *) OpCodeBuf)->Warning;
  } else {
    TimeOut = 0;
    if ((gFormData->BrowserStatus == (BROWSER_NO_SUBMIT_IF)) &&
        (OpCodeBuf != NULL && OpCodeBuf->OpCode == EFI_IFR_NO_SUBMIT_IF_OP)) {
      StringToken = ((EFI_IFR_NO_SUBMIT_IF *) OpCodeBuf)->Error;
    } else if ((gFormData->BrowserStatus == (BROWSER_INCONSISTENT_IF)) &&
               (OpCodeBuf != NULL && OpCodeBuf->OpCode == EFI_IFR_INCONSISTENT_IF_OP)) {
      StringToken = ((EFI_IFR_INCONSISTENT_IF *) OpCodeBuf)->Error;
    }
  }

  if (StringToken != 0) {
    ErrorInfo = GetToken (StringToken, gFormData->HiiHandle);
  } else if (gFormData->ErrorString != NULL) {
    //
    // Only used to compatible with old setup browser.
    // Not use this field in new browser core.
    //
    ErrorInfo = gFormData->ErrorString;
  } else {
    switch (gFormData->BrowserStatus) {
    case BROWSER_SUBMIT_FAIL:
      ErrorInfo = gSaveFailed;
      break;

    case BROWSER_FORM_NOT_FOUND:
      ErrorInfo = gFormNotFound;
      break;

    case BROWSER_FORM_SUPPRESS:
      ErrorInfo = gFormSuppress;
      break;

    case BROWSER_PROTOCOL_NOT_FOUND:
      ErrorInfo = gProtocolNotFound;
      break;

    case BROWSER_SUBMIT_FAIL_NO_SUBMIT_IF:
      ErrorInfo = gNoSubmitIfFailed;
      break;

    case BROWSER_RECONNECT_FAIL:
      ErrorInfo = gReconnectFail;
      break;

    case BROWSER_RECONNECT_SAVE_CHANGES:
      ErrorInfo = gReconnectConfirmChanges;
      break;

    case BROWSER_RECONNECT_REQUIRED:
      ErrorInfo = gReconnectRequired;
      break;

    default:
      ErrorInfo = gBrowserError;
      break;
    }
  }

  switch (gFormData->BrowserStatus) {
  case BROWSER_SUBMIT_FAIL:
  case BROWSER_SUBMIT_FAIL_NO_SUBMIT_IF:
  case BROWSER_RECONNECT_SAVE_CHANGES:
    ASSERT (gUserInput != NULL);
    if (gFormData->BrowserStatus == (BROWSER_SUBMIT_FAIL)) {
      PrintString = gSaveProcess;
      JumpToFormSet = gJumpToFormSet[0];
      DiscardChange = gDiscardChange[0];
    } else if (gFormData->BrowserStatus == (BROWSER_RECONNECT_SAVE_CHANGES)){
      PrintString = gChangesOpt;
      JumpToFormSet = gConfirmOptYes[0];
      DiscardChange = gConfirmOptNo[0];
    } else {
      PrintString = gSaveNoSubmitProcess;
      JumpToFormSet = gCheckError[0];
      DiscardChange = gDiscardChange[0];
    }

    do {
      CreateDialog (&Key, gEmptyString, ErrorInfo, PrintString, gEmptyString, NULL);
    } while (((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (DiscardChange | UPPER_LOWER_CASE_OFFSET)) &&
             ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) != (JumpToFormSet | UPPER_LOWER_CASE_OFFSET)));

    if ((Key.UnicodeChar | UPPER_LOWER_CASE_OFFSET) == (DiscardChange | UPPER_LOWER_CASE_OFFSET)) {
      gUserInput->Action = BROWSER_ACTION_DISCARD;
    } else {
      gUserInput->Action = BROWSER_ACTION_GOTO;
    }
    break;

  default:
    if (TimeOut == 0) {
      do {
        CreateDialog (&Key, gEmptyString, ErrorInfo, gPressEnter, gEmptyString, NULL);
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    } else {
      Status = gBS->CreateEvent (EVT_NOTIFY_WAIT, TPL_CALLBACK, EmptyEventProcess, NULL, &TimeOutEvent);
      ASSERT_EFI_ERROR (Status);

      EventContext.SyncEvent = TimeOutEvent;
      EventContext.TimeOut   = &TimeOut;
      EventContext.ErrorInfo = ErrorInfo;

      Status = gBS->CreateEvent (EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, RefreshTimeOutProcess, &EventContext, &RefreshIntervalEvent);
      ASSERT_EFI_ERROR (Status);

      //
      // Show the dialog first to avoid long time not reaction.
      //
      gBS->SignalEvent (RefreshIntervalEvent);
    
      Status = gBS->SetTimer (RefreshIntervalEvent, TimerPeriodic, ONE_SECOND);
      ASSERT_EFI_ERROR (Status);

      while (TRUE) {
        Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        if (!EFI_ERROR (Status) && Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
          break;
        }

        if (Status != EFI_NOT_READY) {
          continue;
        }

        WaitList[0] = TimeOutEvent;
        WaitList[1] = gST->ConIn->WaitForKey;

        Status = gBS->WaitForEvent (2, WaitList, &Index);
        ASSERT_EFI_ERROR (Status);

        if (Index == 0) {
          //
          // Timeout occur, close the hoot time out event.
          //
          break;
        }
      }

      gBS->CloseEvent (TimeOutEvent);
      gBS->CloseEvent (RefreshIntervalEvent);
    }
    break;
  }

  if (StringToken != 0) {
    FreePool (ErrorInfo);
  }
}

/**
  Display one form, and return user input.
  
  @param FormData                Form Data to be shown.
  @param UserInputData           User input data.
  
  @retval EFI_SUCCESS            1.Form Data is shown, and user input is got.
                                 2.Error info has show and return.
  @retval EFI_INVALID_PARAMETER  The input screen dimension is not valid
  @retval EFI_NOT_FOUND          New form data has some error.
**/
EFI_STATUS
EFIAPI 
FormDisplay (
  IN  FORM_DISPLAY_ENGINE_FORM  *FormData,
  OUT USER_INPUT                *UserInputData
  )
{
  EFI_STATUS  Status;

  ASSERT (FormData != NULL);
  if (FormData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  gUserInput = UserInputData;
  gFormData  = FormData;

  //
  // Process the status info first.
  //
  BrowserStatusProcess();
  if (gFormData->BrowserStatus != BROWSER_SUCCESS) {
    //
    // gFormData->BrowserStatus != BROWSER_SUCCESS, means only need to print the error info, return here.
    //
    return EFI_SUCCESS;
  }

  Status = DisplayPageFrame (FormData, &gStatementDimensions);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Global Widths should be initialized before any MenuOption creation
  // or the GetWidth() used in UiAddMenuOption() will return incorrect value.
  //
  //
  //  Left                                              right
  //   |<-.->|<-.........->|<- .........->|<-...........->|
  //     Skip    Prompt         Option         Help 
  //
  gOptionBlockWidth = (CHAR16) ((gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn) / 3) + 1;
  gHelpBlockWidth   = (CHAR16) (gOptionBlockWidth - 1 - LEFT_SKIPPED_COLUMNS);
  gPromptBlockWidth = (CHAR16) (gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn - 2 * (gOptionBlockWidth - 1) - 1);

  ConvertStatementToMenu();

  //
  // Check whether layout is changed.
  //
  if (mIsFirstForm 
      || (gOldFormEntry.HiiHandle != FormData->HiiHandle)
      || (!CompareGuid (&gOldFormEntry.FormSetGuid, &FormData->FormSetGuid))
      || (gOldFormEntry.FormId != FormData->FormId)) {
    mStatementLayoutIsChanged = TRUE;
  } else {
    mStatementLayoutIsChanged = FALSE;
  }

  Status = UiDisplayMenu(FormData);
  
  //
  // Backup last form info.
  //
  mIsFirstForm            = FALSE;
  gOldFormEntry.HiiHandle = FormData->HiiHandle;
  CopyGuid (&gOldFormEntry.FormSetGuid, &FormData->FormSetGuid);
  gOldFormEntry.FormId    = FormData->FormId;

  //
  //Free the Ui menu option list.
  //
  FreeMenuOptionData(&gMenuOption);

  return Status;
}

/**
  Clear Screen to the initial state.
**/
VOID
EFIAPI 
DriverClearDisplayPage (
  VOID
  )
{
  ClearDisplayPage ();
  mIsFirstForm = TRUE;
}

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
  Initialize Setup Browser driver.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval EFI_SUCCESS    The Setup Browser module is initialized correctly..
  @return Other value if failed to initialize the Setup Browser module.

**/
EFI_STATUS
EFIAPI
InitializeDisplayEngine (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                          Status;
  EFI_INPUT_KEY                       HotKey;
  EFI_STRING                          NewString;
  EDKII_FORM_BROWSER_EXTENSION2_PROTOCOL *FormBrowserEx2;

  //
  // Publish our HII data
  //
  gHiiHandle = HiiAddPackages (
                 &gDisplayEngineGuid,
                 ImageHandle,
                 DisplayEngineStrings,
                 NULL
                 );
  ASSERT (gHiiHandle != NULL);

  //
  // Install Form Display protocol
  //
  Status = gBS->InstallProtocolInterface (
                  &mPrivateData.Handle,
                  &gEdkiiFormDisplayEngineProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mPrivateData.FromDisplayProt
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Install HII Popup Protocol.
  //
  Status = gBS->InstallProtocolInterface (
                 &mPrivateData.Handle,
                 &gEfiHiiPopupProtocolGuid,
                 EFI_NATIVE_INTERFACE,
                 &mPrivateData.HiiPopup
                );
  ASSERT_EFI_ERROR (Status);

  InitializeDisplayStrings();
  
  ZeroMem (&gHighligthMenuInfo, sizeof (gHighligthMenuInfo));
  ZeroMem (&gOldFormEntry, sizeof (gOldFormEntry));

  //
  // Use BrowserEx2 protocol to register HotKey.
  // 
  Status = gBS->LocateProtocol (&gEdkiiFormBrowserEx2ProtocolGuid, NULL, (VOID **) &FormBrowserEx2);
  if (!EFI_ERROR (Status)) {
    //
    // Register the default HotKey F9 and F10 again.
    //
    HotKey.UnicodeChar = CHAR_NULL;
    HotKey.ScanCode   = SCAN_F10;
    NewString         = HiiGetString (gHiiHandle, STRING_TOKEN (FUNCTION_TEN_STRING), NULL);
    ASSERT (NewString != NULL);
    FormBrowserEx2->RegisterHotKey (&HotKey, BROWSER_ACTION_SUBMIT, 0, NewString);

    HotKey.ScanCode   = SCAN_F9;
    NewString         = HiiGetString (gHiiHandle, STRING_TOKEN (FUNCTION_NINE_STRING), NULL);
    ASSERT (NewString != NULL);
    FormBrowserEx2->RegisterHotKey (&HotKey, BROWSER_ACTION_DEFAULT, EFI_HII_DEFAULT_CLASS_STANDARD, NewString);
  }

  return EFI_SUCCESS;
}

/**
  This is the default unload handle for display core drivers.

  @param[in]  ImageHandle       The drivers' driver image.

  @retval EFI_SUCCESS           The image is unloaded.
  @retval Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
UnloadDisplayEngine (
  IN EFI_HANDLE             ImageHandle
  )
{
  HiiRemovePackages(gHiiHandle);

  FreeDisplayStrings ();

  if (gHighligthMenuInfo.HLTOpCode != NULL) {
    FreePool (gHighligthMenuInfo.HLTOpCode);
  }

  if (gHighligthMenuInfo.TOSOpCode != NULL) {
    FreePool (gHighligthMenuInfo.TOSOpCode);
  }

  return EFI_SUCCESS;
}
