/** @file
Parser for IFR binary encoding.

Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "HiiDatabase.h"

#include "UefiIfrParserExpression.h"

UINT16           mStatementIndex;

BOOLEAN          mInScopeSubtitle;
BOOLEAN          mInScopeSuppress;
BOOLEAN          mInScopeGrayOut;

EFI_GUID  mFrameworkHiiCompatibilityGuid = EFI_IFR_FRAMEWORK_GUID;
extern EFI_GUID mTianoHiiIfrGuid;

/**
  Find the question's OneOfOptionMap list in FormSet 
  based on the input question Id. 
  
  @param FormSet     FormSet context.
  @param QuestionId  Unique ID to specicy the question in FormSet.
  
  @return the found OneOfOptionMap list. If not found, NULL will return.
**/
LIST_ENTRY *
GetOneOfOptionMapEntryListHead (
  IN CONST FORM_BROWSER_FORMSET  *FormSet,
  IN       UINT16                 QuestionId
  )
{
  LIST_ENTRY            *Link;
  ONE_OF_OPTION_MAP     *Map;

  Link = GetFirstNode (&FormSet->OneOfOptionMapListHead);

  while (!IsNull (&FormSet->OneOfOptionMapListHead, Link)) {
    Map = ONE_OF_OPTION_MAP_FROM_LINK (Link);
    if (QuestionId == Map->QuestionId) {
      return &Map->OneOfOptionMapEntryListHead;
    }
    Link = GetNextNode (&FormSet->OneOfOptionMapListHead, Link);
  }
  
  return NULL;
}

/**
  Free OneOfOption map list.
  
  @param OneOfOptionMapListHead Pointer to list header of OneOfOptionMap list.

**/
VOID
DestoryOneOfOptionMap (
  IN LIST_ENTRY     *OneOfOptionMapListHead
  )
{
  ONE_OF_OPTION_MAP         *Map;
  ONE_OF_OPTION_MAP_ENTRY   *MapEntry;
  LIST_ENTRY                *Link;
  LIST_ENTRY                *Link2;

  while (!IsListEmpty (OneOfOptionMapListHead)) {
    Link = GetFirstNode (OneOfOptionMapListHead);
    
    Map = ONE_OF_OPTION_MAP_FROM_LINK (Link);

    while (!IsListEmpty (&Map->OneOfOptionMapEntryListHead)) {
      Link2 = GetFirstNode (&Map->OneOfOptionMapEntryListHead);
      
      MapEntry = ONE_OF_OPTION_MAP_ENTRY_FROM_LINK (Link2);

      RemoveEntryList (Link2);

      FreePool (MapEntry);
    }

    RemoveEntryList (Link);
    FreePool (Map);
  }
}


/**
  Initialize Statement header members.

  @param  OpCodeData             Pointer of the raw OpCode data.
  @param  FormSet                Pointer of the current FormSe.
  @param  Form                   Pointer of the current Form.

  @return The Statement.

**/
FORM_BROWSER_STATEMENT *
CreateStatement (
  IN UINT8                        *OpCodeData,
  IN OUT FORM_BROWSER_FORMSET     *FormSet,
  IN OUT FORM_BROWSER_FORM        *Form
  )
{
  FORM_BROWSER_STATEMENT    *Statement;
  EFI_IFR_STATEMENT_HEADER  *StatementHdr;

  if (Form == NULL) {
    //
    // We are currently not in a Form Scope, so just skip this Statement
    //
    return NULL;
  }

  Statement = &FormSet->StatementBuffer[mStatementIndex];
  mStatementIndex++;

  InitializeListHead (&Statement->DefaultListHead);
  InitializeListHead (&Statement->OptionListHead);

  Statement->Signature = FORM_BROWSER_STATEMENT_SIGNATURE;

  Statement->Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;

  StatementHdr = (EFI_IFR_STATEMENT_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->Prompt, &StatementHdr->Prompt, sizeof (EFI_STRING_ID));
  CopyMem (&Statement->Help, &StatementHdr->Help, sizeof (EFI_STRING_ID));

  Statement->InSubtitle = mInScopeSubtitle;

  //
  // Insert this Statement into current Form
  //
  InsertTailList (&Form->StatementListHead, &Statement->Link);

  return Statement;
}

