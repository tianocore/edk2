/** @file
Implementation for handling the User Interface option processing.


Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"


/**
  Process Question Config.

  @param  Selection              The UI menu selection.
  @param  Question               The Question to be peocessed.

  @retval EFI_SUCCESS            Question Config process success.
  @retval Other                  Question Config process fail.

**/
EFI_STATUS
ProcessQuestionConfig (
  IN  UI_MENU_SELECTION       *Selection,
  IN  FORM_BROWSER_STATEMENT  *Question
  )
{
  EFI_STATUS                      Status;
  CHAR16                          *ConfigResp;
  CHAR16                          *Progress;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;

  if (Question->QuestionConfig == 0) {
    return EFI_SUCCESS;
  }

  //
  // Get <ConfigResp>
  //
  ConfigResp = GetToken (Question->QuestionConfig, Selection->FormSet->HiiHandle);
  if (ConfigResp == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Send config to Configuration Driver
  //
  ConfigAccess = Selection->FormSet->ConfigAccess;
  if (ConfigAccess == NULL) {
    return EFI_UNSUPPORTED;
  }
  Status = ConfigAccess->RouteConfig (
                           ConfigAccess,
                           ConfigResp,
                           &Progress
                           );

  return Status;
}


/**
  Search an Option of a Question by its value.

  @param  Question               The Question
  @param  OptionValue            Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
QUESTION_OPTION *
ValueToOption (
  IN FORM_BROWSER_STATEMENT   *Question,
  IN EFI_HII_VALUE            *OptionValue
  )
{
  LIST_ENTRY       *Link;
  QUESTION_OPTION  *Option;
  INTN             Result;

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = QUESTION_OPTION_FROM_LINK (Link);

    if ((CompareHiiValue (&Option->Value, OptionValue, &Result, NULL) == EFI_SUCCESS) && (Result == 0)) {
      return Option;
    }

    Link = GetNextNode (&Question->OptionListHead, Link);
  }

  return NULL;
}


/**
  Return data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.

  @retval Value                  The data to be returned

**/
UINT64
GetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index
  )
{
  UINT64 Data;

  ASSERT (Array != NULL);

  Data = 0;
  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    Data = (UINT64) *(((UINT8 *) Array) + Index);
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    Data = (UINT64) *(((UINT16 *) Array) + Index);
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    Data = (UINT64) *(((UINT32 *) Array) + Index);
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    Data = (UINT64) *(((UINT64 *) Array) + Index);
    break;

  default:
    break;
  }

  return Data;
}


/**
  Set value of a data element in an Array by its Index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.
  @param  Value                  The value to be set.

**/
VOID
SetArrayData (
  IN VOID                     *Array,
  IN UINT8                    Type,
  IN UINTN                    Index,
  IN UINT64                   Value
  )
{

  ASSERT (Array != NULL);

  switch (Type) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    *(((UINT8 *) Array) + Index) = (UINT8) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    *(((UINT16 *) Array) + Index) = (UINT16) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    *(((UINT32 *) Array) + Index) = (UINT32) Value;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    *(((UINT64 *) Array) + Index) = (UINT64) Value;
    break;

  default:
    break;
  }
}


/**
  Print Question Value according to it's storage width and display attributes.

  @param  Question               The Question to be printed.
  @param  FormattedNumber        Buffer for output string.
  @param  BufferSize             The FormattedNumber buffer size in bytes.

  @retval EFI_SUCCESS            Print success.
  @retval EFI_BUFFER_TOO_SMALL   Buffer size is not enough for formatted number.

**/
EFI_STATUS
PrintFormattedNumber (
  IN FORM_BROWSER_STATEMENT   *Question,
  IN OUT CHAR16               *FormattedNumber,
  IN UINTN                    BufferSize
  )
{
  INT64          Value;
  CHAR16         *Format;
  EFI_HII_VALUE  *QuestionValue;

  if (BufferSize < (21 * sizeof (CHAR16))) {
    return EFI_BUFFER_TOO_SMALL;
  }

  QuestionValue = &Question->HiiValue;

  Value = (INT64) QuestionValue->Value.u64;
  switch (Question->Flags & EFI_IFR_DISPLAY) {
  case EFI_IFR_DISPLAY_INT_DEC:
    switch (QuestionValue->Type) {
    case EFI_IFR_NUMERIC_SIZE_1:
      Value = (INT64) ((INT8) QuestionValue->Value.u8);
      break;

    case EFI_IFR_NUMERIC_SIZE_2:
      Value = (INT64) ((INT16) QuestionValue->Value.u16);
      break;

    case EFI_IFR_NUMERIC_SIZE_4:
      Value = (INT64) ((INT32) QuestionValue->Value.u32);
      break;

    case EFI_IFR_NUMERIC_SIZE_8:
    default:
      break;
    }

    if (Value < 0) {
      Value = -Value;
      Format = L"-%ld";
    } else {
      Format = L"%ld";
    }
    break;

  case EFI_IFR_DISPLAY_UINT_DEC:
    Format = L"%ld";
    break;

  case EFI_IFR_DISPLAY_UINT_HEX:
    Format = L"%lx";
    break;

  default:
    return EFI_UNSUPPORTED;
    break;
  }

  UnicodeSPrint (FormattedNumber, BufferSize, Format, Value);

  return EFI_SUCCESS;
}


