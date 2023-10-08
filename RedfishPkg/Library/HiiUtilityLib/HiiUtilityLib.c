/** @file
  Implementation of HII utility library.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HiiInternal.h"

/**
  Initialize the internal data structure of a FormSet.

  @param  Handle                 PackageList Handle
  @param  FormSetGuid            On input, GUID or class GUID of a formset. If not
                                 specified (NULL or zero GUID), take the first
                                 FormSet with class GUID EFI_HII_PLATFORM_SETUP_FORMSET_GUID
                                 found in package list.
                                 On output, GUID of the formset found(if not NULL).
  @param  FormSet                FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The specified FormSet could not be found.

**/
EFI_STATUS
CreateFormSetFromHiiHandle (
  IN     EFI_HII_HANDLE  Handle,
  IN OUT EFI_GUID        *FormSetGuid,
  OUT HII_FORMSET        *FormSet
  )
{
  EFI_STATUS                 Status;
  EFI_HANDLE                 DriverHandle;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;

  if ((FormSetGuid == NULL) || (FormSet == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Locate required Hii Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIfrBinaryData (Handle, FormSetGuid, &FormSet->IfrBinaryLength, &FormSet->IfrBinaryData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FormSet->Signature = HII_FORMSET_SIGNATURE;
  FormSet->HiiHandle = Handle;
  CopyMem (&FormSet->Guid, FormSetGuid, sizeof (EFI_GUID));
  //
  // Retrieve ConfigAccess Protocol associated with this HiiPackageList
  //
  Status = HiiDatabase->GetPackageListHandle (HiiDatabase, Handle, &DriverHandle);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FormSet->DriverHandle = DriverHandle;
  Status                = gBS->HandleProtocol (
                                 DriverHandle,
                                 &gEfiHiiConfigAccessProtocolGuid,
                                 (VOID **)&FormSet->ConfigAccess
                                 );
  if (EFI_ERROR (Status)) {
    //
    // Configuration Driver don't attach ConfigAccess protocol to its HII package
    // list, then there will be no configuration action required
    //
    FormSet->ConfigAccess = NULL;
  }

  Status = gBS->HandleProtocol (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&FormSet->DevicePath
                  );
  if (EFI_ERROR (Status)) {
    //
    // Configuration Driver don't attach ConfigAccess protocol to its HII package
    // list, then there will be no configuration action required
    //
    FormSet->DevicePath = NULL;
  }

  //
  // Parse the IFR binary OpCodes
  //
  Status = ParseOpCodes (FormSet);

  return Status;
}

/**
  Initialize a Formset and get current setting for Questions.

  @param  FormSet                FormSet data structure.

**/
VOID
InitializeFormSet (
  IN OUT HII_FORMSET  *FormSet
  )
{
  LIST_ENTRY           *Link;
  HII_FORMSET_STORAGE  *Storage;
  LIST_ENTRY           *FormLink;
  HII_STATEMENT        *Question;
  HII_FORM             *Form;

  if (FormSet == NULL) {
    return;
  }

  //
  // Load Storage for all questions with storage
  //
  Link = GetFirstNode (&FormSet->StorageListHead);
  while (!IsNull (&FormSet->StorageListHead, Link)) {
    Storage = HII_STORAGE_FROM_LINK (Link);
    LoadFormSetStorage (FormSet, Storage);
    Link = GetNextNode (&FormSet->StorageListHead, Link);
  }

  //
  // Get Current Value for all no storage questions
  //
  FormLink = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, FormLink)) {
    Form = HII_FORM_FROM_LINK (FormLink);
    Link = GetFirstNode (&Form->StatementListHead);
    while (!IsNull (&Form->StatementListHead, Link)) {
      Question = HII_STATEMENT_FROM_LINK (Link);
      if (Question->Storage == NULL) {
        RetrieveQuestion (FormSet, Form, Question);
      }

      Link = GetNextNode (&Form->StatementListHead, Link);
    }

    FormLink = GetNextNode (&FormSet->FormListHead, FormLink);
  }
}

/**
  Free resources allocated for a FormSet.

  @param[in,out]  FormSet                Pointer of the FormSet

**/
VOID
DestroyFormSet (
  IN OUT HII_FORMSET  *FormSet
  )
{
  LIST_ENTRY                *Link;
  HII_FORMSET_STORAGE       *Storage;
  HII_FORMSET_DEFAULTSTORE  *DefaultStore;
  HII_FORM                  *Form;

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
      Link    = GetFirstNode (&FormSet->StorageListHead);
      Storage = HII_STORAGE_FROM_LINK (Link);
      RemoveEntryList (&Storage->Link);

      if (Storage != NULL) {
        FreePool (Storage);
      }
    }
  }

  //
  // Free FormSet Default Store
  //
  if (FormSet->DefaultStoreListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->DefaultStoreListHead)) {
      Link         = GetFirstNode (&FormSet->DefaultStoreListHead);
      DefaultStore = HII_FORMSET_DEFAULTSTORE_FROM_LINK (Link);
      RemoveEntryList (&DefaultStore->Link);

      FreePool (DefaultStore);
    }
  }

  //
  // Free Forms
  //
  if (FormSet->FormListHead.ForwardLink != NULL) {
    while (!IsListEmpty (&FormSet->FormListHead)) {
      Link = GetFirstNode (&FormSet->FormListHead);
      Form = HII_FORM_FROM_LINK (Link);
      RemoveEntryList (&Form->Link);

      DestroyForm (FormSet, Form);
    }
  }

  FreePool (FormSet);
}

