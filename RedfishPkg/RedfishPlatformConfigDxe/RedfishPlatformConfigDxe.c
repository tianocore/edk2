/** @file
  The implementation of EDKII Redfish Platform Config Protocol.

  (C) Copyright 2021-2022 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishPlatformConfigDxe.h"
#include "RedfishPlatformConfigImpl.h"

REDFISH_PLATFORM_CONFIG_PRIVATE  *mRedfishPlatformConfigPrivate = NULL;

/**
  Create a new stack instance with given stack size.

  @param[in]  StackSize  The size of stack.

  @retval REDFISH_STACK * Pointer to created stack.
  @retval NULL            Out of resource.

**/
REDFISH_STACK *
NewRedfishStack (
  IN UINTN  StackSize
  )
{
  REDFISH_STACK  *Buffer;

  if (StackSize == 0) {
    return NULL;
  }

  Buffer = AllocateZeroPool (sizeof (REDFISH_STACK));
  if (Buffer == NULL) {
    return NULL;
  }

  Buffer->Pool = AllocateZeroPool (sizeof (VOID *) * StackSize);
  if (Buffer->Pool == NULL) {
    FreePool (Buffer);
    return NULL;
  }

  Buffer->Size  = StackSize;
  Buffer->Index = 0;

  return Buffer;
}

/**
  Release stack buffer.

  @param[in]  Stack     Pointer to stack instance.

**/
VOID
ReleaseRedfishStack (
  IN REDFISH_STACK  *Stack
  )
{
  if (Stack == NULL) {
    return;
  }

  FreePool (Stack->Pool);
  FreePool (Stack);
}

/**
  Check and see if stack is empty or not.

  @param[in]  Stack     Pointer to stack instance.

  @retval TRUE          Stack is empty.
  @retval FALSE         Stack is not empty.

**/
BOOLEAN
IsEmptyRedfishStack (
  IN REDFISH_STACK  *Stack
  )
{
  return (Stack->Index == 0);
}

/**
  Push an item to stack.

  @param[in]  Stack     Pointer to stack instance.
  @param[in]  Data      Pointer to data.

  @retval EFI_OUT_OF_RESOURCES   Stack is full.
  @retval EFI_SUCCESS            Item is pushed successfully.

**/
EFI_STATUS
PushRedfishStack (
  IN REDFISH_STACK  *Stack,
  IN VOID           *Data
  )
{
  if (Stack->Index == Stack->Size) {
    return EFI_OUT_OF_RESOURCES;
  }

  Stack->Pool[Stack->Index] = Data;
  Stack->Index             += 1;

  return EFI_SUCCESS;
}

/**
  Pop an item from stack.

  @param[in]  Stack     Pointer to stack instance.

  @retval VOID *        Pointer to popped item.
  @retval NULL          Stack is empty.

**/
VOID *
PopRedfishStack (
  IN REDFISH_STACK  *Stack
  )
{
  if (IsEmptyRedfishStack (Stack)) {
    return NULL;
  }

  Stack->Index -= 1;
  return Stack->Pool[Stack->Index];
}

/**
  Seach forms in this HII package and find which form links to give form.

  @param[in]  FormPrivate   Pointer to form private instance.

  @retval REDFISH_PLATFORM_CONFIG_FORM_PRIVATE Pointer to target form
  @retval NULL                                 No form links to give form.

**/
REDFISH_PLATFORM_CONFIG_FORM_PRIVATE *
FindFormLinkToThis (
  IN REDFISH_PLATFORM_CONFIG_FORM_PRIVATE  *FormPrivate
  )
{
  LIST_ENTRY                                 *HiiFormLink;
  LIST_ENTRY                                 *HiiNextFormLink;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE       *HiiFormPrivate;
  LIST_ENTRY                                 *HiiStatementLink;
  LIST_ENTRY                                 *HiiNextStatementLink;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *HiiStatementPrivate;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE   *HiiFormsetPrivate;

  if (FormPrivate == NULL) {
    return NULL;
  }

  HiiFormsetPrivate = FormPrivate->ParentFormset;

  if (IsListEmpty (&HiiFormsetPrivate->HiiFormList)) {
    return NULL;
  }

  HiiFormLink = GetFirstNode (&HiiFormsetPrivate->HiiFormList);
  while (!IsNull (&HiiFormsetPrivate->HiiFormList, HiiFormLink)) {
    HiiFormPrivate  = REDFISH_PLATFORM_CONFIG_FORM_FROM_LINK (HiiFormLink);
    HiiNextFormLink = GetNextNode (&HiiFormsetPrivate->HiiFormList, HiiFormLink);

    //
    // Skip myself
    //
    if (HiiFormPrivate == FormPrivate) {
      HiiFormLink = HiiNextFormLink;
      continue;
    }

    HiiStatementLink = GetFirstNode (&HiiFormPrivate->StatementList);
    while (!IsNull (&HiiFormPrivate->StatementList, HiiStatementLink)) {
      HiiStatementPrivate  = REDFISH_PLATFORM_CONFIG_STATEMENT_FROM_LINK (HiiStatementLink);
      HiiNextStatementLink = GetNextNode (&HiiFormPrivate->StatementList, HiiStatementLink);

      //
      // Check go-to opcode and find form ID. If form ID is the same ID as given form,
      // this go-to opcode links to given form.
      //
      if ((HiiStatementPrivate->HiiStatement->Operand == EFI_IFR_REF_OP) &&
          (HiiStatementPrivate->HiiStatement->Value.Value.ref.FormId == FormPrivate->HiiForm->FormId))
      {
        return HiiFormPrivate;
      }

      HiiStatementLink = HiiNextStatementLink;
    }

    HiiFormLink = HiiNextFormLink;
  }

  return NULL;
}

/**
  Debug dump HII statement value.

  @param[in]  ErrorLevel    DEBUG macro error level
  @param[in]  Value         HII statement value to dump
  @param[in]  Message       Debug message

  @retval EFI_SUCCESS       Dump HII statement value successfully
  @retval Others            Errors occur

**/
EFI_STATUS
DumpHiiStatementValue (
  IN UINTN                ErrorLevel,
  IN HII_STATEMENT_VALUE  *Value,
  IN CHAR8                *Message OPTIONAL
  )
{
  UINT64  Data;

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Value->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Data = Value->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      Data = Value->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      Data = Value->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      Data = Value->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      Data = (Value->Value.b ? 1 : 0);
      break;
    default:
      DEBUG ((ErrorLevel, "%a: unsupported type: 0x%x\n", __func__, Value->Type));
      return EFI_UNSUPPORTED;
  }

  if (IS_EMPTY_STRING (Message)) {
    DEBUG ((ErrorLevel, "0x%lx\n", Data));
  } else {
    DEBUG ((ErrorLevel, "%a: 0x%lx\n", Message, Data));
  }

  return EFI_SUCCESS;
}

/**
  Debug dump HII statement prompt string.

  @param[in]  ErrorLevel    DEBUG macro error level
  @param[in]  HiiHandle     HII handle instance
  @param[in]  HiiStatement  HII statement
  @param[in]  Message       Debug message

  @retval EFI_SUCCESS       Dump HII statement string successfully
  @retval Others            Errors occur

**/
EFI_STATUS
DumpHiiStatementPrompt (
  IN UINTN           ErrorLevel,
  IN EFI_HII_HANDLE  HiiHandle,
  IN HII_STATEMENT   *HiiStatement,
  IN CHAR8           *Message OPTIONAL
  )
{
  EFI_STRING  String;

  if ((HiiHandle == NULL) || (HiiStatement == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (HiiStatement->Prompt == 0) {
    return EFI_NOT_FOUND;
  }

  String = HiiGetString (HiiHandle, HiiStatement->Prompt, NULL);
  if (String == NULL) {
    return EFI_NOT_FOUND;
  }

  if (IS_EMPTY_STRING (Message)) {
    DEBUG ((ErrorLevel, "%s\n", String));
  } else {
    DEBUG ((ErrorLevel, "%a: %s\n", Message, String));
  }

  FreePool (String);

  return EFI_SUCCESS;
}

/**
  Build the menu path to given statement instance. It is caller's
  responsibility to free returned string buffer.

  @param[in]  StatementPrivate   Pointer to statement private instance.

  @retval CHAR8 *                Menu path to given statement.
  @retval NULL                   Can not find menu path.

**/
CHAR8 *
BuildMenuPath (
  IN REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *StatementPrivate
  )
{
  REDFISH_STACK                         *FormStack;
  REDFISH_PLATFORM_CONFIG_FORM_PRIVATE  *FormPrivate;
  UINTN                                 OldBufferSize;
  UINTN                                 NewBufferSize;
  CHAR8                                 *Buffer;
  CHAR8                                 *FormTitle;
  EFI_STATUS                            Status;

  Buffer        = NULL;
  OldBufferSize = 0;
  NewBufferSize = 0;
  FormStack     = NewRedfishStack (REDFISH_MENU_PATH_SIZE);
  if (FormStack == NULL) {
    return NULL;
  }

  //
  // Build form link stack
  //
  FormPrivate = StatementPrivate->ParentForm;
  Status      = PushRedfishStack (FormStack, (VOID *)FormPrivate);
  if (EFI_ERROR (Status)) {
    goto RELEASE;
  }

  do {
    DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "F(%d) <-", FormPrivate->Id));
    FormPrivate = FindFormLinkToThis (FormPrivate);
    if (FormPrivate == NULL) {
      break;
    }

    PushRedfishStack (FormStack, (VOID *)FormPrivate);
    if (EFI_ERROR (Status)) {
      break;
    }
  } while (TRUE);

  if (IsEmptyRedfishStack (FormStack)) {
    goto RELEASE;
  }

  //
  // Initial Buffer to empty string for error case.
  //
  OldBufferSize = AsciiStrSize ("");
  Buffer        = AllocateCopyPool (OldBufferSize, "");
  if (Buffer == NULL) {
    goto RELEASE;
  }

  //
  // Build menu path in string format
  //
  FormPrivate = (REDFISH_PLATFORM_CONFIG_FORM_PRIVATE *)PopRedfishStack (FormStack);
  while (FormPrivate != NULL) {
    FormTitle = HiiGetEnglishAsciiString (FormPrivate->ParentFormset->HiiHandle, FormPrivate->Title);
    if (FormTitle != NULL) {
      NewBufferSize = AsciiStrSize (FormTitle) + OldBufferSize;
      Buffer        = ReallocatePool (OldBufferSize, NewBufferSize, Buffer);
      if (Buffer == NULL) {
        goto RELEASE;
      }

      OldBufferSize = NewBufferSize;
      AsciiStrCatS (Buffer, OldBufferSize, "/");
      AsciiStrCatS (Buffer, OldBufferSize, FormTitle);
      FreePool (FormTitle);
      DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, " %a\n", Buffer));
    }

    FormPrivate = (REDFISH_PLATFORM_CONFIG_FORM_PRIVATE *)PopRedfishStack (FormStack);
  }

