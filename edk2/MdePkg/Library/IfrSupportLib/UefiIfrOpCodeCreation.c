/** @file
  Library Routines to create IFR independent of string data - assume tokens already exist
  Primarily to be used for exporting op-codes at a label in pre-defined forms.


Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/

#include "UefiIfrLibraryInternal.h"

/**
  Check if the input question flags is a valid value.
  The valid combination of question flags includes
  EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY.

  @param Flags The question flags to check.

  @retval TRUE  If the question flag is a valid combination.
  @retval FALSE If the question flag is an invalid combination.
  
**/
BOOLEAN
IsValidQuestionFlags (
  IN UINT8                   Flags
  )
{
  return (BOOLEAN) (((Flags & (~QUESTION_FLAGS)) != 0) ? FALSE : TRUE);
}

/**
  Check if the input value type is a valid type.
  The valid value type is smaller or equal than EFI_IFR_TYPE_OTHER.

  @param Type   The value type to check.

  @retval TRUE  If the value type is valid.
  @retval FALSE If the value type is invalid.
  
**/
BOOLEAN
IsValidValueType (
  IN UINT8                   Type
  )
{
  return (BOOLEAN) ((Type <= EFI_IFR_TYPE_OTHER) ? TRUE : FALSE);
}

/**
  Check if the input numeric flags is a valid value.

  @param Flags The numeric flags to check.

  @retval TRUE  If the numeric flags is valid.
  @retval FALSE If the numeric flags is invalid.
  
**/
BOOLEAN
IsValidNumricFlags (
  IN UINT8                   Flags
  )
{
  if ((Flags & ~(EFI_IFR_NUMERIC_SIZE | EFI_IFR_DISPLAY)) != 0) {
    return FALSE;
  }

  if ((Flags & EFI_IFR_DISPLAY) > EFI_IFR_DISPLAY_UINT_HEX) {
    return FALSE;
  }

  return TRUE;
}

/**
  Check if the checkbox flags is a valid value.

  @param Flags The checkbox flags to check.

  @retval TRUE  If the checkbox flags is valid.
  @retval FALSE If the checkbox flags is invalid.
  
**/
BOOLEAN
IsValidCheckboxFlags (
  IN UINT8                   Flags
  )
{
  return (BOOLEAN) ((Flags <= EFI_IFR_CHECKBOX_DEFAULT_MFG) ? TRUE : FALSE);
}