/**
  Password may be stored as encrypted by Configuration Driver. When change a
  password, user will be challenged with old password. To validate user input old
  password, we will send the clear text to Configuration Driver via Callback().
  Configuration driver is responsible to check the passed in password and return
  the validation result. If validation pass, state machine in password Callback()
  will transit from BROWSER_STATE_VALIDATE_PASSWORD to BROWSER_STATE_SET_PASSWORD.
  After user type in new password twice, Callback() will be invoked to send the
  new password to Configuration Driver.

  @param  Selection              Pointer to UI_MENU_SELECTION.
  @param  MenuOption             The MenuOption for this password Question.
  @param  String                 The clear text of password.

  @retval EFI_NOT_AVAILABLE_YET  Callback() request to terminate password input.
  @return In state of BROWSER_STATE_VALIDATE_PASSWORD:
  @retval EFI_SUCCESS            Password correct, Browser will prompt for new
                                 password.
  @retval EFI_NOT_READY          Password incorrect, Browser will show error
                                 message.
  @retval Other                  Browser will do nothing.
  @return In state of BROWSER_STATE_SET_PASSWORD:
  @retval EFI_SUCCESS            Set password success.
  @retval Other                  Set password failed.

**/
EFI_STATUS
PasswordCallback (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption,
  IN  CHAR16                      *String
  )
{
  EFI_STATUS                      Status;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  EFI_IFR_TYPE_VALUE              IfrTypeValue;

  ConfigAccess = Selection->FormSet->ConfigAccess;
  if (ConfigAccess == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Prepare password string in HII database
  //
  if (String != NULL) {
    IfrTypeValue.string = NewString (String, Selection->FormSet->HiiHandle);
  } else {
    IfrTypeValue.string = 0;
  }

  //
  // Send password to Configuration Driver for validation
  //
  Status = ConfigAccess->Callback (
                           ConfigAccess,
                           EFI_BROWSER_ACTION_CHANGING,
                           MenuOption->ThisTag->QuestionId,
                           MenuOption->ThisTag->HiiValue.Type,
                           &IfrTypeValue,
                           &ActionRequest
                           );

  //
  // Remove password string from HII database
  //
  if (String != NULL) {
    DeleteString (IfrTypeValue.string, Selection->FormSet->HiiHandle);
  }

  return Status;
}


/**
  Display error message for invalid password.

**/
VOID
PasswordInvalid (
  VOID
  )
{
  EFI_INPUT_KEY  Key;

  //
  // Invalid password, prompt error message
  //
  do {
    CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gPassowordInvalid, gPressEnter, gEmptyString);
  } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
}


