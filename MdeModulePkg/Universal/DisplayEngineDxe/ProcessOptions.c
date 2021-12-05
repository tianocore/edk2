/** @file
Implementation for handling the User Interface option processing.


Copyright (c) 2004 - 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "FormDisplay.h"

#define MAX_TIME_OUT_LEN  0x10

/**
  Concatenate a narrow string to another string.

  @param Destination The destination string.
  @param DestMax     The Max length of destination string.
  @param Source      The source string. The string to be concatenated.
                     to the end of Destination.

**/
VOID
NewStrCat (
  IN OUT CHAR16  *Destination,
  IN     UINTN   DestMax,
  IN     CHAR16  *Source
  )
{
  UINTN  Length;

  for (Length = 0; Destination[Length] != 0; Length++) {
  }

  //
  // We now have the length of the original string
  // We can safely assume for now that we are concatenating a narrow value to this string.
  // For instance, the string is "XYZ" and cat'ing ">"
  // If this assumption changes, we need to make this routine a bit more complex
  //
  Destination[Length] = NARROW_CHAR;
  Length++;

  StrCpyS (Destination + Length, DestMax - Length, Source);
}

/**
  Get UINT64 type value.

  @param  Value                  Input Hii value.

  @retval UINT64                 Return the UINT64 type value.

**/
UINT64
HiiValueToUINT64 (
  IN EFI_HII_VALUE  *Value
  )
{
  UINT64  RetVal;

  RetVal = 0;

  switch (Value->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      RetVal = Value->Value.u8;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      RetVal = Value->Value.u16;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      RetVal = Value->Value.u32;
      break;

    case EFI_IFR_TYPE_BOOLEAN:
      RetVal = Value->Value.b;
      break;

    case EFI_IFR_TYPE_DATE:
      RetVal = *(UINT64 *)&Value->Value.date;
      break;

    case EFI_IFR_TYPE_TIME:
      RetVal = (*(UINT64 *)&Value->Value.time) & 0xffffff;
      break;

    default:
      RetVal = Value->Value.u64;
      break;
  }

  return RetVal;
}

/**
  Check whether this value type can be transfer to EFI_IFR_TYPE_BUFFER type.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param  Value                  Expression value to compare on.

  @retval TRUE                   This value type can be transter to EFI_IFR_TYPE_BUFFER type.
  @retval FALSE                  This value type can't be transter to EFI_IFR_TYPE_BUFFER type.

**/
BOOLEAN
IsTypeInBuffer (
  IN  EFI_HII_VALUE  *Value
  )
{
  switch (Value->Type) {
    case EFI_IFR_TYPE_BUFFER:
    case EFI_IFR_TYPE_DATE:
    case EFI_IFR_TYPE_TIME:
    case EFI_IFR_TYPE_REF:
      return TRUE;

    default:
      return FALSE;
  }
}

/**
  Check whether this value type can be transfer to EFI_IFR_TYPE_UINT64

  @param  Value                  Expression value to compare on.

  @retval TRUE                   This value type can be transter to EFI_IFR_TYPE_BUFFER type.
  @retval FALSE                  This value type can't be transter to EFI_IFR_TYPE_BUFFER type.

**/
BOOLEAN
IsTypeInUINT64 (
  IN  EFI_HII_VALUE  *Value
  )
{
  switch (Value->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
    case EFI_IFR_TYPE_NUM_SIZE_16:
    case EFI_IFR_TYPE_NUM_SIZE_32:
    case EFI_IFR_TYPE_NUM_SIZE_64:
    case EFI_IFR_TYPE_BOOLEAN:
      return TRUE;

    default:
      return FALSE;
  }
}

/**
  Return the buffer length and buffer pointer for this value.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param  Value                  Expression value to compare on.
  @param  Buf                    Return the buffer pointer.
  @param  BufLen                 Return the buffer length.

**/
VOID
GetBufAndLenForValue (
  IN  EFI_HII_VALUE  *Value,
  OUT UINT8          **Buf,
  OUT UINT16         *BufLen
  )
{
  switch (Value->Type) {
    case EFI_IFR_TYPE_BUFFER:
      *Buf    = Value->Buffer;
      *BufLen = Value->BufferLen;
      break;

    case EFI_IFR_TYPE_DATE:
      *Buf    = (UINT8 *)(&Value->Value.date);
      *BufLen = (UINT16)sizeof (EFI_HII_DATE);
      break;

    case EFI_IFR_TYPE_TIME:
      *Buf    = (UINT8 *)(&Value->Value.time);
      *BufLen = (UINT16)sizeof (EFI_HII_TIME);
      break;

    case EFI_IFR_TYPE_REF:
      *Buf    = (UINT8 *)(&Value->Value.ref);
      *BufLen = (UINT16)sizeof (EFI_HII_REF);
      break;

    default:
      *Buf    = NULL;
      *BufLen = 0;
  }
}