/**
  Create EFI_IFR_END_OP opcode.

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateEndOpCode (
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
{
  EFI_IFR_END                 End;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (Data->Offset + sizeof (EFI_IFR_END) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  End.Header.Length  = sizeof (EFI_IFR_END);
  End.Header.OpCode  = EFI_IFR_END_OP;
  End.Header.Scope   = 0;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_END to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &End, sizeof (EFI_IFR_END));
  Data->Offset += sizeof (EFI_IFR_END);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_DEFAULT_OP opcode.

  @param  Value                  Value for the default
  @param  Type                   Type for the default
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER The type is not valid.

**/
EFI_STATUS
EFIAPI
CreateDefaultOpCode (
  IN     EFI_IFR_TYPE_VALUE  *Value,
  IN     UINT8               Type,
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
{
  EFI_IFR_DEFAULT            Default;
  UINT8                      *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if ((Value == NULL) || !IsValidValueType (Type)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + sizeof (EFI_IFR_DEFAULT) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Default.Header.OpCode = EFI_IFR_DEFAULT_OP;
  Default.Header.Length = sizeof (EFI_IFR_DEFAULT);
  Default.Header.Scope  = 0;
  Default.Type          = Type;
  Default.DefaultId     = EFI_HII_DEFAULT_CLASS_STANDARD;
  CopyMem (&Default.Value, Value, sizeof(EFI_IFR_TYPE_VALUE));

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_DEFAULT to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &Default, sizeof (EFI_IFR_DEFAULT));
  Data->Offset += sizeof (EFI_IFR_DEFAULT);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_ACTION_OP opcode.

  @param  QuestionId             Question ID
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  QuestionConfig         String ID for configuration
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateActionOpCode (
  IN     EFI_QUESTION_ID      QuestionId,
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     UINT8                QuestionFlags,
  IN     EFI_STRING_ID        QuestionConfig,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_ACTION              Action;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidQuestionFlags (QuestionFlags)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + sizeof (EFI_IFR_ACTION) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Action.Header.OpCode          = EFI_IFR_ACTION_OP;
  Action.Header.Length          = sizeof (EFI_IFR_ACTION);
  Action.Header.Scope           = 0;
  Action.Question.QuestionId    = QuestionId;
  Action.Question.Header.Prompt = Prompt;
  Action.Question.Header.Help   = Help;
  Action.Question.VarStoreId    = INVALID_VARSTORE_ID;
  Action.Question.Flags         = QuestionFlags;
  Action.QuestionConfig         = QuestionConfig;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_ACTION to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &Action, sizeof (EFI_IFR_ACTION));
  Data->Offset += sizeof (EFI_IFR_ACTION);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_SUBTITLE_OP opcode.

  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  Flags                  Subtitle opcode flags
  @param  Scope                  Subtitle Scope bit
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  
**/
EFI_STATUS
EFIAPI
CreateSubTitleOpCode (
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               Flags,
  IN      UINT8               Scope,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_SUBTITLE            Subtitle;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (Data->Offset + sizeof (EFI_IFR_SUBTITLE) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Subtitle.Header.OpCode    = EFI_IFR_SUBTITLE_OP;
  Subtitle.Header.Length    = sizeof (EFI_IFR_SUBTITLE);
  Subtitle.Header.Scope     = Scope;
  Subtitle.Statement.Prompt = Prompt;
  Subtitle.Statement.Help   = Help;
  Subtitle.Flags            = Flags;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_SUBTITLE to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &Subtitle, sizeof (EFI_IFR_SUBTITLE));
  Data->Offset += sizeof (EFI_IFR_SUBTITLE);

  return EFI_SUCCESS;
}


/**
  Create EFI_IFR_TEXT_OP opcode.

  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  TextTwo                String ID for text two
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateTextOpCode (
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     EFI_STRING_ID        TextTwo,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_TEXT                Text;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (Data->Offset + sizeof (EFI_IFR_TEXT) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Text.Header.OpCode    = EFI_IFR_TEXT_OP;
  Text.Header.Length    = sizeof (EFI_IFR_TEXT);
  Text.Header.Scope     = 0;
  Text.Statement.Prompt = Prompt;
  Text.Statement.Help   = Help;
  Text.TextTwo          = TextTwo;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_TEXT to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &Text, sizeof (EFI_IFR_TEXT));
  Data->Offset += sizeof (EFI_IFR_TEXT);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_REF_OP opcode.

  @param  FormId                 Destination Form ID
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  QuestionId             Question ID
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateGotoOpCode (
  IN     EFI_FORM_ID          FormId,
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     UINT8                QuestionFlags,
  IN     EFI_QUESTION_ID      QuestionId,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_REF                 Goto;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidQuestionFlags (QuestionFlags)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + sizeof (EFI_IFR_REF) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Goto.Header.OpCode          = EFI_IFR_REF_OP;
  Goto.Header.Length          = sizeof (EFI_IFR_REF);
  Goto.Header.Scope           = 0;
  Goto.Question.Header.Prompt = Prompt;
  Goto.Question.Header.Help   = Help;
  Goto.Question.VarStoreId    = INVALID_VARSTORE_ID;
  Goto.Question.QuestionId    = QuestionId;
  Goto.Question.Flags         = QuestionFlags;
  Goto.FormId                 = FormId;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_REF to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &Goto, sizeof (EFI_IFR_REF));
  Data->Offset += sizeof (EFI_IFR_REF);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_ONE_OF_OPTION_OP opcode.

  @param  OptionCount            The number of options.
  @param  OptionsList            The list of Options.
  @param  Type                   The data type.
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateOneOfOptionOpCode (
  IN     UINTN                OptionCount,
  IN     IFR_OPTION           *OptionsList,
  IN     UINT8                Type,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  UINTN                       Index;
  UINT8                       *LocalBuffer;
  EFI_IFR_ONE_OF_OPTION       OneOfOption;

  ASSERT (Data != NULL && Data->Data != NULL);

  if ((OptionCount != 0) && (OptionsList == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + OptionCount * sizeof (EFI_IFR_ONE_OF_OPTION) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  for (Index = 0; Index < OptionCount; Index++) {
    OneOfOption.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;
    OneOfOption.Header.Length = sizeof (EFI_IFR_ONE_OF_OPTION);
    OneOfOption.Header.Scope  = 0;

    OneOfOption.Option        = OptionsList[Index].StringToken;
    OneOfOption.Value         = OptionsList[Index].Value;
    OneOfOption.Flags         = (UINT8) (OptionsList[Index].Flags & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG));
    OneOfOption.Type          = Type;

    LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
    //
    // CopyMem is used for EFI_IFR_ONF_OF_OPTION to cover the unaligned address access.
    //
    CopyMem (LocalBuffer, &OneOfOption, sizeof (EFI_IFR_ONE_OF_OPTION));
    Data->Offset += sizeof (EFI_IFR_ONE_OF_OPTION);
  }

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_ONE_OF_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  OneOfFlags             Flags for oneof opcode
  @param  OptionsList            List of options
  @param  OptionCount            Number of options in option list
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateOneOfOpCode (
  IN     EFI_QUESTION_ID      QuestionId,
  IN     EFI_VARSTORE_ID      VarStoreId,
  IN     UINT16               VarOffset,
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     UINT8                QuestionFlags,
  IN     UINT8                OneOfFlags,
  IN     IFR_OPTION           *OptionsList,
  IN     UINTN                OptionCount,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  UINTN                       Length;
  EFI_IFR_ONE_OF              OneOf;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidNumricFlags (OneOfFlags) ||
      !IsValidQuestionFlags (QuestionFlags) ||
      ((OptionCount != 0) && (OptionsList == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  Length = sizeof (EFI_IFR_ONE_OF) + OptionCount * sizeof (EFI_IFR_ONE_OF_OPTION) + sizeof (EFI_IFR_END);
  if (Data->Offset + Length > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  OneOf.Header.OpCode                   = EFI_IFR_ONE_OF_OP;
  OneOf.Header.Length                   = sizeof (EFI_IFR_ONE_OF);
  OneOf.Header.Scope                    = 1;
  OneOf.Question.Header.Prompt          = Prompt;
  OneOf.Question.Header.Help            = Help;
  OneOf.Question.QuestionId             = QuestionId;
  OneOf.Question.VarStoreId             = VarStoreId;
  OneOf.Question.VarStoreInfo.VarOffset = VarOffset;
  OneOf.Question.Flags                  = QuestionFlags;
  OneOf.Flags                           = OneOfFlags;
  ZeroMem ((VOID *) &OneOf.data, sizeof (MINMAXSTEP_DATA));

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_ONF_OF to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &OneOf, sizeof (EFI_IFR_ONE_OF));
  Data->Offset += sizeof (EFI_IFR_ONE_OF);

  CreateOneOfOptionOpCode (OptionCount, OptionsList, (UINT8) (OneOfFlags & EFI_IFR_NUMERIC_SIZE), Data);

  CreateEndOpCode (Data);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_ORDERED_LIST_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  OrderedListFlags       Flags for ordered list opcode
  @param  DataType               Type for option value
  @param  MaxContainers          Maximum count for options in this ordered list
  @param  OptionsList            List of options
  @param  OptionCount            Number of options in option list
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateOrderedListOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               OrderedListFlags,
  IN      UINT8               DataType,
  IN      UINT8               MaxContainers,
  IN      IFR_OPTION          *OptionsList,
  IN     UINTN                OptionCount,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  UINTN                       Length;
  EFI_IFR_ORDERED_LIST        OrderedList;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidQuestionFlags (QuestionFlags) ||
      ((OptionCount != 0) && (OptionsList == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((OrderedListFlags != 0) &&
      (OrderedListFlags != EFI_IFR_UNIQUE_SET) &&
      (OrderedListFlags != EFI_IFR_NO_EMPTY_SET)) {
    return EFI_INVALID_PARAMETER;
  }

  Length = sizeof (EFI_IFR_ORDERED_LIST) + OptionCount * sizeof (EFI_IFR_ONE_OF_OPTION) + sizeof (EFI_IFR_END);
  if (Data->Offset + Length > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  OrderedList.Header.OpCode                   = EFI_IFR_ORDERED_LIST_OP;
  OrderedList.Header.Length                   = sizeof (EFI_IFR_ORDERED_LIST);
  OrderedList.Header.Scope                    = 1;
  OrderedList.Question.Header.Prompt          = Prompt;
  OrderedList.Question.Header.Help            = Help;
  OrderedList.Question.QuestionId             = QuestionId;
  OrderedList.Question.VarStoreId             = VarStoreId;
  OrderedList.Question.VarStoreInfo.VarOffset = VarOffset;
  OrderedList.Question.Flags                  = QuestionFlags;
  OrderedList.MaxContainers                   = MaxContainers;
  OrderedList.Flags                           = OrderedListFlags;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_ORDERED_LIST to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &OrderedList, sizeof (EFI_IFR_ORDERED_LIST));
  Data->Offset += sizeof (EFI_IFR_ORDERED_LIST);

  CreateOneOfOptionOpCode (OptionCount, OptionsList, DataType, Data);

  CreateEndOpCode (Data);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_CHECKBOX_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  CheckBoxFlags          Flags for checkbox opcode
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateCheckBoxOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               CheckBoxFlags,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_CHECKBOX            CheckBox;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidQuestionFlags (QuestionFlags) || !IsValidCheckboxFlags (CheckBoxFlags)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + sizeof (EFI_IFR_CHECKBOX) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  CheckBox.Header.OpCode                   = EFI_IFR_CHECKBOX_OP;
  CheckBox.Header.Length                   = sizeof (EFI_IFR_CHECKBOX);
  CheckBox.Header.Scope                    = 0;
  CheckBox.Question.QuestionId             = QuestionId;
  CheckBox.Question.VarStoreId             = VarStoreId;
  CheckBox.Question.VarStoreInfo.VarOffset = VarOffset;
  CheckBox.Question.Header.Prompt          = Prompt;
  CheckBox.Question.Header.Help            = Help;
  CheckBox.Question.Flags                  = QuestionFlags;
  CheckBox.Flags                           = CheckBoxFlags;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_CHECKBOX to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &CheckBox, sizeof (EFI_IFR_CHECKBOX));
  Data->Offset += sizeof (EFI_IFR_CHECKBOX);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_NUMERIC_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  NumericFlags           Flags for numeric opcode
  @param  Minimum                Numeric minimum value
  @param  Maximum                Numeric maximum value
  @param  Step                   Numeric step for edit
  @param  Default                Numeric default value
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateNumericOpCode (
  IN     EFI_QUESTION_ID     QuestionId,
  IN     EFI_VARSTORE_ID     VarStoreId,
  IN     UINT16              VarOffset,
  IN     EFI_STRING_ID       Prompt,
  IN     EFI_STRING_ID       Help,
  IN     UINT8               QuestionFlags,
  IN     UINT8               NumericFlags,
  IN     UINT64              Minimum,
  IN     UINT64              Maximum,
  IN     UINT64              Step,
  IN     UINT64              Default,
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
{
  EFI_STATUS                  Status;
  EFI_IFR_NUMERIC             Numeric;
  MINMAXSTEP_DATA             MinMaxStep;
  EFI_IFR_TYPE_VALUE          DefaultValue;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidQuestionFlags (QuestionFlags) || !IsValidNumricFlags (NumericFlags)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + sizeof (EFI_IFR_CHECKBOX) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  Numeric.Header.OpCode                   = EFI_IFR_NUMERIC_OP;
  Numeric.Header.Length                   = sizeof (EFI_IFR_NUMERIC);
  Numeric.Header.Scope                    = 1;
  Numeric.Question.QuestionId             = QuestionId;
  Numeric.Question.VarStoreId             = VarStoreId;
  Numeric.Question.VarStoreInfo.VarOffset = VarOffset;
  Numeric.Question.Header.Prompt          = Prompt;
  Numeric.Question.Header.Help            = Help;
  Numeric.Question.Flags                  = QuestionFlags;
  Numeric.Flags                           = NumericFlags;

  switch (NumericFlags & EFI_IFR_NUMERIC_SIZE) {
  case EFI_IFR_NUMERIC_SIZE_1:
    MinMaxStep.u8.MinValue = (UINT8) Minimum;
    MinMaxStep.u8.MaxValue = (UINT8) Maximum;
    MinMaxStep.u8.Step     = (UINT8) Step;
    break;

  case EFI_IFR_NUMERIC_SIZE_2:
    MinMaxStep.u16.MinValue = (UINT16) Minimum;
    MinMaxStep.u16.MaxValue = (UINT16) Maximum;
    MinMaxStep.u16.Step     = (UINT16) Step;
    break;

  case EFI_IFR_NUMERIC_SIZE_4:
    MinMaxStep.u32.MinValue = (UINT32) Minimum;
    MinMaxStep.u32.MaxValue = (UINT32) Maximum;
    MinMaxStep.u32.Step     = (UINT32) Step;
    break;

  case EFI_IFR_NUMERIC_SIZE_8:
    MinMaxStep.u64.MinValue = Minimum;
    MinMaxStep.u64.MaxValue = Maximum;
    MinMaxStep.u64.Step     = Step;
    break;
  }

  CopyMem (&Numeric.data, &MinMaxStep, sizeof (MINMAXSTEP_DATA));

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_NUMERIC to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &Numeric, sizeof (EFI_IFR_NUMERIC));
  Data->Offset += sizeof (EFI_IFR_NUMERIC);

  DefaultValue.u64 = Default;
  Status = CreateDefaultOpCode (&DefaultValue, (UINT8) (NumericFlags & EFI_IFR_NUMERIC_SIZE), Data);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  CreateEndOpCode (Data);

  return EFI_SUCCESS;
}

/**
  Create EFI_IFR_STRING_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  StringFlags            Flags for string opcode
  @param  MinSize                String minimum length
  @param  MaxSize                String maximum length
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateStringOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               StringFlags,
  IN      UINT8               MinSize,
  IN      UINT8               MaxSize,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
{
  EFI_IFR_STRING              String;
  UINT8                       *LocalBuffer;

  ASSERT (Data != NULL && Data->Data != NULL);

  if (!IsValidQuestionFlags (QuestionFlags) || (StringFlags & ~EFI_IFR_STRING_MULTI_LINE) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  if (Data->Offset + sizeof (EFI_IFR_STRING) > Data->BufferSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  String.Header.OpCode                   = EFI_IFR_STRING_OP;
  String.Header.Length                   = sizeof (EFI_IFR_STRING);
  String.Header.Scope                    = 0;
  String.Question.Header.Prompt          = Prompt;
  String.Question.Header.Help            = Help;
  String.Question.QuestionId             = QuestionId;
  String.Question.VarStoreId             = VarStoreId;
  String.Question.VarStoreInfo.VarOffset = VarOffset;
  String.Question.Flags                  = QuestionFlags;
  String.MinSize                         = MinSize;
  String.MaxSize                         = MaxSize;
  String.Flags                           = StringFlags;

  LocalBuffer = (UINT8 *) Data->Data + Data->Offset;
  //
  // CopyMem is used for EFI_IFR_STRING to cover the unaligned address access.
  //
  CopyMem (LocalBuffer, &String, sizeof (EFI_IFR_STRING));
  Data->Offset += sizeof (EFI_IFR_STRING);

  return EFI_SUCCESS;
}