RELEASE:

  ReleaseRedfishStack (FormStack);

  return Buffer;
}

/**
  Get the attribute name from config language.

  For example:  /Bios/Attributes/BiosOption1 is config language
  and attribute name is BiosOption1.

  @param[in]  ConfigLanguage     Config language string.

  @retval CHAR8 *                Attribute name string.
  @retval NULL                   Can not find attribute name.

**/
CHAR8 *
GetAttributeNameFromConfigLanguage (
  IN  CHAR8  *ConfigLanguage
  )
{
  CHAR8  *attributeName;
  CHAR8  *Pointer;
  UINTN  StrLen;
  UINTN  Index;
  UINTN  AttrStrLen;

  if (IS_EMPTY_STRING (ConfigLanguage)) {
    return NULL;
  }

  attributeName = NULL;
  Pointer       = NULL;
  AttrStrLen    = 0;
  StrLen        = AsciiStrLen (ConfigLanguage);

  if (ConfigLanguage[StrLen - 1] == '/') {
    //
    // wrong format
    //
    DEBUG ((DEBUG_ERROR, "%a: invalid format: %a\n", __func__, ConfigLanguage));
    ASSERT (FALSE);
    return NULL;
  }

  Index = StrLen;
  while (TRUE) {
    Index -= 1;

    if (ConfigLanguage[Index] == '/') {
      Pointer = &ConfigLanguage[Index + 1];
      break;
    }

    if (Index == 0) {
      break;
    }
  }

  //
  // Not found. There is no '/' in input string.
  //
  if (Pointer == NULL) {
    return NULL;
  }

  AttrStrLen    = StrLen - Index;
  attributeName = AllocateCopyPool (AttrStrLen, Pointer);

  return attributeName;
}