/**
  Compare two Hii value.

  @param  Value1                 Expression value to compare on left-hand.
  @param  Value2                 Expression value to compare on right-hand.
  @param  Result                 Return value after compare.
                                 retval 0                      Two operators equal.
                                 return Positive value if Value1 is greater than Value2.
                                 retval Negative value if Value1 is less than Value2.
  @param  HiiHandle              Only required for string compare.

  @retval other                  Could not perform compare on two values.
  @retval EFI_SUCCESS            Compare the value success.

**/
EFI_STATUS
CompareHiiValue (
  IN  EFI_HII_VALUE   *Value1,
  IN  EFI_HII_VALUE   *Value2,
  OUT INTN            *Result,
  IN  EFI_HII_HANDLE  HiiHandle OPTIONAL
  )
{
  INT64   Temp64;
  CHAR16  *Str1;
  CHAR16  *Str2;
  UINTN   Len;
  UINT8   *Buf1;
  UINT16  Buf1Len;
  UINT8   *Buf2;
  UINT16  Buf2Len;

  if ((Value1->Type == EFI_IFR_TYPE_STRING) && (Value2->Type == EFI_IFR_TYPE_STRING)) {
    if ((Value1->Value.string == 0) || (Value2->Value.string == 0)) {
      //
      // StringId 0 is reserved
      //
      return EFI_INVALID_PARAMETER;
    }

    if (Value1->Value.string == Value2->Value.string) {
      *Result = 0;
      return EFI_SUCCESS;
    }

    Str1 = GetToken (Value1->Value.string, HiiHandle);
    if (Str1 == NULL) {
      //
      // String not found
      //
      return EFI_NOT_FOUND;
    }

    Str2 = GetToken (Value2->Value.string, HiiHandle);
    if (Str2 == NULL) {
      FreePool (Str1);
      return EFI_NOT_FOUND;
    }

    *Result = StrCmp (Str1, Str2);

    FreePool (Str1);
    FreePool (Str2);

    return EFI_SUCCESS;
  }

  //
  // Take types(date, time, ref, buffer) as buffer
  //
  if (IsTypeInBuffer (Value1) && IsTypeInBuffer (Value2)) {
    GetBufAndLenForValue (Value1, &Buf1, &Buf1Len);
    GetBufAndLenForValue (Value2, &Buf2, &Buf2Len);

    Len     = Buf1Len > Buf2Len ? Buf2Len : Buf1Len;
    *Result = CompareMem (Buf1, Buf2, Len);
    if ((*Result == 0) && (Buf1Len != Buf2Len)) {
      //
      // In this case, means base on samll number buffer, the data is same
      // So which value has more data, which value is bigger.
      //
      *Result = Buf1Len > Buf2Len ? 1 : -1;
    }

    return EFI_SUCCESS;
  }

  //
  // Take remain types(integer, boolean, date/time) as integer
  //
  if (IsTypeInUINT64 (Value1) && IsTypeInUINT64 (Value2)) {
    Temp64 = HiiValueToUINT64 (Value1) - HiiValueToUINT64 (Value2);
    if (Temp64 > 0) {
      *Result = 1;
    } else if (Temp64 < 0) {
      *Result = -1;
    } else {
      *Result = 0;
    }

    return EFI_SUCCESS;
  }

  return EFI_UNSUPPORTED;
}

