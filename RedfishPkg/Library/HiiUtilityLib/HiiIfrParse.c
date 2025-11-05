/** @file
  The implementation of HII IFR parser.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HiiInternal.h"

/**
  Initialize Statement header members.

  @param[in]      OpCodeData             Pointer of the raw OpCode data.
  @param[in,out]  FormSet                Pointer of the current FormSet.
  @param[in,out]  Form                   Pointer of the current Form.

  @return The Statement.

**/
HII_STATEMENT *
CreateStatement (
  IN     UINT8        *OpCodeData,
  IN OUT HII_FORMSET  *FormSet,
  IN OUT HII_FORM     *Form
  )
{
  HII_STATEMENT             *Statement;
  EFI_IFR_STATEMENT_HEADER  *StatementHdr;
  INTN                      ConditionalExprCount;

  if (Form == NULL) {
    //
    // Only guid op may out side the form level.
    //
    if (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode != EFI_IFR_GUID_OP) {
      return NULL;
    }
  }

  Statement = (HII_STATEMENT *)AllocateZeroPool (sizeof (HII_STATEMENT));
  if (Statement == NULL) {
    return NULL;
  }

  InitializeListHead (&Statement->DefaultListHead);
  InitializeListHead (&Statement->OptionListHead);
  InitializeListHead (&Statement->InconsistentListHead);
  InitializeListHead (&Statement->NoSubmitListHead);
  InitializeListHead (&Statement->WarningListHead);

  Statement->Signature               = HII_STATEMENT_SIGNATURE;
  Statement->Operand                 = ((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode;
  Statement->OpCode                  = (EFI_IFR_OP_HEADER *)OpCodeData;
  Statement->QuestionReferToBitField = FALSE;

  StatementHdr = (EFI_IFR_STATEMENT_HEADER *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->Prompt, &StatementHdr->Prompt, sizeof (EFI_STRING_ID));
  CopyMem (&Statement->Help, &StatementHdr->Help, sizeof (EFI_STRING_ID));

  ConditionalExprCount = GetConditionalExpressionCount (ExpressStatement);
  if (ConditionalExprCount > 0) {
    //
    // Form is inside of suppressif
    //
    Statement->ExpressionList = (HII_EXPRESSION_LIST *)AllocatePool (
                                                         (UINTN)(sizeof (HII_EXPRESSION_LIST) + ((ConditionalExprCount - 1) * sizeof (HII_EXPRESSION *)))
                                                         );
    if (Statement->ExpressionList == NULL) {
      return NULL;
    }

    Statement->ExpressionList->Count     = (UINTN)ConditionalExprCount;
    Statement->ExpressionList->Signature = HII_EXPRESSION_LIST_SIGNATURE;
    CopyMem (
      Statement->ExpressionList->Expression,
      GetConditionalExpressionList (ExpressStatement),
      (UINTN)(sizeof (HII_EXPRESSION *) * ConditionalExprCount)
      );
  }

  //
  // Insert this Statement into current Form
  //
  if (Form == NULL) {
    InsertTailList (&FormSet->StatementListOSF, &Statement->Link);
  } else {
    InsertTailList (&Form->StatementListHead, &Statement->Link);
  }

  return Statement;
}

/**
  Initialize Question's members.

  @param[in]      OpCodeData             Pointer of the raw OpCode data.
  @param[in,out]  FormSet                Pointer of the current FormSet.
  @param[in,out]  Form                   Pointer of the current Form.

  @return The Question.

**/
HII_STATEMENT *
CreateQuestion (
  IN     UINT8        *OpCodeData,
  IN OUT HII_FORMSET  *FormSet,
  IN OUT HII_FORM     *Form
  )
{
  HII_STATEMENT            *Statement;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;
  LIST_ENTRY               *Link;
  HII_FORMSET_STORAGE      *Storage;
  HII_NAME_VALUE_NODE      *NameValueNode;
  BOOLEAN                  Find;

  Statement = CreateStatement (OpCodeData, FormSet, Form);
  if (Statement == NULL) {
    return NULL;
  }

  QuestionHdr = (EFI_IFR_QUESTION_HEADER *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->QuestionId, &QuestionHdr->QuestionId, sizeof (EFI_QUESTION_ID));
  CopyMem (&Statement->VarStoreId, &QuestionHdr->VarStoreId, sizeof (EFI_VARSTORE_ID));
  CopyMem (&Statement->VarStoreInfo.VarOffset, &QuestionHdr->VarStoreInfo.VarOffset, sizeof (UINT16));

  Statement->QuestionFlags = QuestionHdr->Flags;

  if (Statement->VarStoreId == 0) {
    //
    // VarStoreId of zero indicates no variable storage
    //
    return Statement;
  }

  //
  // Find Storage for this Question
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    Storage = HII_STORAGE_FROM_LINK (Link);

    if (Statement->VarStoreId == Storage->VarStoreId) {
      Statement->Storage = Storage;
      break;
    }

    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }

  if (Statement->Storage == NULL) {
    return NULL;
  }

  //
  // Initialize varname for Name/Value or EFI Variable
  //
  if ((Statement->Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) ||
      (Statement->Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE))
  {
    Statement->VariableName = GetTokenString (Statement->VarStoreInfo.VarName, FormSet->HiiHandle);
    if (Statement->VariableName == NULL) {
      return NULL;
    }

    if (Statement->Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) {
      //
      // Check whether old string node already exist.
      //
      Find = FALSE;
      if (!IsListEmpty (&Statement->Storage->NameValueList)) {
        Link = GetFirstNode (&Statement->Storage->NameValueList);
        while (!IsNull (&Statement->Storage->NameValueList, Link)) {
          NameValueNode = HII_NAME_VALUE_NODE_FROM_LINK (Link);

          if (StrCmp (Statement->VariableName, NameValueNode->Name) == 0) {
            Find = TRUE;
            break;
          }

          Link = GetNextNode (&Statement->Storage->NameValueList, Link);
        }
      }

      if (!Find) {
        //
        // Insert to Name/Value varstore list
        //
        NameValueNode = AllocateZeroPool (sizeof (HII_NAME_VALUE_NODE));
        if (NameValueNode == NULL) {
          return NULL;
        }

        NameValueNode->Signature = HII_NAME_VALUE_NODE_SIGNATURE;
        NameValueNode->Name      = AllocateCopyPool (StrSize (Statement->VariableName), Statement->VariableName);
        if (NameValueNode->Name == NULL) {
          FreePool (NameValueNode);
          return NULL;
        }

        NameValueNode->Value = AllocateZeroPool (0x10);
        if (NameValueNode->Value == NULL) {
          FreePool (NameValueNode->Name);
          FreePool (NameValueNode);
          return NULL;
        }

        InsertTailList (&Statement->Storage->NameValueList, &NameValueNode->Link);
      }
    }
  }

  return Statement;
}

/**
  Allocate a HII_EXPRESSION node.

  @param[in,out]  Form                   The Form associated with this Expression
  @param[in]      OpCode                 The binary opcode data.

  @return Pointer to a HII_EXPRESSION data structure.

**/
HII_EXPRESSION *
CreateExpression (
  IN OUT HII_FORM  *Form,
  IN     UINT8     *OpCode
  )
{
  HII_EXPRESSION  *Expression;

  Expression = AllocateZeroPool (sizeof (HII_EXPRESSION));
  if (Expression == NULL) {
    return NULL;
  }

  Expression->Signature = HII_EXPRESSION_SIGNATURE;
  InitializeListHead (&Expression->OpCodeListHead);
  Expression->OpCode = (EFI_IFR_OP_HEADER *)OpCode;

  return Expression;
}

/**
  Create ConfigHdr string for a storage.

  @param[in]      FormSet                Pointer of the current FormSet
  @param[in,out]  Storage                Pointer of the storage

  @retval EFI_SUCCESS            Initialize ConfigHdr success

**/
EFI_STATUS
InitializeConfigHdr (
  IN     HII_FORMSET          *FormSet,
  IN OUT HII_FORMSET_STORAGE  *Storage
  )
{
  CHAR16  *Name;

  if ((Storage->Type == EFI_HII_VARSTORE_BUFFER) ||
      (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER))
  {
    Name = Storage->Name;
  } else {
    Name = NULL;
  }

  Storage->ConfigHdr = HiiConstructConfigHdr (
                         &Storage->Guid,
                         Name,
                         FormSet->DriverHandle
                         );

  if (Storage->ConfigHdr == NULL) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Convert Ascii string to Unicode.

  This is an internal function.

  @param[in]  AsciiString             The Ascii string to be converted.
  @param[out] UnicodeString           The Unicode string retrieved.

**/
VOID
AsciiToUnicode (
  IN  CHAR8   *AsciiString,
  OUT CHAR16  *UnicodeString
  )
{
  UINT8  Index;

  Index = 0;
  while (AsciiString[Index] != 0) {
    UnicodeString[Index] = (CHAR16)AsciiString[Index];
    Index++;
  }

  UnicodeString[Index] = L'\0';
}

/**
  Allocate a HII_FORMSET_STORAGE data structure and insert to FormSet Storage List.

  @param[in]  FormSet                    Pointer of the current FormSet
  @param[in]  StorageType                Storage type.
  @param[in]  OpCodeData                 Binary data for this opcode.

  @return Pointer to a HII_FORMSET_STORAGE data structure.

**/
HII_FORMSET_STORAGE *
CreateStorage (
  IN HII_FORMSET  *FormSet,
  IN UINT8        StorageType,
  IN UINT8        *OpCodeData
  )
{
  HII_FORMSET_STORAGE  *Storage;
  CHAR8                *AsciiStorageName;

  AsciiStorageName = NULL;

  Storage = AllocateZeroPool (sizeof (HII_FORMSET_STORAGE));
  if (Storage == NULL) {
    return NULL;
  }

  Storage->Signature = HII_STORAGE_SIGNATURE;
  Storage->Type      = StorageType;

  switch (StorageType) {
    case EFI_HII_VARSTORE_BUFFER:

      CopyMem (&Storage->Guid, &((EFI_IFR_VARSTORE *)OpCodeData)->Guid, sizeof (EFI_GUID));
      CopyMem (&Storage->Size, &((EFI_IFR_VARSTORE *)OpCodeData)->Size, sizeof (UINT16));

      Storage->Buffer = AllocateZeroPool (Storage->Size);
      if (Storage->Buffer == NULL) {
        FreePool (Storage);
        return NULL;
      }

      AsciiStorageName = (CHAR8 *)((EFI_IFR_VARSTORE *)OpCodeData)->Name;
      Storage->Name    = AllocatePool (sizeof (CHAR16) * (AsciiStrLen (AsciiStorageName) + 1));
      if (Storage->Name == NULL) {
        FreePool (Storage);
        return NULL;
      }

      AsciiToUnicode (AsciiStorageName, Storage->Name);

      break;

    case EFI_HII_VARSTORE_EFI_VARIABLE:
    case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:
      CopyMem (&Storage->Guid, &((EFI_IFR_VARSTORE_EFI *)OpCodeData)->Guid, sizeof (EFI_GUID));
      CopyMem (&Storage->Attributes, &((EFI_IFR_VARSTORE_EFI *)OpCodeData)->Attributes, sizeof (UINT32));
      CopyMem (&Storage->Size, &((EFI_IFR_VARSTORE_EFI *)OpCodeData)->Size, sizeof (UINT16));

      if (StorageType ==  EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER) {
        Storage->Buffer = AllocateZeroPool (Storage->Size);
        if (Storage->Buffer == NULL) {
          FreePool (Storage);
          return NULL;
        }
      }

      AsciiStorageName = (CHAR8 *)((EFI_IFR_VARSTORE_EFI *)OpCodeData)->Name;
      Storage->Name    = AllocatePool (sizeof (CHAR16) * (AsciiStrLen (AsciiStorageName) + 1));
      if (Storage->Name == NULL) {
        FreePool (Storage);
        return NULL;
      }

      AsciiToUnicode (AsciiStorageName, Storage->Name);

      break;

    case EFI_HII_VARSTORE_NAME_VALUE:
      CopyMem (&Storage->Guid, &((EFI_IFR_VARSTORE_NAME_VALUE *)OpCodeData)->Guid, sizeof (EFI_GUID));
      InitializeListHead (&Storage->NameValueList);

      break;

    default:
      break;
  }

  InitializeConfigHdr (FormSet, Storage);
  InsertTailList (&FormSet->StorageListHead, &Storage->Link);

  return Storage;
}

/**
  Get formset storage based on the input varstoreid info.

  @param[in]  FormSet                Pointer of the current FormSet.
  @param[in]  VarStoreId             Varstore ID info.

  @return Pointer to a HII_FORMSET_STORAGE data structure.

**/
HII_FORMSET_STORAGE *
GetFstStgFromVarId (
  IN HII_FORMSET      *FormSet,
  IN EFI_VARSTORE_ID  VarStoreId
  )
{
  HII_FORMSET_STORAGE  *Storage;
  LIST_ENTRY           *Link;
  BOOLEAN              Found;

  Found   = FALSE;
  Storage = NULL;
  //
  // Find Formset Storage for this Question
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    Storage = HII_STORAGE_FROM_LINK (Link);

    if (Storage->VarStoreId == VarStoreId) {
      Found = TRUE;
      break;
    }

    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }

  return Found ? Storage : NULL;
}

/**
  Initialize Request Element of a Question. <RequestElement> ::= '&'<BlockName> | '&'<Label>

  @param[in,out]  FormSet                Pointer of the current FormSet.
  @param[in,out]  Question               The Question to be initialized.
  @param[in,out]  Form                   Pointer of the current form.

  @retval EFI_SUCCESS            Function success.
  @retval EFI_INVALID_PARAMETER  No storage associated with the Question.

**/
EFI_STATUS
InitializeRequestElement (
  IN OUT HII_FORMSET    *FormSet,
  IN OUT HII_STATEMENT  *Question,
  IN OUT HII_FORM       *Form
  )
{
  HII_FORMSET_STORAGE      *Storage;
  HII_FORM_CONFIG_REQUEST  *ConfigInfo;
  UINTN                    StrLen;
  UINTN                    StringSize;
  CHAR16                   *NewStr;
  CHAR16                   RequestElement[30];
  LIST_ENTRY               *Link;
  BOOLEAN                  Find;
  UINTN                    MaxLen;

  Storage = Question->Storage;
  if (Storage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
    //
    // <ConfigRequest> is unnecessary for EFI variable storage,
    // GetVariable()/SetVariable() will be used to retrieve/save values
    //
    return EFI_SUCCESS;
  }

  //
  // Prepare <RequestElement>
  //
  if ((Storage->Type == EFI_HII_VARSTORE_BUFFER) ||
      (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER))
  {
    StrLen = UnicodeSPrint (
               RequestElement,
               30 * sizeof (CHAR16),
               L"&OFFSET=%04x&WIDTH=%04x",
               Question->VarStoreInfo.VarOffset,
               Question->StorageWidth
               );
    HiiStringToLowercase (RequestElement);
    Question->BlockName = AllocateCopyPool ((StrLen + 1) * sizeof (CHAR16), RequestElement);
    if (Question->BlockName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    StrLen = UnicodeSPrint (RequestElement, 30 * sizeof (CHAR16), L"&%s", Question->VariableName);
  }

  if ((Question->Operand == EFI_IFR_PASSWORD_OP) &&
      ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) == EFI_IFR_FLAG_CALLBACK))
  {
    //
    // Password with CALLBACK flag is stored in encoded format,
    // so don't need to append it to <ConfigRequest>
    //
    return EFI_SUCCESS;
  }

  StringSize = (Storage->ConfigRequest != NULL) ? StrSize (Storage->ConfigRequest) : sizeof (CHAR16);
  MaxLen     = StringSize / sizeof (CHAR16) + Storage->SpareStrLen;

  //
  // Append <RequestElement> to <ConfigRequest>
  //
  if (StrLen > Storage->SpareStrLen) {
    //
    // Old String buffer is not sufficient for RequestElement, allocate a new one
    //
    MaxLen = StringSize / sizeof (CHAR16) + CONFIG_REQUEST_STRING_INCREMENTAL;
    NewStr = AllocatePool (MaxLen * sizeof (CHAR16));
    if (NewStr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (Storage->ConfigRequest != NULL) {
      CopyMem (NewStr, Storage->ConfigRequest, StringSize);
      FreePool (Storage->ConfigRequest);
    } else {
      NewStr[0] = L'\0';
    }

    Storage->ConfigRequest = NewStr;
    Storage->SpareStrLen   = CONFIG_REQUEST_STRING_INCREMENTAL;
  }

  StrCatS (Storage->ConfigRequest, MaxLen, RequestElement);
  Storage->ElementCount++;
  Storage->SpareStrLen -= StrLen;

  //
  // Update the Config Request info saved in the form.
  //
  ConfigInfo = NULL;
  Find       = FALSE;
  Link       = GetFirstNode (&Form->ConfigRequestHead);
  while (!IsNull (&Form->ConfigRequestHead, Link)) {
    ConfigInfo = HII_FORM_CONFIG_REQUEST_FROM_LINK (Link);
    if ((ConfigInfo != NULL) && (ConfigInfo->Storage == Storage)) {
      Find = TRUE;
      break;
    }

    Link = GetNextNode (&Form->ConfigRequestHead, Link);
  }

  if (!Find) {
    ConfigInfo = AllocateZeroPool (sizeof (HII_FORM_CONFIG_REQUEST));
    if (ConfigInfo == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    ConfigInfo->Signature     = HII_FORM_CONFIG_REQUEST_SIGNATURE;
    ConfigInfo->ConfigRequest = AllocateCopyPool (StrSize (Storage->ConfigHdr), Storage->ConfigHdr);
    if (ConfigInfo->ConfigRequest == NULL) {
      FreePool (ConfigInfo);
      return EFI_OUT_OF_RESOURCES;
    }

    ConfigInfo->SpareStrLen = 0;
    ConfigInfo->Storage     = Storage;
    InsertTailList (&Form->ConfigRequestHead, &ConfigInfo->Link);
  }

  StringSize = (ConfigInfo->ConfigRequest != NULL) ? StrSize (ConfigInfo->ConfigRequest) : sizeof (CHAR16);
  MaxLen     = StringSize / sizeof (CHAR16) + ConfigInfo->SpareStrLen;

  //
  // Append <RequestElement> to <ConfigRequest>
  //
  if (StrLen > ConfigInfo->SpareStrLen) {
    //
    // Old String buffer is not sufficient for RequestElement, allocate a new one
    //
    MaxLen = StringSize / sizeof (CHAR16) + CONFIG_REQUEST_STRING_INCREMENTAL;
    NewStr = AllocatePool (MaxLen * sizeof (CHAR16));
    if (NewStr == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    if (ConfigInfo->ConfigRequest != NULL) {
      CopyMem (NewStr, ConfigInfo->ConfigRequest, StringSize);
      FreePool (ConfigInfo->ConfigRequest);
    } else {
      NewStr[0] = L'\0';
    }

    ConfigInfo->ConfigRequest = NewStr;
    ConfigInfo->SpareStrLen   = CONFIG_REQUEST_STRING_INCREMENTAL;
  }

  StrCatS (ConfigInfo->ConfigRequest, MaxLen, RequestElement);
  ConfigInfo->ElementCount++;
  ConfigInfo->SpareStrLen -= StrLen;

  return EFI_SUCCESS;
}

/**
  Free resources of a Expression.

  @param[in]  FormSet                Pointer of the Expression

**/
VOID
DestroyExpression (
  IN HII_EXPRESSION  *Expression
  )
{
  LIST_ENTRY             *Link;
  HII_EXPRESSION_OPCODE  *OpCode;
  LIST_ENTRY             *SubExpressionLink;
  HII_EXPRESSION         *SubExpression;

  while (!IsListEmpty (&Expression->OpCodeListHead)) {
    Link   = GetFirstNode (&Expression->OpCodeListHead);
    OpCode = HII_EXPRESSION_OPCODE_FROM_LINK (Link);

    RemoveEntryList (&OpCode->Link);
    if ((OpCode->Operand == EFI_IFR_EQ_ID_VAL_LIST_OP) && (OpCode->ExtraData.EqIdListData.ValueList != NULL)) {
      FreePool (OpCode->ExtraData.EqIdListData.ValueList);
    }

    if (((OpCode->Operand == EFI_IFR_GET_OP) || (OpCode->Operand == EFI_IFR_SET_OP)) &&
        (OpCode->ExtraData.GetSetData.ValueName != NULL))
    {
      FreePool (OpCode->ExtraData.GetSetData.ValueName);
    }

    if (OpCode->MapExpressionList.ForwardLink != NULL) {
      while (!IsListEmpty (&OpCode->MapExpressionList)) {
        SubExpressionLink = GetFirstNode (&OpCode->MapExpressionList);
        SubExpression     = HII_EXPRESSION_FROM_LINK (SubExpressionLink);
        RemoveEntryList (&SubExpression->Link);
        DestroyExpression (SubExpression);
      }
    }
  }

  //
  // Free this Expression
  //
  FreePool (Expression);
}

/**
  Delete a string from HII Package List.

  @param[in]  StringId               Id of the string in HII database.
  @param[in]  HiiHandle              The HII package list handle.

  @retval EFI_SUCCESS            The string was deleted successfully.

**/
EFI_STATUS
DeleteString (
  IN  EFI_STRING_ID   StringId,
  IN  EFI_HII_HANDLE  HiiHandle
  )
{
  CHAR16  NullChar;

  NullChar = CHAR_NULL;
  HiiSetString (HiiHandle, StringId, &NullChar, NULL);
  return EFI_SUCCESS;
}

/**
  Free resources of a Statement.

  @param[in]      FormSet                Pointer of the FormSet
  @param[in,out]  Statement              Pointer of the Statement

**/
VOID
DestroyStatement (
  IN     HII_FORMSET    *FormSet,
  IN OUT HII_STATEMENT  *Statement
  )
{
  LIST_ENTRY            *Link;
  HII_QUESTION_DEFAULT  *Default;
  HII_QUESTION_OPTION   *Option;
  HII_EXPRESSION        *Expression;

  //
  // Free Default value List
  //
  while (!IsListEmpty (&Statement->DefaultListHead)) {
    Link    = GetFirstNode (&Statement->DefaultListHead);
    Default = HII_QUESTION_DEFAULT_FROM_LINK (Link);
    RemoveEntryList (&Default->Link);

    if (Default->Value.Buffer != NULL) {
      FreePool (Default->Value.Buffer);
    }

    FreePool (Default);
  }

  //
  // Free Options List
  //
  while (!IsListEmpty (&Statement->OptionListHead)) {
    Link   = GetFirstNode (&Statement->OptionListHead);
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    if (Option->SuppressExpression != NULL) {
      FreePool (Option->SuppressExpression);
    }

    RemoveEntryList (&Option->Link);

    FreePool (Option);
  }

  //
  // Free Inconsistent List
  //
  while (!IsListEmpty (&Statement->InconsistentListHead)) {
    Link       = GetFirstNode (&Statement->InconsistentListHead);
    Expression = HII_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free NoSubmit List
  //
  while (!IsListEmpty (&Statement->NoSubmitListHead)) {
    Link       = GetFirstNode (&Statement->NoSubmitListHead);
    Expression = HII_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free WarningIf List
  //
  while (!IsListEmpty (&Statement->WarningListHead)) {
    Link       = GetFirstNode (&Statement->WarningListHead);
    Expression = HII_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  if (Statement->ExpressionList != NULL) {
    FreePool (Statement->ExpressionList);
  }

  if (Statement->VariableName != NULL) {
    FreePool (Statement->VariableName);
  }

  if (Statement->BlockName != NULL) {
    FreePool (Statement->BlockName);
  }

  if (Statement->Value.Buffer != NULL) {
    FreePool (Statement->Value.Buffer);
  }

  if ((Statement->Operand == EFI_IFR_STRING_OP) || (Statement->Operand == EFI_IFR_PASSWORD_OP)) {
    DeleteString (Statement->Value.Value.string, FormSet->HiiHandle);
  }
}

/**
  Free resources of a Form.

  @param[in]      FormSet                Pointer of the FormSet
  @param[in,out]  Form                   Pointer of the Form.

**/
VOID
DestroyForm (
  IN     HII_FORMSET  *FormSet,
  IN OUT HII_FORM     *Form
  )
{
  LIST_ENTRY               *Link;
  HII_EXPRESSION           *Expression;
  HII_STATEMENT            *Statement;
  HII_FORM_CONFIG_REQUEST  *ConfigInfo;

  //
  // Free Rule Expressions
  //
  while (!IsListEmpty (&Form->RuleListHead)) {
    Link       = GetFirstNode (&Form->RuleListHead);
    Expression = HII_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);
    DestroyExpression (Expression);
  }

  //
  // Free Statements/Questions
  //
  while (!IsListEmpty (&Form->StatementListHead)) {
    Link      = GetFirstNode (&Form->StatementListHead);
    Statement = HII_STATEMENT_FROM_LINK (Link);
    RemoveEntryList (&Statement->Link);
    DestroyStatement (FormSet, Statement);
  }

  //
  // Free ConfigRequest string.
  //
  while (!IsListEmpty (&Form->ConfigRequestHead)) {
    Link       = GetFirstNode (&Form->ConfigRequestHead);
    ConfigInfo = HII_FORM_CONFIG_REQUEST_FROM_LINK (Link);

    RemoveEntryList (&ConfigInfo->Link);

    FreePool (ConfigInfo->ConfigRequest);
    FreePool (ConfigInfo);
  }

  if (Form->SuppressExpression != NULL) {
    FreePool (Form->SuppressExpression);
  }

  //
  // Free this Form
  //
  FreePool (Form);
}

/**
  Tell whether this Operand is an Expression OpCode or not

  @param[in]  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Expression OpCode.
  @retval FALSE                  Not an Expression OpCode.

**/
BOOLEAN
IsExpressionOpCode (
  IN UINT8  Operand
  )
{
  if (((Operand >= EFI_IFR_EQ_ID_VAL_OP) && (Operand <= EFI_IFR_NOT_OP)) ||
      ((Operand >= EFI_IFR_MATCH_OP) && (Operand <= EFI_IFR_SET_OP))  ||
      ((Operand >= EFI_IFR_EQUAL_OP) && (Operand <= EFI_IFR_SPAN_OP)) ||
      (Operand == EFI_IFR_CATENATE_OP) ||
      (Operand == EFI_IFR_TO_LOWER_OP) ||
      (Operand == EFI_IFR_TO_UPPER_OP) ||
      (Operand == EFI_IFR_MAP_OP)      ||
      (Operand == EFI_IFR_VERSION_OP)  ||
      (Operand == EFI_IFR_SECURITY_OP) ||
      (Operand == EFI_IFR_MATCH2_OP))
  {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Tell whether this Operand is an Statement OpCode.

  @param[in]  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Statement OpCode.
  @retval FALSE                  Not an Statement OpCode.

**/
BOOLEAN
IsStatementOpCode (
  IN UINT8  Operand
  )
{
  if ((Operand == EFI_IFR_SUBTITLE_OP) ||
      (Operand == EFI_IFR_TEXT_OP) ||
      (Operand == EFI_IFR_RESET_BUTTON_OP) ||
      (Operand == EFI_IFR_REF_OP) ||
      (Operand == EFI_IFR_ACTION_OP) ||
      (Operand == EFI_IFR_NUMERIC_OP) ||
      (Operand == EFI_IFR_ORDERED_LIST_OP) ||
      (Operand == EFI_IFR_CHECKBOX_OP) ||
      (Operand == EFI_IFR_STRING_OP) ||
      (Operand == EFI_IFR_PASSWORD_OP) ||
      (Operand == EFI_IFR_DATE_OP) ||
      (Operand == EFI_IFR_TIME_OP) ||
      (Operand == EFI_IFR_GUID_OP) ||
      (Operand == EFI_IFR_ONE_OF_OP))
  {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Tell whether this Operand is an known OpCode.

  @param[in]  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Statement OpCode.
  @retval FALSE                  Not an Statement OpCode.

**/
BOOLEAN
IsUnKnownOpCode (
  IN UINT8  Operand
  )
{
  return Operand > EFI_IFR_MATCH2_OP ? TRUE : FALSE;
}

/**
  Calculate number of Statements(Questions) and Expression OpCodes.

  @param[in]      FormSet                The FormSet to be counted.
  @param[in,out]  NumberOfStatement      Number of Statements(Questions)
  @param[in,out]  NumberOfExpression     Number of Expression OpCodes

**/
VOID
CountOpCodes (
  IN     HII_FORMSET  *FormSet,
  IN OUT UINT16       *NumberOfStatement,
  IN OUT UINT16       *NumberOfExpression
  )
{
  UINT16  StatementCount;
  UINT16  ExpressionCount;
  UINT8   *OpCodeData;
  UINTN   Offset;
  UINTN   OpCodeLen;

  Offset          = 0;
  StatementCount  = 0;
  ExpressionCount = 0;

  while (Offset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + Offset;
    OpCodeLen  = ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;
    Offset    += OpCodeLen;

    if (IsExpressionOpCode (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode)) {
      ExpressionCount++;
    } else {
      StatementCount++;
    }
  }

  *NumberOfStatement  = StatementCount;
  *NumberOfExpression = ExpressionCount;
}

/**
  Parse opcodes in the formset IFR binary.

  @param[in]  FormSet                Pointer of the FormSet data structure.

  @retval EFI_SUCCESS            Opcode parse success.
  @retval Other                  Opcode parse fail.

**/
EFI_STATUS
ParseOpCodes (
  IN HII_FORMSET  *FormSet
  )
{
  EFI_STATUS                Status;
  HII_FORM                  *CurrentForm;
  HII_STATEMENT             *CurrentStatement;
  HII_STATEMENT             *ParentStatement;
  HII_EXPRESSION_OPCODE     *ExpressionOpCode;
  HII_EXPRESSION            *CurrentExpression;
  UINT8                     Operand;
  UINT8                     Scope;
  UINTN                     OpCodeOffset;
  UINTN                     OpCodeLength;
  UINT8                     *OpCodeData;
  UINT8                     ScopeOpCode;
  HII_FORMSET_DEFAULTSTORE  *DefaultStore;
  HII_QUESTION_DEFAULT      *CurrentDefault;
  HII_QUESTION_OPTION       *CurrentOption;
  UINT8                     Width;
  UINT16                    NumberOfStatement;
  UINT16                    NumberOfExpression;
  EFI_IMAGE_ID              *ImageId;
  BOOLEAN                   SuppressForQuestion;
  BOOLEAN                   SuppressForOption;
  UINT16                    DepthOfDisable;
  BOOLEAN                   OpCodeDisabled;
  BOOLEAN                   SingleOpCodeExpression;
  BOOLEAN                   InScopeDefault;
  EFI_HII_VALUE             *ExpressionValue;
  HII_STATEMENT_VALUE       *StatementValue;
  EFI_IFR_FORM_MAP_METHOD   *MapMethod;
  UINT8                     MapScopeDepth;
  LIST_ENTRY                *Link;
  HII_FORMSET_STORAGE       *Storage;
  LIST_ENTRY                *MapExpressionList;
  EFI_VARSTORE_ID           TempVarstoreId;
  BOOLEAN                   InScopeDisable;
  INTN                      ConditionalExprCount;
  BOOLEAN                   InUnknownScope;
  UINT8                     UnknownDepth;
  HII_FORMSET_DEFAULTSTORE  *PreDefaultStore;
  LIST_ENTRY                *DefaultLink;
  BOOLEAN                   HaveInserted;
  UINT16                    TotalBits;
  BOOLEAN                   QuestionReferBitField;

  SuppressForQuestion    = FALSE;
  SuppressForOption      = FALSE;
  InScopeDisable         = FALSE;
  DepthOfDisable         = 0;
  OpCodeDisabled         = FALSE;
  SingleOpCodeExpression = FALSE;
  InScopeDefault         = FALSE;
  CurrentExpression      = NULL;
  CurrentDefault         = NULL;
  CurrentOption          = NULL;
  ImageId                = NULL;
  MapMethod              = NULL;
  MapScopeDepth          = 0;
  Link                   = NULL;
  MapExpressionList      = NULL;
  TempVarstoreId         = 0;
  ConditionalExprCount   = 0;
  InUnknownScope         = FALSE;
  UnknownDepth           = 0;
  QuestionReferBitField  = FALSE;

  //
  // Get the number of Statements and Expressions
  //
  CountOpCodes (FormSet, &NumberOfStatement, &NumberOfExpression);

  InitializeListHead (&FormSet->StatementListOSF);
  InitializeListHead (&FormSet->StorageListHead);
  InitializeListHead (&FormSet->DefaultStoreListHead);
  InitializeListHead (&FormSet->FormListHead);
  ResetCurrentExpressionStack ();
  ResetMapExpressionListStack ();

  CurrentForm      = NULL;
  CurrentStatement = NULL;
  ParentStatement  = NULL;

  ResetScopeStack ();

  OpCodeOffset = 0;
  while (OpCodeOffset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + OpCodeOffset;

    OpCodeLength  = ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;
    OpCodeOffset += OpCodeLength;
    Operand       = ((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode;
    Scope         = ((EFI_IFR_OP_HEADER *)OpCodeData)->Scope;
    if (InUnknownScope) {
      if (Operand == EFI_IFR_END_OP) {
        UnknownDepth--;

        if (UnknownDepth == 0) {
          InUnknownScope = FALSE;
        }
      } else {
        if (Scope != 0) {
          UnknownDepth++;
        }
      }

      continue;
    }

    if (IsUnKnownOpCode (Operand)) {
      if (Scope != 0) {
        InUnknownScope = TRUE;
        UnknownDepth++;
      }

      continue;
    }

    //
    // If scope bit set, push onto scope stack
    //
    if (Scope != 0) {
      PushScope (Operand);
    }

    if (OpCodeDisabled) {
      //
      // DisableIf Expression is evaluated to be TRUE, try to find its end.
      // Here only cares the EFI_IFR_DISABLE_IF and EFI_IFR_END
      //
      if (Operand == EFI_IFR_DISABLE_IF_OP) {
        DepthOfDisable++;
      } else if (Operand == EFI_IFR_END_OP) {
        Status = PopScope (&ScopeOpCode);
        if (EFI_ERROR (Status)) {
          return Status;
        }

        if (ScopeOpCode == EFI_IFR_DISABLE_IF_OP) {
          if (DepthOfDisable == 0) {
            InScopeDisable = FALSE;
            OpCodeDisabled = FALSE;
          } else {
            DepthOfDisable--;
          }
        }
      }

      continue;
    }

    if (IsExpressionOpCode (Operand)) {
      ExpressionOpCode = (HII_EXPRESSION_OPCODE *)AllocateZeroPool (sizeof (HII_EXPRESSION_OPCODE));
      if (ExpressionOpCode == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      ExpressionOpCode->Signature = HII_EXPRESSION_OPCODE_SIGNATURE;
      ExpressionOpCode->Operand   = Operand;

      switch (Operand) {
        case EFI_IFR_EQ_ID_VAL_OP:

          CopyMem (&ExpressionOpCode->ExtraData.EqIdValData.QuestionId, &((EFI_IFR_EQ_ID_VAL *)OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

          ExpressionValue       = &ExpressionOpCode->ExtraData.EqIdValData.Value;
          ExpressionValue->Type = EFI_IFR_TYPE_NUM_SIZE_16;
          CopyMem (&ExpressionValue->Value.u16, &((EFI_IFR_EQ_ID_VAL *)OpCodeData)->Value, sizeof (UINT16));
          break;

        case EFI_IFR_EQ_ID_ID_OP:
          CopyMem (&ExpressionOpCode->ExtraData.EqIdIdData.QuestionId1, &((EFI_IFR_EQ_ID_ID *)OpCodeData)->QuestionId1, sizeof (EFI_QUESTION_ID));
          CopyMem (&ExpressionOpCode->ExtraData.EqIdIdData.QuestionId2, &((EFI_IFR_EQ_ID_ID *)OpCodeData)->QuestionId2, sizeof (EFI_QUESTION_ID));
          break;

        case EFI_IFR_EQ_ID_VAL_LIST_OP:

          CopyMem (&ExpressionOpCode->ExtraData.EqIdListData.QuestionId, &((EFI_IFR_EQ_ID_VAL_LIST *)OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));
          CopyMem (&ExpressionOpCode->ExtraData.EqIdListData.ListLength, &((EFI_IFR_EQ_ID_VAL_LIST *)OpCodeData)->ListLength, sizeof (UINT16));
          ExpressionOpCode->ExtraData.EqIdListData.ValueList = AllocateCopyPool (
                                                                 ExpressionOpCode->ExtraData.EqIdListData.ListLength * sizeof (UINT16),
                                                                 &((EFI_IFR_EQ_ID_VAL_LIST *)OpCodeData)->ValueList
                                                                 );
          if (ExpressionOpCode->ExtraData.EqIdListData.ValueList == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

          break;

        case EFI_IFR_TO_STRING_OP:
        case EFI_IFR_FIND_OP:

          ExpressionOpCode->ExtraData.Format = ((EFI_IFR_TO_STRING *)OpCodeData)->Format;
          break;

        case EFI_IFR_STRING_REF1_OP:

          ExpressionValue       = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type = EFI_IFR_TYPE_STRING;
          CopyMem (&ExpressionValue->Value.string, &((EFI_IFR_STRING_REF1 *)OpCodeData)->StringId, sizeof (EFI_STRING_ID));
          break;

        case EFI_IFR_RULE_REF_OP:

          ExpressionOpCode->ExtraData.RuleId = ((EFI_IFR_RULE_REF *)OpCodeData)->RuleId;
          break;

        case EFI_IFR_SPAN_OP:

          ExpressionOpCode->ExtraData.Flags = ((EFI_IFR_SPAN *)OpCodeData)->Flags;
          break;

        case EFI_IFR_THIS_OP:

          if (ParentStatement != NULL) {
            ExpressionOpCode->ExtraData.QuestionRef1Data.QuestionId = ParentStatement->QuestionId;
          }

          break;

        case EFI_IFR_SECURITY_OP:

          CopyMem (&ExpressionOpCode->ExtraData.Guid, &((EFI_IFR_SECURITY *)OpCodeData)->Permissions, sizeof (EFI_GUID));
          break;

        case EFI_IFR_MATCH2_OP:

          CopyMem (&ExpressionOpCode->ExtraData.Guid, &((EFI_IFR_MATCH2 *)OpCodeData)->SyntaxType, sizeof (EFI_GUID));
          break;

        case EFI_IFR_GET_OP:
        case EFI_IFR_SET_OP:

          CopyMem (&TempVarstoreId, &((EFI_IFR_GET *)OpCodeData)->VarStoreId, sizeof (TempVarstoreId));
          if (TempVarstoreId != 0) {
            if (FormSet->StorageListHead.ForwardLink != NULL) {
              Link = GetFirstNode (&FormSet->StorageListHead);
              while (!IsNull (&FormSet->StorageListHead, Link)) {
                Storage = HII_STORAGE_FROM_LINK (Link);
                if (Storage->VarStoreId == ((EFI_IFR_GET *)OpCodeData)->VarStoreId) {
                  ExpressionOpCode->ExtraData.GetSetData.VarStorage = Storage;
                  break;
                }

                Link = GetNextNode (&FormSet->StorageListHead, Link);
              }
            }

            if (ExpressionOpCode->ExtraData.GetSetData.VarStorage == NULL) {
              //
              // VarStorage is not found.
              //
              return EFI_INVALID_PARAMETER;
            }
          }

          ExpressionOpCode->ExtraData.GetSetData.ValueType = ((EFI_IFR_GET *)OpCodeData)->VarStoreType;
          switch (ExpressionOpCode->ExtraData.GetSetData.ValueType) {
            case EFI_IFR_TYPE_BOOLEAN:
            case EFI_IFR_TYPE_NUM_SIZE_8:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = 1;
              break;

            case EFI_IFR_TYPE_NUM_SIZE_16:
            case EFI_IFR_TYPE_STRING:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = 2;
              break;

            case EFI_IFR_TYPE_NUM_SIZE_32:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = 4;
              break;

            case EFI_IFR_TYPE_NUM_SIZE_64:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = 8;
              break;

            case EFI_IFR_TYPE_DATE:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = (UINT8)sizeof (EFI_IFR_DATE);
              break;

            case EFI_IFR_TYPE_TIME:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = (UINT8)sizeof (EFI_IFR_TIME);
              break;

            case EFI_IFR_TYPE_REF:
              ExpressionOpCode->ExtraData.GetSetData.ValueWidth = (UINT8)sizeof (EFI_IFR_REF);
              break;

            case EFI_IFR_TYPE_OTHER:
            case EFI_IFR_TYPE_UNDEFINED:
            case EFI_IFR_TYPE_ACTION:
            case EFI_IFR_TYPE_BUFFER:
            default:
              //
              // Invalid value type for Get/Set opcode.
              //
              return EFI_INVALID_PARAMETER;
          }

          CopyMem (
            &ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarName,
            &((EFI_IFR_GET *)OpCodeData)->VarStoreInfo.VarName,
            sizeof (EFI_STRING_ID)
            );
          CopyMem (
            &ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset,
            &((EFI_IFR_GET *)OpCodeData)->VarStoreInfo.VarOffset,
            sizeof (UINT16)
            );
          if ((ExpressionOpCode->ExtraData.GetSetData.VarStorage != NULL) &&
              ((ExpressionOpCode->ExtraData.GetSetData.VarStorage->Type == EFI_HII_VARSTORE_NAME_VALUE) ||
               (ExpressionOpCode->ExtraData.GetSetData.VarStorage->Type == EFI_HII_VARSTORE_EFI_VARIABLE)))
          {
            ExpressionOpCode->ExtraData.GetSetData.ValueName = GetTokenString (ExpressionOpCode->ExtraData.GetSetData.VarStoreInfo.VarName, FormSet->HiiHandle);
            if (ExpressionOpCode->ExtraData.GetSetData.ValueName == NULL) {
              //
              // String ID is invalid.
              //
              return EFI_INVALID_PARAMETER;
            }
          }

          break;

        case EFI_IFR_QUESTION_REF1_OP:

          CopyMem (&ExpressionOpCode->ExtraData.QuestionRef1Data.QuestionId, &((EFI_IFR_EQ_ID_VAL_LIST *)OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));
          break;

        case EFI_IFR_QUESTION_REF3_OP:

          if (OpCodeLength >= sizeof (EFI_IFR_QUESTION_REF3_2)) {
            CopyMem (&ExpressionOpCode->ExtraData.QuestionRef3Data.DevicePath, &((EFI_IFR_QUESTION_REF3_2 *)OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));

            if (OpCodeLength >= sizeof (EFI_IFR_QUESTION_REF3_3)) {
              CopyMem (&ExpressionOpCode->ExtraData.QuestionRef3Data.Guid, &((EFI_IFR_QUESTION_REF3_3 *)OpCodeData)->Guid, sizeof (EFI_GUID));
            }
          }

          break;

        //
        // constant
        //
        case EFI_IFR_TRUE_OP:

          ExpressionValue          = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type    = EFI_IFR_TYPE_BOOLEAN;
          ExpressionValue->Value.b = TRUE;
          break;

        case EFI_IFR_FALSE_OP:

          ExpressionValue          = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type    = EFI_IFR_TYPE_BOOLEAN;
          ExpressionValue->Value.b = FALSE;
          break;

        case EFI_IFR_ONE_OP:

          ExpressionValue           = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
          ExpressionValue->Value.u8 = 1;
          break;

        case EFI_IFR_ZERO_OP:

          ExpressionValue           = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
          ExpressionValue->Value.u8 = 0;
          break;

        case EFI_IFR_ONES_OP:

          ExpressionValue            = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type      = EFI_IFR_TYPE_NUM_SIZE_64;
          ExpressionValue->Value.u64 = 0xffffffffffffffffULL;
          break;

        case EFI_IFR_UINT8_OP:

          ExpressionValue           = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
          ExpressionValue->Value.u8 = ((EFI_IFR_UINT8 *)OpCodeData)->Value;
          break;

        case EFI_IFR_UINT16_OP:

          ExpressionValue       = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type = EFI_IFR_TYPE_NUM_SIZE_16;
          CopyMem (&ExpressionValue->Value.u16, &((EFI_IFR_UINT16 *)OpCodeData)->Value, sizeof (UINT16));
          break;

        case EFI_IFR_UINT32_OP:

          ExpressionValue       = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type = EFI_IFR_TYPE_NUM_SIZE_32;
          CopyMem (&ExpressionValue->Value.u32, &((EFI_IFR_UINT32 *)OpCodeData)->Value, sizeof (UINT32));
          break;

        case EFI_IFR_UINT64_OP:

          ExpressionValue       = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type = EFI_IFR_TYPE_NUM_SIZE_64;
          CopyMem (&ExpressionValue->Value.u64, &((EFI_IFR_UINT64 *)OpCodeData)->Value, sizeof (UINT64));
          break;

        case EFI_IFR_UNDEFINED_OP:
          ExpressionValue       = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type = EFI_IFR_TYPE_UNDEFINED;
          break;

        case EFI_IFR_VERSION_OP:
          ExpressionValue            = &ExpressionOpCode->ExtraData.Value;
          ExpressionValue->Type      = EFI_IFR_TYPE_NUM_SIZE_16;
          ExpressionValue->Value.u16 = EFI_IFR_SPECIFICATION_VERSION;
          break;

        default:
          break;
      }

      //
      // Create sub expression nested in MAP opcode
      //
      if ((CurrentExpression == NULL) && (MapScopeDepth > 0)) {
        CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
        if (CurrentExpression == NULL) {
          continue;
        }

        InsertTailList (MapExpressionList, &CurrentExpression->Link);
        if (Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }
      }

      InsertTailList (&CurrentExpression->OpCodeListHead, &ExpressionOpCode->Link);
      if (Operand == EFI_IFR_MAP_OP) {
        //
        // Store current Map Expression List.
        //
        if (MapExpressionList != NULL) {
          PushMapExpressionList (MapExpressionList);
        }

        //
        // Initialize new Map Expression List.
        //
        MapExpressionList = &ExpressionOpCode->MapExpressionList;
        InitializeListHead (MapExpressionList);
        //
        // Store current expression.
        //
        PushCurrentExpression (CurrentExpression);
        CurrentExpression = NULL;
        MapScopeDepth++;
      } else if (SingleOpCodeExpression) {
        //
        // There are two cases to indicate the end of an Expression:
        // for single OpCode expression: one Expression OpCode
        // for expression consists of more than one OpCode: EFI_IFR_END
        //
        SingleOpCodeExpression = FALSE;

        if (InScopeDisable && (CurrentForm == NULL)) {
          //
          // This is DisableIf expression for Form, it should be a constant expression
          //
          Status = EvaluateHiiExpression (FormSet, CurrentForm, CurrentExpression);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          OpCodeDisabled = IsHiiValueTrue (&CurrentExpression->Result);
        }

        CurrentExpression = NULL;
      }

      continue;
    }

    //
    // Parse the Opcode
    //
    switch (Operand) {
      case EFI_IFR_FORM_SET_OP:
        //
        // Check the formset GUID
        //

        if (CompareMem (&FormSet->Guid, &((EFI_IFR_FORM_SET *)OpCodeData)->Guid, sizeof (EFI_GUID)) != 0) {
          return EFI_INVALID_PARAMETER;
        }

        CopyMem (&FormSet->FormSetTitle, &((EFI_IFR_FORM_SET *)OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
        CopyMem (&FormSet->Help, &((EFI_IFR_FORM_SET *)OpCodeData)->Help, sizeof (EFI_STRING_ID));

        if (OpCodeLength > OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
          //
          // The formset OpCode contains ClassGuid
          //
          FormSet->NumberOfClassGuid = (UINT8)(((EFI_IFR_FORM_SET *)OpCodeData)->Flags & 0x3);
          CopyMem (FormSet->ClassGuid, OpCodeData + sizeof (EFI_IFR_FORM_SET), FormSet->NumberOfClassGuid * sizeof (EFI_GUID));
        }

        break;

      case EFI_IFR_FORM_OP:

        //
        // Create a new Form for this FormSet
        //
        CurrentForm = AllocateZeroPool (sizeof (HII_FORM));
        if (CurrentForm == NULL) {
          break;
        }

        CurrentForm->Signature = HII_FORM_SIGNATURE;
        InitializeListHead (&CurrentForm->StatementListHead);
        InitializeListHead (&CurrentForm->ConfigRequestHead);
        InitializeListHead (&CurrentForm->RuleListHead);

        CurrentForm->FormType = STANDARD_MAP_FORM_TYPE;
        CopyMem (&CurrentForm->FormId, &((EFI_IFR_FORM *)OpCodeData)->FormId, sizeof (UINT16));
        CopyMem (&CurrentForm->FormTitle, &((EFI_IFR_FORM *)OpCodeData)->FormTitle, sizeof (EFI_STRING_ID));

        ConditionalExprCount = GetConditionalExpressionCount (ExpressForm);
        if ( ConditionalExprCount > 0) {
          //
          // Form is inside of suppressif
          //
          CurrentForm->SuppressExpression = (HII_EXPRESSION_LIST *)AllocatePool (
                                                                     (UINTN)(sizeof (HII_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof (HII_EXPRESSION *)))
                                                                     );
          if (CurrentForm->SuppressExpression == NULL) {
            break;
          }

          CurrentForm->SuppressExpression->Count     = (UINTN)ConditionalExprCount;
          CurrentForm->SuppressExpression->Signature = HII_EXPRESSION_LIST_SIGNATURE;
          CopyMem (CurrentForm->SuppressExpression->Expression, GetConditionalExpressionList (ExpressForm), (UINTN)(sizeof (HII_EXPRESSION *) * ConditionalExprCount));
        }

        if (Scope != 0) {
          //
          // Enter scope of a Form, suppressif will be used for Question or Option
          //
          SuppressForQuestion = TRUE;
        }

        //
        // Insert into Form list of this FormSet
        //
        InsertTailList (&FormSet->FormListHead, &CurrentForm->Link);
        break;

      case EFI_IFR_FORM_MAP_OP:

        //
        // Create a new Form for this FormSet
        //
        CurrentForm = AllocateZeroPool (sizeof (HII_FORM));
        if (CurrentForm == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        CurrentForm->Signature = HII_FORM_SIGNATURE;
        InitializeListHead (&CurrentForm->StatementListHead);
        InitializeListHead (&CurrentForm->ConfigRequestHead);
        InitializeListHead (&CurrentForm->RuleListHead);

        CopyMem (&CurrentForm->FormId, &((EFI_IFR_FORM *)OpCodeData)->FormId, sizeof (UINT16));

        MapMethod = (EFI_IFR_FORM_MAP_METHOD *)(OpCodeData + sizeof (EFI_IFR_FORM_MAP));
        //
        // FormMap Form must contain at least one Map Method.
        //
        if (((EFI_IFR_OP_HEADER *)OpCodeData)->Length < ((UINTN)(UINT8 *)(MapMethod + 1) - (UINTN)OpCodeData)) {
          return EFI_INVALID_PARAMETER;
        }

        //
        // Try to find the standard form map method.
        //
        while (((UINTN)(UINT8 *)MapMethod - (UINTN)OpCodeData) < ((EFI_IFR_OP_HEADER *)OpCodeData)->Length) {
          if (CompareGuid ((EFI_GUID *)(VOID *)&MapMethod->MethodIdentifier, &gEfiHiiStandardFormGuid)) {
            CopyMem (&CurrentForm->FormTitle, &MapMethod->MethodTitle, sizeof (EFI_STRING_ID));
            CurrentForm->FormType = STANDARD_MAP_FORM_TYPE;
            break;
          }

          MapMethod++;
        }

        //
        // If the standard form map method is not found, the first map method title will be used.
        //
        if (CurrentForm->FormTitle == 0) {
          MapMethod = (EFI_IFR_FORM_MAP_METHOD *)(OpCodeData + sizeof (EFI_IFR_FORM_MAP));
          CopyMem (&CurrentForm->FormTitle, &MapMethod->MethodTitle, sizeof (EFI_STRING_ID));
        }

        ConditionalExprCount = GetConditionalExpressionCount (ExpressForm);
        if ( ConditionalExprCount > 0) {
          //
          // Form is inside of suppressif
          //
          CurrentForm->SuppressExpression = (HII_EXPRESSION_LIST *)AllocateZeroPool (
                                                                     (UINTN)(sizeof (HII_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof (HII_EXPRESSION *)))
                                                                     );
          if (CurrentForm->SuppressExpression == NULL) {
            FreePool (CurrentForm);
            return EFI_OUT_OF_RESOURCES;
          }

          CurrentForm->SuppressExpression->Count     = (UINTN)ConditionalExprCount;
          CurrentForm->SuppressExpression->Signature = HII_EXPRESSION_LIST_SIGNATURE;
          CopyMem (CurrentForm->SuppressExpression->Expression, GetConditionalExpressionList (ExpressForm), (UINTN)(sizeof (HII_EXPRESSION *) * ConditionalExprCount));
        }

        if (Scope != 0) {
          //
          // Enter scope of a Form, suppressif will be used for Question or Option
          //
          SuppressForQuestion = TRUE;
        }

        //
        // Insert into Form list of this FormSet
        //
        InsertTailList (&FormSet->FormListHead, &CurrentForm->Link);
        break;

      //
      // Storage
      //
      case EFI_IFR_VARSTORE_OP:

        //
        // Create a buffer Storage for this FormSet
        //
        Storage = CreateStorage (FormSet, EFI_HII_VARSTORE_BUFFER, OpCodeData);
        CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE *)OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
        break;

      case EFI_IFR_VARSTORE_NAME_VALUE_OP:

        //
        // Create a name/value Storage for this FormSet
        //
        Storage = CreateStorage (FormSet, EFI_HII_VARSTORE_NAME_VALUE, OpCodeData);
        CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_NAME_VALUE *)OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
        break;

      case EFI_IFR_VARSTORE_EFI_OP:

        //
        // Create a EFI variable Storage for this FormSet
        //
        if (OpCodeLength < sizeof (EFI_IFR_VARSTORE_EFI)) {
          //
          // Create efi varstore with format follow UEFI spec before 2.3.1.
          //
          Storage = CreateStorage (FormSet, EFI_HII_VARSTORE_EFI_VARIABLE, OpCodeData);
        } else {
          //
          // Create efi varstore with format follow UEFI spec 2.3.1 and later.
          //
          Storage = CreateStorage (FormSet, EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER, OpCodeData);
        }

        CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_EFI *)OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
        break;

      //
      // DefaultStore
      //
      case EFI_IFR_DEFAULTSTORE_OP:

        HaveInserted = FALSE;
        DefaultStore = AllocateZeroPool (sizeof (HII_FORMSET_DEFAULTSTORE));
        if (DefaultStore == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        DefaultStore->Signature = HII_FORMSET_DEFAULTSTORE_SIGNATURE;

        CopyMem (&DefaultStore->DefaultId, &((EFI_IFR_DEFAULTSTORE *)OpCodeData)->DefaultId, sizeof (UINT16));
        CopyMem (&DefaultStore->DefaultName, &((EFI_IFR_DEFAULTSTORE *)OpCodeData)->DefaultName, sizeof (EFI_STRING_ID));
        //
        // Insert it to the DefaultStore list of this Formset with ascending order.
        //
        if (!IsListEmpty (&FormSet->DefaultStoreListHead)) {
          DefaultLink = GetFirstNode (&FormSet->DefaultStoreListHead);
          while (!IsNull (&FormSet->DefaultStoreListHead, DefaultLink)) {
            PreDefaultStore = HII_FORMSET_DEFAULTSTORE_FROM_LINK (DefaultLink);
            DefaultLink     = GetNextNode (&FormSet->DefaultStoreListHead, DefaultLink);
            if (DefaultStore->DefaultId < PreDefaultStore->DefaultId) {
              InsertTailList (&PreDefaultStore->Link, &DefaultStore->Link);
              HaveInserted = TRUE;
              break;
            }
          }
        }

        if (!HaveInserted) {
          InsertTailList (&FormSet->DefaultStoreListHead, &DefaultStore->Link);
        }

        break;

      //
      // Statements
      //
      case EFI_IFR_SUBTITLE_OP:

        CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
        break;

      case EFI_IFR_TEXT_OP:

        CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
        CopyMem (&CurrentStatement->ExtraData.TextTwo, &((EFI_IFR_TEXT *)OpCodeData)->TextTwo, sizeof (EFI_STRING_ID));
        break;

      case EFI_IFR_RESET_BUTTON_OP:

        CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
        CopyMem (&CurrentStatement->ExtraData.DefaultId, &((EFI_IFR_RESET_BUTTON *)OpCodeData)->DefaultId, sizeof (EFI_DEFAULT_ID));
        break;

      //
      // Questions
      //
      case EFI_IFR_ACTION_OP:

        CurrentStatement             = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        CurrentStatement->Value.Type = EFI_IFR_TYPE_ACTION;

        if (OpCodeLength == sizeof (EFI_IFR_ACTION_1)) {
          //
          // No QuestionConfig present, so no configuration string will be processed
          //
          CurrentStatement->ExtraData.QuestionConfig = 0;
        } else {
          CopyMem (&CurrentStatement->ExtraData.QuestionConfig, &((EFI_IFR_ACTION *)OpCodeData)->QuestionConfig, sizeof (EFI_STRING_ID));
        }

        break;

      case EFI_IFR_REF_OP:

        CurrentStatement     = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        StatementValue       = &CurrentStatement->Value;
        StatementValue->Type = EFI_IFR_TYPE_REF;
        if (OpCodeLength >= sizeof (EFI_IFR_REF)) {
          CopyMem (&StatementValue->Value.ref.FormId, &((EFI_IFR_REF *)OpCodeData)->FormId, sizeof (EFI_FORM_ID));

          if (OpCodeLength >= sizeof (EFI_IFR_REF2)) {
            CopyMem (&StatementValue->Value.ref.QuestionId, &((EFI_IFR_REF2 *)OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

            if (OpCodeLength >= sizeof (EFI_IFR_REF3)) {
              CopyMem (&StatementValue->Value.ref.FormSetGuid, &((EFI_IFR_REF3 *)OpCodeData)->FormSetId, sizeof (EFI_GUID));

              if (OpCodeLength >= sizeof (EFI_IFR_REF4)) {
                CopyMem (&StatementValue->Value.ref.DevicePath, &((EFI_IFR_REF4 *)OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));
              }
            }
          }
        }

        CurrentStatement->StorageWidth = (UINT16)sizeof (EFI_HII_REF);
        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
        break;

      case EFI_IFR_ONE_OF_OP:
      case EFI_IFR_NUMERIC_OP:

        CurrentStatement                          = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        CurrentStatement->ExtraData.NumData.Flags = ((EFI_IFR_ONE_OF *)OpCodeData)->Flags;
        StatementValue                            = &CurrentStatement->Value;

        if (QuestionReferBitField) {
          //
          // Get the bit var store info (bit/byte offset, bit/byte offset)
          //
          CurrentStatement->QuestionReferToBitField = TRUE;
          CurrentStatement->BitStorageWidth         = CurrentStatement->ExtraData.NumData.Flags & EDKII_IFR_NUMERIC_SIZE_BIT;
          CurrentStatement->BitVarOffset            = CurrentStatement->VarStoreInfo.VarOffset;
          CurrentStatement->VarStoreInfo.VarOffset  = CurrentStatement->BitVarOffset / 8;
          TotalBits                                 = CurrentStatement->BitVarOffset % 8 + CurrentStatement->BitStorageWidth;
          CurrentStatement->StorageWidth            = (TotalBits % 8 == 0 ? TotalBits / 8 : TotalBits / 8 + 1);

          //
          // Get the Minimum/Maximum/Step value(Note: bit field type has been stored as UINT32 type)
          //
          CurrentStatement->ExtraData.NumData.Minimum = ((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.MinValue;
          CurrentStatement->ExtraData.NumData.Maximum = ((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.MaxValue;
          CurrentStatement->ExtraData.NumData.Step    = ((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.Step;

          //
          // Update the Flag and type of Minimum/Maximum/Step according to the actual width of bit field,
          // in order to make Browser handle these question with bit varstore correctly.
          //
          ((EFI_IFR_NUMERIC *)OpCodeData)->Flags  &=  EDKII_IFR_DISPLAY_BIT;
          ((EFI_IFR_NUMERIC *)OpCodeData)->Flags >>= 2;
          switch (CurrentStatement->StorageWidth) {
            case 1:
              ((EFI_IFR_NUMERIC *)OpCodeData)->Flags           |= EFI_IFR_TYPE_NUM_SIZE_8;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u8.MinValue = (UINT8)CurrentStatement->ExtraData.NumData.Minimum;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u8.MaxValue = (UINT8)CurrentStatement->ExtraData.NumData.Maximum;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u8.Step     = (UINT8)CurrentStatement->ExtraData.NumData.Step;
              StatementValue->Type                              = EFI_IFR_TYPE_NUM_SIZE_8;
              break;
            case 2:
              ((EFI_IFR_NUMERIC *)OpCodeData)->Flags            |= EFI_IFR_TYPE_NUM_SIZE_16;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u16.MinValue = (UINT16)CurrentStatement->ExtraData.NumData.Minimum;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u16.MaxValue = (UINT16)CurrentStatement->ExtraData.NumData.Maximum;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u16.Step     = (UINT16)CurrentStatement->ExtraData.NumData.Step;
              StatementValue->Type                               = EFI_IFR_TYPE_NUM_SIZE_16;
              break;
            case 3:
            case 4:
              ((EFI_IFR_NUMERIC *)OpCodeData)->Flags            |= EFI_IFR_TYPE_NUM_SIZE_32;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.MinValue = (UINT32)CurrentStatement->ExtraData.NumData.Minimum;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.MaxValue = (UINT32)CurrentStatement->ExtraData.NumData.Maximum;
              ((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.Step     = (UINT32)CurrentStatement->ExtraData.NumData.Step;
              StatementValue->Type                               = EFI_IFR_TYPE_NUM_SIZE_32;
              break;
            default:
              break;
          }
        } else {
          switch (CurrentStatement->ExtraData.NumData.Flags & EFI_IFR_NUMERIC_SIZE) {
            case EFI_IFR_NUMERIC_SIZE_1:
              CurrentStatement->ExtraData.NumData.Minimum = ((EFI_IFR_NUMERIC *)OpCodeData)->data.u8.MinValue;
              CurrentStatement->ExtraData.NumData.Maximum = ((EFI_IFR_NUMERIC *)OpCodeData)->data.u8.MaxValue;
              CurrentStatement->ExtraData.NumData.Step    = ((EFI_IFR_NUMERIC *)OpCodeData)->data.u8.Step;
              CurrentStatement->StorageWidth              = (UINT16)sizeof (UINT8);
              StatementValue->Type                        = EFI_IFR_TYPE_NUM_SIZE_8;

              break;

            case EFI_IFR_NUMERIC_SIZE_2:
              CopyMem (&CurrentStatement->ExtraData.NumData.Minimum, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u16.MinValue, sizeof (UINT16));
              CopyMem (&CurrentStatement->ExtraData.NumData.Maximum, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u16.MaxValue, sizeof (UINT16));
              CopyMem (&CurrentStatement->ExtraData.NumData.Step, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u16.Step, sizeof (UINT16));
              CurrentStatement->StorageWidth = (UINT16)sizeof (UINT16);
              StatementValue->Type           = EFI_IFR_TYPE_NUM_SIZE_16;

              break;

            case EFI_IFR_NUMERIC_SIZE_4:
              CopyMem (&CurrentStatement->ExtraData.NumData.Minimum, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.MinValue, sizeof (UINT32));
              CopyMem (&CurrentStatement->ExtraData.NumData.Maximum, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.MaxValue, sizeof (UINT32));
              CopyMem (&CurrentStatement->ExtraData.NumData.Step, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u32.Step, sizeof (UINT32));
              CurrentStatement->StorageWidth = (UINT16)sizeof (UINT32);
              StatementValue->Type           = EFI_IFR_TYPE_NUM_SIZE_32;

              break;

            case EFI_IFR_NUMERIC_SIZE_8:
              CopyMem (&CurrentStatement->ExtraData.NumData.Minimum, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u64.MinValue, sizeof (UINT64));
              CopyMem (&CurrentStatement->ExtraData.NumData.Maximum, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u64.MaxValue, sizeof (UINT64));
              CopyMem (&CurrentStatement->ExtraData.NumData.Step, &((EFI_IFR_NUMERIC *)OpCodeData)->data.u64.Step, sizeof (UINT64));
              CurrentStatement->StorageWidth = (UINT16)sizeof (UINT64);
              StatementValue->Type           = EFI_IFR_TYPE_NUM_SIZE_64;

              break;

            default:

              break;
          }
        }

        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);

        if ((Operand == EFI_IFR_ONE_OF_OP) && (Scope != 0)) {
          SuppressForOption = TRUE;
        }

        break;

      case EFI_IFR_ORDERED_LIST_OP:

        CurrentStatement                                        = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        CurrentStatement->ExtraData.OrderListData.Flags         = ((EFI_IFR_ORDERED_LIST *)OpCodeData)->Flags;
        CurrentStatement->ExtraData.OrderListData.MaxContainers = ((EFI_IFR_ORDERED_LIST *)OpCodeData)->MaxContainers;
        CurrentStatement->Value.Type                            = EFI_IFR_TYPE_BUFFER;
        CurrentStatement->Value.Buffer                          = NULL;

        if (Scope != 0) {
          SuppressForOption = TRUE;
        }

        break;

      case EFI_IFR_CHECKBOX_OP:

        CurrentStatement                  = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        CurrentStatement->ExtraData.Flags = ((EFI_IFR_CHECKBOX *)OpCodeData)->Flags;
        CurrentStatement->StorageWidth    = (UINT16)sizeof (BOOLEAN);
        CurrentStatement->Value.Type      = EFI_IFR_TYPE_BOOLEAN;

        if (QuestionReferBitField) {
          //
          // Get the bit var store info (bit/byte offset, bit/byte offset)
          //
          CurrentStatement->QuestionReferToBitField = TRUE;
          CurrentStatement->BitStorageWidth         = 1;
          CurrentStatement->BitVarOffset            = CurrentStatement->VarStoreInfo.VarOffset;
          CurrentStatement->VarStoreInfo.VarOffset  = CurrentStatement->BitVarOffset / 8;
          TotalBits                                 = CurrentStatement->BitVarOffset % 8 + CurrentStatement->BitStorageWidth;
          CurrentStatement->StorageWidth            = (TotalBits % 8 == 0 ? TotalBits / 8 : TotalBits / 8 + 1);
        }

        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);

        break;

      case EFI_IFR_STRING_OP:

        CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);

        //
        // MinSize is the minimum number of characters that can be accepted for this opcode,
        // MaxSize is the maximum number of characters that can be accepted for this opcode.
        // The characters are stored as Unicode, so the storage width should multiply 2.
        //
        CurrentStatement->ExtraData.StrData.MinSize = ((EFI_IFR_STRING *)OpCodeData)->MinSize;
        CurrentStatement->ExtraData.StrData.MaxSize = ((EFI_IFR_STRING *)OpCodeData)->MaxSize;
        CurrentStatement->ExtraData.StrData.Flags   = ((EFI_IFR_STRING *)OpCodeData)->Flags;
        CurrentStatement->StorageWidth              = (UINT16)((UINTN)CurrentStatement->ExtraData.StrData.MaxSize * sizeof (CHAR16));

        CurrentStatement->Value.Type   = EFI_IFR_TYPE_STRING;
        CurrentStatement->Value.Buffer = AllocateZeroPool (CurrentStatement->StorageWidth + sizeof (CHAR16));
        if (CurrentStatement->Value.Buffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        CurrentStatement->Value.Value.string = NewHiiString ((CHAR16 *)CurrentStatement->Value.Buffer, FormSet->HiiHandle);

        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
        break;

      case EFI_IFR_PASSWORD_OP:

        CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);

        //
        // MinSize is the minimum number of characters that can be accepted for this opcode,
        // MaxSize is the maximum number of characters that can be accepted for this opcode.
        // The characters are stored as Unicode, so the storage width should multiply 2.
        //
        CopyMem (&CurrentStatement->ExtraData.PwdData.MinSize, &((EFI_IFR_PASSWORD *)OpCodeData)->MinSize, sizeof (UINT16));
        CopyMem (&CurrentStatement->ExtraData.PwdData.MaxSize, &((EFI_IFR_PASSWORD *)OpCodeData)->MaxSize, sizeof (UINT16));
        CurrentStatement->StorageWidth = (UINT16)((UINTN)CurrentStatement->ExtraData.PwdData.MaxSize * sizeof (CHAR16));

        CurrentStatement->Value.Type   = EFI_IFR_TYPE_STRING;
        CurrentStatement->Value.Buffer = AllocateZeroPool ((CurrentStatement->StorageWidth + sizeof (CHAR16)));
        if (CurrentStatement->Value.Buffer == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        CurrentStatement->Value.Value.string = NewHiiString ((CHAR16 *)CurrentStatement->Value.Buffer, FormSet->HiiHandle);

        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
        break;

      case EFI_IFR_DATE_OP:

        CurrentStatement                  = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        CurrentStatement->ExtraData.Flags = ((EFI_IFR_DATE *)OpCodeData)->Flags;
        CurrentStatement->Value.Type      = EFI_IFR_TYPE_DATE;

        if ((CurrentStatement->ExtraData.Flags & EFI_QF_DATE_STORAGE) == QF_DATE_STORAGE_NORMAL) {
          CurrentStatement->StorageWidth = (UINT16)sizeof (EFI_HII_DATE);

          InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
        } else {
          //
          // Don't assign storage for RTC type of date/time
          //
          CurrentStatement->Storage      = NULL;
          CurrentStatement->StorageWidth = 0;
        }

        break;

      case EFI_IFR_TIME_OP:

        CurrentStatement                  = CreateQuestion (OpCodeData, FormSet, CurrentForm);
        CurrentStatement->ExtraData.Flags = ((EFI_IFR_TIME *)OpCodeData)->Flags;
        CurrentStatement->Value.Type      = EFI_IFR_TYPE_TIME;

        if ((CurrentStatement->ExtraData.Flags & QF_TIME_STORAGE) == QF_TIME_STORAGE_NORMAL) {
          CurrentStatement->StorageWidth = (UINT16)sizeof (EFI_HII_TIME);

          InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
        } else {
          //
          // Don't assign storage for RTC type of date/time
          //
          CurrentStatement->Storage      = NULL;
          CurrentStatement->StorageWidth = 0;
        }

        break;

      //
      // Default
      //
      case EFI_IFR_DEFAULT_OP:

        //
        // EFI_IFR_DEFAULT appear in scope of a Question,
        // It creates a default value for the current question.
        // A Question may have more than one Default value which have different default types.
        //
        CurrentDefault = AllocateZeroPool (sizeof (HII_QUESTION_DEFAULT));
        if (CurrentDefault == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        CurrentDefault->Signature = HII_QUESTION_DEFAULT_SIGNATURE;

        CurrentDefault->Value.Type = ((EFI_IFR_DEFAULT *)OpCodeData)->Type;
        CopyMem (&CurrentDefault->DefaultId, &((EFI_IFR_DEFAULT *)OpCodeData)->DefaultId, sizeof (UINT16));
        if (CurrentDefault->Value.Type == EFI_IFR_TYPE_BUFFER) {
          CurrentDefault->Value.BufferLen = (UINT16)(OpCodeLength - OFFSET_OF (EFI_IFR_DEFAULT, Value));
          CurrentDefault->Value.Buffer    = AllocateCopyPool (CurrentDefault->Value.BufferLen, &((EFI_IFR_DEFAULT *)OpCodeData)->Value);
          if (CurrentDefault->Value.Buffer == NULL) {
            FreePool (CurrentDefault);
            return EFI_OUT_OF_RESOURCES;
          }

          if (ParentStatement->Operand == EFI_IFR_ORDERED_LIST_OP) {
            ParentStatement->Value.BufferLen = CurrentDefault->Value.BufferLen;
            CopyMem (ParentStatement->Value.Buffer, CurrentDefault->Value.Buffer, ParentStatement->Value.BufferLen);
          }
        } else {
          CopyMem (&CurrentDefault->Value.Value, &((EFI_IFR_DEFAULT *)OpCodeData)->Value, OpCodeLength - OFFSET_OF (EFI_IFR_DEFAULT, Value));
          ExtendValueToU64 (&CurrentDefault->Value);

          CopyMem (&ParentStatement->Value.Value, &((EFI_IFR_DEFAULT *)OpCodeData)->Value, OpCodeLength - OFFSET_OF (EFI_IFR_DEFAULT, Value));
          ExtendValueToU64 (&ParentStatement->Value);
        }

        //
        // Insert to Default Value list of current Question
        //
        InsertTailList (&ParentStatement->DefaultListHead, &CurrentDefault->Link);

        if (Scope != 0) {
          InScopeDefault = TRUE;
        }

        break;

      //
      // Option
      //
      case EFI_IFR_ONE_OF_OPTION_OP:

        if (ParentStatement == NULL) {
          break;
        }

        CurrentDefault = NULL;
        if (((ParentStatement->Operand == EFI_IFR_ORDERED_LIST_OP) || (ParentStatement->Operand == EFI_IFR_ONE_OF_OP)) &&
            ((((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Flags & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0))
        {
          CurrentDefault = AllocateZeroPool (sizeof (HII_QUESTION_DEFAULT));
          if (CurrentDefault == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

          CurrentDefault->Signature = HII_QUESTION_DEFAULT_SIGNATURE;
          if ((((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Flags & EFI_IFR_OPTION_DEFAULT) != 0) {
            CurrentDefault->DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
          } else {
            CurrentDefault->DefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
          }

          if (ParentStatement->Operand == EFI_IFR_ORDERED_LIST_OP) {
            CurrentDefault->Value.Type      = EFI_IFR_TYPE_BUFFER;
            CurrentDefault->Value.BufferLen = (UINT16)(OpCodeLength - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
            CurrentDefault->Value.Buffer    = AllocateCopyPool (CurrentDefault->Value.BufferLen, &((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Value);
            if (CurrentDefault->Value.Buffer == NULL) {
              FreePool (CurrentDefault);
              return EFI_OUT_OF_RESOURCES;
            }
          } else {
            CurrentDefault->Value.Type = ((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Type;
            CopyMem (&CurrentDefault->Value.Value, &((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Value, sizeof (EFI_IFR_TYPE_VALUE));
            ExtendValueToU64 (&CurrentDefault->Value);
          }

          //
          // Insert to Default Value list of current Question
          //
          InsertTailList (&ParentStatement->DefaultListHead, &CurrentDefault->Link);
        }

        //
        // EFI_IFR_ONE_OF_OPTION appear in scope of a Question.
        // It create a selection for use in current Question.
        //
        CurrentOption = AllocateZeroPool (sizeof (HII_QUESTION_OPTION));
        if (CurrentOption == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        CurrentOption->Signature  = HII_QUESTION_OPTION_SIGNATURE;
        CurrentOption->OpCode     = (EFI_IFR_ONE_OF_OPTION *)OpCodeData;
        CurrentOption->Flags      = ((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Flags;
        CurrentOption->Value.Type = ((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Type;
        CopyMem (&CurrentOption->Text, &((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Option, sizeof (EFI_STRING_ID));
        CopyMem (&CurrentOption->Value.Value, &((EFI_IFR_ONE_OF_OPTION *)OpCodeData)->Value, OpCodeLength - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
        ExtendValueToU64 (&CurrentOption->Value);

        ConditionalExprCount = GetConditionalExpressionCount (ExpressOption);
        if ( ConditionalExprCount > 0) {
          //
          // Form is inside of suppressif
          //
          CurrentOption->SuppressExpression = (HII_EXPRESSION_LIST *)AllocatePool (
                                                                       (UINTN)(sizeof (HII_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof (HII_EXPRESSION *)))
                                                                       );
          if (CurrentOption->SuppressExpression == NULL) {
            FreePool (CurrentOption);
            return EFI_OUT_OF_RESOURCES;
          }

          CurrentOption->SuppressExpression->Count     = (UINTN)ConditionalExprCount;
          CurrentOption->SuppressExpression->Signature = HII_EXPRESSION_LIST_SIGNATURE;
          CopyMem (CurrentOption->SuppressExpression->Expression, GetConditionalExpressionList (ExpressOption), (UINTN)(sizeof (HII_EXPRESSION *) * ConditionalExprCount));
        }

        //
        // Insert to Option list of current Question
        //
        InsertTailList (&ParentStatement->OptionListHead, &CurrentOption->Link);

        //
        // Now we know the Storage width of nested Ordered List
        //
        if ((ParentStatement->Operand == EFI_IFR_ORDERED_LIST_OP) && (ParentStatement->Value.Buffer == NULL)) {
          Width = 1;
          switch (CurrentOption->Value.Type) {
            case EFI_IFR_TYPE_NUM_SIZE_8:
              Width = 1;
              break;

            case EFI_IFR_TYPE_NUM_SIZE_16:
              Width = 2;
              break;

            case EFI_IFR_TYPE_NUM_SIZE_32:
              Width = 4;
              break;

            case EFI_IFR_TYPE_NUM_SIZE_64:
              Width = 8;
              break;

            default:
              //
              // Invalid type for Ordered List
              //
              break;
          }

          ParentStatement->StorageWidth = (UINT16)(ParentStatement->ExtraData.OrderListData.MaxContainers * Width);
          ParentStatement->Value.Buffer = AllocateZeroPool (ParentStatement->StorageWidth);
          if (ParentStatement->Value.Buffer == NULL) {
            return EFI_OUT_OF_RESOURCES;
          }

          ParentStatement->Value.BufferLen       = 0;
          ParentStatement->Value.BufferValueType = CurrentOption->Value.Type;
          InitializeRequestElement (FormSet, ParentStatement, CurrentForm);
        }

        break;

      //
      // Conditional
      //
      case EFI_IFR_NO_SUBMIT_IF_OP:
      case EFI_IFR_INCONSISTENT_IF_OP:

        //
        // Create an Expression node
        //
        CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
        CopyMem (&CurrentExpression->ExtraData.Error, &((EFI_IFR_INCONSISTENT_IF *)OpCodeData)->Error, sizeof (EFI_STRING_ID));

        if (Operand == EFI_IFR_NO_SUBMIT_IF_OP) {
          CurrentExpression->Type = EFI_HII_EXPRESSION_NO_SUBMIT_IF;
          InsertTailList (&ParentStatement->NoSubmitListHead, &CurrentExpression->Link);
        } else {
          CurrentExpression->Type = EFI_HII_EXPRESSION_INCONSISTENT_IF;
          InsertTailList (&ParentStatement->InconsistentListHead, &CurrentExpression->Link);
        }

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_WARNING_IF_OP:

        //
        // Create an Expression node
        //
        CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
        CopyMem (&CurrentExpression->ExtraData.WarningIfData.WarningIfError, &((EFI_IFR_WARNING_IF *)OpCodeData)->Warning, sizeof (EFI_STRING_ID));
        CurrentExpression->ExtraData.WarningIfData.TimeOut = ((EFI_IFR_WARNING_IF *)OpCodeData)->TimeOut;
        CurrentExpression->Type                            = EFI_HII_EXPRESSION_WARNING_IF;
        InsertTailList (&ParentStatement->WarningListHead, &CurrentExpression->Link);

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_SUPPRESS_IF_OP:

        //
        // Question and Option will appear in scope of this OpCode
        //
        CurrentExpression       = CreateExpression (CurrentForm, OpCodeData);
        CurrentExpression->Type = EFI_HII_EXPRESSION_SUPPRESS_IF;

        if (SuppressForOption) {
          PushConditionalExpression (CurrentExpression, ExpressOption);
        } else if (SuppressForQuestion) {
          PushConditionalExpression (CurrentExpression, ExpressStatement);
        } else {
          PushConditionalExpression (CurrentExpression, ExpressForm);
        }

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_GRAY_OUT_IF_OP:

        //
        // Questions will appear in scope of this OpCode
        //
        CurrentExpression       = CreateExpression (CurrentForm, OpCodeData);
        CurrentExpression->Type = EFI_HII_EXPRESSION_GRAY_OUT_IF;
        PushConditionalExpression (CurrentExpression, ExpressStatement);

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_DISABLE_IF_OP:

        //
        // The DisableIf expression should only rely on constant, so it could be
        // evaluated at initialization and it will not be queued
        //
        CurrentExpression = AllocateZeroPool (sizeof (HII_EXPRESSION));
        if (CurrentExpression == NULL) {
          return EFI_OUT_OF_RESOURCES;
        }

        CurrentExpression->Signature = HII_EXPRESSION_SIGNATURE;
        CurrentExpression->Type      = EFI_HII_EXPRESSION_DISABLE_IF;
        InitializeListHead (&CurrentExpression->OpCodeListHead);

        if (CurrentForm != NULL) {
          //
          // This is DisableIf for Question, enqueue it to Form expression list
          //
          PushConditionalExpression (CurrentExpression, ExpressStatement);
        }

        OpCodeDisabled = FALSE;
        InScopeDisable = TRUE;
        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      //
      // Expression
      //
      case EFI_IFR_VALUE_OP:

        CurrentExpression       = CreateExpression (CurrentForm, OpCodeData);
        CurrentExpression->Type = EFI_HII_EXPRESSION_VALUE;

        if (InScopeDefault) {
          //
          // Used for default (EFI_IFR_DEFAULT)
          //
          CurrentDefault->ValueExpression = CurrentExpression;
        } else {
          //
          // If used for a question, then the question will be read-only
          //
          // Make sure CurrentStatement is not NULL.
          // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
          // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
          //
          if (ParentStatement == NULL) {
            break;
          }

          ParentStatement->ValueExpression = CurrentExpression;
        }

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_RULE_OP:

        CurrentExpression       = CreateExpression (CurrentForm, OpCodeData);
        CurrentExpression->Type = EFI_HII_EXPRESSION_RULE;

        CurrentExpression->ExtraData.RuleId = ((EFI_IFR_RULE *)OpCodeData)->RuleId;
        InsertTailList (&CurrentForm->RuleListHead, &CurrentExpression->Link);

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_READ_OP:

        CurrentExpression       = CreateExpression (CurrentForm, OpCodeData);
        CurrentExpression->Type = EFI_HII_EXPRESSION_READ;

        //
        // Make sure CurrentStatement is not NULL.
        // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
        // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
        //
        if (ParentStatement == NULL) {
          break;
        }

        ParentStatement->ReadExpression = CurrentExpression;

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      case EFI_IFR_WRITE_OP:

        CurrentExpression       = CreateExpression (CurrentForm, OpCodeData);
        CurrentExpression->Type = EFI_HII_EXPRESSION_WRITE;

        //
        // Make sure CurrentStatement is not NULL.
        // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
        // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
        //
        if (ParentStatement == NULL) {
          break;
        }

        ParentStatement->WriteExpression = CurrentExpression;

        //
        // Take a look at next OpCode to see whether current expression consists
        // of single OpCode
        //
        if (((EFI_IFR_OP_HEADER *)(OpCodeData + OpCodeLength))->Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }

        break;

      //
      // Image
      //
      case EFI_IFR_IMAGE_OP:
        //
        // Get ScopeOpcode from top of stack
        //
        PopScope (&ScopeOpCode);
        PushScope (ScopeOpCode);

        switch (ScopeOpCode) {
          case EFI_IFR_FORM_SET_OP:

            ImageId = &FormSet->ImageId;
            break;

          case EFI_IFR_FORM_OP:
          case EFI_IFR_FORM_MAP_OP:

            if (CurrentForm != NULL) {
              ImageId = &CurrentForm->ImageId;
            }

            break;

          case EFI_IFR_ONE_OF_OPTION_OP:

            if (CurrentOption != NULL) {
              ImageId = &CurrentOption->ImageId;
            }

            break;

          default:

            //
            // Make sure CurrentStatement is not NULL.
            // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
            // file is wrongly generated by tools such as VFR Compiler.
            //
            if (ParentStatement != NULL) {
              ImageId = &ParentStatement->ImageId;
            }

            break;
        }

        CopyMem (ImageId, &((EFI_IFR_IMAGE *)OpCodeData)->Id, sizeof (EFI_IMAGE_ID));
        break;

      //
      // Refresh
      //
      case EFI_IFR_REFRESH_OP:

        if (ParentStatement != NULL) {
          ParentStatement->RefreshInterval = ((EFI_IFR_REFRESH *)OpCodeData)->RefreshInterval;
        }

        break;

      //
      // Refresh guid.
      //
      case EFI_IFR_REFRESH_ID_OP:
        //
        // Get ScopeOpcode from top of stack
        //
        PopScope (&ScopeOpCode);
        PushScope (ScopeOpCode);

        switch (ScopeOpCode) {
          case EFI_IFR_FORM_OP:
          case EFI_IFR_FORM_MAP_OP:

            if (CurrentForm != NULL) {
              CopyMem (&CurrentForm->RefreshGuid, &((EFI_IFR_REFRESH_ID *)OpCodeData)->RefreshEventGroupId, sizeof (EFI_GUID));
            }

            break;

          default:

            if (ParentStatement != NULL) {
              if (ParentStatement->Operand == EFI_IFR_NUMERIC_OP) {
                CopyMem (&ParentStatement->ExtraData.NumData.Guid, &((EFI_IFR_REFRESH_ID *)OpCodeData)->RefreshEventGroupId, sizeof (EFI_GUID));
              }
            }

            break;
        }

        break;

      //
      // Modal tag
      //
      case EFI_IFR_MODAL_TAG_OP:

        if (CurrentForm != NULL) {
          CurrentForm->ModalForm = TRUE;
        }

        break;

      //
      // Lock tag, used by form and statement.
      //
      case EFI_IFR_LOCKED_OP:
        //
        // Get ScopeOpcode from top of stack
        //
        PopScope (&ScopeOpCode);
        PushScope (ScopeOpCode);

        switch (ScopeOpCode) {
          case EFI_IFR_FORM_OP:
          case EFI_IFR_FORM_MAP_OP:

            if (CurrentForm != NULL) {
              CurrentForm->Locked = TRUE;
            }

            break;

          default:

            if (ParentStatement != NULL) {
              ParentStatement->Locked = TRUE;
            }
        }

        break;

      //
      // Vendor specific
      //
      case EFI_IFR_GUID_OP:
        CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
        if (CompareGuid ((EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)), &gEdkiiIfrBitVarstoreGuid)) {
          Scope                 = 0;
          QuestionReferBitField = TRUE;
        }

        break;

      //
      // Scope End
      //
      case EFI_IFR_END_OP:
        QuestionReferBitField = FALSE;
        Status                = PopScope (&ScopeOpCode);
        if (EFI_ERROR (Status)) {
          ResetScopeStack ();
          return Status;
        }

        //
        // Parent statement end tag found, update ParentStatement info.
        //
        if (IsStatementOpCode (ScopeOpCode) && (ParentStatement != NULL) && (ParentStatement->Operand == ScopeOpCode)) {
          ParentStatement = ParentStatement->ParentStatement;
        }

        switch (ScopeOpCode) {
          case EFI_IFR_FORM_SET_OP:
            //
            // End of FormSet, update FormSet IFR binary length
            // to stop parsing substantial OpCodes
            //
            FormSet->IfrBinaryLength = OpCodeOffset;
            break;

          case EFI_IFR_FORM_OP:
          case EFI_IFR_FORM_MAP_OP:
            //
            // End of Form
            //
            CurrentForm         = NULL;
            SuppressForQuestion = FALSE;
            break;

          case EFI_IFR_ONE_OF_OPTION_OP:
            //
            // End of Option
            //
            CurrentOption = NULL;
            break;

          case EFI_IFR_NO_SUBMIT_IF_OP:
          case EFI_IFR_INCONSISTENT_IF_OP:
          case EFI_IFR_WARNING_IF_OP:
            //
            // Ignore end of EFI_IFR_NO_SUBMIT_IF and EFI_IFR_INCONSISTENT_IF
            //
            break;

          case EFI_IFR_SUPPRESS_IF_OP:
            if (SuppressForOption) {
              PopConditionalExpression (ExpressOption);
            } else if (SuppressForQuestion) {
              PopConditionalExpression (ExpressStatement);
            } else {
              PopConditionalExpression (ExpressForm);
            }

            break;

          case EFI_IFR_GRAY_OUT_IF_OP:
            PopConditionalExpression (ExpressStatement);
            break;

          case EFI_IFR_DISABLE_IF_OP:
            if (CurrentForm != NULL) {
              PopConditionalExpression (ExpressStatement);
            }

            InScopeDisable = FALSE;
            OpCodeDisabled = FALSE;
            break;

          case EFI_IFR_ONE_OF_OP:
          case EFI_IFR_ORDERED_LIST_OP:
            SuppressForOption = FALSE;
            break;

          case EFI_IFR_DEFAULT_OP:
            InScopeDefault = FALSE;
            break;

          case EFI_IFR_MAP_OP:

            //
            // Get current Map Expression List.
            //
            Status = PopMapExpressionList ((VOID **)&MapExpressionList);
            if (Status == EFI_ACCESS_DENIED) {
              MapExpressionList = NULL;
            }

            //
            // Get current expression.
            //
            Status = PopCurrentExpression ((VOID **)&CurrentExpression);
            if (EFI_ERROR (Status)) {
              return Status;
            }

            if (MapScopeDepth > 0) {
              MapScopeDepth--;
            }

            break;

          default:

            if (IsExpressionOpCode (ScopeOpCode)) {
              if (InScopeDisable && (CurrentForm == NULL)) {
                //
                // This is DisableIf expression for Form, it should be a constant expression
                //
                Status = EvaluateHiiExpression (FormSet, CurrentForm, CurrentExpression);
                if (EFI_ERROR (Status)) {
                  return Status;
                }

                OpCodeDisabled = IsHiiValueTrue (&CurrentExpression->Result);

                //
                // DisableIf Expression is only used once and not queued, free it
                //
                DestroyExpression (CurrentExpression);
              }

              //
              // End of current Expression
              //
              CurrentExpression = NULL;
            }

            break;
        }

        break;

      default:
        break;
    }

    if (IsStatementOpCode (Operand)) {
      CurrentStatement->ParentStatement = ParentStatement;
      if (Scope != 0) {
        //
        // Scope != 0, other statements or options may nest in this statement.
        // Update the ParentStatement info.
        //
        ParentStatement = CurrentStatement;
      }
    }
  }

  return EFI_SUCCESS;
}