/**
  Convert one-of options to string array in Redfish attribute.

  @param[in]  HiiHandle          HII handle.
  @param[in]  SchemaName         Schema string.
  @param[in]  StatementPrivate   Pointer to statement instance.
  @param[out] Values             Attribute value array.

  @retval EFI_SUCCESS            Options are converted successfully.
  @retval Other                  Error occurs.

**/
EFI_STATUS
OneOfStatementToAttributeValues (
  IN  EFI_HII_HANDLE                             HiiHandle,
  IN  CHAR8                                      *SchemaName,
  IN  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *StatementPrivate,
  OUT EDKII_REDFISH_POSSIBLE_VALUES              *Values
  )
{
  LIST_ENTRY           *Link;
  HII_QUESTION_OPTION  *Option;
  UINTN                Index;
  HII_STATEMENT        *HiiStatement;

  if ((HiiHandle == NULL) || (StatementPrivate == NULL) || (Values == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HiiStatement = StatementPrivate->HiiStatement;
  ASSERT (HiiStatement != NULL);

  if (IsListEmpty (&HiiStatement->OptionListHead)) {
    return EFI_NOT_FOUND;
  }

  //
  // Loop through the option to get count
  //
  Values->ValueCount = 0;
  Link               = GetFirstNode (&HiiStatement->OptionListHead);
  while (!IsNull (&HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    if ((Option->SuppressExpression != NULL) &&
        (EvaluateExpressionList (Option->SuppressExpression, TRUE, StatementPrivate->ParentForm->ParentFormset->HiiFormSet, StatementPrivate->ParentForm->HiiForm) != ExpressFalse))
    {
      Link = GetNextNode (&HiiStatement->OptionListHead, Link);
      continue;
    }

    Values->ValueCount += 1;
    Link                = GetNextNode (&HiiStatement->OptionListHead, Link);
  }

  Values->ValueArray = AllocateZeroPool (sizeof (EDKII_REDFISH_ATTRIBUTE_VALUE) * Values->ValueCount);
  if (Values->ValueArray == NULL) {
    Values->ValueCount = 0;
    return EFI_OUT_OF_RESOURCES;
  }

  Index = 0;
  Link  = GetFirstNode (&HiiStatement->OptionListHead);
  while (!IsNull (&HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    if ((Option->SuppressExpression != NULL) &&
        (EvaluateExpressionList (Option->SuppressExpression, FALSE, NULL, NULL) != ExpressFalse))
    {
      Link = GetNextNode (&HiiStatement->OptionListHead, Link);
      continue;
    }

    if (Option->Text != 0) {
      Values->ValueArray[Index].ValueName        = HiiGetRedfishAsciiString (HiiHandle, SchemaName, Option->Text);
      Values->ValueArray[Index].ValueDisplayName = HiiGetEnglishAsciiString (HiiHandle, Option->Text);
    }

    Index += 1;
    Link   = GetNextNode (&HiiStatement->OptionListHead, Link);
  }

  return EFI_SUCCESS;
}

/**
  Return Redfish attribute type from given HII statement operand.

  @param[in]  HiiStatement       Target HII statement.

  @retval EDKII_REDFISH_ATTRIBUTE_TYPES    Attribute type.

**/
EDKII_REDFISH_ATTRIBUTE_TYPES
HiiStatementToAttributeType (
  IN  HII_STATEMENT  *HiiStatement
  )
{
  EDKII_REDFISH_ATTRIBUTE_TYPES  type;

  if (HiiStatement == NULL) {
    return RedfishAttributeTypeUnknown;
  }

  type = RedfishAttributeTypeUnknown;
  switch (HiiStatement->Operand) {
    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_ORDERED_LIST_OP:
      type = RedfishAttributeTypeEnumeration;
      break;
    case EFI_IFR_STRING_OP:
      type = RedfishAttributeTypeString;
      break;
    case EFI_IFR_NUMERIC_OP:
      type = RedfishAttributeTypeInteger;
      break;
    case EFI_IFR_CHECKBOX_OP:
      type = RedfishAttributeTypeBoolean;
      break;
    case EFI_IFR_DATE_OP:
    case EFI_IFR_TIME_OP:
    default:
      DEBUG ((DEBUG_ERROR, "%a: unsupported operand: 0x%x\n", __func__, HiiStatement->Operand));
      break;
  }

  return type;
}

/**
  Zero extend integer/boolean to UINT64 for comparing.

  @param  Value                  HII Value to be converted.

**/
UINT64
ExtendHiiValueToU64 (
  IN HII_STATEMENT_VALUE  *Value
  )
{
  UINT64  Temp;

  Temp = 0;
  switch (Value->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Temp = Value->Value.u8;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_16:
      Temp = Value->Value.u16;
      break;

    case EFI_IFR_TYPE_NUM_SIZE_32:
      Temp = Value->Value.u32;
      break;

    case EFI_IFR_TYPE_BOOLEAN:
      Temp = Value->Value.b;
      break;

    case EFI_IFR_TYPE_TIME:
    case EFI_IFR_TYPE_DATE:
    default:
      break;
  }

  return Temp;
}

/**
  Set value of a data element in an Array by its Index in ordered list buffer.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.
  @param  Value                  The value to be set.

**/
VOID
OrderedListSetArrayData (
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
  Return data element in an Array by its Index in ordered list array buffer.

  @param  Array                  The data array.
  @param  Type                   Type of the data in this array.
  @param  Index                  Zero based index for data in this array.

  @retval Value                  The data to be returned

**/
UINT64
OrderedListGetArrayData (
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
  Find string ID of option if its value equals to given value.

  @param[in]  HiiStatement  Statement to search.
  @param[in]  Value         Target value.

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STRING_ID
OrderedListOptionValueToStringId (
  IN  HII_STATEMENT  *HiiStatement,
  IN  UINT64         Value
  )
{
  LIST_ENTRY           *Link;
  HII_QUESTION_OPTION  *Option;
  UINT64               CurrentValue;

  if (HiiStatement == NULL) {
    return 0;
  }

  if (HiiStatement->Operand != EFI_IFR_ORDERED_LIST_OP) {
    return 0;
  }

  if (IsListEmpty (&HiiStatement->OptionListHead)) {
    return 0;
  }

  Link = GetFirstNode (&HiiStatement->OptionListHead);
  while (!IsNull (&HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    CurrentValue = ExtendHiiValueToU64 (&Option->Value);
    if (Value == CurrentValue) {
      return Option->Text;
    }

    Link = GetNextNode (&HiiStatement->OptionListHead, Link);
  }

  return 0;
}

/**
  Compare two value in HII statement format.

  @param[in]  Value1        First value to compare.
  @param[in]  Value2        Second value to be compared.

  @retval INTN          0 is returned when two values are equal.
                        1 is returned when first value is greater than second value.
                        -1 is returned when second value is greater than first value.

**/
INTN
CompareHiiStatementValue (
  IN HII_STATEMENT_VALUE  *Value1,
  IN HII_STATEMENT_VALUE  *Value2
  )
{
  INTN    Result;
  UINT64  Data1;
  UINT64  Data2;

  if ((Value1 == NULL) || (Value2 == NULL)) {
    return -1;
  }

  switch (Value1->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Data1 = Value1->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      Data1 = Value1->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      Data1 = Value1->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      Data1 = Value1->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      Data1 = (Value1->Value.b ? 1 : 0);
      break;
    default:
      return -1;
  }

  switch (Value2->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Data2 = Value2->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      Data2 = Value2->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      Data2 = Value2->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      Data2 = Value2->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      Data2 = (Value2->Value.b ? 1 : 0);
      break;
    default:
      return -1;
  }

  Result = (Data1 == Data2 ? 0 : (Data1 > Data2 ? 1 : -1));

  return Result;
}

/**
  Convert HII value to the string in HII one-of opcode.

  @param[in]  HiiStatement  HII Statement private instance
  @param[in]  Value         HII Statement value

  @retval EFI_STRING_ID     The string ID in HII database.
                            0 is returned when something goes wrong.

**/
EFI_STRING_ID
HiiValueToOneOfOptionStringId (
  IN HII_STATEMENT        *HiiStatement,
  IN HII_STATEMENT_VALUE  *Value
  )
{
  LIST_ENTRY           *Link;
  HII_QUESTION_OPTION  *Option;

  if ((HiiStatement == NULL) || (Value == NULL)) {
    return 0;
  }

  if (HiiStatement->Operand != EFI_IFR_ONE_OF_OP) {
    return 0;
  }

  if (IsListEmpty (&HiiStatement->OptionListHead)) {
    return 0;
  }

  Link = GetFirstNode (&HiiStatement->OptionListHead);
  while (!IsNull (&HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    if (CompareHiiStatementValue (Value, &Option->Value) == 0) {
      return Option->Text;
    }

    Link = GetNextNode (&HiiStatement->OptionListHead, Link);
  }

  return 0;
}

/**
  Convert HII string to the value in HII one-of opcode.

  @param[in]  Statement     Statement private instance
  @param[in]  Schema        Schema string
  @param[in]  HiiString     Input string
  @param[out] Value         Value returned

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
HiiStringToOneOfOptionValue (
  IN  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *Statement,
  IN  CHAR8                                      *Schema,
  IN  EFI_STRING                                 HiiString,
  OUT HII_STATEMENT_VALUE                        *Value
  )
{
  LIST_ENTRY           *Link;
  HII_QUESTION_OPTION  *Option;
  EFI_STRING           TmpString;
  BOOLEAN              Found;

  if ((Statement == NULL) || IS_EMPTY_STRING (HiiString) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Statement->HiiStatement->Operand != EFI_IFR_ONE_OF_OP) {
    return EFI_UNSUPPORTED;
  }

  if (IsListEmpty (&Statement->HiiStatement->OptionListHead)) {
    return EFI_NOT_FOUND;
  }

  Found = FALSE;
  Link  = GetFirstNode (&Statement->HiiStatement->OptionListHead);
  while (!IsNull (&Statement->HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    TmpString = HiiGetRedfishString (Statement->ParentForm->ParentFormset->HiiHandle, Schema, Option->Text);
    if (TmpString == NULL) {
      TmpString = HiiGetRedfishString (Statement->ParentForm->ParentFormset->HiiHandle, ENGLISH_LANGUAGE_CODE, Option->Text);
    }

    if (TmpString != NULL) {
      if (StrCmp (TmpString, HiiString) == 0) {
        CopyMem (Value, &Option->Value, sizeof (HII_STATEMENT_VALUE));
        Found = TRUE;
      }

      FreePool (TmpString);
    }

    if (Found) {
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Statement->HiiStatement->OptionListHead, Link);
  }

  return EFI_NOT_FOUND;
}

/**
  Convert HII value to numeric value in Redfish format.

  @param[in]  Value         Value to be converted.
  @param[out] RedfishValue  Value in Redfish format.

  @retval EFI_SUCCESS       Redfish value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
HiiValueToRedfishNumeric (
  IN  HII_STATEMENT_VALUE  *Value,
  OUT EDKII_REDFISH_VALUE  *RedfishValue
  )
{
  if ((Value == NULL) || (RedfishValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (Value->Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      RedfishValue->Type          = RedfishValueTypeInteger;
      RedfishValue->Value.Integer = (INT64)Value->Value.u8;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      RedfishValue->Type          = RedfishValueTypeInteger;
      RedfishValue->Value.Integer = (INT64)Value->Value.u16;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      RedfishValue->Type          = RedfishValueTypeInteger;
      RedfishValue->Value.Integer = (INT64)Value->Value.u32;
      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      RedfishValue->Type          = RedfishValueTypeInteger;
      RedfishValue->Value.Integer = (INT64)Value->Value.u64;
      break;
    case EFI_IFR_TYPE_BOOLEAN:
      RedfishValue->Type          = RedfishValueTypeBoolean;
      RedfishValue->Value.Boolean = Value->Value.b;
      break;
    default:
      RedfishValue->Type = RedfishValueTypeUnknown;
      DEBUG ((DEBUG_ERROR, "%a: Unsupported value type: 0x%x\n", __func__, Value->Type));
      break;
  }

  return EFI_SUCCESS;
}

/**
  Convert numeric value in Redfish format to HII value.

  @param[in]   RedfishValue  Value in Redfish format to be converted.
  @param[out]  Value         HII value returned.

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishNumericToHiiValue (
  IN  EDKII_REDFISH_VALUE  *RedfishValue,
  OUT HII_STATEMENT_VALUE  *Value
  )
{
  if ((Value == NULL) || (RedfishValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  switch (RedfishValue->Type) {
    case RedfishValueTypeInteger:
      Value->Type      = EFI_IFR_TYPE_NUM_SIZE_64;
      Value->Value.u64 = (UINT64)RedfishValue->Value.Integer;
      break;
    case RedfishValueTypeBoolean:
      Value->Type    = EFI_IFR_TYPE_BOOLEAN;
      Value->Value.b = RedfishValue->Value.Boolean;
      break;
    default:
      Value->Type = EFI_IFR_TYPE_UNDEFINED;
      break;
  }

  return EFI_SUCCESS;
}

/**
  Dump the value in ordered list buffer.

  @param[in]   OrderedListStatement Ordered list statement.

**/
VOID
DumpOrderedListValue (
  IN  HII_STATEMENT  *OrderedListStatement
  )
{
  UINT8   *Value8;
  UINT16  *Value16;
  UINT32  *Value32;
  UINT64  *Value64;
  UINTN   Count;
  UINTN   Index;

  if ((OrderedListStatement == NULL) || (OrderedListStatement->Operand != EFI_IFR_ORDERED_LIST_OP)) {
    return;
  }

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "Value.Type= 0x%x\n", OrderedListStatement->Value.Type));
  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "Value.BufferValueType= 0x%x\n", OrderedListStatement->Value.BufferValueType));
  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "Value.BufferLen= 0x%x\n", OrderedListStatement->Value.BufferLen));
  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "Value.Buffer= 0x%x\n", OrderedListStatement->Value.Buffer));
  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "Value.MaxContainers= 0x%x\n", OrderedListStatement->ExtraData.OrderListData.MaxContainers));
  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "StorageWidth= 0x%x\n", OrderedListStatement->StorageWidth));

  if (OrderedListStatement->Value.Buffer == NULL) {
    return;
  }

  Value8  = NULL;
  Value16 = NULL;
  Value32 = NULL;
  Value64 = NULL;
  Count   = 0;

  switch (OrderedListStatement->Value.BufferValueType) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
      Value8 = (UINT8 *)OrderedListStatement->Value.Buffer;
      Count  = OrderedListStatement->StorageWidth / sizeof (UINT8);
      for (Index = 0; Index < Count; Index++) {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%d ", Value8[Index]));
      }

      break;
    case EFI_IFR_TYPE_NUM_SIZE_16:
      Value16 = (UINT16 *)OrderedListStatement->Value.Buffer;
      Count   = OrderedListStatement->StorageWidth / sizeof (UINT16);
      for (Index = 0; Index < Count; Index++) {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%d ", Value16[Index]));
      }

      break;
    case EFI_IFR_TYPE_NUM_SIZE_32:
      Value32 = (UINT32 *)OrderedListStatement->Value.Buffer;
      Count   = OrderedListStatement->StorageWidth / sizeof (UINT32);
      for (Index = 0; Index < Count; Index++) {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%d ", Value32[Index]));
      }

      break;
    case EFI_IFR_TYPE_NUM_SIZE_64:
      Value64 = (UINT64 *)OrderedListStatement->Value.Buffer;
      Count   = OrderedListStatement->StorageWidth / sizeof (UINT64);
      for (Index = 0; Index < Count; Index++) {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%d ", Value64[Index]));
      }

      break;
    default:
      Value8 = (UINT8 *)OrderedListStatement->Value.Buffer;
      Count  = OrderedListStatement->StorageWidth / sizeof (UINT8);
      for (Index = 0; Index < Count; Index++) {
        DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%d ", Value8[Index]));
      }

      break;
  }

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "\n"));
}