/**
  Process a Question's Option (whether selected or un-selected).

  @param  Selection              Pointer to UI_MENU_SELECTION.
  @param  MenuOption             The MenuOption for this Question.
  @param  Selected               TRUE: if Question is selected.
  @param  OptionString           Pointer of the Option String to be displayed.

  @retval EFI_SUCCESS            Question Option process success.
  @retval Other                  Question Option process fail.

**/
EFI_STATUS
ProcessOptions (
  IN  UI_MENU_SELECTION           *Selection,
  IN  UI_MENU_OPTION              *MenuOption,
  IN  BOOLEAN                     Selected,
  OUT CHAR16                      **OptionString
  )
{
  EFI_STATUS                      Status;
  CHAR16                          *StringPtr;
  CHAR16                          *TempString;
  UINTN                           Index;
  FORM_BROWSER_STATEMENT          *Question;
  CHAR16                          FormattedNumber[21];
  UINT16                          Number;
  CHAR16                          Character[2];
  EFI_INPUT_KEY                   Key;
  UINTN                           BufferSize;
  QUESTION_OPTION                 *OneOfOption;
  LIST_ENTRY                      *Link;
  EFI_HII_VALUE                   HiiValue;
  EFI_HII_VALUE                   *QuestionValue;
  BOOLEAN                         Suppress;
  UINT16                          Maximum;
  QUESTION_OPTION                 *Option;
  UINTN                           Index2;
  UINT8                           *ValueArray;
  UINT8                           ValueType;
  EFI_STRING_ID                   StringId;

  Status        = EFI_SUCCESS;

  StringPtr     = NULL;
  Character[1]  = L'\0';
  *OptionString = NULL;
  StringId      = 0;

  ZeroMem (FormattedNumber, 21 * sizeof (CHAR16));
  BufferSize = (gOptionBlockWidth + 1) * 2 * gScreenDimensions.BottomRow;

  Question = MenuOption->ThisTag;
  QuestionValue = &Question->HiiValue;
  Maximum = (UINT16) Question->Maximum;

  ValueArray = Question->BufferValue;
  ValueType = Question->ValueType;

  switch (Question->Operand) {
  case EFI_IFR_ORDERED_LIST_OP:
    //
    // Check whether there are Options of this OrderedList
    //
    if (IsListEmpty (&Question->OptionListHead)) {
      break;
    }
    //
    // Initialize Option value array
    //
    if (GetArrayData (ValueArray, ValueType, 0) == 0) {
      GetQuestionDefault (Selection->FormSet, Selection->Form, Question, 0);
    }

    if (Selected) {
      //
      // Go ask for input
      //
      Status = GetSelectionInputPopUp (Selection, MenuOption);
    } else {
      //
      // We now know how many strings we will have, so we can allocate the
      // space required for the array or strings.
      //
      *OptionString = AllocateZeroPool (Question->MaxContainers * BufferSize);
      ASSERT (*OptionString);

      HiiValue.Type = ValueType;
      HiiValue.Value.u64 = 0;
      for (Index = 0; Index < Question->MaxContainers; Index++) {
        HiiValue.Value.u64 = GetArrayData (ValueArray, ValueType, Index);
        if (HiiValue.Value.u64 == 0) {
          //
          // Values for the options in ordered lists should never be a 0
          //
          break;
        }

        OneOfOption = ValueToOption (Question, &HiiValue);
        if (OneOfOption == NULL) {
          //
          // Show error message
          //
          do {
            CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gOptionMismatch, gPressEnter, gEmptyString);
          } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

          //
          // The initial value of the orderedlist is invalid, force to be valid value
          //
          Link = GetFirstNode (&Question->OptionListHead);
          Index2 = 0;
          while (!IsNull (&Question->OptionListHead, Link) && Index2 < Question->MaxContainers) {
            Option = QUESTION_OPTION_FROM_LINK (Link);
            SetArrayData (ValueArray, ValueType, Index2, Option->Value.Value.u64);
            Index2++;
            Link = GetNextNode (&Question->OptionListHead, Link);
          }
          SetArrayData (ValueArray, ValueType, Index2, 0);

          Status = SetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithEditBuffer);
          UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);

          FreePool (*OptionString);
          *OptionString = NULL;
          return EFI_NOT_FOUND;
        }

        Suppress = FALSE;
        if ((OneOfOption->SuppressExpression != NULL) &&
            (EvaluateExpressionList(OneOfOption->SuppressExpression, FALSE, NULL, NULL) == ExpressSuppress)) {
          //
          // This option is suppressed
          //
          Suppress = TRUE;
        }

        if (!Suppress) {
          Character[0] = LEFT_ONEOF_DELIMITER;
          NewStrCat (OptionString[0], Character);
          StringPtr = GetToken (OneOfOption->Text, Selection->Handle);
          ASSERT (StringPtr != NULL);
          NewStrCat (OptionString[0], StringPtr);
          Character[0] = RIGHT_ONEOF_DELIMITER;
          NewStrCat (OptionString[0], Character);
          Character[0] = CHAR_CARRIAGE_RETURN;
          NewStrCat (OptionString[0], Character);

          FreePool (StringPtr);
        }
      }
    }
    break;

  case EFI_IFR_ONE_OF_OP:
    //
    // Check whether there are Options of this OneOf
    //
    if (IsListEmpty (&Question->OptionListHead)) {
      break;
    }
    if (Selected) {
      //
      // Go ask for input
      //
      Status = GetSelectionInputPopUp (Selection, MenuOption);
    } else {
      *OptionString = AllocateZeroPool (BufferSize);
      ASSERT (*OptionString);

      OneOfOption = ValueToOption (Question, QuestionValue);
      if (OneOfOption == NULL) {
        //
        // Show error message
        //
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gOptionMismatch, gPressEnter, gEmptyString);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

        //
        // Force the Question value to be valid
        //
        Link = GetFirstNode (&Question->OptionListHead);
        while (!IsNull (&Question->OptionListHead, Link)) {
          Option = QUESTION_OPTION_FROM_LINK (Link);

          if ((Option->SuppressExpression == NULL) ||
              (EvaluateExpressionList(Option->SuppressExpression, FALSE, NULL, NULL) == ExpressFalse)) {
            CopyMem (QuestionValue, &Option->Value, sizeof (EFI_HII_VALUE));
            SetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithEditBuffer);
            UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);
            break;
          }

          Link = GetNextNode (&Question->OptionListHead, Link);
        }

        FreePool (*OptionString);
        *OptionString = NULL;
        return EFI_NOT_FOUND;
      }

      if ((OneOfOption->SuppressExpression != NULL) &&
          ((EvaluateExpressionList(OneOfOption->SuppressExpression, FALSE, NULL, NULL) == ExpressSuppress))) {
        //
        // This option is suppressed
        //
        Suppress = TRUE;
      } else {
        Suppress = FALSE;
      }

      if (Suppress) {
        //
        // Current selected option happen to be suppressed,
        // enforce to select on a non-suppressed option
        //
        Link = GetFirstNode (&Question->OptionListHead);
        while (!IsNull (&Question->OptionListHead, Link)) {
          OneOfOption = QUESTION_OPTION_FROM_LINK (Link);

          if ((OneOfOption->SuppressExpression == NULL) ||
              (EvaluateExpressionList(OneOfOption->SuppressExpression, FALSE, NULL, NULL) == ExpressFalse)) {
            Suppress = FALSE;
            CopyMem (QuestionValue, &OneOfOption->Value, sizeof (EFI_HII_VALUE));
            SetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithEditBuffer);
            UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);
            gST->ConOut->SetAttribute (gST->ConOut, PcdGet8 (PcdBrowserFieldTextColor) | FIELD_BACKGROUND);
            break;
          }

          Link = GetNextNode (&Question->OptionListHead, Link);
        }
      }

      if (!Suppress) {
        Character[0] = LEFT_ONEOF_DELIMITER;
        NewStrCat (OptionString[0], Character);
        StringPtr = GetToken (OneOfOption->Text, Selection->Handle);
        ASSERT (StringPtr != NULL);
        NewStrCat (OptionString[0], StringPtr);
        Character[0] = RIGHT_ONEOF_DELIMITER;
        NewStrCat (OptionString[0], Character);

        FreePool (StringPtr);
      }
    }
    break;

  case EFI_IFR_CHECKBOX_OP:
    *OptionString = AllocateZeroPool (BufferSize);
    ASSERT (*OptionString);

    *OptionString[0] = LEFT_CHECKBOX_DELIMITER;

    if (Selected) {
      //
      // Since this is a BOOLEAN operation, flip it upon selection
      //
      QuestionValue->Value.b = (BOOLEAN) (QuestionValue->Value.b ? FALSE : TRUE);

      //
      // Perform inconsistent check
      //
      Status = ValidateQuestion (Selection->FormSet, Selection->Form, Question, EFI_HII_EXPRESSION_INCONSISTENT_IF);
      if (EFI_ERROR (Status)) {
        //
        // Inconsistent check fail, restore Question Value
        //
        QuestionValue->Value.b = (BOOLEAN) (QuestionValue->Value.b ? FALSE : TRUE);
        FreePool (*OptionString);
        *OptionString = NULL;
        return Status;
      }

      //
      // Save Question value
      //
      Status = SetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithEditBuffer);
      UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);
    }

    if (QuestionValue->Value.b) {
      *(OptionString[0] + 1) = CHECK_ON;
    } else {
      *(OptionString[0] + 1) = CHECK_OFF;
    }
    *(OptionString[0] + 2) = RIGHT_CHECKBOX_DELIMITER;
    break;

  case EFI_IFR_NUMERIC_OP:
    if (Selected) {
      //
      // Go ask for input
      //
      Status = GetNumericInput (Selection, MenuOption);
    } else {
      *OptionString = AllocateZeroPool (BufferSize);
      ASSERT (*OptionString);

      *OptionString[0] = LEFT_NUMERIC_DELIMITER;

      //
      // Formatted print
      //
      PrintFormattedNumber (Question, FormattedNumber, 21 * sizeof (CHAR16));
      Number = (UINT16) GetStringWidth (FormattedNumber);
      CopyMem (OptionString[0] + 1, FormattedNumber, Number);

      *(OptionString[0] + Number / 2) = RIGHT_NUMERIC_DELIMITER;
    }
    break;

  case EFI_IFR_DATE_OP:
    if (Selected) {
      //
      // This is similar to numerics
      //
      Status = GetNumericInput (Selection, MenuOption);
    } else {
      *OptionString = AllocateZeroPool (BufferSize);
      ASSERT (*OptionString);

      switch (MenuOption->Sequence) {
      case 0:
        *OptionString[0] = LEFT_NUMERIC_DELIMITER;
        UnicodeSPrint (OptionString[0] + 1, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.date.Month);
        *(OptionString[0] + 3) = DATE_SEPARATOR;
        break;

      case 1:
        SetUnicodeMem (OptionString[0], 4, L' ');
        UnicodeSPrint (OptionString[0] + 4, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.date.Day);
        *(OptionString[0] + 6) = DATE_SEPARATOR;
        break;

      case 2:
        SetUnicodeMem (OptionString[0], 7, L' ');
        UnicodeSPrint (OptionString[0] + 7, 21 * sizeof (CHAR16), L"%04d", QuestionValue->Value.date.Year);
        *(OptionString[0] + 11) = RIGHT_NUMERIC_DELIMITER;
        break;
      }
    }
    break;

  case EFI_IFR_TIME_OP:
    if (Selected) {
      //
      // This is similar to numerics
      //
      Status = GetNumericInput (Selection, MenuOption);
    } else {
      *OptionString = AllocateZeroPool (BufferSize);
      ASSERT (*OptionString);

      switch (MenuOption->Sequence) {
      case 0:
        *OptionString[0] = LEFT_NUMERIC_DELIMITER;
        UnicodeSPrint (OptionString[0] + 1, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.time.Hour);
        *(OptionString[0] + 3) = TIME_SEPARATOR;
        break;

      case 1:
        SetUnicodeMem (OptionString[0], 4, L' ');
        UnicodeSPrint (OptionString[0] + 4, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.time.Minute);
        *(OptionString[0] + 6) = TIME_SEPARATOR;
        break;

      case 2:
        SetUnicodeMem (OptionString[0], 7, L' ');
        UnicodeSPrint (OptionString[0] + 7, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.time.Second);
        *(OptionString[0] + 9) = RIGHT_NUMERIC_DELIMITER;
        break;
      }
    }
    break;

  case EFI_IFR_STRING_OP:
    if (Selected) {
      StringPtr = AllocateZeroPool ((Maximum + 1) * sizeof (CHAR16));
      ASSERT (StringPtr);
      CopyMem(StringPtr, Question->BufferValue, Maximum * sizeof (CHAR16));

      Status = ReadString (MenuOption, gPromptForData, StringPtr);
      if (!EFI_ERROR (Status)) {
        HiiSetString(Selection->FormSet->HiiHandle, Question->HiiValue.Value.string, StringPtr, NULL);
        Status = ValidateQuestion(Selection->FormSet, Selection->Form, Question, EFI_HII_EXPRESSION_INCONSISTENT_IF);
        if (EFI_ERROR (Status)) {
          HiiSetString(Selection->FormSet->HiiHandle, Question->HiiValue.Value.string, (CHAR16*)Question->BufferValue, NULL);
        } else {
          CopyMem (Question->BufferValue, StringPtr, Maximum * sizeof (CHAR16));
          SetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithEditBuffer);

          UpdateStatusBar (Selection, NV_UPDATE_REQUIRED, Question->QuestionFlags, TRUE);
        }
      }

      FreePool (StringPtr);
    } else {
      *OptionString = AllocateZeroPool (BufferSize);
      ASSERT (*OptionString);

      if (((CHAR16 *) Question->BufferValue)[0] == 0x0000) {
        *(OptionString[0]) = '_';
      } else {
        if ((Maximum * sizeof (CHAR16)) < BufferSize) {
          BufferSize = Maximum * sizeof (CHAR16);
        }
        CopyMem (OptionString[0], (CHAR16 *) Question->BufferValue, BufferSize);
      }
    }
    break;

  case EFI_IFR_PASSWORD_OP:
    if (Selected) {
      StringPtr = AllocateZeroPool ((Maximum + 1) * sizeof (CHAR16));
      ASSERT (StringPtr);

      //
      // For interactive passwords, old password is validated by callback
      //
      if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK)  != 0) {
        //
        // Use a NULL password to test whether old password is required
        //
        *StringPtr = 0;
        Status = PasswordCallback (Selection, MenuOption, StringPtr);
        if (Status == EFI_NOT_AVAILABLE_YET || Status == EFI_UNSUPPORTED) {
          //
          // Callback is not supported, or
          // Callback request to terminate password input
          //
          FreePool (StringPtr);
          return EFI_SUCCESS;
        }

        if (EFI_ERROR (Status)) {
          //
          // Old password exist, ask user for the old password
          //
          Status = ReadString (MenuOption, gPromptForPassword, StringPtr);
          if (EFI_ERROR (Status)) {
            FreePool (StringPtr);
            return Status;
          }

          //
          // Check user input old password
          //
          Status = PasswordCallback (Selection, MenuOption, StringPtr);
          if (EFI_ERROR (Status)) {
            if (Status == EFI_NOT_READY) {
              //
              // Typed in old password incorrect
              //
              PasswordInvalid ();
            } else {
              Status = EFI_SUCCESS;
            }

            FreePool (StringPtr);
            return Status;
          }
        }
      } else {
        //
        // For non-interactive password, validate old password in local
        //
        if (*((CHAR16 *) Question->BufferValue) != 0) {
          //
          // There is something there!  Prompt for password
          //
          Status = ReadString (MenuOption, gPromptForPassword, StringPtr);
          if (EFI_ERROR (Status)) {
            FreePool (StringPtr);
            return Status;
          }

          TempString = AllocateCopyPool ((Maximum + 1) * sizeof (CHAR16), Question->BufferValue);
          ASSERT (TempString != NULL);

          TempString[Maximum] = L'\0';

          if (StrCmp (StringPtr, TempString) != 0) {
            //
            // Typed in old password incorrect
            //
            PasswordInvalid ();

            FreePool (StringPtr);
            FreePool (TempString);
            return Status;
          }

          FreePool (TempString);
        }
      }

      //
      // Ask for new password
      //
      ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
      Status = ReadString (MenuOption, gPromptForNewPassword, StringPtr);
      if (EFI_ERROR (Status)) {
        //
        // Reset state machine for interactive password
        //
        if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          PasswordCallback (Selection, MenuOption, NULL);
        }

        FreePool (StringPtr);
        return Status;
      }

      //
      // Confirm new password
      //
      TempString = AllocateZeroPool ((Maximum + 1) * sizeof (CHAR16));
      ASSERT (TempString);
      Status = ReadString (MenuOption, gConfirmPassword, TempString);
      if (EFI_ERROR (Status)) {
        //
        // Reset state machine for interactive password
        //
        if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          PasswordCallback (Selection, MenuOption, NULL);
        }

        FreePool (StringPtr);
        FreePool (TempString);
        return Status;
      }

      //
      // Compare two typed-in new passwords
      //
      if (StrCmp (StringPtr, TempString) == 0) {
        //
        // Prepare the  Question->HiiValue.Value.string for ValidateQuestion use.
        //
        if((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          StringId = Question->HiiValue.Value.string;
          Question->HiiValue.Value.string = NewString (StringPtr, Selection->FormSet->HiiHandle);
        } else {
          HiiSetString(Selection->FormSet->HiiHandle, Question->HiiValue.Value.string, StringPtr, NULL);
        }
        
        Status = ValidateQuestion(Selection->FormSet, Selection->Form, Question, EFI_HII_EXPRESSION_INCONSISTENT_IF);

        //
        //  Researve the Question->HiiValue.Value.string.
        //
        if((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          DeleteString(Question->HiiValue.Value.string, Selection->FormSet->HiiHandle);
          Question->HiiValue.Value.string = StringId;
        }   
        
        if (EFI_ERROR (Status)) {
          //
          // Reset state machine for interactive password
          //
          if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
            PasswordCallback (Selection, MenuOption, NULL);
          } else {
            //
            // Researve the Question->HiiValue.Value.string.
            //
            HiiSetString(Selection->FormSet->HiiHandle, Question->HiiValue.Value.string, (CHAR16*)Question->BufferValue, NULL);            
          }
        } else {
          //
          // Two password match, send it to Configuration Driver
          //
          if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
            PasswordCallback (Selection, MenuOption, StringPtr);
          } else {
            CopyMem (Question->BufferValue, StringPtr, Maximum * sizeof (CHAR16));
            SetQuestionValue (Selection->FormSet, Selection->Form, Question, GetSetValueWithHiiDriver);
          }
        }
      } else {
        //
        // Reset state machine for interactive password
        //
        if ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) {
          PasswordCallback (Selection, MenuOption, NULL);
        }

        //
        // Two password mismatch, prompt error message
        //
        do {
          CreateDialog (4, TRUE, 0, NULL, &Key, gEmptyString, gConfirmError, gPressEnter, gEmptyString);
        } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
      }

      FreePool (TempString);
      FreePool (StringPtr);
    }
    break;

  default:
    break;
  }

  return Status;
}


