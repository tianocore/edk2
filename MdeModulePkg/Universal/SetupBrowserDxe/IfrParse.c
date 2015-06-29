/** @file
Parser for IFR binary encoding.

Copyright (c) 2007 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Setup.h"

UINT16           mStatementIndex;
UINT16           mExpressionOpCodeIndex;
EFI_QUESTION_ID  mUsedQuestionId;
extern LIST_ENTRY      gBrowserStorageList;
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
  INTN                      ConditionalExprCount; 

  if (Form == NULL) {
    //
    // Only guid op may out side the form level.
    //
    ASSERT (((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode == EFI_IFR_GUID_OP);
  }

  Statement = &FormSet->StatementBuffer[mStatementIndex];
  mStatementIndex++;

  InitializeListHead (&Statement->DefaultListHead);
  InitializeListHead (&Statement->OptionListHead);
  InitializeListHead (&Statement->InconsistentListHead);
  InitializeListHead (&Statement->NoSubmitListHead);
  InitializeListHead (&Statement->WarningListHead);

  Statement->Signature = FORM_BROWSER_STATEMENT_SIGNATURE;

  Statement->Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;
  Statement->OpCode  = (EFI_IFR_OP_HEADER *) OpCodeData;

  StatementHdr = (EFI_IFR_STATEMENT_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
  CopyMem (&Statement->Prompt, &StatementHdr->Prompt, sizeof (EFI_STRING_ID));
  CopyMem (&Statement->Help, &StatementHdr->Help, sizeof (EFI_STRING_ID));

  ConditionalExprCount = GetConditionalExpressionCount(ExpressStatement);
  if (ConditionalExprCount > 0) {
    //
    // Form is inside of suppressif
    //
    
    Statement->Expression = (FORM_EXPRESSION_LIST *) AllocatePool( 
                                             (UINTN) (sizeof(FORM_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof(FORM_EXPRESSION *))));
    ASSERT (Statement->Expression != NULL);
    Statement->Expression->Count     = (UINTN) ConditionalExprCount;
    Statement->Expression->Signature = FORM_EXPRESSION_LIST_SIGNATURE;
    CopyMem (Statement->Expression->Expression, GetConditionalExpressionList(ExpressStatement), (UINTN) (sizeof (FORM_EXPRESSION *) * ConditionalExprCount));
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
  Convert a numeric value to a Unicode String and insert it to String Package.
  This string is used as the Unicode Name for the EFI Variable. This is to support
  the deprecated vareqval opcode.

  @param FormSet        The FormSet.
  @param Statement      The numeric question whose VarStoreInfo.VarName is the
                        numeric value which is used to produce the Unicode Name
                        for the EFI Variable.

  If the Statement is NULL, the ASSERT.
  If the opcode is not Numeric, then ASSERT.

  @retval EFI_SUCCESS The funtion always succeeds.
**/
EFI_STATUS
UpdateCheckBoxStringToken (
  IN CONST FORM_BROWSER_FORMSET *FormSet,
  IN       FORM_BROWSER_STATEMENT *Statement
  )
{
  CHAR16                  Str[MAXIMUM_VALUE_CHARACTERS];
  EFI_STRING_ID           Id;

  ASSERT (Statement != NULL);
  ASSERT (Statement->Operand == EFI_IFR_NUMERIC_OP);

  UnicodeValueToString (Str, 0, Statement->VarStoreInfo.VarName, MAXIMUM_VALUE_CHARACTERS - 1);

  Id = HiiSetString (FormSet->HiiHandle, 0, Str, NULL);
  if (Id == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  Statement->VarStoreInfo.VarName = Id;

  return EFI_SUCCESS;
}

/**
  Check if the next opcode is the EFI_IFR_EXTEND_OP_VAREQNAME.

  @param OpCodeData     The current opcode.

  @retval TRUE Yes.
  @retval FALSE No.
**/
BOOLEAN
IsNextOpCodeGuidedVarEqName (
  IN UINT8 *OpCodeData
  )
{
  //
  // Get next opcode
  //
  OpCodeData += ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
  if (*OpCodeData == EFI_IFR_GUID_OP) {
    if (CompareGuid (&gEfiIfrFrameworkGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
      //
      // Specific GUIDed opcodes to support IFR generated from Framework HII VFR
      //
      if ((((EFI_IFR_GUID_VAREQNAME *) OpCodeData)->ExtendOpCode) == EFI_IFR_EXTEND_OP_VAREQNAME) {
        return TRUE;
      }
    }
  }

  return FALSE;
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
  NAME_VALUE_NODE          *NameValueNode;
  EFI_STATUS               Status;
  BOOLEAN                  Find;

  Statement = CreateStatement (OpCodeData, FormSet, Form);
  if (Statement == NULL) {
    return NULL;
  }

  QuestionHdr = (EFI_IFR_QUESTION_HEADER *) (OpCodeData + sizeof (EFI_IFR_OP_HEADER));
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
  // Take a look at next OpCode to see whether it is a GUIDed opcode to support
  // Framework Compatibility
  //
  if (FeaturePcdGet (PcdFrameworkCompatibilitySupport)) {
    if ((*OpCodeData == EFI_IFR_NUMERIC_OP) && IsNextOpCodeGuidedVarEqName (OpCodeData)) {
      Status = UpdateCheckBoxStringToken (FormSet, Statement);
      if (EFI_ERROR (Status)) {
        return NULL;
      }
    }
  }

  //
  // Find Storage for this Question
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    Storage = FORMSET_STORAGE_FROM_LINK (Link);

    if (Storage->VarStoreId == Statement->VarStoreId) {
      Statement->Storage = Storage->BrowserStorage;
      break;
    }

    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }
  ASSERT (Statement->Storage != NULL);

  //
  // Initialilze varname for Name/Value or EFI Variable
  //
  if ((Statement->Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) ||
      (Statement->Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE)) {
    Statement->VariableName = GetToken (Statement->VarStoreInfo.VarName, FormSet->HiiHandle);
    ASSERT (Statement->VariableName != NULL);

    if (Statement->Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) {
      //
      // Check whether old string node already exist.
      //
      Find = FALSE;
      if (!IsListEmpty(&Statement->Storage->NameValueListHead)) {  
        Link = GetFirstNode (&Statement->Storage->NameValueListHead);
        while (!IsNull (&Statement->Storage->NameValueListHead, Link)) {
          NameValueNode = NAME_VALUE_NODE_FROM_LINK (Link);

          if (StrCmp (Statement->VariableName, NameValueNode->Name) == 0) {
            Find = TRUE;
            break;
          }

          Link = GetNextNode (&Statement->Storage->NameValueListHead, Link);
        }
      }

      if (!Find) {
        //
        // Insert to Name/Value varstore list
        //
        NameValueNode = AllocateZeroPool (sizeof (NAME_VALUE_NODE));
        ASSERT (NameValueNode != NULL);
        NameValueNode->Signature = NAME_VALUE_NODE_SIGNATURE;
        NameValueNode->Name = AllocateCopyPool (StrSize (Statement->VariableName), Statement->VariableName);
        ASSERT (NameValueNode->Name != NULL);
        NameValueNode->Value = AllocateZeroPool (0x10);
        ASSERT (NameValueNode->Value != NULL);
        NameValueNode->EditValue = AllocateZeroPool (0x10);
        ASSERT (NameValueNode->EditValue != NULL);

        InsertTailList (&Statement->Storage->NameValueListHead, &NameValueNode->Link);
      }
    }
  }

  return Statement;
}


/**
  Allocate a FORM_EXPRESSION node.

  @param  Form                   The Form associated with this Expression
  @param  OpCode                 The binary opcode data.

  @return Pointer to a FORM_EXPRESSION data structure.

**/
FORM_EXPRESSION *
CreateExpression (
  IN OUT FORM_BROWSER_FORM        *Form,
  IN     UINT8                    *OpCode
  )
{
  FORM_EXPRESSION  *Expression;

  Expression = AllocateZeroPool (sizeof (FORM_EXPRESSION));
  ASSERT (Expression != NULL);
  Expression->Signature = FORM_EXPRESSION_SIGNATURE;
  InitializeListHead (&Expression->OpCodeListHead);
  Expression->OpCode = (EFI_IFR_OP_HEADER *) OpCode;

  return Expression;
}

/**
  Create ConfigHdr string for a storage.

  @param  FormSet                Pointer of the current FormSet
  @param  Storage                Pointer of the storage

  @retval EFI_SUCCESS            Initialize ConfigHdr success

**/
EFI_STATUS
InitializeConfigHdr (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN OUT FORMSET_STORAGE   *Storage
  )
{
  CHAR16      *Name;

  if (Storage->BrowserStorage->Type == EFI_HII_VARSTORE_BUFFER || 
      Storage->BrowserStorage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER) {
    Name = Storage->BrowserStorage->Name;
  } else {
    Name = NULL;
  }

  Storage->ConfigHdr = HiiConstructConfigHdr (
                         &Storage->BrowserStorage->Guid,
                         Name,
                         FormSet->DriverHandle
                         );

  if (Storage->ConfigHdr == NULL) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Find the global storage link base on the input storate type, name and guid.

  For EFI_HII_VARSTORE_EFI_VARIABLE and EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER,
  same guid + name = same storage

  For EFI_HII_VARSTORE_NAME_VALUE:
  same guid + HiiHandle = same storage

  For EFI_HII_VARSTORE_BUFFER:
  same guid + name + HiiHandle = same storage

  @param  StorageType                Storage type.
  @param  StorageGuid                Storage guid.
  @param  StorageName                Storage Name.
  @param  HiiHandle                  HiiHandle for this varstore.

  @return Pointer to a GLOBAL_STORAGE data structure.

**/
BROWSER_STORAGE *
FindStorageInList (
  IN UINT8                 StorageType,
  IN EFI_GUID              *StorageGuid,
  IN CHAR16                *StorageName,
  IN EFI_HII_HANDLE        HiiHandle
  )
{
  LIST_ENTRY       *Link;
  BROWSER_STORAGE  *BrowserStorage;

  Link  = GetFirstNode (&gBrowserStorageList);
  while (!IsNull (&gBrowserStorageList, Link)) {
    BrowserStorage = BROWSER_STORAGE_FROM_LINK (Link);
    Link = GetNextNode (&gBrowserStorageList, Link);

    if ((BrowserStorage->Type == StorageType) && CompareGuid (&BrowserStorage->Guid, StorageGuid)) {
      if (StorageType == EFI_HII_VARSTORE_NAME_VALUE) {
        if (BrowserStorage->HiiHandle == HiiHandle) {
          return BrowserStorage;
        }

        continue;
      }

      ASSERT (StorageName != NULL);
      if (StrCmp (BrowserStorage->Name, StorageName) == 0) {
        if (StorageType == EFI_HII_VARSTORE_EFI_VARIABLE || StorageType == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER) {
          return BrowserStorage;
        } else if (StorageType == EFI_HII_VARSTORE_BUFFER && BrowserStorage->HiiHandle == HiiHandle) {
          return BrowserStorage;
        }
      }
    }
  }

  return NULL;
}

/**
  Intialize the Global Storage.

  @param  BrowserStorage              Pointer to the global storage.
  @param  StorageType                Storage type.
  @param  OpCodeData                 Binary data for this opcode.

**/
VOID
IntializeBrowserStorage (
  IN BROWSER_STORAGE       *BrowserStorage,
  IN UINT8                 StorageType,
  IN UINT8                 *OpCodeData
  )
{
  switch (StorageType) {
    case EFI_HII_VARSTORE_BUFFER:
      CopyMem (&BrowserStorage->Guid, &((EFI_IFR_VARSTORE *) OpCodeData)->Guid, sizeof (EFI_GUID));
      CopyMem (&BrowserStorage->Size, &((EFI_IFR_VARSTORE *) OpCodeData)->Size, sizeof (UINT16));

      BrowserStorage->Buffer     = AllocateZeroPool (BrowserStorage->Size);
      BrowserStorage->EditBuffer = AllocateZeroPool (BrowserStorage->Size);
      break;

    case EFI_HII_VARSTORE_EFI_VARIABLE:
    case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:
      CopyMem (&BrowserStorage->Guid,       &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Guid,       sizeof (EFI_GUID));
      CopyMem (&BrowserStorage->Attributes, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Attributes, sizeof (UINT32));
      CopyMem (&BrowserStorage->Size,       &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Size,       sizeof (UINT16));

      if (StorageType ==  EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER) {
        BrowserStorage->Buffer     = AllocateZeroPool (BrowserStorage->Size);
        BrowserStorage->EditBuffer = AllocateZeroPool (BrowserStorage->Size);
      }
      break;

    case EFI_HII_VARSTORE_NAME_VALUE:
      CopyMem (&BrowserStorage->Guid, &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->Guid, sizeof (EFI_GUID));

      InitializeListHead (&BrowserStorage->NameValueListHead);
      break;

    default:
      break;
  }
}

/**
  Check whether exist device path info in the ConfigHdr string.

  @param  String                 UEFI configuration string

  @retval TRUE                   Device Path exist.
  @retval FALSE                  Not exist device path info.

**/
BOOLEAN
IsDevicePathExist (
  IN  EFI_STRING                   String
  )
{
  UINTN                    Length;

  for (; (*String != 0 && StrnCmp (String, L"PATH=", StrLen (L"PATH=")) != 0); String++);
  if (*String == 0) {
    return FALSE;
  }

  String += StrLen (L"PATH=");
  if (*String == 0) {
    return FALSE;
  }

  for (Length = 0; *String != 0 && *String != L'&'; String++, Length++);
  if (((Length + 1) / 2) < sizeof (EFI_DEVICE_PATH_PROTOCOL)) {
    return FALSE;
  }

  return TRUE;
}

/**
  Allocate a FORMSET_STORAGE data structure and insert to FormSet Storage List.

  @param  FormSet                    Pointer of the current FormSet
  @param  StorageType                Storage type.
  @param  OpCodeData                 Binary data for this opcode.

  @return Pointer to a FORMSET_STORAGE data structure.

**/
FORMSET_STORAGE *
CreateStorage (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN UINT8                 StorageType,
  IN UINT8                 *OpCodeData
  )
{
  FORMSET_STORAGE         *Storage;
  CHAR16                  *UnicodeString;
  UINT16                  Index;
  BROWSER_STORAGE         *BrowserStorage;
  EFI_GUID                *StorageGuid;
  CHAR8                   *StorageName;

  UnicodeString = NULL;
  StorageName   = NULL;
  switch (StorageType) {
    case EFI_HII_VARSTORE_BUFFER:
      StorageGuid = (EFI_GUID *) (CHAR8*) &((EFI_IFR_VARSTORE *) OpCodeData)->Guid;
      StorageName = (CHAR8 *) ((EFI_IFR_VARSTORE *) OpCodeData)->Name;
      break;

    case EFI_HII_VARSTORE_EFI_VARIABLE:
    case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:
      StorageGuid = (EFI_GUID *) (CHAR8*) &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Guid;
      StorageName = (CHAR8 *) ((EFI_IFR_VARSTORE_EFI *) OpCodeData)->Name;
      break;

    default:
      ASSERT (StorageType == EFI_HII_VARSTORE_NAME_VALUE);
      StorageGuid = &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->Guid;
      break;
  }

  if (StorageType != EFI_HII_VARSTORE_NAME_VALUE) {
    ASSERT (StorageName != NULL);

    UnicodeString = AllocateZeroPool (AsciiStrSize (StorageName) * 2);
    ASSERT (UnicodeString != NULL);
    for (Index = 0; StorageName[Index] != 0; Index++) {
      UnicodeString[Index] = (CHAR16) StorageName[Index];
    }
  }

  Storage = AllocateZeroPool (sizeof (FORMSET_STORAGE));
  ASSERT (Storage != NULL);
  Storage->Signature = FORMSET_STORAGE_SIGNATURE;
  InsertTailList (&FormSet->StorageListHead, &Storage->Link);

  BrowserStorage = FindStorageInList(StorageType, StorageGuid, UnicodeString, FormSet->HiiHandle);
  if (BrowserStorage == NULL) {
    BrowserStorage = AllocateZeroPool (sizeof (BROWSER_STORAGE));
    ASSERT (BrowserStorage != NULL);

    BrowserStorage->Signature = BROWSER_STORAGE_SIGNATURE;
    InsertTailList (&gBrowserStorageList, &BrowserStorage->Link);

    IntializeBrowserStorage (BrowserStorage, StorageType, OpCodeData);
    BrowserStorage->Type = StorageType;
    if (StorageType != EFI_HII_VARSTORE_NAME_VALUE) {
      BrowserStorage->Name = UnicodeString;
    }

    BrowserStorage->HiiHandle = FormSet->HiiHandle;

    BrowserStorage->Initialized = FALSE;
  }

  Storage->BrowserStorage = BrowserStorage;
  InitializeConfigHdr (FormSet, Storage);
  Storage->ConfigRequest = AllocateCopyPool (StrSize (Storage->ConfigHdr), Storage->ConfigHdr);
  Storage->SpareStrLen = 0;

  return Storage;
}

/**
  Get Formset_storage base on the input varstoreid info.

  @param  FormSet                Pointer of the current FormSet.
  @param  VarStoreId             Varstore ID info.

  @return Pointer to a FORMSET_STORAGE data structure.

**/
FORMSET_STORAGE *
GetFstStgFromVarId (
  IN FORM_BROWSER_FORMSET  *FormSet,
  IN EFI_VARSTORE_ID       VarStoreId
  )
{
  FORMSET_STORAGE  *FormsetStorage;
  LIST_ENTRY       *Link;
  BOOLEAN          Found;

  Found = FALSE;
  FormsetStorage = NULL;
  //
  // Find Formset Storage for this Question
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    FormsetStorage = FORMSET_STORAGE_FROM_LINK (Link);

    if (FormsetStorage->VarStoreId == VarStoreId) {
      Found = TRUE;
      break;
    }

    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }

  return Found ? FormsetStorage : NULL;
}

/**
  Get Formset_storage base on the input browser storage.

  More than one formsets may share the same browser storage,
  this function just get the first formset storage which
  share the browser storage.

  @param  Storage              browser storage info.

  @return Pointer to a FORMSET_STORAGE data structure.
  

**/
FORMSET_STORAGE *
GetFstStgFromBrsStg (
  IN BROWSER_STORAGE       *Storage
  )
{
  FORMSET_STORAGE      *FormsetStorage;
  LIST_ENTRY           *Link;
  LIST_ENTRY           *FormsetLink;
  FORM_BROWSER_FORMSET *FormSet;
  BOOLEAN              Found;

  Found = FALSE;
  FormsetStorage = NULL;

  FormsetLink = GetFirstNode (&gBrowserFormSetList);
  while (!IsNull (&gBrowserFormSetList, FormsetLink)) {
    FormSet = FORM_BROWSER_FORMSET_FROM_LINK (FormsetLink);
    FormsetLink = GetNextNode (&gBrowserFormSetList, FormsetLink);

    Link = GetFirstNode (&FormSet->StorageListHead);
    while (!IsNull (&FormSet->StorageListHead, Link)) {
      FormsetStorage = FORMSET_STORAGE_FROM_LINK (Link);
      Link = GetNextNode (&FormSet->StorageListHead, Link);

      if (FormsetStorage->BrowserStorage == Storage) {
        Found = TRUE;
        break;
      }
    }

    if (Found) {
      break;
    }
  }

  return Found ? FormsetStorage : NULL;
}

/**
  Initialize Request Element of a Question. <RequestElement> ::= '&'<BlockName> | '&'<Label>

  @param  FormSet                Pointer of the current FormSet.
  @param  Question               The Question to be initialized.
  @param  Form                   Pointer of the current form.

  @retval EFI_SUCCESS            Function success.
  @retval EFI_INVALID_PARAMETER  No storage associated with the Question.

**/
EFI_STATUS
InitializeRequestElement (
  IN OUT FORM_BROWSER_FORMSET     *FormSet,
  IN OUT FORM_BROWSER_STATEMENT   *Question,
  IN OUT FORM_BROWSER_FORM        *Form
  )
{
  BROWSER_STORAGE  *Storage;
  FORMSET_STORAGE  *FormsetStorage;
  UINTN            StrLen;
  UINTN            StringSize;
  CHAR16           *NewStr;
  CHAR16           RequestElement[30];
  LIST_ENTRY       *Link;
  BOOLEAN          Find;
  FORM_BROWSER_CONFIG_REQUEST  *ConfigInfo;
  UINTN            MaxLen;

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
  if (Storage->Type == EFI_HII_VARSTORE_BUFFER || 
      Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER) {
    StrLen = UnicodeSPrint (
               RequestElement,
               30 * sizeof (CHAR16),
               L"&OFFSET=%04x&WIDTH=%04x",
               Question->VarStoreInfo.VarOffset,
               Question->StorageWidth
               );
    Question->BlockName = AllocateCopyPool ((StrLen + 1) * sizeof (CHAR16), RequestElement);
  } else {
    StrLen = UnicodeSPrint (RequestElement, 30 * sizeof (CHAR16), L"&%s", Question->VariableName);
  }

  if ((Question->Operand == EFI_IFR_PASSWORD_OP) && ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) == EFI_IFR_FLAG_CALLBACK)) {
    //
    // Password with CALLBACK flag is stored in encoded format,
    // so don't need to append it to <ConfigRequest>
    //
    return EFI_SUCCESS;
  }

  //
  // Find Formset Storage for this Question
  //
  FormsetStorage = GetFstStgFromVarId(FormSet, Question->VarStoreId);
  ASSERT (FormsetStorage != NULL);
  StringSize = (FormsetStorage->ConfigRequest != NULL) ? StrSize (FormsetStorage->ConfigRequest) : sizeof (CHAR16);
  MaxLen = StringSize / sizeof (CHAR16) + FormsetStorage->SpareStrLen;

  //
  // Append <RequestElement> to <ConfigRequest>
  //
  if (StrLen > FormsetStorage->SpareStrLen) {
    //
    // Old String buffer is not sufficient for RequestElement, allocate a new one
    //
    MaxLen = StringSize / sizeof (CHAR16) + CONFIG_REQUEST_STRING_INCREMENTAL;
    NewStr = AllocateZeroPool (MaxLen * sizeof (CHAR16));
    ASSERT (NewStr != NULL);
    if (FormsetStorage->ConfigRequest != NULL) {
      CopyMem (NewStr, FormsetStorage->ConfigRequest, StringSize);
      FreePool (FormsetStorage->ConfigRequest);
    }
    FormsetStorage->ConfigRequest = NewStr;
    FormsetStorage->SpareStrLen   = CONFIG_REQUEST_STRING_INCREMENTAL;
  }

  StrCatS (FormsetStorage->ConfigRequest, MaxLen, RequestElement);
  FormsetStorage->ElementCount++;
  FormsetStorage->SpareStrLen -= StrLen;

  //
  // Update the Config Request info saved in the form.
  //
  ConfigInfo = NULL;
  Find       = FALSE;
  Link = GetFirstNode (&Form->ConfigRequestHead);
  while (!IsNull (&Form->ConfigRequestHead, Link)) {
    ConfigInfo = FORM_BROWSER_CONFIG_REQUEST_FROM_LINK (Link);

    if (ConfigInfo != NULL && ConfigInfo->Storage == FormsetStorage->BrowserStorage) {
      Find = TRUE;
      break;
    }

    Link = GetNextNode (&Form->ConfigRequestHead, Link);
  }

  if (!Find) {
    ConfigInfo = AllocateZeroPool(sizeof (FORM_BROWSER_CONFIG_REQUEST));
    ASSERT (ConfigInfo != NULL);
    ConfigInfo->Signature     = FORM_BROWSER_CONFIG_REQUEST_SIGNATURE;
    ConfigInfo->ConfigRequest = AllocateCopyPool (StrSize (FormsetStorage->ConfigHdr), FormsetStorage->ConfigHdr);
    ASSERT (ConfigInfo->ConfigRequest != NULL);
    ConfigInfo->SpareStrLen   = 0;
    ConfigInfo->Storage       = FormsetStorage->BrowserStorage;
    InsertTailList(&Form->ConfigRequestHead, &ConfigInfo->Link);
  }
  StringSize = (ConfigInfo->ConfigRequest != NULL) ? StrSize (ConfigInfo->ConfigRequest) : sizeof (CHAR16);
  MaxLen = StringSize / sizeof (CHAR16) + ConfigInfo->SpareStrLen;

  //
  // Append <RequestElement> to <ConfigRequest>
  //
  if (StrLen > ConfigInfo->SpareStrLen) {
    //
    // Old String buffer is not sufficient for RequestElement, allocate a new one
    //
    MaxLen = StringSize / sizeof (CHAR16) + CONFIG_REQUEST_STRING_INCREMENTAL;
    NewStr = AllocateZeroPool (MaxLen * sizeof (CHAR16));
    ASSERT (NewStr != NULL);
    if (ConfigInfo->ConfigRequest != NULL) {
      CopyMem (NewStr, ConfigInfo->ConfigRequest, StringSize);
      FreePool (ConfigInfo->ConfigRequest);
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

  @param  FormSet                Pointer of the Expression

**/
VOID
DestroyExpression (
  IN FORM_EXPRESSION   *Expression
  )
{
  LIST_ENTRY         *Link;
  EXPRESSION_OPCODE  *OpCode;
  LIST_ENTRY         *SubExpressionLink;
  FORM_EXPRESSION    *SubExpression;

  while (!IsListEmpty (&Expression->OpCodeListHead)) {
    Link = GetFirstNode (&Expression->OpCodeListHead);
    OpCode = EXPRESSION_OPCODE_FROM_LINK (Link);
    RemoveEntryList (&OpCode->Link);

    if (OpCode->ValueList != NULL) {
      FreePool (OpCode->ValueList);
    }

    if (OpCode->ValueName != NULL) {
      FreePool (OpCode->ValueName);
    }

    if (OpCode->MapExpressionList.ForwardLink != NULL) {
      while (!IsListEmpty (&OpCode->MapExpressionList)) {
        SubExpressionLink = GetFirstNode(&OpCode->MapExpressionList);
        SubExpression     = FORM_EXPRESSION_FROM_LINK (SubExpressionLink);
        RemoveEntryList(&SubExpression->Link);
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
  Free resources of a storage.

  @param  Storage                Pointer of the storage

**/
VOID
DestroyStorage (
  IN FORMSET_STORAGE   *Storage
  )
{
  if (Storage == NULL) {
    return;
  }

  if (Storage->ConfigRequest != NULL) {
    FreePool (Storage->ConfigRequest);
  }

  FreePool (Storage);
}


/**
  Free resources of a Statement.

  @param  FormSet                Pointer of the FormSet
  @param  Statement              Pointer of the Statement

**/
VOID
DestroyStatement (
  IN     FORM_BROWSER_FORMSET    *FormSet,
  IN OUT FORM_BROWSER_STATEMENT  *Statement
  )
{
  LIST_ENTRY        *Link;
  QUESTION_DEFAULT  *Default;
  QUESTION_OPTION   *Option;
  FORM_EXPRESSION   *Expression;

  //
  // Free Default value List
  //
  while (!IsListEmpty (&Statement->DefaultListHead)) {
    Link = GetFirstNode (&Statement->DefaultListHead);
    Default = QUESTION_DEFAULT_FROM_LINK (Link);
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
    Link = GetFirstNode (&Statement->OptionListHead);
    Option = QUESTION_OPTION_FROM_LINK (Link);
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
    Link = GetFirstNode (&Statement->InconsistentListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free NoSubmit List
  //
  while (!IsListEmpty (&Statement->NoSubmitListHead)) {
    Link = GetFirstNode (&Statement->NoSubmitListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free WarningIf List
  //
  while (!IsListEmpty (&Statement->WarningListHead)) {
    Link = GetFirstNode (&Statement->WarningListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  if (Statement->Expression != NULL) {
    FreePool (Statement->Expression);
  }

  if (Statement->VariableName != NULL) {
    FreePool (Statement->VariableName);
  }
  if (Statement->BlockName != NULL) {
    FreePool (Statement->BlockName);
  }
  if (Statement->BufferValue != NULL) {
    FreePool (Statement->BufferValue);
  }
  if (Statement->Operand == EFI_IFR_STRING_OP || Statement->Operand == EFI_IFR_PASSWORD_OP) {
    DeleteString(Statement->HiiValue.Value.string, FormSet->HiiHandle);
  }
}


/**
  Free resources of a Form.

  @param  FormSet                Pointer of the FormSet
  @param  Form                   Pointer of the Form.

**/
VOID
DestroyForm (
  IN     FORM_BROWSER_FORMSET  *FormSet,
  IN OUT FORM_BROWSER_FORM     *Form
  )
{
  LIST_ENTRY              *Link;
  FORM_EXPRESSION         *Expression;
  FORM_BROWSER_STATEMENT  *Statement;
  FORM_BROWSER_CONFIG_REQUEST  *ConfigInfo;

  //
  // Free Form Expressions
  //
  while (!IsListEmpty (&Form->ExpressionListHead)) {
    Link = GetFirstNode (&Form->ExpressionListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free Statements/Questions
  //
  while (!IsListEmpty (&Form->StatementListHead)) {
    Link = GetFirstNode (&Form->StatementListHead);
    Statement = FORM_BROWSER_STATEMENT_FROM_LINK (Link);
    RemoveEntryList (&Statement->Link);

    DestroyStatement (FormSet, Statement);
  }

  //
  // Free ConfigRequest string.
  //
  while (!IsListEmpty (&Form->ConfigRequestHead)) {
    Link = GetFirstNode (&Form->ConfigRequestHead);
    ConfigInfo = FORM_BROWSER_CONFIG_REQUEST_FROM_LINK (Link);
    RemoveEntryList (&ConfigInfo->Link);

    FreePool (ConfigInfo->ConfigRequest);
    FreePool (ConfigInfo);
  }

  if (Form->SuppressExpression != NULL) {
    FreePool (Form->SuppressExpression);
  }

  UiFreeMenuList (&Form->FormViewListHead);

  //
  // Free this Form
  //
  FreePool (Form);
}


/**
  Free resources allocated for a FormSet.

  @param  FormSet                Pointer of the FormSet

**/
VOID
DestroyFormSet (
  IN OUT FORM_BROWSER_FORMSET  *FormSet
  )
{
  LIST_ENTRY            *Link;
  FORMSET_STORAGE       *Storage;
  FORMSET_DEFAULTSTORE  *DefaultStore;
  FORM_EXPRESSION       *Expression;
  FORM_BROWSER_FORM     *Form;

  if (FormSet->IfrBinaryData == NULL) {
    //
    // Uninitialized FormSet
    //
    FreePool (FormSet);
    return;
  }

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

      FreePool (DefaultStore);
    }
  }

  //
  // Free Formset Expressions
  //
  while (!IsListEmpty (&FormSet->ExpressionListHead)) {
    Link = GetFirstNode (&FormSet->ExpressionListHead);
    Expression = FORM_EXPRESSION_FROM_LINK (Link);
    RemoveEntryList (&Expression->Link);

    DestroyExpression (Expression);
  }

  //
  // Free Forms
  //
  if (FormSet->FormListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->FormListHead)) {
      Link = GetFirstNode (&FormSet->FormListHead);
      Form = FORM_BROWSER_FORM_FROM_LINK (Link);
      RemoveEntryList (&Form->Link);

      DestroyForm (FormSet, Form);
    }
  }

  if (FormSet->StatementBuffer != NULL) {
    FreePool (FormSet->StatementBuffer);
  }
  if (FormSet->ExpressionBuffer != NULL) {
    FreePool (FormSet->ExpressionBuffer);
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
      (Operand == EFI_IFR_SECURITY_OP) ||
      (Operand == EFI_IFR_MATCH2_OP)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Tell whether this Operand is an Statement OpCode.

  @param  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Statement OpCode.
  @retval FALSE                  Not an Statement OpCode.

**/
BOOLEAN
IsStatementOpCode (
  IN UINT8              Operand
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
      (Operand == EFI_IFR_ONE_OF_OP)) {
    return TRUE;
  } else {
    return FALSE;
  }
}

/**
  Tell whether this Operand is an known OpCode.

  @param  Operand                Operand of an IFR OpCode.

  @retval TRUE                   This is an Statement OpCode.
  @retval FALSE                  Not an Statement OpCode.

**/
BOOLEAN
IsUnKnownOpCode (
  IN UINT8              Operand
  )
{
  return Operand > EFI_IFR_MATCH2_OP ? TRUE : FALSE;
}

/**
  Calculate number of Statemens(Questions) and Expression OpCodes.

  @param  FormSet                The FormSet to be counted.
  @param  NumberOfStatement      Number of Statemens(Questions)
  @param  NumberOfExpression     Number of Expression OpCodes

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
  FORM_BROWSER_FORM       *CurrentForm;
  FORM_BROWSER_STATEMENT  *CurrentStatement;
  FORM_BROWSER_STATEMENT  *ParentStatement;
  EXPRESSION_OPCODE       *ExpressionOpCode;
  FORM_EXPRESSION         *CurrentExpression;
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
  UINT8                   Width;
  UINT16                  NumberOfStatement;
  UINT16                  NumberOfExpression;
  EFI_IMAGE_ID            *ImageId;
  BOOLEAN                 SuppressForQuestion;
  BOOLEAN                 SuppressForOption;
  UINT16                  DepthOfDisable;
  BOOLEAN                 OpCodeDisabled;
  BOOLEAN                 SingleOpCodeExpression;
  BOOLEAN                 InScopeDefault;
  EFI_HII_VALUE           *Value;
  EFI_IFR_FORM_MAP_METHOD *MapMethod;
  UINT8                   MapScopeDepth;
  LIST_ENTRY              *Link;
  FORMSET_STORAGE         *VarStorage;
  LIST_ENTRY              *MapExpressionList;
  EFI_VARSTORE_ID         TempVarstoreId;
  BOOLEAN                 InScopeDisable;
  INTN                    ConditionalExprCount;
  BOOLEAN                 InUnknownScope;
  UINT8                   UnknownDepth;

  SuppressForQuestion      = FALSE;
  SuppressForOption        = FALSE;
  InScopeDisable           = FALSE;
  DepthOfDisable           = 0;
  OpCodeDisabled           = FALSE;
  SingleOpCodeExpression   = FALSE;
  InScopeDefault           = FALSE;
  CurrentExpression        = NULL;
  CurrentDefault           = NULL;
  CurrentOption            = NULL;
  ImageId                  = NULL;
  MapMethod                = NULL;
  MapScopeDepth            = 0;
  Link                     = NULL;
  VarStorage               = NULL;
  MapExpressionList        = NULL;
  TempVarstoreId           = 0;
  ConditionalExprCount     = 0;
  InUnknownScope           = FALSE;
  UnknownDepth             = 0;

  //
  // Get the number of Statements and Expressions
  //
  CountOpCodes (FormSet, &NumberOfStatement, &NumberOfExpression);

  mStatementIndex = 0;
  mUsedQuestionId = 1;
  FormSet->StatementBuffer = AllocateZeroPool (NumberOfStatement * sizeof (FORM_BROWSER_STATEMENT));
  if (FormSet->StatementBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mExpressionOpCodeIndex = 0;
  FormSet->ExpressionBuffer = AllocateZeroPool (NumberOfExpression * sizeof (EXPRESSION_OPCODE));
  if (FormSet->ExpressionBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  InitializeListHead (&FormSet->StatementListOSF);
  InitializeListHead (&FormSet->StorageListHead);
  InitializeListHead (&FormSet->SaveFailStorageListHead);
  InitializeListHead (&FormSet->DefaultStoreListHead);
  InitializeListHead (&FormSet->FormListHead);
  InitializeListHead (&FormSet->ExpressionListHead);
  ResetCurrentExpressionStack ();
  ResetMapExpressionListStack ();

  CurrentForm = NULL;
  CurrentStatement = NULL;
  ParentStatement  = NULL;

  ResetScopeStack ();

  OpCodeOffset = 0;
  while (OpCodeOffset < FormSet->IfrBinaryLength) {
    OpCodeData = FormSet->IfrBinaryData + OpCodeOffset;

    OpCodeLength = ((EFI_IFR_OP_HEADER *) OpCodeData)->Length;
    OpCodeOffset += OpCodeLength;
    Operand = ((EFI_IFR_OP_HEADER *) OpCodeData)->OpCode;
    Scope = ((EFI_IFR_OP_HEADER *) OpCodeData)->Scope;

    if (InUnknownScope) {
      if (Operand == EFI_IFR_END_OP) {
        UnknownDepth --;

        if (UnknownDepth == 0) {
          InUnknownScope = FALSE;
        }
      } else {
        if (Scope != 0) {
          UnknownDepth ++;
        }
      }

      continue;
    }

    if (IsUnKnownOpCode(Operand)) {
      if (Scope != 0) {
        InUnknownScope = TRUE;
        UnknownDepth ++;
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
      ExpressionOpCode = &FormSet->ExpressionBuffer[mExpressionOpCodeIndex];
      mExpressionOpCodeIndex++;

      ExpressionOpCode->Signature = EXPRESSION_OPCODE_SIGNATURE;
      ExpressionOpCode->Operand = Operand;
      Value = &ExpressionOpCode->Value;

      switch (Operand) {
      case EFI_IFR_EQ_ID_VAL_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_VAL *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        CopyMem (&Value->Value.u16, &((EFI_IFR_EQ_ID_VAL *) OpCodeData)->Value, sizeof (UINT16));
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_ID *) OpCodeData)->QuestionId1, sizeof (EFI_QUESTION_ID));
        CopyMem (&ExpressionOpCode->QuestionId2, &((EFI_IFR_EQ_ID_ID *) OpCodeData)->QuestionId2, sizeof (EFI_QUESTION_ID));
        break;

      case EFI_IFR_EQ_ID_VAL_LIST_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));
        CopyMem (&ExpressionOpCode->ListLength, &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->ListLength, sizeof (UINT16));
        ExpressionOpCode->ValueList = AllocateCopyPool (ExpressionOpCode->ListLength * sizeof (UINT16), &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->ValueList);
        break;

      case EFI_IFR_TO_STRING_OP:
      case EFI_IFR_FIND_OP:
        ExpressionOpCode->Format = (( EFI_IFR_TO_STRING *) OpCodeData)->Format;
        break;

      case EFI_IFR_STRING_REF1_OP:
        Value->Type = EFI_IFR_TYPE_STRING;
        CopyMem (&Value->Value.string, &(( EFI_IFR_STRING_REF1 *) OpCodeData)->StringId, sizeof (EFI_STRING_ID));
        break;

      case EFI_IFR_RULE_REF_OP:
        ExpressionOpCode->RuleId = (( EFI_IFR_RULE_REF *) OpCodeData)->RuleId;
        break;

      case EFI_IFR_SPAN_OP:
        ExpressionOpCode->Flags = (( EFI_IFR_SPAN *) OpCodeData)->Flags;
        break;

      case EFI_IFR_THIS_OP:
        ASSERT (ParentStatement != NULL);
        ExpressionOpCode->QuestionId = ParentStatement->QuestionId;
        break;

      case EFI_IFR_SECURITY_OP:
        CopyMem (&ExpressionOpCode->Guid, &((EFI_IFR_SECURITY *) OpCodeData)->Permissions, sizeof (EFI_GUID));
        break;

      case EFI_IFR_MATCH2_OP:
        CopyMem (&ExpressionOpCode->Guid, &((EFI_IFR_MATCH2 *) OpCodeData)->SyntaxType, sizeof (EFI_GUID));
        break;

      case EFI_IFR_GET_OP:
      case EFI_IFR_SET_OP:
        CopyMem (&TempVarstoreId, &((EFI_IFR_GET *) OpCodeData)->VarStoreId, sizeof (TempVarstoreId));
        if (TempVarstoreId != 0) {
          if (FormSet->StorageListHead.ForwardLink != NULL) {
            Link = GetFirstNode (&FormSet->StorageListHead);
            while (!IsNull (&FormSet->StorageListHead, Link)) {
              VarStorage = FORMSET_STORAGE_FROM_LINK (Link);
              if (VarStorage->VarStoreId == ((EFI_IFR_GET *) OpCodeData)->VarStoreId) {
                ExpressionOpCode->VarStorage = VarStorage->BrowserStorage;
                break;
              }
              Link = GetNextNode (&FormSet->StorageListHead, Link);
            }
          }
          if (ExpressionOpCode->VarStorage == NULL) {
            //
            // VarStorage is not found.
            //
            return EFI_INVALID_PARAMETER;
          }
        }
        ExpressionOpCode->ValueType = ((EFI_IFR_GET *) OpCodeData)->VarStoreType;
        switch (ExpressionOpCode->ValueType) {
        case EFI_IFR_TYPE_BOOLEAN:
        case EFI_IFR_TYPE_NUM_SIZE_8: 
          ExpressionOpCode->ValueWidth = 1;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_16:
        case EFI_IFR_TYPE_STRING:
          ExpressionOpCode->ValueWidth = 2;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_32:
          ExpressionOpCode->ValueWidth = 4;
          break;

        case EFI_IFR_TYPE_NUM_SIZE_64:
          ExpressionOpCode->ValueWidth = 8;
          break;

        case EFI_IFR_TYPE_DATE:
          ExpressionOpCode->ValueWidth = (UINT8) sizeof (EFI_IFR_DATE);
          break;

        case EFI_IFR_TYPE_TIME:
          ExpressionOpCode->ValueWidth = (UINT8) sizeof (EFI_IFR_TIME);
          break;

        case EFI_IFR_TYPE_REF:
          ExpressionOpCode->ValueWidth = (UINT8) sizeof (EFI_IFR_REF);
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
        CopyMem (&ExpressionOpCode->VarStoreInfo.VarName,   &((EFI_IFR_GET *) OpCodeData)->VarStoreInfo.VarName,   sizeof (EFI_STRING_ID));
        CopyMem (&ExpressionOpCode->VarStoreInfo.VarOffset, &((EFI_IFR_GET *) OpCodeData)->VarStoreInfo.VarOffset, sizeof (UINT16));
        if ((ExpressionOpCode->VarStorage != NULL) && 
            (ExpressionOpCode->VarStorage->Type == EFI_HII_VARSTORE_NAME_VALUE || 
             ExpressionOpCode->VarStorage->Type == EFI_HII_VARSTORE_EFI_VARIABLE)) {
          ExpressionOpCode->ValueName = GetToken (ExpressionOpCode->VarStoreInfo.VarName, FormSet->HiiHandle);
          if (ExpressionOpCode->ValueName == NULL) {
            //
            // String ID is invalid.
            //
            return EFI_INVALID_PARAMETER;
          }
        }
        break;

      case EFI_IFR_QUESTION_REF1_OP:
        CopyMem (&ExpressionOpCode->QuestionId, &((EFI_IFR_EQ_ID_VAL_LIST *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));
        break;

      case EFI_IFR_QUESTION_REF3_OP:
        if (OpCodeLength >= sizeof (EFI_IFR_QUESTION_REF3_2)) {
          CopyMem (&ExpressionOpCode->DevicePath, &(( EFI_IFR_QUESTION_REF3_2 *) OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));

          if (OpCodeLength >= sizeof (EFI_IFR_QUESTION_REF3_3)) {
            CopyMem (&ExpressionOpCode->Guid, &(( EFI_IFR_QUESTION_REF3_3 *) OpCodeData)->Guid, sizeof (EFI_GUID));
          }
        }
        break;

      //
      // constant
      //
      case EFI_IFR_TRUE_OP:
        Value->Type = EFI_IFR_TYPE_BOOLEAN;
        Value->Value.b = TRUE;
        break;

      case EFI_IFR_FALSE_OP:
        Value->Type = EFI_IFR_TYPE_BOOLEAN;
        Value->Value.b = FALSE;
        break;

      case EFI_IFR_ONE_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        Value->Value.u8 = 1;
        break;

      case EFI_IFR_ZERO_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        Value->Value.u8 = 0;
        break;

      case EFI_IFR_ONES_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
        Value->Value.u64 = 0xffffffffffffffffULL;
        break;

      case EFI_IFR_UINT8_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_8;
        Value->Value.u8 = (( EFI_IFR_UINT8 *) OpCodeData)->Value;
        break;

      case EFI_IFR_UINT16_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        CopyMem (&Value->Value.u16, &(( EFI_IFR_UINT16 *) OpCodeData)->Value, sizeof (UINT16));
        break;

      case EFI_IFR_UINT32_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_32;
        CopyMem (&Value->Value.u32, &(( EFI_IFR_UINT32 *) OpCodeData)->Value, sizeof (UINT32));
        break;

      case EFI_IFR_UINT64_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;
        CopyMem (&Value->Value.u64, &(( EFI_IFR_UINT64 *) OpCodeData)->Value, sizeof (UINT64));
        break;

      case EFI_IFR_UNDEFINED_OP:
        Value->Type = EFI_IFR_TYPE_UNDEFINED;
        break;

      case EFI_IFR_VERSION_OP:
        Value->Type = EFI_IFR_TYPE_NUM_SIZE_16;
        Value->Value.u16 = EFI_IFR_SPECIFICATION_VERSION;
        break;

      default:
        break;
      }
      //
      // Create sub expression nested in MAP opcode
      //
      if (CurrentExpression == NULL && MapScopeDepth > 0) {
        CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
        ASSERT (MapExpressionList != NULL);
        InsertTailList (MapExpressionList, &CurrentExpression->Link);
        if (Scope == 0) {
          SingleOpCodeExpression = TRUE;
        }
      }
      ASSERT (CurrentExpression != NULL);
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
        MapScopeDepth ++;
      } else if (SingleOpCodeExpression) {
        //
        // There are two cases to indicate the end of an Expression:
        // for single OpCode expression: one Expression OpCode
        // for expression consists of more than one OpCode: EFI_IFR_END
        //
        SingleOpCodeExpression = FALSE;

        if (InScopeDisable && CurrentForm == NULL) {
          //
          // This is DisableIf expression for Form, it should be a constant expression
          //
          Status = EvaluateExpression (FormSet, CurrentForm, CurrentExpression);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          OpCodeDisabled = IsTrue(&CurrentExpression->Result);
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
      if (CompareMem (&FormSet->Guid, &((EFI_IFR_FORM_SET *) OpCodeData)->Guid, sizeof (EFI_GUID)) != 0) {
        return EFI_INVALID_PARAMETER;
      }

      CopyMem (&FormSet->FormSetTitle, &((EFI_IFR_FORM_SET *) OpCodeData)->FormSetTitle, sizeof (EFI_STRING_ID));
      CopyMem (&FormSet->Help,         &((EFI_IFR_FORM_SET *) OpCodeData)->Help,         sizeof (EFI_STRING_ID));
      FormSet->OpCode = (EFI_IFR_OP_HEADER *) OpCodeData;//save the opcode address of formset

      if (OpCodeLength > OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
        //
        // The formset OpCode contains ClassGuid
        //
        FormSet->NumberOfClassGuid = (UINT8) (((EFI_IFR_FORM_SET *) OpCodeData)->Flags & 0x3);
        CopyMem (FormSet->ClassGuid, OpCodeData + sizeof (EFI_IFR_FORM_SET), FormSet->NumberOfClassGuid * sizeof (EFI_GUID));
      }
      break;

    case EFI_IFR_FORM_OP:
      //
      // Create a new Form for this FormSet
      //
      CurrentForm = AllocateZeroPool (sizeof (FORM_BROWSER_FORM));
      ASSERT (CurrentForm != NULL);
      CurrentForm->Signature = FORM_BROWSER_FORM_SIGNATURE;
      InitializeListHead (&CurrentForm->ExpressionListHead);
      InitializeListHead (&CurrentForm->StatementListHead);
      InitializeListHead (&CurrentForm->ConfigRequestHead);
      InitializeListHead (&CurrentForm->FormViewListHead);

      CurrentForm->FormType = STANDARD_MAP_FORM_TYPE;
      CopyMem (&CurrentForm->FormId,    &((EFI_IFR_FORM *) OpCodeData)->FormId,    sizeof (UINT16));
      CopyMem (&CurrentForm->FormTitle, &((EFI_IFR_FORM *) OpCodeData)->FormTitle, sizeof (EFI_STRING_ID));

      ConditionalExprCount = GetConditionalExpressionCount(ExpressForm);
      if ( ConditionalExprCount > 0) {
        //
        // Form is inside of suppressif
        //
        CurrentForm->SuppressExpression = (FORM_EXPRESSION_LIST *) AllocatePool( 
                                                 (UINTN) (sizeof(FORM_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof(FORM_EXPRESSION *))));
        ASSERT (CurrentForm->SuppressExpression != NULL);
        CurrentForm->SuppressExpression->Count     = (UINTN) ConditionalExprCount;
        CurrentForm->SuppressExpression->Signature = FORM_EXPRESSION_LIST_SIGNATURE;
        CopyMem (CurrentForm->SuppressExpression->Expression, GetConditionalExpressionList(ExpressForm), (UINTN) (sizeof (FORM_EXPRESSION *) * ConditionalExprCount));
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
      CurrentForm = AllocateZeroPool (sizeof (FORM_BROWSER_FORM));
      ASSERT (CurrentForm != NULL);
      CurrentForm->Signature = FORM_BROWSER_FORM_SIGNATURE;
      InitializeListHead (&CurrentForm->ExpressionListHead);
      InitializeListHead (&CurrentForm->StatementListHead);
      InitializeListHead (&CurrentForm->ConfigRequestHead);
      InitializeListHead (&CurrentForm->FormViewListHead);

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
          CurrentForm->FormType = STANDARD_MAP_FORM_TYPE;
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

      ConditionalExprCount = GetConditionalExpressionCount(ExpressForm);
      if ( ConditionalExprCount > 0) {
        //
        // Form is inside of suppressif
        //
        CurrentForm->SuppressExpression = (FORM_EXPRESSION_LIST *) AllocatePool( 
                                                 (UINTN) (sizeof(FORM_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof(FORM_EXPRESSION *))));
        ASSERT (CurrentForm->SuppressExpression != NULL);
        CurrentForm->SuppressExpression->Count     = (UINTN) ConditionalExprCount;
        CurrentForm->SuppressExpression->Signature = FORM_EXPRESSION_LIST_SIGNATURE;
        CopyMem (CurrentForm->SuppressExpression->Expression, GetConditionalExpressionList(ExpressForm), (UINTN) (sizeof (FORM_EXPRESSION *) * ConditionalExprCount));
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
      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
      break;

    case EFI_IFR_VARSTORE_NAME_VALUE_OP:
      //
      // Create a name/value Storage for this FormSet
      //
      Storage = CreateStorage (FormSet, EFI_HII_VARSTORE_NAME_VALUE, OpCodeData);
      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_NAME_VALUE *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
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
      CopyMem (&Storage->VarStoreId, &((EFI_IFR_VARSTORE_EFI *) OpCodeData)->VarStoreId, sizeof (EFI_VARSTORE_ID));
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
      CurrentStatement->FakeQuestionId = mUsedQuestionId++;
      break;

    case EFI_IFR_TEXT_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->FakeQuestionId = mUsedQuestionId++;
      CopyMem (&CurrentStatement->TextTwo, &((EFI_IFR_TEXT *) OpCodeData)->TextTwo, sizeof (EFI_STRING_ID));
      break;

    case EFI_IFR_RESET_BUTTON_OP:
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->FakeQuestionId = mUsedQuestionId++;
      CopyMem (&CurrentStatement->DefaultId, &((EFI_IFR_RESET_BUTTON *) OpCodeData)->DefaultId, sizeof (EFI_DEFAULT_ID));
      break;

    //
    // Questions
    //
    case EFI_IFR_ACTION_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_ACTION;

      if (OpCodeLength == sizeof (EFI_IFR_ACTION_1)) {
        //
        // No QuestionConfig present, so no configuration string will be processed
        //
        CurrentStatement->QuestionConfig = 0;
      } else {
        CopyMem (&CurrentStatement->QuestionConfig, &((EFI_IFR_ACTION *) OpCodeData)->QuestionConfig, sizeof (EFI_STRING_ID));
      }
      break;

    case EFI_IFR_REF_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT (CurrentStatement != NULL);
      Value = &CurrentStatement->HiiValue;
      Value->Type = EFI_IFR_TYPE_REF;
      if (OpCodeLength >= sizeof (EFI_IFR_REF)) {
        CopyMem (&Value->Value.ref.FormId, &((EFI_IFR_REF *) OpCodeData)->FormId, sizeof (EFI_FORM_ID));

        if (OpCodeLength >= sizeof (EFI_IFR_REF2)) {
          CopyMem (&Value->Value.ref.QuestionId, &((EFI_IFR_REF2 *) OpCodeData)->QuestionId, sizeof (EFI_QUESTION_ID));

          if (OpCodeLength >= sizeof (EFI_IFR_REF3)) {
            CopyMem (&Value->Value.ref.FormSetGuid, &((EFI_IFR_REF3 *) OpCodeData)->FormSetId, sizeof (EFI_GUID));

            if (OpCodeLength >= sizeof (EFI_IFR_REF4)) {
              CopyMem (&Value->Value.ref.DevicePath, &((EFI_IFR_REF4 *) OpCodeData)->DevicePath, sizeof (EFI_STRING_ID));
            }
          }
        }
      }
      CurrentStatement->StorageWidth = (UINT16) sizeof (EFI_HII_REF);        
      InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_NUMERIC_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

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

      InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);

      if ((Operand == EFI_IFR_ONE_OF_OP) && Scope != 0) {
        SuppressForOption = TRUE;
      }
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_ORDERED_LIST *) OpCodeData)->Flags;
      CurrentStatement->MaxContainers = ((EFI_IFR_ORDERED_LIST *) OpCodeData)->MaxContainers;

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_BUFFER;
      CurrentStatement->BufferValue = NULL;

      if (Scope != 0) {
        SuppressForOption = TRUE;
      }
      break;

    case EFI_IFR_CHECKBOX_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_CHECKBOX *) OpCodeData)->Flags;
      CurrentStatement->StorageWidth = (UINT16) sizeof (BOOLEAN);
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_BOOLEAN;

      InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);

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
      CurrentStatement->StorageWidth = (UINT16)((UINTN) CurrentStatement->Maximum * sizeof (CHAR16));
      CurrentStatement->Flags = ((EFI_IFR_STRING *) OpCodeData)->Flags;

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_STRING;
      CurrentStatement->BufferValue = AllocateZeroPool (CurrentStatement->StorageWidth + sizeof (CHAR16));
      CurrentStatement->HiiValue.Value.string = NewString ((CHAR16*) CurrentStatement->BufferValue, FormSet->HiiHandle);

      InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
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
      CurrentStatement->StorageWidth = (UINT16)((UINTN) CurrentStatement->Maximum * sizeof (CHAR16));

      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_STRING;
      CurrentStatement->BufferValue = AllocateZeroPool ((CurrentStatement->StorageWidth + sizeof (CHAR16)));
      CurrentStatement->HiiValue.Value.string = NewString ((CHAR16*) CurrentStatement->BufferValue, FormSet->HiiHandle);

      InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
      break;

    case EFI_IFR_DATE_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_DATE *) OpCodeData)->Flags;
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_DATE;

      if ((CurrentStatement->Flags & EFI_QF_DATE_STORAGE) == QF_DATE_STORAGE_NORMAL) {
        CurrentStatement->StorageWidth = (UINT16) sizeof (EFI_HII_DATE);

        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
      } else {
        //
        // Don't assign storage for RTC type of date/time
        //
        CurrentStatement->Storage = NULL;
        CurrentStatement->StorageWidth = 0;
      }
      break;

    case EFI_IFR_TIME_OP:
      CurrentStatement = CreateQuestion (OpCodeData, FormSet, CurrentForm);
      ASSERT(CurrentStatement != NULL);

      CurrentStatement->Flags = ((EFI_IFR_TIME *) OpCodeData)->Flags;
      CurrentStatement->HiiValue.Type = EFI_IFR_TYPE_TIME;

      if ((CurrentStatement->Flags & QF_TIME_STORAGE) == QF_TIME_STORAGE_NORMAL) {
        CurrentStatement->StorageWidth = (UINT16) sizeof (EFI_HII_TIME);

        InitializeRequestElement (FormSet, CurrentStatement, CurrentForm);
      } else {
        //
        // Don't assign storage for RTC type of date/time
        //
        CurrentStatement->Storage = NULL;
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
      CurrentDefault = AllocateZeroPool (sizeof (QUESTION_DEFAULT));
      ASSERT (CurrentDefault != NULL);
      CurrentDefault->Signature = QUESTION_DEFAULT_SIGNATURE;

      CurrentDefault->Value.Type = ((EFI_IFR_DEFAULT *) OpCodeData)->Type;
      CopyMem (&CurrentDefault->DefaultId, &((EFI_IFR_DEFAULT *) OpCodeData)->DefaultId, sizeof (UINT16));
      if (CurrentDefault->Value.Type == EFI_IFR_TYPE_BUFFER) {
        CurrentDefault->Value.BufferLen = (UINT16) (OpCodeLength - OFFSET_OF (EFI_IFR_DEFAULT, Value));
        CurrentDefault->Value.Buffer = AllocateCopyPool (CurrentDefault->Value.BufferLen, &((EFI_IFR_DEFAULT *) OpCodeData)->Value);
        ASSERT (CurrentDefault->Value.Buffer != NULL);
      } else {
        CopyMem (&CurrentDefault->Value.Value, &((EFI_IFR_DEFAULT *) OpCodeData)->Value, OpCodeLength - OFFSET_OF (EFI_IFR_DEFAULT, Value));
        ExtendValueToU64 (&CurrentDefault->Value);
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
      ASSERT (ParentStatement != NULL);
      if (ParentStatement->Operand == EFI_IFR_ORDERED_LIST_OP && ((((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Flags & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0)) {
        //
        // It's keep the default value for ordered list opcode.
        //
        CurrentDefault = AllocateZeroPool (sizeof (QUESTION_DEFAULT));
        ASSERT (CurrentDefault != NULL);
        CurrentDefault->Signature = QUESTION_DEFAULT_SIGNATURE;

        CurrentDefault->Value.Type = EFI_IFR_TYPE_BUFFER;
        if ((((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Flags & EFI_IFR_OPTION_DEFAULT) != 0) {
          CurrentDefault->DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
        } else {
          CurrentDefault->DefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
        }

        CurrentDefault->Value.BufferLen = (UINT16) (OpCodeLength - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
        CurrentDefault->Value.Buffer = AllocateCopyPool (CurrentDefault->Value.BufferLen, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Value);
        ASSERT (CurrentDefault->Value.Buffer != NULL);

        //
        // Insert to Default Value list of current Question
        //
        InsertTailList (&ParentStatement->DefaultListHead, &CurrentDefault->Link);
        break;
      }

      //
      // EFI_IFR_ONE_OF_OPTION appear in scope of a Question.
      // It create a selection for use in current Question.
      //
      CurrentOption = AllocateZeroPool (sizeof (QUESTION_OPTION));
      ASSERT (CurrentOption != NULL);
      CurrentOption->Signature = QUESTION_OPTION_SIGNATURE;
      CurrentOption->OpCode    = (EFI_IFR_ONE_OF_OPTION *) OpCodeData;

      CurrentOption->Flags = ((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Flags;
      CurrentOption->Value.Type = ((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Type;
      CopyMem (&CurrentOption->Text, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Option, sizeof (EFI_STRING_ID));
      CopyMem (&CurrentOption->Value.Value, &((EFI_IFR_ONE_OF_OPTION *) OpCodeData)->Value, OpCodeLength - OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value));
      ExtendValueToU64 (&CurrentOption->Value);

      ConditionalExprCount = GetConditionalExpressionCount(ExpressOption);
      if ( ConditionalExprCount > 0) {
        //
        // Form is inside of suppressif
        //
        CurrentOption->SuppressExpression = (FORM_EXPRESSION_LIST *) AllocatePool( 
                                                 (UINTN) (sizeof(FORM_EXPRESSION_LIST) + ((ConditionalExprCount -1) * sizeof(FORM_EXPRESSION *))));
        ASSERT (CurrentOption->SuppressExpression != NULL);
        CurrentOption->SuppressExpression->Count     = (UINTN) ConditionalExprCount;
        CurrentOption->SuppressExpression->Signature = FORM_EXPRESSION_LIST_SIGNATURE;
        CopyMem (CurrentOption->SuppressExpression->Expression, GetConditionalExpressionList(ExpressOption), (UINTN) (sizeof (FORM_EXPRESSION *) * ConditionalExprCount));
      }

      //
      // Insert to Option list of current Question
      //
      InsertTailList (&ParentStatement->OptionListHead, &CurrentOption->Link);
      //
      // Now we know the Storage width of nested Ordered List
      //
      if ((ParentStatement->Operand == EFI_IFR_ORDERED_LIST_OP) && (ParentStatement->BufferValue == NULL)) {
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

        ParentStatement->StorageWidth = (UINT16) (ParentStatement->MaxContainers * Width);
        ParentStatement->BufferValue = AllocateZeroPool (ParentStatement->StorageWidth);
        ParentStatement->ValueType = CurrentOption->Value.Type;
        if (ParentStatement->HiiValue.Type == EFI_IFR_TYPE_BUFFER) {
          ParentStatement->HiiValue.Buffer = ParentStatement->BufferValue;
          ParentStatement->HiiValue.BufferLen = ParentStatement->StorageWidth;
        }

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
      CopyMem (&CurrentExpression->Error, &((EFI_IFR_INCONSISTENT_IF *) OpCodeData)->Error, sizeof (EFI_STRING_ID));

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
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_WARNING_IF_OP:
      //
      // Create an Expression node
      //
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CopyMem (&CurrentExpression->Error, &((EFI_IFR_WARNING_IF *) OpCodeData)->Warning, sizeof (EFI_STRING_ID));
      CurrentExpression->TimeOut = ((EFI_IFR_WARNING_IF *) OpCodeData)->TimeOut;
      CurrentExpression->Type    = EFI_HII_EXPRESSION_WARNING_IF;
      InsertTailList (&ParentStatement->WarningListHead, &CurrentExpression->Link);

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_SUPPRESS_IF_OP:
      //
      // Question and Option will appear in scope of this OpCode
      //
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CurrentExpression->Type = EFI_HII_EXPRESSION_SUPPRESS_IF;

      if (CurrentForm == NULL) {
        InsertTailList (&FormSet->ExpressionListHead, &CurrentExpression->Link);
      } else {
        InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);
      }

      if (SuppressForOption) {
        PushConditionalExpression(CurrentExpression, ExpressOption);       
      } else if (SuppressForQuestion) {
        PushConditionalExpression(CurrentExpression, ExpressStatement);  
      } else {
        PushConditionalExpression(CurrentExpression, ExpressForm);  
      }

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_GRAY_OUT_IF_OP:
      //
      // Questions will appear in scope of this OpCode
      //
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CurrentExpression->Type = EFI_HII_EXPRESSION_GRAY_OUT_IF;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);
      PushConditionalExpression(CurrentExpression, ExpressStatement);

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_DISABLE_IF_OP:
      //
      // The DisableIf expression should only rely on constant, so it could be
      // evaluated at initialization and it will not be queued
      //
      CurrentExpression = AllocateZeroPool (sizeof (FORM_EXPRESSION));
      ASSERT (CurrentExpression != NULL);
      CurrentExpression->Signature = FORM_EXPRESSION_SIGNATURE;
      CurrentExpression->Type = EFI_HII_EXPRESSION_DISABLE_IF;
      InitializeListHead (&CurrentExpression->OpCodeListHead);

      if (CurrentForm != NULL) {
        //
        // This is DisableIf for Question, enqueue it to Form expression list
        //
        InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);
        PushConditionalExpression(CurrentExpression, ExpressStatement);
      }

      OpCodeDisabled  = FALSE;
      InScopeDisable  = TRUE;
      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    //
    // Expression
    //
    case EFI_IFR_VALUE_OP:
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CurrentExpression->Type = EFI_HII_EXPRESSION_VALUE;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      if (InScopeDefault) {
        //
        // Used for default (EFI_IFR_DEFAULT)
        //
        CurrentDefault->ValueExpression = CurrentExpression;
      } else {
        //
        // If used for a question, then the question will be read-only
        //
        //
        // Make sure CurrentStatement is not NULL.
        // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
        // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
        //
        ASSERT (ParentStatement != NULL);
        ParentStatement->ValueExpression = CurrentExpression;
      }

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_RULE_OP:
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CurrentExpression->Type = EFI_HII_EXPRESSION_RULE;

      CurrentExpression->RuleId = ((EFI_IFR_RULE *) OpCodeData)->RuleId;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_READ_OP:
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CurrentExpression->Type = EFI_HII_EXPRESSION_READ;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      //
      // Make sure CurrentStatement is not NULL.
      // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
      // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
      //
      ASSERT (ParentStatement != NULL);
      ParentStatement->ReadExpression = CurrentExpression;

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
        SingleOpCodeExpression = TRUE;
      }
      break;

    case EFI_IFR_WRITE_OP:
      CurrentExpression = CreateExpression (CurrentForm, OpCodeData);
      CurrentExpression->Type = EFI_HII_EXPRESSION_WRITE;
      InsertTailList (&CurrentForm->ExpressionListHead, &CurrentExpression->Link);

      //
      // Make sure CurrentStatement is not NULL.
      // If it is NULL, 1) ParseOpCodes functions may parse the IFR wrongly. Or 2) the IFR
      // file is wrongly generated by tools such as VFR Compiler. There may be a bug in VFR Compiler.
      //
      ASSERT (ParentStatement != NULL);
      ParentStatement->WriteExpression = CurrentExpression;

      //
      // Take a look at next OpCode to see whether current expression consists
      // of single OpCode
      //
      if (((EFI_IFR_OP_HEADER *) (OpCodeData + OpCodeLength))->Scope == 0) {
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
        ASSERT (ParentStatement != NULL);
        ImageId = &ParentStatement->ImageId;
        break;
      }

      ASSERT (ImageId != NULL);
      CopyMem (ImageId, &((EFI_IFR_IMAGE *) OpCodeData)->Id, sizeof (EFI_IMAGE_ID));
      break;

    //
    // Refresh
    //
    case EFI_IFR_REFRESH_OP:
      ASSERT (ParentStatement != NULL);
      ParentStatement->RefreshInterval = ((EFI_IFR_REFRESH *) OpCodeData)->RefreshInterval;
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
        ASSERT (CurrentForm != NULL);
        CopyMem (&CurrentForm->RefreshGuid, &((EFI_IFR_REFRESH_ID *) OpCodeData)->RefreshEventGroupId, sizeof (EFI_GUID));
        break;

      default:
        ASSERT (ParentStatement != NULL);
        CopyMem (&ParentStatement->RefreshGuid, &((EFI_IFR_REFRESH_ID *) OpCodeData)->RefreshEventGroupId, sizeof (EFI_GUID));
        break;
      }
      break;

    //
    // Modal tag
    //
    case EFI_IFR_MODAL_TAG_OP:
      ASSERT (CurrentForm != NULL);
      CurrentForm->ModalForm = TRUE;
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
        ASSERT (CurrentForm != NULL);
        CurrentForm->Locked = TRUE;
        break;

      default:
        ASSERT (ParentStatement != NULL);
        ParentStatement->Locked = TRUE;
      }      
      break;

    //
    // Vendor specific
    //
    case EFI_IFR_GUID_OP:     
      CurrentStatement = CreateStatement (OpCodeData, FormSet, CurrentForm);
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
      
      //
      // Parent statement end tag found, update ParentStatement info.
      //
      if (IsStatementOpCode(ScopeOpCode) && (ParentStatement != NULL) && (ParentStatement->Operand == ScopeOpCode)) {
        ParentStatement  = ParentStatement->ParentStatement;
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
          PopConditionalExpression(ExpressOption);      
        } else if (SuppressForQuestion) {
          PopConditionalExpression(ExpressStatement);
        } else {
          PopConditionalExpression(ExpressForm);
        }
        break;

      case EFI_IFR_GRAY_OUT_IF_OP:
        PopConditionalExpression(ExpressStatement);
        break;

      case EFI_IFR_DISABLE_IF_OP:
        if (CurrentForm != NULL) {
          PopConditionalExpression(ExpressStatement);
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
        Status = PopMapExpressionList ((VOID **) &MapExpressionList);
        if (Status == EFI_ACCESS_DENIED) {
          MapExpressionList = NULL;
        }
        //
        // Get current expression.
        //
        Status = PopCurrentExpression ((VOID **) &CurrentExpression);
        ASSERT_EFI_ERROR (Status);
        ASSERT (MapScopeDepth > 0);
        MapScopeDepth --;
        break;

      default:
        if (IsExpressionOpCode (ScopeOpCode)) {
          if (InScopeDisable && CurrentForm == NULL) {
            //
            // This is DisableIf expression for Form, it should be a constant expression
            //
            ASSERT (CurrentExpression != NULL);
            Status = EvaluateExpression (FormSet, CurrentForm, CurrentExpression);
            if (EFI_ERROR (Status)) {
              return Status;
            }

            OpCodeDisabled = IsTrue (&CurrentExpression->Result);

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

    if (IsStatementOpCode(Operand)) {
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