/**
  Convert HII value to the string in HII ordered list opcode. It's caller's
  responsibility to free returned buffer using FreePool().

  @param[in]  HiiStatement  HII Statement private instance
  @param[out] ReturnSize    The size of returned array

  @retval EFI_STRING_ID     The string ID array for options in ordered list.

**/
EFI_STRING_ID *
HiiValueToOrderedListOptionStringId (
  IN  HII_STATEMENT  *HiiStatement,
  OUT UINTN          *ReturnSize
  )
{
  LIST_ENTRY     *Link;
  UINTN          OptionCount;
  EFI_STRING_ID  *ReturnedArray;
  UINTN          Index;
  UINT64         Value;

  if ((HiiStatement == NULL) || (ReturnSize == NULL)) {
    return NULL;
  }

  *ReturnSize = 0;

  if (HiiStatement->Operand != EFI_IFR_ORDERED_LIST_OP) {
    return NULL;
  }

  if (IsListEmpty (&HiiStatement->OptionListHead)) {
    return NULL;
  }

  DEBUG_CODE (
    DumpOrderedListValue (HiiStatement);
    );

  OptionCount = 0;
  Link        = GetFirstNode (&HiiStatement->OptionListHead);
  while (!IsNull (&HiiStatement->OptionListHead, Link)) {
    ++OptionCount;
    Link = GetNextNode (&HiiStatement->OptionListHead, Link);
  }

  *ReturnSize   = OptionCount;
  ReturnedArray = AllocatePool (sizeof (EFI_STRING_ID) * OptionCount);
  if (ReturnedArray == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: out of resource\n", __func__));
    *ReturnSize = 0;
    return NULL;
  }

  for (Index = 0; Index < OptionCount; Index++) {
    Value                = OrderedListGetArrayData (HiiStatement->Value.Buffer, HiiStatement->Value.BufferValueType, Index);
    ReturnedArray[Index] = OrderedListOptionValueToStringId (HiiStatement, Value);
  }

  return ReturnedArray;
}

/**
  Convert HII string to the value in HII ordered list opcode.

  @param[in]  Statement     Statement private instance
  @param[in]  Schema        Schema string
  @param[in]  HiiString     Input string
  @param[out] Value         Value returned

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
HiiStringToOrderedListOptionValue (
  IN  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *Statement,
  IN  CHAR8                                      *Schema,
  IN  EFI_STRING                                 HiiString,
  OUT UINT64                                     *Value
  )
{
  LIST_ENTRY           *Link;
  HII_QUESTION_OPTION  *Option;
  EFI_STRING           TmpString;
  BOOLEAN              Found;

  if ((Statement == NULL) || IS_EMPTY_STRING (HiiString) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Value = 0;

  if (Statement->HiiStatement->Operand != EFI_IFR_ORDERED_LIST_OP) {
    return EFI_UNSUPPORTED;
  }

  if (IsListEmpty (&Statement->HiiStatement->OptionListHead)) {
    return EFI_NOT_FOUND;
  }

  Found = FALSE;
  Link  = GetFirstNode (&Statement->HiiStatement->OptionListHead);
  while (!IsNull (&Statement->HiiStatement->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    TmpString = HiiGetRedfishString (Statement->ParentForm->ParentFormset->HiiHandle, Schema, Option->Text);
    if (TmpString == NULL) {
      TmpString = HiiGetRedfishString (Statement->ParentForm->ParentFormset->HiiHandle, ENGLISH_LANGUAGE_CODE, Option->Text);
    }

    if (TmpString != NULL) {
      if (StrCmp (TmpString, HiiString) == 0) {
        *Value = ExtendHiiValueToU64 (&Option->Value);
        Found  = TRUE;
      }

      FreePool (TmpString);
    }

    if (Found) {
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Statement->HiiStatement->OptionListHead, Link);
  }

  return EFI_NOT_FOUND;
}

/**
  Convert HII value to Redfish value.

  @param[in]  HiiHandle     HII handle.
  @param[in]  FullSchema    Schema string.
  @param[in]  HiiStatement  HII statement.
  @param[in]  Value         Value to be converted.
  @param[out] RedfishValue  Value in Redfish format.

  @retval EFI_SUCCESS       Redfish value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
HiiValueToRedfishValue (
  IN  EFI_HII_HANDLE       HiiHandle,
  IN  CHAR8                *FullSchema,
  IN  HII_STATEMENT        *HiiStatement,
  IN  HII_STATEMENT_VALUE  *Value,
  OUT EDKII_REDFISH_VALUE  *RedfishValue
  )
{
  EFI_STATUS     Status;
  EFI_STRING_ID  StringId;
  UINTN          Index;
  UINTN          Count;
  EFI_STRING_ID  *StringIdArray;
  CHAR8          NullChar;

  if ((HiiHandle == NULL) || (HiiStatement == NULL) || (Value == NULL) || (RedfishValue == NULL) || IS_EMPTY_STRING (FullSchema)) {
    return EFI_INVALID_PARAMETER;
  }

  StringIdArray = NULL;
  Count         = 0;
  Status        = EFI_SUCCESS;
  NullChar      = '\0';

  switch (HiiStatement->Operand) {
    case EFI_IFR_ONE_OF_OP:
      StringId = HiiValueToOneOfOptionStringId (HiiStatement, Value);
      if (StringId == 0) {
        //
        // Print prompt string of HII statement for ease of debugging
        //
        DumpHiiStatementPrompt (DEBUG_ERROR, HiiHandle, HiiStatement, "Can not find string ID");
        DumpHiiStatementValue (DEBUG_ERROR, Value, "Current value");
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        break;
      }

      RedfishValue->Value.Buffer = HiiGetRedfishAsciiString (HiiHandle, FullSchema, StringId);
      if (RedfishValue->Value.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      RedfishValue->Type = RedfishValueTypeString;
      break;
    case EFI_IFR_STRING_OP:
      if (Value->Type != EFI_IFR_TYPE_STRING) {
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        break;
      }

      if (Value->Buffer == NULL) {
        RedfishValue->Value.Buffer = AllocateCopyPool (sizeof (NullChar), &NullChar);
      } else {
        RedfishValue->Value.Buffer = StrToAsciiStr ((EFI_STRING)Value->Buffer);
      }

      if (RedfishValue->Value.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      RedfishValue->Type = RedfishValueTypeString;
      break;
    case EFI_IFR_CHECKBOX_OP:
      //
      // There is case where HII driver defines UINT8 for checked-box opcode storage.
      // IFR compiler will assign EFI_IFR_TYPE_NUM_SIZE_8 to its value type instead of
      // EFI_IFR_TYPE_BOOLEAN. We do a patch here and use boolean value type for this
      // case.
      //
      if (Value->Type != EFI_IFR_TYPE_BOOLEAN) {
        Value->Type = EFI_IFR_TYPE_BOOLEAN;
      }

    case EFI_IFR_NUMERIC_OP:
      Status = HiiValueToRedfishNumeric (Value, RedfishValue);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: failed to convert HII value to Redfish value: %r\n", __func__, Status));
        break;
      }

      break;
    case EFI_IFR_ACTION_OP:
      if (Value->Type != EFI_IFR_TYPE_ACTION) {
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        break;
      }

      //
      // Action has no value. Just return unknown type.
      //
      RedfishValue->Type = RedfishValueTypeUnknown;
      break;
    case EFI_IFR_ORDERED_LIST_OP:
      StringIdArray = HiiValueToOrderedListOptionStringId (HiiStatement, &Count);
      if (StringIdArray == NULL) {
        //
        // Print prompt string of HII statement for ease of debugging
        //
        DumpHiiStatementPrompt (DEBUG_ERROR, HiiHandle, HiiStatement, "Can not get string ID array");
        ASSERT (FALSE);
        Status = EFI_DEVICE_ERROR;
        break;
      }

      RedfishValue->Value.StringArray = AllocatePool (sizeof (CHAR8 *) * Count);
      if (RedfishValue->Value.StringArray == NULL) {
        //
        // Print prompt string of HII statement for ease of debugging
        //
        DumpHiiStatementPrompt (DEBUG_ERROR, HiiHandle, HiiStatement, "Can not allocate memory");
        ASSERT (FALSE);
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      for (Index = 0; Index < Count; Index++) {
        if (StringIdArray[Index] == 0) {
          //
          // Print prompt string of HII statement for ease of debugging
          //
          DumpHiiStatementPrompt (DEBUG_ERROR, HiiHandle, HiiStatement, "String ID in array is 0");
          ASSERT (FALSE);
        }

        RedfishValue->Value.StringArray[Index] = HiiGetRedfishAsciiString (HiiHandle, FullSchema, StringIdArray[Index]);
        ASSERT (RedfishValue->Value.StringArray[Index] != NULL);
      }

      RedfishValue->ArrayCount = Count;
      RedfishValue->Type       = RedfishValueTypeStringArray;

      FreePool (StringIdArray);
      break;
    case EFI_IFR_TEXT_OP:
      //
      // Use text two as the value
      //
      if (HiiStatement->ExtraData.TextTwo == 0x00) {
        Status = EFI_NOT_FOUND;
        break;
      }

      RedfishValue->Value.Buffer = HiiGetRedfishAsciiString (HiiHandle, FullSchema, HiiStatement->ExtraData.TextTwo);
      if (RedfishValue->Value.Buffer == NULL) {
        //
        // No x-UEFI-redfish string defined. Try to get string in English.
        //
        RedfishValue->Value.Buffer = HiiGetEnglishAsciiString (HiiHandle, HiiStatement->ExtraData.TextTwo);
      }

      if (RedfishValue->Value.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      RedfishValue->Type = RedfishValueTypeString;
      break;
    default:
      DEBUG ((DEBUG_ERROR, "%a: catch unsupported type: 0x%x! Please contact with author if we need to support this type.\n", __func__, HiiStatement->Operand));
      ASSERT (FALSE);
      Status = EFI_UNSUPPORTED;
      break;
  }

  return Status;
}

/**
  Convert input ascii string to unicode string. It's caller's
  responsibility to free returned buffer using FreePool().

  @param[in]  AsciiString     Ascii string to be converted.

  @retval CHAR16 *            Unicode string on return.

**/
EFI_STRING
StrToUnicodeStr (
  IN  CHAR8  *AsciiString
  )
{
  UINTN       StringLen;
  EFI_STRING  Buffer;
  EFI_STATUS  Status;

  if (AsciiString == NULL) {
    return NULL;
  }

  StringLen = AsciiStrLen (AsciiString) + 1;
  Buffer    = AllocatePool (StringLen * sizeof (CHAR16));
  if (Buffer == NULL) {
    return NULL;
  }

  Status = AsciiStrToUnicodeStrS (AsciiString, Buffer, StringLen);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return NULL;
  }

  return Buffer;
}