/**
  Search an Option of a Question by its value.

  @param  Question               The Question
  @param  OptionValue            Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
DISPLAY_QUESTION_OPTION *
ValueToOption (
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Question,
  IN EFI_HII_VALUE                  *OptionValue
  )
{
  LIST_ENTRY               *Link;
  DISPLAY_QUESTION_OPTION  *Option;
  INTN                     Result;
  EFI_HII_VALUE            Value;

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);

    ZeroMem (&Value, sizeof (EFI_HII_VALUE));
    Value.Type = Option->OptionOpCode->Type;
    CopyMem (&Value.Value, &Option->OptionOpCode->Value, Option->OptionOpCode->Header.Length - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));

    if ((CompareHiiValue (&Value, OptionValue, &Result, NULL) == EFI_SUCCESS) && (Result == 0)) {
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
  IN VOID   *Array,
  IN UINT8  Type,
  IN UINTN  Index
  )
{
  UINT64  Data;

  ASSERT (Array != NULL);

  Data = 0;
  switch (Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Data = (UINT64)*(((UINT8 *)Array) + Index);
      break;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      Data = (UINT64)*(((UINT16 *)Array) + Index);
      break;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      Data = (UINT64)*(((UINT32 *)Array) + Index);
      break;

    case EFI_IFR_TYPE_NUM_SIZE_64:
      Data = (UINT64)*(((UINT64 *)Array) + Index);
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
  IN VOID    *Array,
  IN UINT8   Type,
  IN UINTN   Index,
  IN UINT64  Value
  )
{
  ASSERT (Array != NULL);

  switch (Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      *(((UINT8 *)Array) + Index) = (UINT8)Value;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      *(((UINT16 *)Array) + Index) = (UINT16)Value;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      *(((UINT32 *)Array) + Index) = (UINT32)Value;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_64:
      *(((UINT64 *)Array) + Index) = (UINT64)Value;
      break;

    default:
      break;
  }
}

/**
  Check whether this value already in the array, if yes, return the index.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Value                  The value to be find.
  @param  Index                  The index in the array which has same value with Value.

  @retval   TRUE Found the value in the array.
  @retval   FALSE Not found the value.

**/
BOOLEAN
FindArrayData (
  IN VOID    *Array,
  IN UINT8   Type,
  IN UINT64  Value,
  OUT UINTN  *Index OPTIONAL
  )
{
  UINTN   Count;
  UINT64  TmpValue;
  UINT64  ValueComp;

  ASSERT (Array != NULL);

  Count    = 0;
  TmpValue = 0;

  switch (Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      ValueComp = (UINT8)Value;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      ValueComp = (UINT16)Value;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      ValueComp = (UINT32)Value;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_64:
      ValueComp = (UINT64)Value;
      break;

    default:
      ValueComp = 0;
      break;
  }

  while ((TmpValue = GetArrayData (Array, Type, Count)) != 0) {
    if (ValueComp == TmpValue) {
      if (Index != NULL) {
        *Index = Count;
      }

      return TRUE;
    }

    Count++;
  }

  return FALSE;
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
  IN FORM_DISPLAY_ENGINE_STATEMENT  *Question,
  IN OUT CHAR16                     *FormattedNumber,
  IN UINTN                          BufferSize
  )
{
  INT64            Value;
  CHAR16           *Format;
  EFI_HII_VALUE    *QuestionValue;
  EFI_IFR_NUMERIC  *NumericOp;

  if (BufferSize < (21 * sizeof (CHAR16))) {
    return EFI_BUFFER_TOO_SMALL;
  }

  QuestionValue = &Question->CurrentValue;
  NumericOp     = (EFI_IFR_NUMERIC *)Question->OpCode;

  Value = (INT64)QuestionValue->Value.u64;
  switch (NumericOp->Flags & EFI_IFR_DISPLAY) {
    case EFI_IFR_DISPLAY_INT_DEC:
      switch (QuestionValue->Type) {
        case EFI_IFR_NUMERIC_SIZE_1:
          Value = (INT64)((INT8)QuestionValue->Value.u8);
          break;

        case EFI_IFR_NUMERIC_SIZE_2:
          Value = (INT64)((INT16)QuestionValue->Value.u16);
          break;

        case EFI_IFR_NUMERIC_SIZE_4:
          Value = (INT64)((INT32)QuestionValue->Value.u32);
          break;

        case EFI_IFR_NUMERIC_SIZE_8:
        default:
          break;
      }

      if (Value < 0) {
        Value  = -Value;
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
  }

  UnicodeSPrint (FormattedNumber, BufferSize, Format, Value);

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
  IN  UINTN    RequestedWidth,
  IN  UINTN    NumberOfLines,
  IN  VA_LIST  Marker
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

  DimensionsWidth  = gStatementDimensions.RightColumn - gStatementDimensions.LeftColumn;
  DimensionsHeight = gStatementDimensions.BottomRow - gStatementDimensions.TopRow;

  gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());

  if ((RequestedWidth + 2) > DimensionsWidth) {
    RequestedWidth = DimensionsWidth - 2;
  }

  //
  // Subtract the PopUp width from total Columns, allow for one space extra on
  // each end plus a border.
  //
  Start = (DimensionsWidth - RequestedWidth - 2) / 2 + gStatementDimensions.LeftColumn + 1;
  End   = Start + RequestedWidth + 1;

  Top    = ((DimensionsHeight - NumberOfLines - 2) / 2) + gStatementDimensions.TopRow - 1;
  Bottom = Top + NumberOfLines + 2;

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
  for (Index = Top; Index + 2 < Bottom; Index++, Count++) {
    String = VA_ARG (Marker, CHAR16 *);

    //
    // This will clear the background of the line - we never know who might have been
    // here before us.  This differs from the next clear in that it used the non-reverse
    // video for normal printing.
    //
    if (GetStringWidth (String) / 2 > 1) {
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
    }

    //
    // Passing in a space results in the assumption that this is where typing will occur
    //
    if (String[0] == L' ') {
      ClearLines (Start + 1, End - 1, Index + 1, Index + 1, GetPopupInverseColor ());
    }

    //
    // Passing in a NULL results in a blank space
    //
    if (String[0] == CHAR_NULL) {
      ClearLines (Start, End, Index + 1, Index + 1, GetPopupColor ());
    }

    PrintStringAt (
      ((DimensionsWidth - GetStringWidth (String) / 2) / 2) + gStatementDimensions.LeftColumn + 1,
      Index + 1,
      String
      );
    gST->ConOut->SetAttribute (gST->ConOut, GetPopupColor ());
    PrintCharAt (Start, Index + 1, Character);
    PrintCharAt (End - 1, Index + 1, Character);
  }

  Character = BOXDRAW_UP_RIGHT;
  PrintCharAt (Start, Bottom - 1, Character);
  Character = BOXDRAW_HORIZONTAL;
  for (Index = Start; Index + 2 < End; Index++) {
    PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
  }

  Character = BOXDRAW_UP_LEFT;
  PrintCharAt ((UINTN)-1, (UINTN)-1, Character);
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
  IN  UINTN  RequestedWidth,
  IN  UINTN  NumberOfLines,
  ...
  )
{
  VA_LIST  Marker;

  VA_START (Marker, NumberOfLines);

  CreateSharedPopUp (RequestedWidth, NumberOfLines, Marker);

  VA_END (Marker);
}

/**
  Process nothing.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
EmptyEventProcess (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
}

/**
  Process for the refresh interval statement.

  @param Event    The Event need to be process
  @param Context  The context of the event.

**/
VOID
EFIAPI
RefreshTimeOutProcess (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  WARNING_IF_CONTEXT  *EventInfo;
  CHAR16              TimeOutString[MAX_TIME_OUT_LEN];

  EventInfo = (WARNING_IF_CONTEXT *)Context;

  if (*(EventInfo->TimeOut) == 0) {
    gBS->CloseEvent (Event);

    gBS->SignalEvent (EventInfo->SyncEvent);
    return;
  }

  UnicodeSPrint (TimeOutString, MAX_TIME_OUT_LEN, L"%d", *(EventInfo->TimeOut));

  CreateDialog (NULL, gEmptyString, EventInfo->ErrorInfo, gPressEnter, gEmptyString, TimeOutString, NULL);

  *(EventInfo->TimeOut) -= 1;
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
    CreateDialog (&Key, gEmptyString, gPassowordInvalid, gPressEnter, gEmptyString, NULL);
  } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
}