/**
  Process the help string: Split StringPtr to several lines of strings stored in
  FormattedString and the glyph width of each line cannot exceed gHelpBlockWidth.

  @param  StringPtr              The entire help string.
  @param  FormattedString        The oupput formatted string.
  @param  EachLineWidth          The max string length of each line in the formatted string.
  @param  RowCount               TRUE: if Question is selected.

**/
UINTN
ProcessHelpString (
  IN  CHAR16  *StringPtr,
  OUT CHAR16  **FormattedString,
  OUT UINT16  *EachLineWidth,
  IN  UINTN   RowCount
  )
{
  UINTN   Index;
  CHAR16  *OutputString;
  UINTN   TotalRowNum;
  UINTN   CheckedNum;
  UINT16  GlyphWidth;
  UINT16  LineWidth;
  UINT16  MaxStringLen;
  UINT16  StringLen;

  TotalRowNum    = 0;
  CheckedNum     = 0;
  GlyphWidth     = 1;
  Index          = 0;
  MaxStringLen   = 0;
  StringLen      = 0;

  //
  // Set default help string width.
  //
  LineWidth      = (UINT16) (gHelpBlockWidth - 1);

  //
  // Get row number of the String.
  //
  while ((StringLen = GetLineByWidth (StringPtr, LineWidth, &GlyphWidth, &Index, &OutputString)) != 0) {
    if (StringLen > MaxStringLen) {
      MaxStringLen = StringLen;
    }

    TotalRowNum ++;
    FreePool (OutputString);
  }
  *EachLineWidth = MaxStringLen;

  *FormattedString = AllocateZeroPool (TotalRowNum * MaxStringLen * sizeof (CHAR16));
  ASSERT (*FormattedString != NULL);

  //
  // Generate formatted help string array.
  //
  GlyphWidth  = 1;
  Index       = 0;
  while((StringLen = GetLineByWidth (StringPtr, LineWidth, &GlyphWidth, &Index, &OutputString)) != 0) {
    CopyMem (*FormattedString + CheckedNum * MaxStringLen, OutputString, StringLen * sizeof (CHAR16));
    CheckedNum ++;
    FreePool (OutputString);
  }

  return TotalRowNum; 
}