/**
  Submit data for a form.

  @param[in]  FormSet                FormSet which contains the Form.
  @param[in]  Form                   Form to submit.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval Others                 Other errors occur.

**/
EFI_STATUS
SubmitForm (
  IN HII_FORMSET  *FormSet,
  IN HII_FORM     *Form
  )
{
  EFI_STATUS                       Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  LIST_ENTRY                       *Link;
  EFI_STRING                       ConfigResp;
  EFI_STRING                       Progress;
  HII_FORMSET_STORAGE              *Storage;
  HII_FORM_CONFIG_REQUEST          *ConfigInfo;

  if ((FormSet == NULL) || (Form == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = NoSubmitCheck (FormSet, &Form, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Link = GetFirstNode (&Form->ConfigRequestHead);
  while (!IsNull (&Form->ConfigRequestHead, Link)) {
    ConfigInfo = HII_FORM_CONFIG_REQUEST_FROM_LINK (Link);
    Link       = GetNextNode (&Form->ConfigRequestHead, Link);

    Storage = ConfigInfo->Storage;
    if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
      continue;
    }

    //
    // Skip if there is no RequestElement
    //
    if (ConfigInfo->ElementCount == 0) {
      continue;
    }

    Status = StorageToConfigResp (ConfigInfo->Storage, &ConfigResp, ConfigInfo->ConfigRequest);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = gBS->LocateProtocol (
                    &gEfiHiiConfigRoutingProtocolGuid,
                    NULL,
                    (VOID **)&HiiConfigRouting
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = HiiConfigRouting->RouteConfig (
                                 HiiConfigRouting,
                                 ConfigResp,
                                 &Progress
                                 );

    if (EFI_ERROR (Status)) {
      FreePool (ConfigResp);
      continue;
    }

    FreePool (ConfigResp);
  }

  return Status;
}

/**
  Save Question Value to the memory, but not to storage.

  @param[in]     FormSet                FormSet data structure.
  @param[in]     Form                   Form data structure.
  @param[in,out] Question               Pointer to the Question.
  @param[in]     QuestionValue          New Question Value to be set.

  @retval EFI_SUCCESS            The question value has been set successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
EFI_STATUS
SetQuestionValue (
  IN     HII_FORMSET          *FormSet,
  IN     HII_FORM             *Form,
  IN OUT HII_STATEMENT        *Question,
  IN     HII_STATEMENT_VALUE  *QuestionValue
  )
{
  UINT8                *Src;
  UINTN                BufferLen;
  UINTN                StorageWidth;
  HII_FORMSET_STORAGE  *Storage;
  CHAR16               *ValueStr;
  BOOLEAN              IsBufferStorage;
  UINT8                *TemBuffer;
  CHAR16               *TemName;
  CHAR16               *TemString;
  UINTN                Index;
  HII_NAME_VALUE_NODE  *Node;
  EFI_STATUS           Status;

  if ((FormSet == NULL) || (Form == NULL) || (Question == NULL) || (QuestionValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  Node   = NULL;

  //
  // If Question value is provided by an Expression, then it is read only
  //
  if ((Question->ValueExpression != NULL) || (Question->Value.Type != QuestionValue->Type)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Before set question value, evaluate its write expression.
  //
  if ((Question->WriteExpression != NULL) && (Form->FormType == STANDARD_MAP_FORM_TYPE)) {
    Status = EvaluateHiiExpression (FormSet, Form, Question->WriteExpression);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Storage = Question->Storage;
  if (Storage != NULL) {
    StorageWidth = Question->StorageWidth;
    if (Question->Value.Type == EFI_IFR_TYPE_BUFFER) {
      Question->Value.BufferLen = QuestionValue->BufferLen;
      Question->Value.Buffer    = AllocateCopyPool (QuestionValue->BufferLen, QuestionValue->Buffer);
      if (Question->Value.Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Question->Value.BufferValueType = QuestionValue->BufferValueType;
      Src                             = Question->Value.Buffer;
    } else if (Question->Value.Type == EFI_IFR_TYPE_STRING) {
      Question->Value.Value.string = QuestionValue->Value.string;
      TemString                    = HiiGetString (FormSet->HiiHandle, QuestionValue->Value.string, NULL);
      if (TemString == NULL) {
        return EFI_ABORTED;
      }

      Question->Value.BufferLen = Question->StorageWidth;
      Question->Value.Buffer    = AllocateZeroPool (Question->StorageWidth);
      if (Question->Value.Buffer == NULL) {
        FreePool (TemString);
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (Question->Value.Buffer, TemString, StrSize (TemString));
      Src = Question->Value.Buffer;
      FreePool (TemString);
    } else {
      CopyMem (&Question->Value.Value, &QuestionValue->Value, sizeof (EFI_IFR_TYPE_VALUE));
      Src = (UINT8 *)&Question->Value.Value;
    }

    if ((Storage->Type == EFI_HII_VARSTORE_BUFFER) || (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER)) {
      IsBufferStorage = TRUE;
    } else {
      IsBufferStorage = FALSE;
    }

    if (IsBufferStorage) {
      //
      // If the Question refer to bit filed, copy the value in related bit filed to storage edit buffer.
      //
      if (Question->QuestionReferToBitField) {
        SetBitsQuestionValue (Question, Storage->Buffer + Question->VarStoreInfo.VarOffset, (UINT32)(*Src));
      } else {
        CopyMem (Storage->Buffer + Question->VarStoreInfo.VarOffset, Src, StorageWidth);
      }
    } else {
      if (Question->Value.Type == EFI_IFR_TYPE_STRING) {
        //
        // Allocate enough string buffer.
        //
        ValueStr  = NULL;
        BufferLen = ((StrLen ((CHAR16 *)Src) * 4) + 1) * sizeof (CHAR16);
        ValueStr  = AllocatePool (BufferLen);
        if (ValueStr == NULL) {
          if (Question->Value.Buffer != NULL) {
            FreePool (Question->Value.Buffer);
          }

          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Convert Unicode String to Config String, e.g. "ABCD" => "0041004200430044"
        //
        TemName   = (CHAR16 *)Src;
        TemString = ValueStr;
        for ( ; *TemName != L'\0'; TemName++) {
          UnicodeValueToStringS (
            TemString,
            BufferLen - ((UINTN)TemString - (UINTN)ValueStr),
            PREFIX_ZERO | RADIX_HEX,
            *TemName,
            4
            );
          TemString += StrnLenS (TemString, (BufferLen - ((UINTN)TemString - (UINTN)ValueStr)) / sizeof (CHAR16));
        }
      } else {
        BufferLen = StorageWidth * 2 + 1;
        ValueStr  = AllocateZeroPool (BufferLen * sizeof (CHAR16));
        if (ValueStr == NULL) {
          if (Question->Value.Buffer != NULL) {
            FreePool (Question->Value.Buffer);
          }

          return EFI_OUT_OF_RESOURCES;
        }

        //
        // Convert Buffer to Hex String
        //
        TemBuffer = Src + StorageWidth - 1;
        TemString = ValueStr;
        for (Index = 0; Index < StorageWidth; Index++, TemBuffer--) {
          UnicodeValueToStringS (
            TemString,
            BufferLen * sizeof (CHAR16) - ((UINTN)TemString - (UINTN)ValueStr),
            PREFIX_ZERO | RADIX_HEX,
            *TemBuffer,
            2
            );
          TemString += StrnLenS (TemString, BufferLen - ((UINTN)TemString - (UINTN)ValueStr) / sizeof (CHAR16));
        }
      }

      Status = SetValueByName (Storage, Question->VariableName, ValueStr, &Node);
      FreePool (ValueStr);
      if (EFI_ERROR (Status)) {
        if (Question->Value.Buffer != NULL) {
          FreePool (Question->Value.Buffer);
        }

        return Status;
      }
    }
  } else {
    if (Question->Value.Type == EFI_IFR_TYPE_BUFFER) {
      Question->Value.BufferLen = QuestionValue->BufferLen;
      Question->Value.Buffer    = AllocateCopyPool (QuestionValue->BufferLen, QuestionValue->Buffer);
      if (Question->Value.Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Question->Value.BufferValueType = QuestionValue->BufferValueType;
    } else if (Question->Value.Type == EFI_IFR_TYPE_STRING) {
      Question->Value.Value.string = QuestionValue->Value.string;
      TemString                    = HiiGetString (FormSet->HiiHandle, QuestionValue->Value.string, NULL);
      if (TemString == NULL) {
        return EFI_ABORTED;
      }

      Question->Value.BufferLen = (UINT16)StrSize (TemString);
      Question->Value.Buffer    = AllocateZeroPool (QuestionValue->BufferLen);
      if (Question->Value.Buffer == NULL) {
        FreePool (TemString);
        return EFI_OUT_OF_RESOURCES;
      }

      CopyMem (Question->Value.Buffer, TemString, StrSize (TemString));
      FreePool (TemString);
    } else {
      CopyMem (&Question->Value.Value, &QuestionValue->Value, sizeof (EFI_IFR_TYPE_VALUE));
    }
  }

  return Status;
}

/**
  Get Question's current Value from storage.

  @param[in]     FormSet                FormSet data structure.
  @param[in]     Form                   Form data structure.
  @param[in,out] Question               Question to be initialized.

  @return the current Question Value in storage if success.
  @return NULL if Question is not found or any error occurs.

**/
HII_STATEMENT_VALUE *
RetrieveQuestion (
  IN     HII_FORMSET    *FormSet,
  IN     HII_FORM       *Form,
  IN OUT HII_STATEMENT  *Question
  )
{
  EFI_STATUS                       Status;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_BROWSER_ACTION_REQUEST       ActionRequest;
  HII_FORMSET_STORAGE              *Storage;
  HII_STATEMENT_VALUE              *QuestionValue;
  EFI_IFR_TYPE_VALUE               *TypeValue;
  EFI_TIME                         EfiTime;
  BOOLEAN                          Enabled;
  BOOLEAN                          Pending;
  UINT8                            *Dst;
  UINTN                            StorageWidth;
  CHAR16                           *ConfigRequest;
  CHAR16                           *Progress;
  CHAR16                           *Result;
  CHAR16                           *ValueStr;
  UINTN                            Length;
  BOOLEAN                          IsBufferStorage;
  CHAR16                           *NewHiiString;

  if ((FormSet == NULL) || (Form == NULL) || (Question == NULL)) {
    return NULL;
  }

  Status   = EFI_SUCCESS;
  ValueStr = NULL;
  Result   = NULL;

  QuestionValue = AllocateZeroPool (sizeof (HII_STATEMENT_VALUE));
  if (QuestionValue == NULL) {
    return NULL;
  }

  QuestionValue->Type      = Question->Value.Type;
  QuestionValue->BufferLen = Question->Value.BufferLen;
  if (QuestionValue->BufferLen != 0) {
    QuestionValue->Buffer = AllocateZeroPool (QuestionValue->BufferLen);
    if (QuestionValue->Buffer == NULL) {
      FreePool (QuestionValue);
      return NULL;
    }
  }

  //
  // Question value is provided by RTC
  //
  Storage      = Question->Storage;
  StorageWidth = Question->StorageWidth;

  if (Storage == NULL) {
    //
    // It's a Question without storage, or RTC date/time
    //
    if ((Question->Operand == EFI_IFR_DATE_OP) || (Question->Operand == EFI_IFR_TIME_OP)) {
      //
      // Date and time define the same Flags bit
      //
      switch (Question->ExtraData.Flags & EFI_QF_DATE_STORAGE) {
        case QF_DATE_STORAGE_TIME:

          Status = gRT->GetTime (&EfiTime, NULL);
          break;

        case QF_DATE_STORAGE_WAKEUP:

          Status = gRT->GetWakeupTime (&Enabled, &Pending, &EfiTime);
          break;

        case QF_DATE_STORAGE_NORMAL:
        default:

          goto ON_ERROR;
      }

      if (EFI_ERROR (Status)) {
        if (Question->Operand == EFI_IFR_DATE_OP) {
          QuestionValue->Value.date.Year  = 0xff;
          QuestionValue->Value.date.Month = 0xff;
          QuestionValue->Value.date.Day   = 0xff;
        } else {
          QuestionValue->Value.time.Hour   = 0xff;
          QuestionValue->Value.time.Minute = 0xff;
          QuestionValue->Value.time.Second = 0xff;
        }

        return QuestionValue;
      }

      if (Question->Operand == EFI_IFR_DATE_OP) {
        QuestionValue->Value.date.Year  = EfiTime.Year;
        QuestionValue->Value.date.Month = EfiTime.Month;
        QuestionValue->Value.date.Day   = EfiTime.Day;
      } else {
        QuestionValue->Value.time.Hour   = EfiTime.Hour;
        QuestionValue->Value.time.Minute = EfiTime.Minute;
        QuestionValue->Value.time.Second = EfiTime.Second;
      }
    } else {
      if (((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != EFI_IFR_FLAG_CALLBACK) ||
          (FormSet->ConfigAccess == NULL))
      {
        goto ON_ERROR;
      }

      if (QuestionValue->Type == EFI_IFR_TYPE_BUFFER) {
        //
        // For OrderedList, passing in the value buffer to Callback()
        //
        TypeValue = (EFI_IFR_TYPE_VALUE *)QuestionValue->Buffer;
      } else {
        TypeValue = &QuestionValue->Value;
      }

      ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
      Status        = FormSet->ConfigAccess->Callback (
                                               FormSet->ConfigAccess,
                                               EFI_BROWSER_ACTION_RETRIEVE,
                                               Question->QuestionId,
                                               QuestionValue->Type,
                                               TypeValue,
                                               &ActionRequest
                                               );

      if (!EFI_ERROR (Status) && (QuestionValue->Type == EFI_IFR_TYPE_STRING)) {
        if (TypeValue->string == 0) {
          goto ON_ERROR;
        }

        NewHiiString = GetTokenString (TypeValue->string, FormSet->HiiHandle);
        if (NewHiiString == NULL) {
          goto ON_ERROR;
        }

        QuestionValue->Buffer = AllocatePool (StrSize (NewHiiString));
        if (QuestionValue->Buffer == NULL) {
          FreePool (NewHiiString);
          goto ON_ERROR;
        }

        CopyMem (QuestionValue->Buffer, NewHiiString, StrSize (NewHiiString));
        QuestionValue->BufferLen = (UINT16)StrSize (NewHiiString);

        FreePool (NewHiiString);
      }
    }

    return QuestionValue;
  }

  //
  // Question value is provided by EFI variable
  //
  if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
    if ((QuestionValue->Type != EFI_IFR_TYPE_BUFFER) && (QuestionValue->Type != EFI_IFR_TYPE_STRING)) {
      Dst          = QuestionValue->Buffer;
      StorageWidth = QuestionValue->BufferLen;
    } else {
      Dst          = (UINT8 *)&QuestionValue->Value;
      StorageWidth = sizeof (EFI_IFR_TYPE_VALUE);
    }

    Status = gRT->GetVariable (
                    Question->VariableName,
                    &Storage->Guid,
                    NULL,
                    &StorageWidth,
                    Dst
                    );

    return QuestionValue;
  }

  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (QuestionValue->BufferLen != 0) {
    Dst = QuestionValue->Buffer;
  } else {
    Dst = (UINT8 *)&QuestionValue->Value;
  }

  if ((Storage->Type == EFI_HII_VARSTORE_BUFFER) || (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER)) {
    IsBufferStorage = TRUE;
  } else {
    IsBufferStorage = FALSE;
  }

  Storage = GetFstStgFromVarId (FormSet, Question->VarStoreId);
  if (Storage == NULL) {
    goto ON_ERROR;
  }

  //
  // <ConfigRequest> ::= <ConfigHdr> + <BlockName> || <ConfigHdr> + "&" + <VariableName>
  //
  if (IsBufferStorage) {
    Length  = StrLen (Storage->ConfigHdr);
    Length += StrLen (Question->BlockName);
  } else {
    Length  = StrLen (Storage->ConfigHdr);
    Length += StrLen (Question->VariableName) + 1;
  }

  ConfigRequest = AllocatePool ((Length + 1) * sizeof (CHAR16));
  if (ConfigRequest == NULL) {
    goto ON_ERROR;
  }

  StrCpyS (ConfigRequest, Length + 1, Storage->ConfigHdr);
  if (IsBufferStorage) {
    StrCatS (ConfigRequest, Length + 1, Question->BlockName);
  } else {
    StrCatS (ConfigRequest, Length + 1, L"&");
    StrCatS (ConfigRequest, Length + 1, Question->VariableName);
  }

  //
  // Request current settings from Configuration Driver
  //
  Status = HiiConfigRouting->ExtractConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               &Progress,
                               &Result
                               );
  FreePool (ConfigRequest);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  if (IsBufferStorage) {
    ValueStr = StrStr (Result, L"&VALUE");
    if (ValueStr == NULL) {
      FreePool (Result);
      goto ON_ERROR;
    }

    ValueStr = ValueStr + 6;
  } else {
    ValueStr = Result + Length;
  }

  if (*ValueStr != '=') {
    FreePool (Result);
    goto ON_ERROR;
  }

  ValueStr++;
  Status = BufferToQuestionValue (Question, ValueStr, QuestionValue);
  if (EFI_ERROR (Status)) {
    FreePool (Result);
    goto ON_ERROR;
  }

  if (Result != NULL) {
    FreePool (Result);
  }

  return QuestionValue;

ON_ERROR:

  if (QuestionValue->Buffer != NULL) {
    FreePool (QuestionValue->Buffer);
  }

  FreePool (QuestionValue);

  return NULL;
}