/**
  Process password op code.

  @param  MenuOption             The menu for current password op code.

  @retval EFI_SUCCESS            Question Option process success.
  @retval Other                  Question Option process fail.

**/
EFI_STATUS
PasswordProcess (
  IN  UI_MENU_OPTION  *MenuOption
  )
{
  CHAR16                         *StringPtr;
  CHAR16                         *TempString;
  UINTN                          Maximum;
  EFI_STATUS                     Status;
  EFI_IFR_PASSWORD               *PasswordInfo;
  FORM_DISPLAY_ENGINE_STATEMENT  *Question;
  EFI_INPUT_KEY                  Key;

  Question     = MenuOption->ThisTag;
  PasswordInfo = (EFI_IFR_PASSWORD *)Question->OpCode;
  Maximum      = PasswordInfo->MaxSize;
  Status       = EFI_SUCCESS;

  StringPtr = AllocateZeroPool ((Maximum + 1) * sizeof (CHAR16));
  ASSERT (StringPtr);

  //
  // Use a NULL password to test whether old password is required
  //
  *StringPtr = 0;
  Status     = Question->PasswordCheck (gFormData, Question, StringPtr);
  if ((Status == EFI_NOT_AVAILABLE_YET) || (Status == EFI_UNSUPPORTED)) {
    //
    // Password can't be set now.
    //
    if (Status == EFI_UNSUPPORTED) {
      do {
        CreateDialog (&Key, gEmptyString, gPasswordUnsupported, gPressEnter, gEmptyString, NULL);
      } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);
    }

    FreePool (StringPtr);
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    //
    // Old password exist, ask user for the old password
    //
    Status = ReadString (MenuOption, gPromptForPassword, StringPtr);
    if (EFI_ERROR (Status)) {
      ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
      FreePool (StringPtr);
      return Status;
    }

    //
    // Check user input old password
    //
    Status = Question->PasswordCheck (gFormData, Question, StringPtr);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_READY) {
        //
        // Typed in old password incorrect
        //
        PasswordInvalid ();
      } else {
        Status = EFI_SUCCESS;
      }

      ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
      FreePool (StringPtr);
      return Status;
    }
  }

  //
  // Ask for new password
  //
  ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
  Status = ReadString (MenuOption, gPromptForNewPassword, StringPtr);
  if (EFI_ERROR (Status)) {
    //
    // Reset state machine for password
    //
    Question->PasswordCheck (gFormData, Question, NULL);
    ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
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
    // Reset state machine for password
    //
    Question->PasswordCheck (gFormData, Question, NULL);
    ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
    ZeroMem (TempString, (Maximum + 1) * sizeof (CHAR16));
    FreePool (StringPtr);
    FreePool (TempString);
    return Status;
  }

  //
  // Compare two typed-in new passwords
  //
  if (StrCmp (StringPtr, TempString) == 0) {
    gUserInput->InputValue.Buffer       = AllocateCopyPool (Question->CurrentValue.BufferLen, StringPtr);
    gUserInput->InputValue.BufferLen    = Question->CurrentValue.BufferLen;
    gUserInput->InputValue.Type         = Question->CurrentValue.Type;
    gUserInput->InputValue.Value.string = HiiSetString (gFormData->HiiHandle, gUserInput->InputValue.Value.string, StringPtr, NULL);

    Status = EFI_SUCCESS;
  } else {
    //
    // Reset state machine for password
    //
    Question->PasswordCheck (gFormData, Question, NULL);

    //
    // Two password mismatch, prompt error message
    //
    do {
      CreateDialog (&Key, gEmptyString, gConfirmError, gPressEnter, gEmptyString, NULL);
    } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

    Status = EFI_INVALID_PARAMETER;
  }

  ZeroMem (TempString, (Maximum + 1) * sizeof (CHAR16));
  ZeroMem (StringPtr, (Maximum + 1) * sizeof (CHAR16));
  FreePool (TempString);
  FreePool (StringPtr);

  return Status;
}