/**
  Initialize Question's members.

  @param  OpCodeData             Pointer of the raw OpCode data.
  @param  FormSet                Pointer of the current FormSet.
  @param  Form                   Pointer of the current Form.

  @return The Question.

**/
FORM_BROWSER_STATEMENT *
CreateQuestion (
  IN UINT8                        *OpCodeData,
  IN OUT FORM_BROWSER_FORMSET     *FormSet,
  IN OUT FORM_BROWSER_FORM        *Form
  )
{
  FORM_BROWSER_STATEMENT   *Statement;
  EFI_IFR_QUESTION_HEADER  *QuestionHdr;
  LIST_ENTRY               *Link;
  FORMSET_STORAGE          *Storage;

  Statement = CreateStatement (OpCodeData, FormSet, Form);
  if (Statement == NULL) {
    return NULL;
  }

  QuestionHdr = (EFI_IFR_QUESTION_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->QuestionId, &QuestionHdr->QuestionId, sizeof (EFI_QUESTION_ID));
  CopyMem (&Statement->VarStoreId, &QuestionHdr->VarStoreId, sizeof (EFI_VARSTORE_ID));
  CopyMem (&Statement->VarStoreInfo.VarOffset, &QuestionHdr->VarStoreInfo.VarOffset, sizeof (UINT16));

  if (FormSet->MaxQuestionId < QuestionHdr->QuestionId) {
    FormSet->MaxQuestionId = QuestionHdr->QuestionId;
  }

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
    Storage = FORMSET_STORAGE_FROM_LINK (Link);

    if (Storage->VarStoreId == Statement->VarStoreId) {
      Statement->Storage = Storage;
      break;
    }

    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }
  ASSERT (Statement->Storage != NULL);

  return Statement;
}

/**
  Allocate a FORMSET_STORAGE data structure and insert to FormSet Storage List.

  @param  FormSet                Pointer of the current FormSet

  @return Pointer to a FORMSET_STORAGE data structure.

**/
FORMSET_STORAGE *
CreateStorage (
  IN FORM_BROWSER_FORMSET  *FormSet
  )
{
  FORMSET_STORAGE  *Storage;

  Storage = AllocateZeroPool (sizeof (FORMSET_STORAGE));
  ASSERT (Storage != NULL);
  Storage->Signature = FORMSET_STORAGE_SIGNATURE;
  InsertTailList (&FormSet->StorageListHead, &Storage->Link);

  return Storage;
}

/**
  Free resources of a storage

  @param  Storage                Pointer of the storage

  @return None.

**/
VOID
DestroyStorage (
  IN FORMSET_STORAGE   *Storage
  )
{
  if (Storage == NULL) {
    return;
  }

  if (Storage->Name!= NULL) {
    FreePool (Storage->Name);
  }

  FreePool (Storage);
}


/**
  Free resources of a Statement

  @param  Statement              Pointer of the Statement

  @return None.

**/
VOID
DestroyStatement (
  IN OUT FORM_BROWSER_STATEMENT  *Statement
  )
{
  LIST_ENTRY        *Link;
  QUESTION_DEFAULT  *Default;
  QUESTION_OPTION   *Option;

  //
  // Free Default value List
  //
  while (!IsListEmpty (&Statement->DefaultListHead)) {
    Link = GetFirstNode (&Statement->DefaultListHead);
    Default = QUESTION_DEFAULT_FROM_LINK (Link);
    RemoveEntryList (&Default->Link);

    gBS->FreePool (Default);
  }

  //
  // Free Options List
  //
  while (!IsListEmpty (&Statement->OptionListHead)) {
    Link = GetFirstNode (&Statement->OptionListHead);
    Option = QUESTION_OPTION_FROM_LINK (Link);
    RemoveEntryList (&Option->Link);

    gBS->FreePool (Option);
  }

}



/**
  Free resources of a Form

  @param  Form                   Pointer of the Form

  @return None.

**/
VOID
DestroyForm (
  IN OUT FORM_BROWSER_FORM  *Form
  )
{
  LIST_ENTRY              *Link;
  FORM_BROWSER_STATEMENT  *Statement;

  //
  // Free Statements/Questions
  //
  while (!IsListEmpty (&Form->StatementListHead)) {
    Link = GetFirstNode (&Form->StatementListHead);
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    RemoveEntryList (&Statement->Link);

    DestroyStatement (Statement);
  }

  //
  // Free this Form
  //
  gBS->FreePool (Form);
}