/**
  Convert input unicode string to ascii string. It's caller's
  responsibility to free returned buffer using FreePool().

  @param[in]  UnicodeString     Unicode string to be converted.

  @retval CHAR8 *               Ascii string on return.

**/
CHAR8 *
StrToAsciiStr (
  IN  EFI_STRING  UnicodeString
  )
{
  UINTN       StringLen;
  CHAR8       *Buffer;
  EFI_STATUS  Status;

  if (UnicodeString == NULL) {
    return NULL;
  }

  StringLen = HiiStrLen (UnicodeString) + 1;
  Buffer    = AllocatePool (StringLen * sizeof (CHAR8));
  if (Buffer == NULL) {
    return NULL;
  }

  Status = UnicodeStrToAsciiStrS (UnicodeString, Buffer, StringLen);
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return NULL;
  }

  return Buffer;
}

/**
  Return the full Redfish schema string from the given Schema and Version.

  Returned schema string is: Schema + '.' + Version

  @param[in]  Schema      Schema string
  @param[in]  Version     Schema version string

  @retval CHAR8 *         Schema string. NULL when errors occur.

**/
CHAR8 *
GetFullSchemaString (
  IN CHAR8  *Schema,
  IN CHAR8  *Version
  )
{
  UINTN  Size;
  CHAR8  *FullName;

  if (IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version)) {
    return NULL;
  }

  Size = AsciiStrSize (CONFIGURE_LANGUAGE_PREFIX) + AsciiStrSize (Schema) + AsciiStrSize (Version);

  FullName = AllocatePool (Size);
  if (FullName == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: out-of-resource\n", __func__));
    return NULL;
  }

  AsciiSPrint (FullName, Size, "%a%a.%a", CONFIGURE_LANGUAGE_PREFIX, Schema, Version);

  return FullName;
}