/**
  Print some debug message about mismatched menu info.

  @param  MenuOption             The MenuOption for this Question.

**/
VOID
PrintMismatchMenuInfo (
  IN  UI_MENU_OPTION  *MenuOption
  )
{
  CHAR16                         *FormTitleStr;
  CHAR16                         *FormSetTitleStr;
  CHAR16                         *OneOfOptionStr;
  CHAR16                         *QuestionName;
  LIST_ENTRY                     *Link;
  FORM_DISPLAY_ENGINE_STATEMENT  *Question;
  EFI_IFR_ORDERED_LIST           *OrderList;
  UINT8                          Index;
  EFI_HII_VALUE                  HiiValue;
  EFI_HII_VALUE                  *QuestionValue;
  DISPLAY_QUESTION_OPTION        *Option;
  UINT8                          *ValueArray;
  UINT8                          ValueType;
  EFI_IFR_FORM_SET               *FormsetBuffer;
  UINTN                          FormsetBufferSize;

  Question = MenuOption->ThisTag;
  HiiGetFormSetFromHiiHandle (gFormData->HiiHandle, &FormsetBuffer, &FormsetBufferSize);

  FormSetTitleStr = GetToken (FormsetBuffer->FormSetTitle, gFormData->HiiHandle);
  FormTitleStr    = GetToken (gFormData->FormTitle, gFormData->HiiHandle);

  DEBUG ((DEBUG_ERROR, "\n[%a]: Mismatch Formset    : Formset Guid = %g,  FormSet title = %s\n", gEfiCallerBaseName, &gFormData->FormSetGuid, FormSetTitleStr));
  DEBUG ((DEBUG_ERROR, "[%a]: Mismatch Form       : FormId = %d,  Form title = %s.\n", gEfiCallerBaseName, gFormData->FormId, FormTitleStr));

  if (Question->OpCode->OpCode == EFI_IFR_ORDERED_LIST_OP) {
    QuestionName = GetToken (((EFI_IFR_ORDERED_LIST *)MenuOption->ThisTag->OpCode)->Question.Header.Prompt, gFormData->HiiHandle);
    Link         = GetFirstNode (&Question->OptionListHead);
    Option       = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
    ValueType    = Option->OptionOpCode->Type;
    DEBUG ((DEBUG_ERROR, "[%a]: Mismatch Error      : OrderedList value in the array doesn't match with option value.\n", gEfiCallerBaseName));
    DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OrderedList: Name = %s.\n", gEfiCallerBaseName, QuestionName));
    DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OrderedList: OrderedList array value :\n", gEfiCallerBaseName));

    OrderList = (EFI_IFR_ORDERED_LIST *)Question->OpCode;
    for (Index = 0; Index < OrderList->MaxContainers; Index++) {
      ValueArray         = Question->CurrentValue.Buffer;
      HiiValue.Value.u64 = GetArrayData (ValueArray, ValueType, Index);
      DEBUG ((DEBUG_ERROR, "                                       Value[%d] =%ld.\n", Index, HiiValue.Value.u64));
    }
  } else if (Question->OpCode->OpCode == EFI_IFR_ONE_OF_OP) {
    QuestionName  = GetToken (((EFI_IFR_ONE_OF *)MenuOption->ThisTag->OpCode)->Question.Header.Prompt, gFormData->HiiHandle);
    QuestionValue = &Question->CurrentValue;
    DEBUG ((DEBUG_ERROR, "[%a]: Mismatch Error      : OneOf value doesn't match with option value.\n", gEfiCallerBaseName));
    DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OneOf      : Name = %s.\n", gEfiCallerBaseName, QuestionName));
    switch (QuestionValue->Type) {
      case EFI_IFR_TYPE_NUM_SIZE_64:
        DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OneOf      : OneOf value = %ld.\n", gEfiCallerBaseName, QuestionValue->Value.u64));
        break;

      case EFI_IFR_TYPE_NUM_SIZE_32:
        DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OneOf      : OneOf value = %d.\n", gEfiCallerBaseName, QuestionValue->Value.u32));
        break;

      case EFI_IFR_TYPE_NUM_SIZE_16:
        DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OneOf      : OneOf value = %d.\n", gEfiCallerBaseName, QuestionValue->Value.u16));
        break;

      case EFI_IFR_TYPE_NUM_SIZE_8:
        DEBUG ((DEBUG_ERROR, "[%a]: Mismatch OneOf      : OneOf value = %d.\n", gEfiCallerBaseName, QuestionValue->Value.u8));
        break;

      default:
        ASSERT (FALSE);
        break;
    }
  }

  Index = 0;
  Link  = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option         = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
    OneOfOptionStr = GetToken (Option->OptionOpCode->Option, gFormData->HiiHandle);
    switch (Option->OptionOpCode->Type) {
      case EFI_IFR_TYPE_NUM_SIZE_64:
        DEBUG ((DEBUG_ERROR, "[%a]: Option %d            : Option Value = %ld,  Option Name = %s.\n", gEfiCallerBaseName, Index, Option->OptionOpCode->Value.u64, OneOfOptionStr));
        break;

      case EFI_IFR_TYPE_NUM_SIZE_32:
        DEBUG ((DEBUG_ERROR, "[%a]: Option %d            : Option Value = %d,  Option Name = %s.\n", gEfiCallerBaseName, Index, Option->OptionOpCode->Value.u32, OneOfOptionStr));
        break;

      case EFI_IFR_TYPE_NUM_SIZE_16:
        DEBUG ((DEBUG_ERROR, "[%a]: Option %d            : Option Value = %d,  Option Name = %s.\n", gEfiCallerBaseName, Index, Option->OptionOpCode->Value.u16, OneOfOptionStr));
        break;

      case EFI_IFR_TYPE_NUM_SIZE_8:
        DEBUG ((DEBUG_ERROR, "[%a]: Option %d            : Option Value = %d,  Option Name = %s.\n", gEfiCallerBaseName, Index, Option->OptionOpCode->Value.u8, OneOfOptionStr));
        break;

      default:
        ASSERT (FALSE);
        break;
    }

    Link = GetNextNode (&Question->OptionListHead, Link);
    Index++;
  }
}