/**
  Free resources allocated for a FormSet

  @param  FormSet                Pointer of the FormSet

  @return None.

**/
VOID
DestroyFormSet (
  IN OUT FORM_BROWSER_FORMSET  *FormSet
  )
{
  LIST_ENTRY            *Link;
  FORMSET_STORAGE       *Storage;
  FORMSET_DEFAULTSTORE  *DefaultStore;
  FORM_BROWSER_FORM     *Form;

  //
  // Free IFR binary buffer
  //
  FreePool (FormSet->IfrBinaryData);

  //
  // Free FormSet Storage
  //
  if (FormSet->StorageListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->StorageListHead)) {
      Link = GetFirstNode (&FormSet->StorageListHead);
      Storage = FORMSET_STORAGE_FROM_LINK (Link);
      RemoveEntryList (&Storage->Link);

      DestroyStorage (Storage);
    }
  }

  //
  // Free FormSet Default Store
  //
  if (FormSet->DefaultStoreListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->DefaultStoreListHead)) {
      Link = GetFirstNode (&FormSet->DefaultStoreListHead);
      DefaultStore = FORMSET_DEFAULTSTORE_FROM_LINK (Link);
      RemoveEntryList (&DefaultStore->Link);

      gBS->FreePool (DefaultStore);
    }
  }

  //
  // Free Forms
  //
  if (FormSet->FormListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->FormListHead)) {
      Link = GetFirstNode (&FormSet->FormListHead);
      Form = FORM_BROWSER_FORM_FROM_LINK (Link);
      RemoveEntryList (&Form->Link);

      DestroyForm (Form);
    }
  }

  if (FormSet->StatementBuffer != NULL) {
    FreePool (FormSet->StatementBuffer);
  }

  DestoryOneOfOptionMap (&FormSet->OneOfOptionMapListHead);

  if (FormSet->OriginalDefaultVarStoreName != NULL) {
    FreePool (FormSet->OriginalDefaultVarStoreName);
  }
  
  FreePool (FormSet);
}


/**
  Tell whether this Operand is an Expression OpCode or not

  @param  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Expression OpCode.
  @retval FALSE                  Not an Expression OpCode.

**/
BOOLEAN
IsExpressionOpCode (
  IN UINT8              Operand
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
      (Operand == EFI_IFR_SECURITY_OP)) {
    return TRUE;
  } else {
    return FALSE;
  }
}


/**
  Calculate number of Statemens(Questions) and Expression OpCodes.

  @param  FormSet                The FormSet to be counted.
  @param  NumberOfStatement      Number of Statemens(Questions)
  @param  NumberOfExpression     Number of Expression OpCodes

  @return None.

**/
VOID
CountOpCodes (
  IN  FORM_BROWSER_FORMSET  *FormSet,
  IN OUT  UINT16            *NumberOfStatement,
  IN OUT  UINT16            *NumberOfExpression
  )
{
  UINT16  StatementCount;
  UINT16  ExpressionCount;
  UINT8   *OpCodeData;
  UINTN   Offset;
  UINTN   OpCodeLen;

  Offset = 0;
  StatementCount = 0;
  ExpressionCount = 0;

  while (Offset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + Offset;
    OpCodeLen = ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
    Offset += OpCodeLen;

    if (IsExpressionOpCode (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode)) {
      ExpressionCount++;
    } else {
      StatementCount++;
    }
  }

  *NumberOfStatement = StatementCount;
  *NumberOfExpression = ExpressionCount;
}