/**
  Common implementation to get statement private instance.

  @param[in]   RedfishPlatformConfigPrivate   Private instance.
  @param[in]   Schema                         Redfish schema string.
  @param[in]   ConfigureLang                  Configure language that refers to this statement.
  @param[out]  Statement                      Statement instance

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishPlatformConfigGetStatementCommon (
  IN     REDFISH_PLATFORM_CONFIG_PRIVATE            *RedfishPlatformConfigPrivate,
  IN     CHAR8                                      *Schema,
  IN     EFI_STRING                                 ConfigureLang,
  OUT    REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  **Statement
  )
{
  EFI_STATUS                                 Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *TargetStatement;

  if ((RedfishPlatformConfigPrivate == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (ConfigureLang) || (Statement == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Statement = NULL;

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ProcessPendingList failure: %r\n", __func__, Status));
    return Status;
  }

  TargetStatement = GetStatementPrivateByConfigureLang (&RedfishPlatformConfigPrivate->FormsetList, Schema, ConfigureLang);
  if (TargetStatement == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No match HII statement is found by the given %s in schema %a\n", __func__, ConfigureLang, Schema));
    return EFI_NOT_FOUND;
  }

  //
  // Find current HII question value.
  //
  Status = GetQuestionValue (
             TargetStatement->ParentForm->ParentFormset->HiiFormSet,
             TargetStatement->ParentForm->HiiForm,
             TargetStatement->HiiStatement,
             GetSetValueWithBuffer
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to get question current value: %r\n", __func__, Status));
    return Status;
  }

  if (TargetStatement->HiiStatement->Value.Type == EFI_IFR_TYPE_UNDEFINED) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Return Value.
  //
  *Statement = TargetStatement;

  return EFI_SUCCESS;
}

/**
  Get Redfish value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetValue (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL  *This,
  IN     CHAR8                                   *Schema,
  IN     CHAR8                                   *Version,
  IN     EFI_STRING                              ConfigureLang,
  OUT    EDKII_REDFISH_VALUE                     *Value
  )
{
  EFI_STATUS                                 Status;
  REDFISH_PLATFORM_CONFIG_PRIVATE            *RedfishPlatformConfigPrivate;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *TargetStatement;
  CHAR8                                      *FullSchema;

  if ((This == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || IS_EMPTY_STRING (ConfigureLang) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  Value->Type                  = RedfishValueTypeUnknown;
  Value->ArrayCount            = 0;
  FullSchema                   = NULL;

  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = RedfishPlatformConfigGetStatementCommon (RedfishPlatformConfigPrivate, FullSchema, ConfigureLang, &TargetStatement);
  if (EFI_ERROR (Status)) {
    goto RELEASE_RESOURCE;
  }

  if (TargetStatement->Suppressed) {
    Status = EFI_ACCESS_DENIED;
    goto RELEASE_RESOURCE;
  }

  Status = HiiValueToRedfishValue (
             TargetStatement->ParentForm->ParentFormset->HiiHandle,
             FullSchema,
             TargetStatement->HiiStatement,
             &TargetStatement->HiiStatement->Value,
             Value
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: HiiValueToRedfishValue failed: %r\n", __func__, Status));
  }

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  return Status;
}

/**
  Function to save question value into HII database.

  @param[in]   HiiFormset       HII form-set instance
  @param[in]   HiiForm          HII form instance
  @param[in]   HiiStatement     HII statement that keeps new value.
  @param[in]   Value            New value to apply.

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishPlatformConfigSaveQuestionValue (
  IN  HII_FORMSET          *HiiFormset,
  IN  HII_FORM             *HiiForm,
  IN  HII_STATEMENT        *HiiStatement,
  IN  HII_STATEMENT_VALUE  *Value
  )
{
  EFI_STATUS  Status;

  if ((HiiFormset == NULL) || (HiiForm == NULL) || (HiiStatement == NULL) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = SetQuestionValue (
             HiiFormset,
             HiiForm,
             HiiStatement,
             Value
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to set question value: %r\n", __func__, Status));
    return Status;
  }

  Status = SubmitForm (HiiFormset, HiiForm);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to submit form: %r\n", __func__, Status));
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Common implementation to set statement private instance.

  @param[in]   RedfishPlatformConfigPrivate   Private instance.
  @param[in]   Schema                         Redfish schema string.
  @param[in]   ConfigureLang                  Configure language that refers to this statement.
  @param[in]   StatementValue                 Statement value.

  @retval EFI_SUCCESS       HII value is returned successfully.
  @retval Others            Errors occur

**/
EFI_STATUS
RedfishPlatformConfigSetStatementCommon (
  IN     REDFISH_PLATFORM_CONFIG_PRIVATE  *RedfishPlatformConfigPrivate,
  IN     CHAR8                            *Schema,
  IN     EFI_STRING                       ConfigureLang,
  IN     HII_STATEMENT_VALUE              *StatementValue
  )
{
  EFI_STATUS                                 Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *TargetStatement;
  EFI_STRING                                 TempBuffer;
  UINT8                                      *StringArray;
  UINTN                                      Index;
  UINT64                                     Value;
  CHAR8                                      **CharArray;

  if ((RedfishPlatformConfigPrivate == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (ConfigureLang) || (StatementValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TempBuffer  = NULL;
  StringArray = NULL;

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ProcessPendingList failure: %r\n", __func__, Status));
    return Status;
  }

  TargetStatement = GetStatementPrivateByConfigureLang (&RedfishPlatformConfigPrivate->FormsetList, Schema, ConfigureLang);
  if (TargetStatement == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: No match HII statement is found by the given %s in schema %a\n", __func__, ConfigureLang, Schema));
    return EFI_NOT_FOUND;
  }

  if (StatementValue->Type != TargetStatement->HiiStatement->Value.Type) {
    //
    // We treat one-of type as string in Redfish. But one-of statement is not
    // in string format from HII point of view. Do a patch here.
    //
    if ((TargetStatement->HiiStatement->Operand == EFI_IFR_ONE_OF_OP) && (StatementValue->Type == EFI_IFR_TYPE_STRING)) {
      //
      // Keep input buffer to TempBuffer because StatementValue will be
      // assigned in HiiStringToOneOfOptionValue().
      //
      TempBuffer                = (EFI_STRING)StatementValue->Buffer;
      StatementValue->Buffer    = NULL;
      StatementValue->BufferLen = 0;

      Status = HiiStringToOneOfOptionValue (TargetStatement, Schema, TempBuffer, StatementValue);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: failed to find option value by the given %s\n", __func__, TempBuffer));
        FreePool (TempBuffer);
        return EFI_NOT_FOUND;
      }

      FreePool (TempBuffer);
    } else if ((TargetStatement->HiiStatement->Operand == EFI_IFR_ORDERED_LIST_OP) && (StatementValue->Type == EFI_IFR_TYPE_STRING)) {
      //
      // We treat ordered list type as string in Redfish. But ordered list statement is not
      // in string format from HII point of view. Do a patch here.
      //
      StringArray = AllocateZeroPool (TargetStatement->HiiStatement->StorageWidth);
      if (StringArray == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Arrange new option order from input string array
      //
      CharArray = (CHAR8 **)StatementValue->Buffer;
      for (Index = 0; Index < StatementValue->BufferLen; Index++) {
        TempBuffer = StrToUnicodeStr (CharArray[Index]);
        if (TempBuffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        Status = HiiStringToOrderedListOptionValue (TargetStatement, Schema, TempBuffer, &Value);
        if (EFI_ERROR (Status)) {
          ASSERT (FALSE);
          continue;
        }

        FreePool (TempBuffer);
        OrderedListSetArrayData (StringArray, TargetStatement->HiiStatement->Value.BufferValueType, Index, Value);
      }

      StatementValue->Type            = EFI_IFR_TYPE_BUFFER;
      StatementValue->Buffer          = StringArray;
      StatementValue->BufferLen       = TargetStatement->HiiStatement->StorageWidth;
      StatementValue->BufferValueType = TargetStatement->HiiStatement->Value.BufferValueType;
    } else if ((TargetStatement->HiiStatement->Operand == EFI_IFR_NUMERIC_OP) && (StatementValue->Type == EFI_IFR_TYPE_NUM_SIZE_64)) {
      //
      // Redfish only has numeric value type and it does not care about the value size.
      // Do a patch here so we have proper value size applied.
      //
      StatementValue->Type = TargetStatement->HiiStatement->Value.Type;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: catch value type mismatch! input type: 0x%x but target value type: 0x%x\n", __func__, StatementValue->Type, TargetStatement->HiiStatement->Value.Type));
      ASSERT (FALSE);
    }
  }

  if ((TargetStatement->HiiStatement->Operand == EFI_IFR_STRING_OP) && (StatementValue->Type == EFI_IFR_TYPE_STRING)) {
    //
    // Create string ID for new string.
    //
    StatementValue->Value.string = HiiSetString (TargetStatement->ParentForm->ParentFormset->HiiHandle, 0x00, (EFI_STRING)StatementValue->Buffer, NULL);
    if (StatementValue->Value.string == 0) {
      DEBUG ((DEBUG_ERROR, "%a: can not create string id\n", __func__));
      return EFI_OUT_OF_RESOURCES;
    }
  }

  Status = RedfishPlatformConfigSaveQuestionValue (
             TargetStatement->ParentForm->ParentFormset->HiiFormSet,
             TargetStatement->ParentForm->HiiForm,
             TargetStatement->HiiStatement,
             StatementValue
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to save question value: %r\n", __func__, Status));
  }

  if ((TargetStatement->HiiStatement->Operand == EFI_IFR_STRING_OP) && (StatementValue->Type == EFI_IFR_TYPE_STRING)) {
    if (StatementValue->Value.string != 0) {
      // Delete HII string which was created for HII statement operand = EFI_IFR_STRING_OP and Type = EFI_IFR_TYPE_STRING.
      HiiDeleteString (StatementValue->Value.string, TargetStatement->ParentForm->ParentFormset->HiiHandle);
    }
  }

  return Status;
}

/**
  Set Redfish value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   Value               The value to set.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolSetValue (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL  *This,
  IN     CHAR8                                   *Schema,
  IN     CHAR8                                   *Version,
  IN     EFI_STRING                              ConfigureLang,
  IN     EDKII_REDFISH_VALUE                     Value
  )
{
  EFI_STATUS                       Status;
  REDFISH_PLATFORM_CONFIG_PRIVATE  *RedfishPlatformConfigPrivate;
  CHAR8                            *FullSchema;
  HII_STATEMENT_VALUE              NewValue;

  if ((This == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || IS_EMPTY_STRING (ConfigureLang)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Value.Type == RedfishValueTypeUnknown) || (Value.Type >= RedfishValueTypeMax)) {
    return EFI_INVALID_PARAMETER;
  }

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  FullSchema                   = NULL;

  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (&NewValue, sizeof (HII_STATEMENT_VALUE));

  switch (Value.Type) {
    case RedfishValueTypeInteger:
    case RedfishValueTypeBoolean:
      Status = RedfishNumericToHiiValue (&Value, &NewValue);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: failed to convert Redfish value to Hii value: %r\n", __func__, Status));
        goto RELEASE_RESOURCE;
      }

      break;
    case RedfishValueTypeString:
      if (Value.Value.Buffer == NULL) {
        Status = EFI_INVALID_PARAMETER;
        goto RELEASE_RESOURCE;
      }

      NewValue.Type      = EFI_IFR_TYPE_STRING;
      NewValue.BufferLen = (UINT16)(AsciiStrSize (Value.Value.Buffer) * sizeof (CHAR16));
      NewValue.Buffer    = (UINT8 *)StrToUnicodeStr (Value.Value.Buffer);
      if (NewValue.Buffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto RELEASE_RESOURCE;
      }

      break;
    case RedfishValueTypeStringArray:
      NewValue.Type      = EFI_IFR_TYPE_STRING;
      NewValue.BufferLen = (UINT16)Value.ArrayCount;
      NewValue.Buffer    = (UINT8 *)Value.Value.StringArray;
      break;
    default:
      ASSERT (FALSE);
      break;
  }

  Status = RedfishPlatformConfigSetStatementCommon (RedfishPlatformConfigPrivate, FullSchema, ConfigureLang, &NewValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to set value to statement: %r\n", __func__, Status));
  }

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  if ((Value.Type == RedfishValueTypeString) && (NewValue.Buffer != NULL)) {
    FreePool (NewValue.Buffer);
  }

  return Status;
}

/**
  Get the list of Configure Language from platform configuration by the given Schema and RegexPattern.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   RegexPattern        The target Configure Language pattern. This is used for regular expression matching.
  @param[out]  ConfigureLangList   The list of Configure Language.
  @param[out]  Count               The number of Configure Language in ConfigureLangList.

  @retval EFI_SUCCESS              ConfigureLangList is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetConfigureLang (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL  *This,
  IN     CHAR8                                   *Schema,
  IN     CHAR8                                   *Version,
  IN     EFI_STRING                              RegexPattern,
  OUT    EFI_STRING                              **ConfigureLangList,
  OUT    UINTN                                   *Count
  )
{
  REDFISH_PLATFORM_CONFIG_PRIVATE                 *RedfishPlatformConfigPrivate;
  EFI_STATUS                                      Status;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_LIST  StatementList;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE_REF   *StatementRef;
  LIST_ENTRY                                      *NextLink;
  EFI_STRING                                      *TmpConfigureLangList;
  UINTN                                           Index;
  CHAR8                                           *FullSchema;

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: Harvest config language of %a_%a (Regex: %s).\n", __func__, Schema, Version, RegexPattern));

  if ((This == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || (Count == NULL) || (ConfigureLangList == NULL) || IS_EMPTY_STRING (RegexPattern)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&StatementList, sizeof (StatementList));
  *Count                       = 0;
  *ConfigureLangList           = NULL;
  FullSchema                   = NULL;
  TmpConfigureLangList         = NULL;
  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ProcessPendingList failure: %r\n", __func__, Status));
    return Status;
  }

  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = GetStatementPrivateByConfigureLangRegex (
             RedfishPlatformConfigPrivate->RegularExpressionProtocol,
             &RedfishPlatformConfigPrivate->FormsetList,
             FullSchema,
             RegexPattern,
             &StatementList
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: GetStatementPrivateByConfigureLangRegex failure: %r\n", __func__, Status));
    goto RELEASE_RESOURCE;
  }

  if (!IsListEmpty (&StatementList.StatementList)) {
    TmpConfigureLangList = AllocateZeroPool (sizeof (CHAR16 *) * StatementList.Count);
    if (TmpConfigureLangList == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto RELEASE_RESOURCE;
    }

    Index    = 0;
    NextLink = GetFirstNode (&StatementList.StatementList);
    while (!IsNull (&StatementList.StatementList, NextLink)) {
      StatementRef = REDFISH_PLATFORM_CONFIG_STATEMENT_REF_FROM_LINK (NextLink);
      NextLink     = GetNextNode (&StatementList.StatementList, NextLink);

      ASSERT (StatementRef->Statement->Description != 0);
      if (StatementRef->Statement->Description != 0) {
        ASSERT (StatementRef->Statement->XuefiRedfishStr != NULL);
        TmpConfigureLangList[Index] = AllocateCopyPool (HiiStrSize (StatementRef->Statement->XuefiRedfishStr), (VOID *)StatementRef->Statement->XuefiRedfishStr);
        ++Index;
      }
    }
  }

  *Count             = StatementList.Count;
  *ConfigureLangList = TmpConfigureLangList;

  DEBUG_REDFISH_THIS_MODULE (
    REDFISH_PLATFORM_CONFIG_DEBUG_CONFIG_LANG_REGEX,
    "%a: Number of configure language strings harvested: %d\n",
    __func__,
    StatementList.Count
    );

  DEBUG_REDFISH_THIS_MODULE_CODE (
    REDFISH_PLATFORM_CONFIG_DEBUG_CONFIG_LANG_REGEX,
    DEBUG_REDFISH (DEBUG_REDFISH_COMPONENT_PLATFORM_CONFIG_DXE, "%a: Number of configure language strings harvested: %d\n", __func__, StatementList.Count);
    for (Index = 0; Index < *Count; Index++) {
    DEBUG_REDFISH (DEBUG_REDFISH_COMPONENT_PLATFORM_CONFIG_DXE, "   (%d) %s\n", Index, TmpConfigureLangList[Index]);
  }

    );

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  if (StatementList.Count > 0) {
    ReleaseStatementList (&StatementList);
  }

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: exit.\n", __func__));
  return Status;
}

/**
  Get the list of supported Redfish schema from platform configuration.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[out]  SupportedSchema     The supported schema list which is separated by ';'.
                                   For example: "x-UEFI-redfish-Memory.v1_7_1;x-UEFI-redfish-Boot.v1_0_1"
                                   The SupportedSchema is allocated by the callee. It's caller's
                                   responsibility to free this buffer using FreePool().

  @retval EFI_SUCCESS              Schema is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetSupportedSchema (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL  *This,
  OUT    CHAR8                                   **SupportedSchema
  )
{
  REDFISH_PLATFORM_CONFIG_PRIVATE           *RedfishPlatformConfigPrivate;
  EFI_STATUS                                Status;
  LIST_ENTRY                                *HiiFormsetLink;
  LIST_ENTRY                                *HiiFormsetNextLink;
  REDFISH_PLATFORM_CONFIG_FORM_SET_PRIVATE  *HiiFormsetPrivate;
  UINTN                                     Index;
  UINTN                                     StringSize;
  CHAR8                                     *StringBuffer;
  UINTN                                     StringIndex;

  if ((This == NULL) || (SupportedSchema == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *SupportedSchema = NULL;

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);

  Status = ProcessPendingList (&RedfishPlatformConfigPrivate->FormsetList, &RedfishPlatformConfigPrivate->PendingList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: ProcessPendingList failure: %r\n", __func__, Status));
    return Status;
  }

  if (IsListEmpty (&RedfishPlatformConfigPrivate->FormsetList)) {
    return EFI_NOT_FOUND;
  }

  //
  // Calculate for string buffer size.
  //
  StringSize     = 0;
  HiiFormsetLink = GetFirstNode (&RedfishPlatformConfigPrivate->FormsetList);
  while (!IsNull (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    if (HiiFormsetPrivate->SupportedSchema.Count > 0) {
      for (Index = 0; Index < HiiFormsetPrivate->SupportedSchema.Count; Index++) {
        StringSize += AsciiStrSize (HiiFormsetPrivate->SupportedSchema.SchemaList[Index]);
      }
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  if (StringSize == 0) {
    return EFI_NOT_FOUND;
  }

  StringBuffer = AllocatePool (StringSize);
  if (StringBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  StringIndex    = 0;
  HiiFormsetLink = GetFirstNode (&RedfishPlatformConfigPrivate->FormsetList);
  while (!IsNull (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink)) {
    HiiFormsetNextLink = GetNextNode (&RedfishPlatformConfigPrivate->FormsetList, HiiFormsetLink);
    HiiFormsetPrivate  = REDFISH_PLATFORM_CONFIG_FORMSET_FROM_LINK (HiiFormsetLink);

    if (HiiFormsetPrivate->SupportedSchema.Count > 0) {
      for (Index = 0; Index < HiiFormsetPrivate->SupportedSchema.Count; Index++) {
        AsciiStrCpyS (&StringBuffer[StringIndex], (StringSize - StringIndex), HiiFormsetPrivate->SupportedSchema.SchemaList[Index]);
        StringIndex              += AsciiStrLen (HiiFormsetPrivate->SupportedSchema.SchemaList[Index]);
        StringBuffer[StringIndex] = ';';
        ++StringIndex;
      }
    }

    HiiFormsetLink = HiiFormsetNextLink;
  }

  StringBuffer[--StringIndex] = '\0';

  *SupportedSchema = StringBuffer;

  return EFI_SUCCESS;
}

/**
  Get Redfish default value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[in]   DefaultClass        The UEFI defined default class.
                                   Please refer to UEFI spec. 33.2.5.8 "defaults" for details.
  @param[out]  Value               The returned value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetDefaultValue (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL  *This,
  IN     CHAR8                                   *Schema,
  IN     CHAR8                                   *Version,
  IN     EFI_STRING                              ConfigureLang,
  IN     UINT16                                  DefaultClass,
  OUT    EDKII_REDFISH_VALUE                     *Value
  )
{
  EFI_STATUS                                 Status;
  REDFISH_PLATFORM_CONFIG_PRIVATE            *RedfishPlatformConfigPrivate;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *TargetStatement;
  CHAR8                                      *FullSchema;
  HII_STATEMENT_VALUE                        DefaultValue;

  if ((This == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || IS_EMPTY_STRING (ConfigureLang) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  ZeroMem (&DefaultValue, sizeof (HII_STATEMENT_VALUE));
  ZeroMem (Value, sizeof (EDKII_REDFISH_VALUE));

  FullSchema = NULL;
  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = RedfishPlatformConfigGetStatementCommon (RedfishPlatformConfigPrivate, FullSchema, ConfigureLang, &TargetStatement);
  if (EFI_ERROR (Status)) {
    goto RELEASE_RESOURCE;
  }

  if (TargetStatement->Suppressed) {
    Status = EFI_ACCESS_DENIED;
    goto RELEASE_RESOURCE;
  }

  Status = GetQuestionDefault (TargetStatement->ParentForm->ParentFormset->HiiFormSet, TargetStatement->ParentForm->HiiForm, TargetStatement->HiiStatement, DefaultClass, &DefaultValue);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: GetQuestionDefault failed: %r\n", __func__, Status));
    goto RELEASE_RESOURCE;
  }

  Status = HiiValueToRedfishValue (
             TargetStatement->ParentForm->ParentFormset->HiiHandle,
             FullSchema,
             TargetStatement->HiiStatement,
             &DefaultValue,
             Value
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: HiiValueToRedfishValue failed: %r\n", __func__, Status));
  }

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  return Status;
}

/**
  Get Redfish attribute value with the given Schema and Configure Language.

  @param[in]   This                Pointer to EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL instance.
  @param[in]   Schema              The Redfish schema to query.
  @param[in]   Version             The Redfish version to query.
  @param[in]   ConfigureLang       The target value which match this configure Language.
  @param[out]  AttributeValue      The attribute value.

  @retval EFI_SUCCESS              Value is returned successfully.
  @retval Others                   Some error happened.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigProtocolGetAttribute (
  IN     EDKII_REDFISH_PLATFORM_CONFIG_PROTOCOL  *This,
  IN     CHAR8                                   *Schema,
  IN     CHAR8                                   *Version,
  IN     EFI_STRING                              ConfigureLang,
  OUT    EDKII_REDFISH_ATTRIBUTE                 *AttributeValue
  )
{
  EFI_STATUS                                 Status;
  REDFISH_PLATFORM_CONFIG_PRIVATE            *RedfishPlatformConfigPrivate;
  REDFISH_PLATFORM_CONFIG_STATEMENT_PRIVATE  *TargetStatement;
  CHAR8                                      *FullSchema;
  CHAR8                                      *Buffer;

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: Entry\n", __func__));
  if ((This == NULL) || IS_EMPTY_STRING (Schema) || IS_EMPTY_STRING (Version) || IS_EMPTY_STRING (ConfigureLang) || (AttributeValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  RedfishPlatformConfigPrivate = REDFISH_PLATFORM_CONFIG_PRIVATE_FROM_THIS (This);
  ZeroMem (AttributeValue, sizeof (EDKII_REDFISH_ATTRIBUTE));
  FullSchema = NULL;
  FullSchema = GetFullSchemaString (Schema, Version);
  if (FullSchema == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = RedfishPlatformConfigGetStatementCommon (RedfishPlatformConfigPrivate, FullSchema, ConfigureLang, &TargetStatement);
  if (EFI_ERROR (Status)) {
    goto RELEASE_RESOURCE;
  }

  if (TargetStatement->Description != 0) {
    AttributeValue->AttributeName = HiiGetRedfishAsciiString (TargetStatement->ParentForm->ParentFormset->HiiHandle, FullSchema, TargetStatement->Description);
    Buffer                        = GetAttributeNameFromConfigLanguage (AttributeValue->AttributeName);
    if (Buffer != NULL) {
      FreePool (AttributeValue->AttributeName);
      AttributeValue->AttributeName = Buffer;
    }

    AttributeValue->DisplayName = HiiGetEnglishAsciiString (TargetStatement->ParentForm->ParentFormset->HiiHandle, TargetStatement->Description);
  }

  if (TargetStatement->Help != 0) {
    AttributeValue->HelpText = HiiGetEnglishAsciiString (TargetStatement->ParentForm->ParentFormset->HiiHandle, TargetStatement->Help);
  }

  AttributeValue->ReadOnly      = ((TargetStatement->Flags & EFI_IFR_FLAG_READ_ONLY) == 0 ? FALSE : TRUE);
  AttributeValue->ResetRequired = ((TargetStatement->Flags & EFI_IFR_FLAG_RESET_REQUIRED) == 0 ? FALSE : TRUE);
  AttributeValue->Type          = HiiStatementToAttributeType (TargetStatement->HiiStatement);
  AttributeValue->Suppress      = TargetStatement->Suppressed;
  AttributeValue->GrayedOut     = TargetStatement->GrayedOut;

  //
  // Build up menu path
  //
  if (RedfishPlatformConfigFeatureProp (REDFISH_PLATFORM_CONFIG_BUILD_MENU_PATH)) {
    AttributeValue->MenuPath = BuildMenuPath (TargetStatement);
    if (AttributeValue->MenuPath == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: failed to build menu path for \"%a\"\n", __func__, AttributeValue->AttributeName));
    }
  }

  //
  // Deal with maximum and minimum
  //
  if (AttributeValue->Type == RedfishAttributeTypeString) {
    AttributeValue->StrMaxSize = TargetStatement->StatementData.StrMaxSize;
    AttributeValue->StrMinSize = TargetStatement->StatementData.StrMinSize;
  } else if (AttributeValue->Type == RedfishAttributeTypeInteger) {
    AttributeValue->NumMaximum = TargetStatement->StatementData.NumMaximum;
    AttributeValue->NumMinimum = TargetStatement->StatementData.NumMinimum;
    AttributeValue->NumStep    = TargetStatement->StatementData.NumStep;
  }

  //
  // Provide value array if this is enumeration type.
  //
  if (TargetStatement->HiiStatement->Operand == EFI_IFR_ONE_OF_OP) {
    Status = OneOfStatementToAttributeValues (TargetStatement->ParentForm->ParentFormset->HiiHandle, FullSchema, TargetStatement, &AttributeValue->Values);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to convert one-of options to attribute values: %r\n", __func__, Status));
    }
  }

RELEASE_RESOURCE:

  if (FullSchema != NULL) {
    FreePool (FullSchema);
  }

  DEBUG ((DEBUG_REDFISH_PLATFORM_CONFIG, "%a: Exit\n", __func__));
  return Status;
}

/**
  Functions which are registered to receive notification of
  database events have this prototype. The actual event is encoded
  in NotifyType. The following table describes how PackageType,
  PackageGuid, Handle, and Package are used for each of the
  notification types.

  @param[in] PackageType  Package type of the notification.
  @param[in] PackageGuid  If PackageType is
                          EFI_HII_PACKAGE_TYPE_GUID, then this is
                          the pointer to the GUID from the Guid
                          field of EFI_HII_PACKAGE_GUID_HEADER.
                          Otherwise, it must be NULL.
  @param[in] Package      Points to the package referred to by the
                          notification Handle The handle of the package
                          list which contains the specified package.
  @param[in] Handle       The HII handle.
  @param[in] NotifyType   The type of change concerning the
                          database. See
                          EFI_HII_DATABASE_NOTIFY_TYPE.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigFormUpdateNotify (
  IN UINT8                         PackageType,
  IN CONST EFI_GUID                *PackageGuid,
  IN CONST EFI_HII_PACKAGE_HEADER  *Package,
  IN EFI_HII_HANDLE                Handle,
  IN EFI_HII_DATABASE_NOTIFY_TYPE  NotifyType
  )
{
  EFI_STATUS  Status;

  if ((NotifyType == EFI_HII_DATABASE_NOTIFY_NEW_PACK) || (NotifyType == EFI_HII_DATABASE_NOTIFY_ADD_PACK)) {
    //
    // HII formset on this handle is updated by driver during run-time. The formset needs to be reloaded.
    //
    Status = NotifyFormsetUpdate (Handle, &mRedfishPlatformConfigPrivate->PendingList);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to notify updated formset of HII handle: 0x%x\n", __func__, Handle));
      return Status;
    }
  } else if (NotifyType == EFI_HII_DATABASE_NOTIFY_REMOVE_PACK) {
    //
    // HII resource is removed. The formset is no longer exist.
    //
    Status = NotifyFormsetDeleted (Handle, &mRedfishPlatformConfigPrivate->PendingList);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: failed to notify deleted formset of HII handle: 0x%x\n", __func__, Handle));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/**
  This is a EFI_HII_STRING_PROTOCOL notification event handler.

  Install HII package notification.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
HiiStringProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate HII database protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiStringProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigPrivate->HiiString
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: locate EFI_HII_STRING_PROTOCOL failure: %r\n", __func__, Status));
    return;
  }

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent = NULL;
}

/**
  This is a EFI_HII_DATABASE_PROTOCOL notification event handler.

  Install HII package notification.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
HiiDatabaseProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate HII database protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigPrivate->HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: locate EFI_HII_DATABASE_PROTOCOL failure: %r\n", __func__, Status));
    return;
  }

  //
  // Register package notification when new form package is installed.
  //
  Status = mRedfishPlatformConfigPrivate->HiiDatabase->RegisterPackageNotify (
                                                         mRedfishPlatformConfigPrivate->HiiDatabase,
                                                         EFI_HII_PACKAGE_FORMS,
                                                         NULL,
                                                         RedfishPlatformConfigFormUpdateNotify,
                                                         EFI_HII_DATABASE_NOTIFY_NEW_PACK,
                                                         &mRedfishPlatformConfigPrivate->NotifyHandle
                                                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: RegisterPackageNotify for EFI_HII_DATABASE_NOTIFY_NEW_PACK failure: %r\n", __func__, Status));
  }

  //
  // Register package notification when new form package is updated.
  //
  Status = mRedfishPlatformConfigPrivate->HiiDatabase->RegisterPackageNotify (
                                                         mRedfishPlatformConfigPrivate->HiiDatabase,
                                                         EFI_HII_PACKAGE_FORMS,
                                                         NULL,
                                                         RedfishPlatformConfigFormUpdateNotify,
                                                         EFI_HII_DATABASE_NOTIFY_ADD_PACK,
                                                         &mRedfishPlatformConfigPrivate->NotifyHandle
                                                         );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: RegisterPackageNotify for EFI_HII_DATABASE_NOTIFY_NEW_PACK failure: %r\n", __func__, Status));
  }

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent = NULL;
}

/**
  This is a EFI_REGULAR_EXPRESSION_PROTOCOL notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
RegexProtocolInstalled (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS  Status;

  //
  // Locate regular expression protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiRegularExpressionProtocolGuid,
                  NULL,
                  (VOID **)&mRedfishPlatformConfigPrivate->RegularExpressionProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: locate EFI_REGULAR_EXPRESSION_PROTOCOL failure: %r\n", __func__, Status));
    return;
  }

  gBS->CloseEvent (Event);
  mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent = NULL;
}

/**
  Unloads an image.

  @param  ImageHandle           Handle that identifies the image to be unloaded.

  @retval EFI_SUCCESS           The image has been unloaded.
  @retval EFI_INVALID_PARAMETER ImageHandle is not a valid image handle.

**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigDxeUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS  Status;

  if (mRedfishPlatformConfigPrivate != NULL) {
    Status = gBS->UninstallProtocolInterface (
                    mRedfishPlatformConfigPrivate->ImageHandle,
                    &gEdkIIRedfishPlatformConfigProtocolGuid,
                    (VOID *)&mRedfishPlatformConfigPrivate->Protocol
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: can not uninstall gEdkIIRedfishPlatformConfigProtocolGuid: %r\n", __func__, Status));
      ASSERT (FALSE);
    }

    //
    // Close events
    //
    if (mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent != NULL) {
      gBS->CloseEvent (mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent);
    }

    if (mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent != NULL) {
      gBS->CloseEvent (mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent);
    }

    if (mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent != NULL) {
      gBS->CloseEvent (mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent);
    }

    //
    // Unregister package notification.
    //
    if (mRedfishPlatformConfigPrivate->NotifyHandle != NULL) {
      mRedfishPlatformConfigPrivate->HiiDatabase->UnregisterPackageNotify (
                                                    mRedfishPlatformConfigPrivate->HiiDatabase,
                                                    mRedfishPlatformConfigPrivate->NotifyHandle
                                                    );
    }

    ReleaseFormsetList (&mRedfishPlatformConfigPrivate->FormsetList);
    FreePool (mRedfishPlatformConfigPrivate);
    mRedfishPlatformConfigPrivate = NULL;
  }

  return EFI_SUCCESS;
}

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param  ImageHandle           The firmware allocated handle for the UEFI image.
  @param  SystemTable           A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RedfishPlatformConfigDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  mRedfishPlatformConfigPrivate = (REDFISH_PLATFORM_CONFIG_PRIVATE *)AllocateZeroPool (sizeof (REDFISH_PLATFORM_CONFIG_PRIVATE));
  if (mRedfishPlatformConfigPrivate == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: can not allocate pool for REDFISH_PLATFORM_CONFIG_PRIVATE\n", __func__));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Protocol initialization
  //
  mRedfishPlatformConfigPrivate->ImageHandle                 = ImageHandle;
  mRedfishPlatformConfigPrivate->Protocol.Revision           = REDFISH_PLATFORM_CONFIG_VERSION;
  mRedfishPlatformConfigPrivate->Protocol.GetValue           = RedfishPlatformConfigProtocolGetValue;
  mRedfishPlatformConfigPrivate->Protocol.SetValue           = RedfishPlatformConfigProtocolSetValue;
  mRedfishPlatformConfigPrivate->Protocol.GetConfigureLang   = RedfishPlatformConfigProtocolGetConfigureLang;
  mRedfishPlatformConfigPrivate->Protocol.GetSupportedSchema = RedfishPlatformConfigProtocolGetSupportedSchema;
  mRedfishPlatformConfigPrivate->Protocol.GetAttribute       = RedfishPlatformConfigProtocolGetAttribute;
  mRedfishPlatformConfigPrivate->Protocol.GetDefaultValue    = RedfishPlatformConfigProtocolGetDefaultValue;

  InitializeListHead (&mRedfishPlatformConfigPrivate->FormsetList);
  InitializeListHead (&mRedfishPlatformConfigPrivate->PendingList);

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEdkIIRedfishPlatformConfigProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  (VOID *)&mRedfishPlatformConfigPrivate->Protocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: can not install gEdkIIRedfishPlatformConfigProtocolGuid: %r\n", __func__, Status));
    ASSERT (FALSE);
  }

  //
  // Install protocol notification if HII database protocol is installed.
  //
  mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                               &gEfiHiiDatabaseProtocolGuid,
                                                               TPL_CALLBACK,
                                                               HiiDatabaseProtocolInstalled,
                                                               NULL,
                                                               &mRedfishPlatformConfigPrivate->HiiDbNotify.Registration
                                                               );
  if (mRedfishPlatformConfigPrivate->HiiDbNotify.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create protocol notification for gEfiHiiDatabaseProtocolGuid\n", __func__));
    ASSERT (FALSE);
  }

  //
  // Install protocol notification if HII string protocol is installed.
  //
  mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                                   &gEfiHiiStringProtocolGuid,
                                                                   TPL_CALLBACK,
                                                                   HiiStringProtocolInstalled,
                                                                   NULL,
                                                                   &mRedfishPlatformConfigPrivate->HiiStringNotify.Registration
                                                                   );
  if (mRedfishPlatformConfigPrivate->HiiStringNotify.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create protocol notification for gEfiHiiStringProtocolGuid\n", __func__));
    ASSERT (FALSE);
  }

  //
  // Install protocol notification if regular expression protocol is installed.
  //
  mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent = EfiCreateProtocolNotifyEvent (
                                                               &gEfiRegularExpressionProtocolGuid,
                                                               TPL_CALLBACK,
                                                               RegexProtocolInstalled,
                                                               NULL,
                                                               &mRedfishPlatformConfigPrivate->RegexNotify.Registration
                                                               );
  if (mRedfishPlatformConfigPrivate->RegexNotify.ProtocolEvent == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: failed to create protocol notification for gEfiRegularExpressionProtocolGuid\n", __func__));
    ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}