/**
  Process a Question's Option (whether selected or un-selected).

  @param  MenuOption             The MenuOption for this Question.
  @param  Selected               TRUE: if Question is selected.
  @param  OptionString           Pointer of the Option String to be displayed.
  @param  SkipErrorValue         Whether need to return when value without option for it.

  @retval EFI_SUCCESS            Question Option process success.
  @retval Other                  Question Option process fail.

**/
EFI_STATUS
ProcessOptions (
  IN  UI_MENU_OPTION  *MenuOption,
  IN  BOOLEAN         Selected,
  OUT CHAR16          **OptionString,
  IN  BOOLEAN         SkipErrorValue
  )
{
  EFI_STATUS                     Status;
  CHAR16                         *StringPtr;
  UINTN                          Index;
  FORM_DISPLAY_ENGINE_STATEMENT  *Question;
  CHAR16                         FormattedNumber[21];
  UINT16                         Number;
  CHAR16                         Character[2];
  EFI_INPUT_KEY                  Key;
  UINTN                          BufferSize;
  DISPLAY_QUESTION_OPTION        *OneOfOption;
  LIST_ENTRY                     *Link;
  EFI_HII_VALUE                  HiiValue;
  EFI_HII_VALUE                  *QuestionValue;
  DISPLAY_QUESTION_OPTION        *Option;
  UINTN                          Index2;
  UINT8                          *ValueArray;
  UINT8                          ValueType;
  EFI_IFR_ORDERED_LIST           *OrderList;
  BOOLEAN                        ValueInvalid;
  UINTN                          MaxLen;

  Status = EFI_SUCCESS;

  StringPtr     = NULL;
  Character[1]  = L'\0';
  *OptionString = NULL;
  ValueInvalid  = FALSE;

  ZeroMem (FormattedNumber, 21 * sizeof (CHAR16));
  BufferSize = (gOptionBlockWidth + 1) * 2 * gStatementDimensions.BottomRow;

  Question      = MenuOption->ThisTag;
  QuestionValue = &Question->CurrentValue;

  switch (Question->OpCode->OpCode) {
    case EFI_IFR_ORDERED_LIST_OP:

      //
      // Check whether there are Options of this OrderedList
      //
      if (IsListEmpty (&Question->OptionListHead)) {
        break;
      }

      OrderList = (EFI_IFR_ORDERED_LIST *)Question->OpCode;

      Link        = GetFirstNode (&Question->OptionListHead);
      OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);

      ValueType  =  OneOfOption->OptionOpCode->Type;
      ValueArray = Question->CurrentValue.Buffer;

      if (Selected) {
        //
        // Go ask for input
        //
        Status = GetSelectionInputPopUp (MenuOption);
      } else {
        //
        // We now know how many strings we will have, so we can allocate the
        // space required for the array or strings.
        //
        MaxLen        = OrderList->MaxContainers * BufferSize / sizeof (CHAR16);
        *OptionString = AllocateZeroPool (MaxLen * sizeof (CHAR16));
        ASSERT (*OptionString);

        HiiValue.Type      = ValueType;
        HiiValue.Value.u64 = 0;
        for (Index = 0; Index < OrderList->MaxContainers; Index++) {
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
            // Print debug msg for the mistach menu.
            //
            PrintMismatchMenuInfo (MenuOption);

            if (SkipErrorValue) {
              //
              // Just try to get the option string, skip the value which not has option.
              //
              continue;
            }

            //
            // Show error message
            //
            do {
              CreateDialog (&Key, gEmptyString, gOptionMismatch, gPressEnter, gEmptyString, NULL);
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

            //
            // The initial value of the orderedlist is invalid, force to be valid value
            // Exit current DisplayForm with new value.
            //
            gUserInput->SelectedStatement = Question;
            gMisMatch                     = TRUE;
            ValueArray                    = AllocateZeroPool (Question->CurrentValue.BufferLen);
            ASSERT (ValueArray != NULL);
            gUserInput->InputValue.Buffer    = ValueArray;
            gUserInput->InputValue.BufferLen = Question->CurrentValue.BufferLen;
            gUserInput->InputValue.Type      = Question->CurrentValue.Type;

            Link   = GetFirstNode (&Question->OptionListHead);
            Index2 = 0;
            while (!IsNull (&Question->OptionListHead, Link) && Index2 < OrderList->MaxContainers) {
              Option = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
              Link   = GetNextNode (&Question->OptionListHead, Link);
              SetArrayData (ValueArray, ValueType, Index2, Option->OptionOpCode->Value.u64);
              Index2++;
            }

            SetArrayData (ValueArray, ValueType, Index2, 0);

            FreePool (*OptionString);
            *OptionString = NULL;
            return EFI_NOT_FOUND;
          }

          Character[0] = LEFT_ONEOF_DELIMITER;
          NewStrCat (OptionString[0], MaxLen, Character);
          StringPtr = GetToken (OneOfOption->OptionOpCode->Option, gFormData->HiiHandle);
          ASSERT (StringPtr != NULL);
          NewStrCat (OptionString[0], MaxLen, StringPtr);
          Character[0] = RIGHT_ONEOF_DELIMITER;
          NewStrCat (OptionString[0], MaxLen, Character);
          Character[0] = CHAR_CARRIAGE_RETURN;
          NewStrCat (OptionString[0], MaxLen, Character);
          FreePool (StringPtr);
        }

        //
        // If valid option more than the max container, skip these options.
        //
        if (Index >= OrderList->MaxContainers) {
          break;
        }

        //
        // Search the other options, try to find the one not in the container.
        //
        Link = GetFirstNode (&Question->OptionListHead);
        while (!IsNull (&Question->OptionListHead, Link)) {
          OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
          Link        = GetNextNode (&Question->OptionListHead, Link);

          if (FindArrayData (ValueArray, ValueType, OneOfOption->OptionOpCode->Value.u64, NULL)) {
            continue;
          }

          //
          // Print debug msg for the mistach menu.
          //
          PrintMismatchMenuInfo (MenuOption);

          if (SkipErrorValue) {
            //
            // Not report error, just get the correct option string info.
            //
            Character[0] = LEFT_ONEOF_DELIMITER;
            NewStrCat (OptionString[0], MaxLen, Character);
            StringPtr = GetToken (OneOfOption->OptionOpCode->Option, gFormData->HiiHandle);
            ASSERT (StringPtr != NULL);
            NewStrCat (OptionString[0], MaxLen, StringPtr);
            Character[0] = RIGHT_ONEOF_DELIMITER;
            NewStrCat (OptionString[0], MaxLen, Character);
            Character[0] = CHAR_CARRIAGE_RETURN;
            NewStrCat (OptionString[0], MaxLen, Character);
            FreePool (StringPtr);

            continue;
          }

          if (!ValueInvalid) {
            ValueInvalid = TRUE;
            //
            // Show error message
            //
            do {
              CreateDialog (&Key, gEmptyString, gOptionMismatch, gPressEnter, gEmptyString, NULL);
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

            //
            // The initial value of the orderedlist is invalid, force to be valid value
            // Exit current DisplayForm with new value.
            //
            gUserInput->SelectedStatement = Question;
            gMisMatch                     = TRUE;
            ValueArray                    = AllocateCopyPool (Question->CurrentValue.BufferLen, Question->CurrentValue.Buffer);
            ASSERT (ValueArray != NULL);
            gUserInput->InputValue.Buffer    = ValueArray;
            gUserInput->InputValue.BufferLen = Question->CurrentValue.BufferLen;
            gUserInput->InputValue.Type      = Question->CurrentValue.Type;
          }

          SetArrayData (ValueArray, ValueType, Index++, OneOfOption->OptionOpCode->Value.u64);
        }

        if (ValueInvalid) {
          FreePool (*OptionString);
          *OptionString = NULL;
          return EFI_NOT_FOUND;
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
        Status = GetSelectionInputPopUp (MenuOption);
      } else {
        MaxLen        = BufferSize / sizeof (CHAR16);
        *OptionString = AllocateZeroPool (BufferSize);
        ASSERT (*OptionString);

        OneOfOption = ValueToOption (Question, QuestionValue);
        if (OneOfOption == NULL) {
          //
          // Print debug msg for the mistach menu.
          //
          PrintMismatchMenuInfo (MenuOption);

          if (SkipErrorValue) {
            //
            // Not report error, just get the correct option string info.
            //
            Link        = GetFirstNode (&Question->OptionListHead);
            OneOfOption = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);
          } else {
            //
            // Show error message
            //
            do {
              CreateDialog (&Key, gEmptyString, gOptionMismatch, gPressEnter, gEmptyString, NULL);
            } while (Key.UnicodeChar != CHAR_CARRIAGE_RETURN);

            //
            // Force the Question value to be valid
            // Exit current DisplayForm with new value.
            //
            Link   = GetFirstNode (&Question->OptionListHead);
            Option = DISPLAY_QUESTION_OPTION_FROM_LINK (Link);

            gUserInput->InputValue.Type = Option->OptionOpCode->Type;
            switch (gUserInput->InputValue.Type) {
              case EFI_IFR_TYPE_NUM_SIZE_8:
                gUserInput->InputValue.Value.u8 = Option->OptionOpCode->Value.u8;
                break;
              case EFI_IFR_TYPE_NUM_SIZE_16:
                CopyMem (&gUserInput->InputValue.Value.u16, &Option->OptionOpCode->Value.u16, sizeof (UINT16));
                break;
              case EFI_IFR_TYPE_NUM_SIZE_32:
                CopyMem (&gUserInput->InputValue.Value.u32, &Option->OptionOpCode->Value.u32, sizeof (UINT32));
                break;
              case EFI_IFR_TYPE_NUM_SIZE_64:
                CopyMem (&gUserInput->InputValue.Value.u64, &Option->OptionOpCode->Value.u64, sizeof (UINT64));
                break;
              default:
                ASSERT (FALSE);
                break;
            }

            gUserInput->SelectedStatement = Question;
            gMisMatch                     = TRUE;
            FreePool (*OptionString);
            *OptionString = NULL;
            return EFI_NOT_FOUND;
          }
        }

        Character[0] = LEFT_ONEOF_DELIMITER;
        NewStrCat (OptionString[0], MaxLen, Character);
        StringPtr = GetToken (OneOfOption->OptionOpCode->Option, gFormData->HiiHandle);
        ASSERT (StringPtr != NULL);
        NewStrCat (OptionString[0], MaxLen, StringPtr);
        Character[0] = RIGHT_ONEOF_DELIMITER;
        NewStrCat (OptionString[0], MaxLen, Character);

        FreePool (StringPtr);
      }

      break;

    case EFI_IFR_CHECKBOX_OP:
      if (Selected) {
        //
        // Since this is a BOOLEAN operation, flip it upon selection
        //
        gUserInput->InputValue.Type    = QuestionValue->Type;
        gUserInput->InputValue.Value.b = (BOOLEAN)(QuestionValue->Value.b ? FALSE : TRUE);

        //
        // Perform inconsistent check
        //
        return EFI_SUCCESS;
      } else {
        *OptionString = AllocateZeroPool (BufferSize);
        ASSERT (*OptionString);

        *OptionString[0] = LEFT_CHECKBOX_DELIMITER;

        if (QuestionValue->Value.b) {
          *(OptionString[0] + 1) = CHECK_ON;
        } else {
          *(OptionString[0] + 1) = CHECK_OFF;
        }

        *(OptionString[0] + 2) = RIGHT_CHECKBOX_DELIMITER;
      }

      break;

    case EFI_IFR_NUMERIC_OP:
      if (Selected) {
        //
        // Go ask for input
        //
        Status = GetNumericInput (MenuOption);
      } else {
        *OptionString = AllocateZeroPool (BufferSize);
        ASSERT (*OptionString);

        *OptionString[0] = LEFT_NUMERIC_DELIMITER;

        //
        // Formatted print
        //
        PrintFormattedNumber (Question, FormattedNumber, 21 * sizeof (CHAR16));
        Number = (UINT16)GetStringWidth (FormattedNumber);
        CopyMem (OptionString[0] + 1, FormattedNumber, Number);

        *(OptionString[0] + Number / 2) = RIGHT_NUMERIC_DELIMITER;
      }

      break;

    case EFI_IFR_DATE_OP:
      if (Selected) {
        //
        // This is similar to numerics
        //
        Status = GetNumericInput (MenuOption);
      } else {
        *OptionString = AllocateZeroPool (BufferSize);
        ASSERT (*OptionString);

        switch (MenuOption->Sequence) {
          case 0:
            *OptionString[0] = LEFT_NUMERIC_DELIMITER;
            if (QuestionValue->Value.date.Month == 0xff) {
              UnicodeSPrint (OptionString[0] + 1, 21 * sizeof (CHAR16), L"??");
            } else {
              UnicodeSPrint (OptionString[0] + 1, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.date.Month);
            }

            *(OptionString[0] + 3) = DATE_SEPARATOR;
            break;

          case 1:
            SetUnicodeMem (OptionString[0], 4, L' ');
            if (QuestionValue->Value.date.Day == 0xff) {
              UnicodeSPrint (OptionString[0] + 4, 21 * sizeof (CHAR16), L"??");
            } else {
              UnicodeSPrint (OptionString[0] + 4, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.date.Day);
            }

            *(OptionString[0] + 6) = DATE_SEPARATOR;
            break;

          case 2:
            SetUnicodeMem (OptionString[0], 7, L' ');
            if (QuestionValue->Value.date.Year == 0xff) {
              UnicodeSPrint (OptionString[0] + 7, 21 * sizeof (CHAR16), L"????");
            } else {
              UnicodeSPrint (OptionString[0] + 7, 21 * sizeof (CHAR16), L"%04d", QuestionValue->Value.date.Year);
            }

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
        Status = GetNumericInput (MenuOption);
      } else {
        *OptionString = AllocateZeroPool (BufferSize);
        ASSERT (*OptionString);

        switch (MenuOption->Sequence) {
          case 0:
            *OptionString[0] = LEFT_NUMERIC_DELIMITER;
            if (QuestionValue->Value.time.Hour == 0xff) {
              UnicodeSPrint (OptionString[0] + 1, 21 * sizeof (CHAR16), L"??");
            } else {
              UnicodeSPrint (OptionString[0] + 1, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.time.Hour);
            }

            *(OptionString[0] + 3) = TIME_SEPARATOR;
            break;

          case 1:
            SetUnicodeMem (OptionString[0], 4, L' ');
            if (QuestionValue->Value.time.Minute == 0xff) {
              UnicodeSPrint (OptionString[0] + 4, 21 * sizeof (CHAR16), L"??");
            } else {
              UnicodeSPrint (OptionString[0] + 4, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.time.Minute);
            }

            *(OptionString[0] + 6) = TIME_SEPARATOR;
            break;

          case 2:
            SetUnicodeMem (OptionString[0], 7, L' ');
            if (QuestionValue->Value.time.Second == 0xff) {
              UnicodeSPrint (OptionString[0] + 7, 21 * sizeof (CHAR16), L"??");
            } else {
              UnicodeSPrint (OptionString[0] + 7, 21 * sizeof (CHAR16), L"%02d", QuestionValue->Value.time.Second);
            }

            *(OptionString[0] + 9) = RIGHT_NUMERIC_DELIMITER;
            break;
        }
      }

      break;

    case EFI_IFR_STRING_OP:
      if (Selected) {
        StringPtr = AllocateZeroPool (Question->CurrentValue.BufferLen + sizeof (CHAR16));
        ASSERT (StringPtr);
        CopyMem (StringPtr, Question->CurrentValue.Buffer, Question->CurrentValue.BufferLen);

        Status = ReadString (MenuOption, gPromptForData, StringPtr);
        if (EFI_ERROR (Status)) {
          FreePool (StringPtr);
          return Status;
        }

        gUserInput->InputValue.Buffer       = AllocateCopyPool (Question->CurrentValue.BufferLen, StringPtr);
        gUserInput->InputValue.BufferLen    = Question->CurrentValue.BufferLen;
        gUserInput->InputValue.Type         = Question->CurrentValue.Type;
        gUserInput->InputValue.Value.string = HiiSetString (gFormData->HiiHandle, gUserInput->InputValue.Value.string, StringPtr, NULL);
        FreePool (StringPtr);
        return EFI_SUCCESS;
      } else {
        *OptionString = AllocateZeroPool (BufferSize);
        ASSERT (*OptionString);

        if (((CHAR16 *)Question->CurrentValue.Buffer)[0] == 0x0000) {
          *(OptionString[0]) = '_';
        } else {
          if (Question->CurrentValue.BufferLen < BufferSize) {
            BufferSize = Question->CurrentValue.BufferLen;
          }

          CopyMem (OptionString[0], (CHAR16 *)Question->CurrentValue.Buffer, BufferSize);
        }
      }

      break;

    case EFI_IFR_PASSWORD_OP:
      if (Selected) {
        Status = PasswordProcess (MenuOption);
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

  TotalRowNum  = 0;
  CheckedNum   = 0;
  GlyphWidth   = 1;
  Index        = 0;
  MaxStringLen = 0;
  StringLen    = 0;

  //
  // Set default help string width.
  //
  LineWidth = (UINT16)(gHelpBlockWidth - 1);

  //
  // Get row number of the String.
  //
  while ((StringLen = GetLineByWidth (StringPtr, LineWidth, &GlyphWidth, &Index, &OutputString)) != 0) {
    if (StringLen > MaxStringLen) {
      MaxStringLen = StringLen;
    }

    TotalRowNum++;
    FreePool (OutputString);
  }

  *EachLineWidth = MaxStringLen;

  *FormattedString = AllocateZeroPool (TotalRowNum * MaxStringLen * sizeof (CHAR16));
  ASSERT (*FormattedString != NULL);

  //
  // Generate formatted help string array.
  //
  GlyphWidth = 1;
  Index      = 0;
  while ((StringLen = GetLineByWidth (StringPtr, LineWidth, &GlyphWidth, &Index, &OutputString)) != 0) {
    CopyMem (*FormattedString + CheckedNum * MaxStringLen, OutputString, StringLen * sizeof (CHAR16));
    CheckedNum++;
    FreePool (OutputString);
  }

  return TotalRowNum;
}