/**
  Parse opcodes in the formset IFR binary.

  @param  FormSet                Pointer of the FormSet data structure.

  @retval EFI_SUCCESS            Opcode parse success.
  @retval Other                  Opcode parse fail.

**/
EFI_STATUS
ParseOpCodes (
  IN FORM_BROWSER_FORMSET              *FormSet
  )
{
  EFI_STATUS              Status;
  UINT16                  Index;
  FORM_BROWSER_FORM       *CurrentForm;
  FORM_BROWSER_STATEMENT  *CurrentStatement;
  UINT8                   Operand;
  UINT8                   Scope;
  UINTN                   OpCodeOffset;
  UINTN                   OpCodeLength;
  UINT8                   *OpCodeData;
  UINT8                   ScopeOpCode;
  FORMSET_STORAGE         *Storage;
  FORMSET_DEFAULTSTORE    *DefaultStore;
  QUESTION_DEFAULT        *CurrentDefault;
  QUESTION_OPTION         *CurrentOption;
  CHAR8                   *AsciiString;
  UINT16                  NumberOfStatement;
  UINT16                  NumberOfExpression;
  EFI_IMAGE_ID            *ImageId;
  EFI_HII_VALUE           *Value;
  LIST_ENTRY              *OneOfOptinMapEntryListHead;
  EFI_IFR_GUID_OPTIONKEY  *OptionMap;
  ONE_OF_OPTION_MAP       *OneOfOptionMap;
  ONE_OF_OPTION_MAP_ENTRY *OneOfOptionMapEntry;
  UINT8                   OneOfType;
  EFI_IFR_ONE_OF          *OneOfOpcode;
  HII_THUNK_CONTEXT       *ThunkContext;
  EFI_IFR_FORM_MAP_METHOD *MapMethod;

  mInScopeSubtitle = FALSE;
  mInScopeSuppress = FALSE;
  mInScopeGrayOut  = FALSE;
  CurrentDefault   = NULL;
  CurrentOption    = NULL;
  MapMethod        = NULL;
  ThunkContext     = UefiHiiHandleToThunkContext ((CONST HII_THUNK_PRIVATE_DATA*) mHiiThunkPrivateData, FormSet->HiiHandle);

  //
  // Set to a invalid value.
  //
  OneOfType = (UINT8) -1;

  //
  // Get the number of Statements and Expressions
  //
  CountOpCodes (FormSet, &NumberOfStatement, &NumberOfExpression);
  FormSet->NumberOfStatement = NumberOfStatement;

  mStatementIndex = 0;
  FormSet->StatementBuffer = AllocateZeroPool (NumberOfStatement * sizeof (FORM_BROWSER_STATEMENT));
  if (FormSet->StatementBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&FormSet->StorageListHead);
  InitializeListHead (&FormSet->DefaultStoreListHead);
  InitializeListHead (&FormSet->FormListHead);
  InitializeListHead (&FormSet->OneOfOptionMapListHead);

  CurrentForm = NULL;
  CurrentStatement = NULL;

  ResetScopeStack ();

  OpCodeOffset = 0;
  while (OpCodeOffset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + OpCodeOffset;

    OpCodeLength = ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
    OpCodeOffset += OpCodeLength;
    Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;
    Scope = ((EFI_IFR_OP_HEADER *) OpCodeData)->Scope;

    //
    // If scope bit set, push onto scope stack
    //
    if (Scope != 0) {
      PushScope (Operand);
    }

    if (IsExpressionOpCode (Operand)) {
      continue;
    }

    //
    // Parse the Opcode
    //
    switch (Operand) {

    case EFI_IFR_FORM_SET_OP:
      //
      // check the formset GUID
      //
      if (!CompareGuid ((EFI_GUID *)(VOID *)&FormSet->Guid, (EFI_GUID *)(VOID *)&((EFI_IFR_FORM_SET *) OpCodeData)->Guid)) {
        return EFI_INVALID_PARAMETER;
      }

      CopyMem (&FormSet->FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
      CopyMem (&FormSet->Help,         &((EFI_IFR_FORM_SET *) OpCodeData)->Help,         sizeof (EFI_STRING_ID));
      break;

    case EFI_IFR_FORM_OP:
      //
      // Create a new Form for this FormSet
      //
      CurrentForm = AllocateZeroPool (sizeof (FORM_BROWSER_FORM));
      ASSERT (CurrentForm != NULL);
      CurrentForm->Signature = FORM_BROWSER_FORM_SIGNATURE;

      InitializeListHead (&CurrentForm->StatementListHead);

      CopyMem (&CurrentForm->FormId,    &((EFI_IFR_FORM *) OpCodeData)->FormId,    sizeof (UINT16));
      CopyMem (&CurrentForm->FormTitle, &((EFI_IFR_FORM *) OpCodeData)->FormTitle, sizeof (EFI_STRING_ID));

      //
      // Insert into Form list of this FormSet
      //
      InsertTailList (&FormSet->FormListHead, &CurrentForm->Link);
      break;

    case EFI_IFR_FORM_MAP_OP:
      //
      // Create a new Form Map for this FormSet
      //
      CurrentForm = AllocateZeroPool (sizeof (FORM_BROWSER_FORM));
      ASSERT (CurrentForm != NULL);
      CurrentForm->Signature = FORM_BROWSER_FORM_SIGNATURE;

      InitializeListHead (&CurrentForm->StatementListHead);

      CopyMem (&CurrentForm->FormId, &((EFI_IFR_FORM *) OpCodeData)->FormId, sizeof (UINT16));
      MapMethod = (EFI_IFR_FORM_MAP_METHOD *) (OpCodeData + sizeof (EFI_IFR_FORM_MAP));

      //
      // FormMap Form must contain at least one Map Method.
      //
      if (((EFI_IFR_OP_HEADER *) OpCodeData)->Length < ((UINTN) (UINT8 *) (MapMethod + 1) - (UINTN) OpCodeData)) {
        return EFI_INVALID_PARAMETER;
      }

      //
      // Try to find the standard form map method.
      //
      while (((UINTN) (UINT8 *) MapMethod - (UINTN) OpCodeData) < ((EFI_IFR_OP_HEADER *) OpCodeData)->Length) {
        if (CompareGuid ((EFI_GUID *) (VOID *) &MapMethod->MethodIdentifier, &gEfiHiiStandardFormGuid)) {
          CopyMem (&CurrentForm->FormTitle, &MapMethod->MethodTitle, sizeof (EFI_STRING_ID));
          break;
        }
        MapMethod ++;
      }
      //
      // If the standard form map method is not found, the first map method title will be used.
      //
      if (CurrentForm->FormTitle == 0) {
        MapMethod = (EFI_IFR_FORM_MAP_METHOD *) (OpCodeData + sizeof (EFI_IFR_FORM_MAP));
        CopyMem (&CurrentForm->FormTitle, &MapMethod->MethodTitle, sizeof (EFI_STRING_ID));
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
      Storage = CreateStorage (FormSet);
      Storage->Type = EFI_HII_VARSTORE_BUFFER;

      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      CopyMem (&Storage->Guid,       &((EFI_IFR_VARSTORE *) OpCodeData)->Guid,       sizeof (EFI_GUID));
      CopyMem (&Storage->Size,       &((EFI_IFR_VARSTORE *) OpCodeData)->Size,       sizeof (UINT16));

      AsciiString = (CHAR8 *) ((EFI_IFR_VARSTORE *) OpCodeData)->Name;
      Storage->Name = AllocateZeroPool (AsciiStrSize (AsciiString) * 2);
      ASSERT (Storage->Name != NULL);
      for (Index = 0; AsciiString[Index] != 0; Index++) {
        Storage->Name[Index] = (CHAR16) AsciiString[Index];
      }

      break;

    case EFI_IFR_VARSTORE_NAME_VALUE_OP:
      //
      // Framework IFR doesn't support Name/Value VarStore opcode
      //
      if (ThunkContext != NULL && ThunkContext->ByFrameworkHiiNewPack) {
        ASSERT (FALSE);
      }

      //
      // Create a name/value Storage for this FormSet
      //
      Storage = CreateStorage (FormSet);
      Storage->Type = EFI_HII_VARSTORE_NAME_VALUE;

      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      CopyMem (&Storage->Guid,       &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->Guid,       sizeof (EFI_GUID));

      break;

    case EFI_IFR_VARSTORE_EFI_OP:
      //
      // Create a EFI variable Storage for this FormSet
      //
      Storage = CreateStorage (FormSet);
      Storage->Type = EFI_HII_VARSTORE_EFI_VARIABLE;

      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      CopyMem (&Storage->Guid,       &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Guid,       sizeof (EFI_GUID));
      CopyMem (&Storage->Attributes, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Attributes, sizeof (UINT32));
      break;

    //
    // DefaultStore
    //
    case EFI_IFR_DEFAULTSTORE_OP:
      DefaultStore = AllocateZeroPool (sizeof (FORMSET_DEFAULTSTORE));
      ASSERT (DefaultStore != NULL);
      DefaultStore->Signature = FORMSET_DEFAULTSTORE_SIGNATURE;

      CopyMem (&DefaultStore->DefaultId,   &((EFI_IFR_DEFAULTSTORE *) OpCodeData)->DefaultId,   sizeof (UINT16));
      CopyMem (&DefaultStore->DefaultName, &((EFI_IFR_DEFAULTSTORE *) OpCodeData)->DefaultName, sizeof (EFI_STRING_ID));

      //
      // Insert to DefaultStore list of this Formset
      //
      InsertTailList (&FormSet->DefaultStoreListHead, &DefaultStore->Link);
      break;

    //
    // Statements
    //
    case EFI_IFR_SUBTITLE_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->Flags = ((EFI_IFR_SUBTITLE *) OpCodeData)->Flags;

      if (Scope != 0) {
        mInScopeSubtitle = TRUE;
      }
      break;

    case EFI_IFR_TEXT_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CopyMem (&CurrentStatement->TextTwo, &((EFI_IFR_TEXT *) OpCodeData)->TextTwo, sizeof (EFI_STRING_ID));
      break;

    //
    // Questions
    //
    case EFI_IFR_ACTION_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      if (OpCodeLength == sizeof (EFI_IFR_ACTION_1)) {
        //
        // No QuestionConfig present, so no configuration string will be processed
        //
        CurrentStatement->QuestionConfig = 0;
      } else {
        CopyMem (&CurrentStatement->QuestionConfig, &((EFI_IFR_ACTION *) OpCodeData)->QuestionConfig, sizeof (EFI_STRING_ID));
      }
      break;

    case EFI_IFR_RESET_BUTTON_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CopyMem (&CurrentStatement->DefaultId, &((EFI_IFR_RESET_BUTTON *) OpCodeData)->DefaultId, sizeof (EFI_DEFAULT_ID));
      break;

    case EFI_IFR_REF_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CopyMem (&CurrentStatement->RefFormId, &((EFI_IFR_REF *) OpCodeData)->FormId, sizeof (EFI_FORM_ID));
      if (OpCodeLength >= sizeof (EFI_IFR_REF2)) {
        CopyMem (&CurrentStatement->RefQuestionId, &((EFI_IFR_REF2 *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

        if (OpCodeLength >= sizeof (EFI_IFR_REF3)) {
          CopyMem (&CurrentStatement->RefFormSetId, &((EFI_IFR_REF3 *) OpCodeData)->FormSetId, sizeof (EFI_GUID));

          if (OpCodeLength >= sizeof (EFI_IFR_REF4)) {
            CopyMem (&CurrentStatement->RefDevicePath, &((EFI_IFR_REF4 *) OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));
          }
        }
      }
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_NUMERIC_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_ONE_OF *) OpCodeData)->Flags;
      Value = &CurrentStatement->HiiValue;

      switch (CurrentStatement->Flags & EFI_IFR_NUMERIC_SIZE) {
      case EFI_IFR_NUMERIC_SIZE_1:
        CurrentStatement->Minimum = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u8.MinValue;
        CurrentStatement->Maximum = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u8.MaxValue;
        CurrentStatement->Step    = ((EFI_IFR_NUMERIC *) OpCodeData)->data.u8.Step;
        CurrentStatement->StorageWidth = (UINT16) sizeof (UINT8);
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        break;

      case EFI_IFR_NUMERIC_SIZE_2:
        CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u16.MinValue, sizeof (UINT16));
        CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u16.MaxValue, sizeof (UINT16));
        CopyMem (&CurrentStatement->Step,    &((EFI_IFR_NUMERIC *) OpCodeData)->data.u16.Step,     sizeof (UINT16));
        CurrentStatement->StorageWidth = (UINT16) sizeof (UINT16);
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        break;

      case EFI_IFR_NUMERIC_SIZE_4:
        CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.MinValue, sizeof (UINT32));
        CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.MaxValue, sizeof (UINT32));
        CopyMem (&CurrentStatement->Step,    &((EFI_IFR_NUMERIC *) OpCodeData)->data.u32.Step,     sizeof (UINT32));
        CurrentStatement->StorageWidth = (UINT16) sizeof (UINT32);
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_32;
        break;

      case EFI_IFR_NUMERIC_SIZE_8:
        CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u64.MinValue, sizeof (UINT64));
        CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_NUMERIC *) OpCodeData)->data.u64.MaxValue, sizeof (UINT64));
        CopyMem (&CurrentStatement->Step,    &((EFI_IFR_NUMERIC *) OpCodeData)->data.u64.Step,     sizeof (UINT64));
        CurrentStatement->StorageWidth = (UINT16) sizeof (UINT64);
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
        break;

      default:
        break;
      }

      if (Operand == EFI_IFR_ONE_OF_OP) {
        OneOfOpcode = (EFI_IFR_ONE_OF *) OpCodeData;
        OneOfType   = (UINT8) (OneOfOpcode->Flags & EFI_IFR_NUMERIC_SIZE);
      }
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_ORDERED_LIST *) OpCodeData)->Flags;
      CurrentStatement->MaxContainers = ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers;
      CurrentStatement->StorageWidth = (UINT16)(CurrentStatement->MaxContainers * sizeof (UINT8));

      //
      // No buffer type is defined in EFI_IFR_TYPE_VALUE, so a Configuration Driver
      // has to use FormBrowser2.Callback() to retrieve the uncommited data for
      // an interactive orderedlist (i.e. with EFI_IFR_FLAG_CALLBACK flag set).
      //
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_OTHER;
      CurrentStatement->BufferValue = AllocateZeroPool (CurrentStatement->StorageWidth);

      break;

    case EFI_IFR_CHECKBOX_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_CHECKBOX *) OpCodeData)->Flags;
      CurrentStatement->StorageWidth = (UINT16) sizeof (BOOLEAN);
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_BOOLEAN;

      break;

    case EFI_IFR_STRING_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      //
      // MinSize is the minimum number of characters that can be accepted for this opcode,
      // MaxSize is the maximum number of characters that can be accepted for this opcode.
      // The characters are stored as Unicode, so the storage width should multiply 2.
      //
      CurrentStatement->Minimum = ((EFI_IFR_STRING *) OpCodeData)->MinSize;
      CurrentStatement->Maximum = ((EFI_IFR_STRING *) OpCodeData)->MaxSize;
      CurrentStatement->StorageWidth = (UINT16)((UINTN) CurrentStatement->Maximum * sizeof (UINT16));
      CurrentStatement->Flags = ((EFI_IFR_STRING *) OpCodeData)->Flags;

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_STRING;
      CurrentStatement->BufferValue = AllocateZeroPool (CurrentStatement->StorageWidth);

      break;

    case EFI_IFR_PASSWORD_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      //
      // MinSize is the minimum number of characters that can be accepted for this opcode,
      // MaxSize is the maximum number of characters that can be accepted for this opcode.
      // The characters are stored as Unicode, so the storage width should multiply 2.
      //
      CopyMem (&CurrentStatement->Minimum, &((EFI_IFR_PASSWORD *) OpCodeData)->MinSize, sizeof (UINT16));
      CopyMem (&CurrentStatement->Maximum, &((EFI_IFR_PASSWORD *) OpCodeData)->MaxSize, sizeof (UINT16));
      CurrentStatement->StorageWidth = (UINT16)((UINTN) CurrentStatement->Maximum * sizeof (UINT16));

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_STRING;
      CurrentStatement->BufferValue = AllocateZeroPool (CurrentStatement->StorageWidth);

      break;

    case EFI_IFR_DATE_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_DATE *) OpCodeData)->Flags;
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_DATE;

      break;

    case EFI_IFR_TIME_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_TIME *) OpCodeData)->Flags;
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_TIME;

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
      CurrentDefault = AllocateZeroPool (sizeof (QUESTION_DEFAULT));
      ASSERT (CurrentDefault != NULL);
      CurrentDefault->Signature = QUESTION_DEFAULT_SIGNATURE;

      CurrentDefault->Value.Type = ((EFI_IFR_DEFAULT *) OpCodeData)->Type;
      CopyMem (&CurrentDefault->DefaultId, &((EFI_IFR_DEFAULT *) OpCodeData)->DefaultId, sizeof (UINT16));
      CopyMem (&CurrentDefault->Value.Value, &((EFI_IFR_DEFAULT *) OpCodeData)->Value, sizeof (EFI_IFR_TYPE_VALUE));
      ExtendValueToU64 (&CurrentDefault->Value);

      //
      // Insert to Default Value list of current Question
      //
      InsertTailList (&CurrentStatement->DefaultListHead, &CurrentDefault->Link);

      break;

    //
    // Option
    //
    case EFI_IFR_ONE_OF_OPTION_OP:
      //
      // EFI_IFR_ONE_OF_OPTION appear in scope of a Question.
      // It create a selection for use in current Question.
      //
      CurrentOption = AllocateZeroPool (sizeof (QUESTION_OPTION));
      ASSERT (CurrentOption != NULL);
      CurrentOption->Signature = QUESTION_OPTION_SIGNATURE;

      CurrentOption->Flags = ((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Flags;
      CurrentOption->Value.Type = ((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Type;
      CopyMem (&CurrentOption->Text, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Option, sizeof (EFI_STRING_ID));
      CopyMem (&CurrentOption->Value.Value, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Value, sizeof (EFI_IFR_TYPE_VALUE));
      ExtendValueToU64 (&CurrentOption->Value);

      //
      // Insert to Option list of current Question
      //
      InsertTailList (&CurrentStatement->OptionListHead, &CurrentOption->Link);
      break;

    //
    // Conditional
    //
    case EFI_IFR_NO_SUBMIT_IF_OP:
    case EFI_IFR_INCONSISTENT_IF_OP:
      break;

    case EFI_IFR_SUPPRESS_IF_OP:
      break;

    case EFI_IFR_GRAY_OUT_IF_OP:
      break;

    case EFI_IFR_DISABLE_IF_OP:
      //
      // Framework IFR doesn't support DisableIf opcode
      //
      if (ThunkContext != NULL && ThunkContext->ByFrameworkHiiNewPack) {
        ASSERT (FALSE);
      }

    //
    // Expression
    //
    case EFI_IFR_VALUE_OP:
    case EFI_IFR_READ_OP:
    case EFI_IFR_WRITE_OP:
      break;

    case EFI_IFR_RULE_OP:
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
        ASSERT (CurrentForm != NULL);
        ImageId = &CurrentForm->ImageId;
        break;

      case EFI_IFR_ONE_OF_OPTION_OP:
        ASSERT (CurrentOption != NULL);
        ImageId = &CurrentOption->ImageId;
        break;

      default:
        //
        // Make sure CurrentStatement is not NULL.
        // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
        // file is wrongly generated by tools such as VFR Compiler.
        //
        ASSERT (CurrentStatement != NULL);
        ImageId = &CurrentStatement->ImageId;
        break;
      }
      
      ASSERT (ImageId != NULL);
      CopyMem (ImageId, &((EFI_IFR_IMAGE *) OpCodeData)->Id, sizeof (EFI_IMAGE_ID));
      break;

    //
    // Refresh
    //
    case EFI_IFR_REFRESH_OP:
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->RefreshInterval = ((EFI_IFR_REFRESH *) OpCodeData)->RefreshInterval;
      break;

    //
    // Vendor specific
    //
    case EFI_IFR_GUID_OP:
      OptionMap = (EFI_IFR_GUID_OPTIONKEY *) OpCodeData;
      
      if (CompareGuid (&mTianoHiiIfrGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
        //
        // Tiano specific GUIDed opcodes
        //
        switch (((EFI_IFR_GUID_LABEL *) OpCodeData)->ExtendOpCode) {
        case EFI_IFR_EXTEND_OP_LABEL:
          //
          // just ignore label
          //
          break;


        case EFI_IFR_EXTEND_OP_CLASS:
          CopyMem (&FormSet->Class, &((EFI_IFR_GUID_CLASS *) OpCodeData)->Class, sizeof (UINT16));
          break;

        case EFI_IFR_EXTEND_OP_SUBCLASS:
          CopyMem (&FormSet->SubClass, &((EFI_IFR_GUID_SUBCLASS *) OpCodeData)->SubClass, sizeof (UINT16));
          break;

        default:
          break;
        }
      } else if (CompareGuid ((EFI_GUID *)(VOID *)&OptionMap->Guid, &mFrameworkHiiCompatibilityGuid)) {
        if (OptionMap->ExtendOpCode == EFI_IFR_EXTEND_OP_OPTIONKEY) {
          OneOfOptinMapEntryListHead = GetOneOfOptionMapEntryListHead (FormSet, OptionMap->QuestionId);
          if (OneOfOptinMapEntryListHead == NULL) {
            OneOfOptionMap = AllocateZeroPool (sizeof (ONE_OF_OPTION_MAP));
            ASSERT (OneOfOptionMap != NULL);

            OneOfOptionMap->Signature = ONE_OF_OPTION_MAP_SIGNATURE;
            OneOfOptionMap->QuestionId = OptionMap->QuestionId;

            //
            // Make sure OneOfType is initialized.
            //
            ASSERT (OneOfType != (UINT8) -1);
            OneOfOptionMap->ValueType = OneOfType;
            InitializeListHead (&OneOfOptionMap->OneOfOptionMapEntryListHead);
            OneOfOptinMapEntryListHead = &OneOfOptionMap->OneOfOptionMapEntryListHead;
            InsertTailList (&FormSet->OneOfOptionMapListHead, &OneOfOptionMap->Link);
          }
          OneOfOptionMapEntry = AllocateZeroPool (sizeof (ONE_OF_OPTION_MAP_ENTRY));
          ASSERT (OneOfOptionMapEntry != NULL);

          OneOfOptionMapEntry->Signature = ONE_OF_OPTION_MAP_ENTRY_SIGNATURE;
          OneOfOptionMapEntry->FwKey = OptionMap->KeyValue;
          CopyMem (&OneOfOptionMapEntry->Value, &OptionMap->OptionValue, sizeof (EFI_IFR_TYPE_VALUE));
          
          InsertTailList (OneOfOptinMapEntryListHead, &OneOfOptionMapEntry->Link);
        }
     }
      break;

    //
    // Scope End
    //
    case EFI_IFR_END_OP:
      Status = PopScope (&ScopeOpCode);
      if (EFI_ERROR (Status)) {
        ResetScopeStack ();
        return Status;
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
        CurrentForm = NULL;
        break;

      case EFI_IFR_ONE_OF_OPTION_OP:
        //
        // End of Option
        //
        CurrentOption = NULL;
        break;

      case EFI_IFR_SUBTITLE_OP:
        mInScopeSubtitle = FALSE;
        break;

      case EFI_IFR_NO_SUBMIT_IF_OP:
      case EFI_IFR_INCONSISTENT_IF_OP:
        //
        // Ignore end of EFI_IFR_NO_SUBMIT_IF and EFI_IFR_INCONSISTENT_IF
        //
        break;

      case EFI_IFR_GRAY_OUT_IF_OP:
        mInScopeGrayOut = FALSE;
        break;

      default:
        if (IsExpressionOpCode (ScopeOpCode)) {
        }
        break;
      }
      break;

    default:
      break;
    }
  }

  return EFI_SUCCESS;
}




