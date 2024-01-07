/** @file
  HII utility internal functions.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "HiiInternal.h"

CHAR16  *mUnknownString = L"!";
CHAR16  *gEmptyString   = L"";

EFI_HII_VALUE  *mExpressionEvaluationStack        = NULL;
EFI_HII_VALUE  *mExpressionEvaluationStackEnd     = NULL;
EFI_HII_VALUE  *mExpressionEvaluationStackPointer = NULL;
UINTN          mExpressionEvaluationStackOffset   = 0;

//
// Unicode collation protocol interface
//
EFI_UNICODE_COLLATION_PROTOCOL  *mUnicodeCollation = NULL;
EFI_USER_MANAGER_PROTOCOL       *mUserManager      = NULL;

/**
  Allocate new memory and then copy the Unicode string Source to Destination.

  @param[in,out]  Dest                   Location to copy string
  @param[in]      Src                    String to copy

**/
VOID
NewStringCopy (
  IN OUT CHAR16  **Dest,
  IN     CHAR16  *Src
  )
{
  if (*Dest != NULL) {
    FreePool (*Dest);
  }

  *Dest = AllocateCopyPool (StrSize (Src), Src);
}

/**
  Set Value of given Name in a NameValue Storage.

  @param[in]  Storage                The NameValue Storage.
  @param[in]  Name                   The Name.
  @param[in]  Value                  The Value to set.
  @param[out] ReturnNode             The node use the input name.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.

**/
EFI_STATUS
SetValueByName (
  IN     HII_FORMSET_STORAGE  *Storage,
  IN     CHAR16               *Name,
  IN     CHAR16               *Value,
  OUT HII_NAME_VALUE_NODE     **ReturnNode
  )
{
  LIST_ENTRY           *Link;
  HII_NAME_VALUE_NODE  *Node;
  CHAR16               *Buffer;

  Link = GetFirstNode (&Storage->NameValueList);
  while (!IsNull (&Storage->NameValueList, Link)) {
    Node = HII_NAME_VALUE_NODE_FROM_LINK (Link);

    if (StrCmp (Name, Node->Name) == 0) {
      Buffer = Node->Value;
      if (Buffer != NULL) {
        FreePool (Buffer);
      }

      Buffer = AllocateCopyPool (StrSize (Value), Value);
      if (Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Node->Value = Buffer;

      if (ReturnNode != NULL) {
        *ReturnNode = Node;
      }

      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Storage->NameValueList, Link);
  }

  return EFI_NOT_FOUND;
}

/**
  Get bit field value from the buffer and then set the value for the question.
  Note: Data type UINT32 can cover all the bit field value.

  @param[in]  Question        The question refer to bit field.
  @param[in]  Buffer          Point to the buffer which the question value get from.
  @param[out] QuestionValue   The Question Value retrieved from Bits.

**/
VOID
GetBitsQuestionValue (
  IN     HII_STATEMENT     *Question,
  IN     UINT8             *Buffer,
  OUT HII_STATEMENT_VALUE  *QuestionValue
  )
{
  UINTN   StartBit;
  UINTN   EndBit;
  UINT32  RetVal;
  UINT32  BufferValue;

  StartBit = Question->BitVarOffset % 8;
  EndBit   = StartBit + Question->BitStorageWidth - 1;

  CopyMem ((UINT8 *)&BufferValue, Buffer, Question->StorageWidth);

  RetVal = BitFieldRead32 (BufferValue, StartBit, EndBit);

  //
  // Set question value.
  // Note: Since Question with BufferValue (orderedlist, password, string)are not supported to refer bit field.
  // Only oneof/checkbox/oneof can support bit field.So we can copy the value to the HiiValue of Question directly.
  //
  CopyMem ((UINT8 *)&QuestionValue->Value, (UINT8 *)&RetVal, Question->StorageWidth);
}

/**
  Set bit field value to the buffer.
  Note: Data type UINT32 can cover all the bit field value.

  @param[in]     Question        The question refer to bit field.
  @param[in,out] Buffer          Point to the buffer which the question value set to.
  @param[in]     Value           The bit field value need to set.

**/
VOID
SetBitsQuestionValue (
  IN     HII_STATEMENT  *Question,
  IN OUT UINT8          *Buffer,
  IN     UINT32         Value
  )
{
  UINT32  Operand;
  UINTN   StartBit;
  UINTN   EndBit;
  UINT32  RetVal;

  StartBit = Question->BitVarOffset % 8;
  EndBit   = StartBit + Question->BitStorageWidth - 1;

  CopyMem ((UINT8 *)&Operand, Buffer, Question->StorageWidth);
  RetVal = BitFieldWrite32 (Operand, StartBit, EndBit, Value);
  CopyMem (Buffer, (UINT8 *)&RetVal, Question->StorageWidth);
}

/**
  Convert the buffer value to HiiValue.

  @param[in]  Question              The question.
  @param[in]  Value                 Unicode buffer save the question value.
  @param[out] QuestionValue         The Question Value retrieved from Buffer.

  @retval  Status whether convert the value success.

**/
EFI_STATUS
BufferToQuestionValue (
  IN     HII_STATEMENT     *Question,
  IN     CHAR16            *Value,
  OUT HII_STATEMENT_VALUE  *QuestionValue
  )
{
  CHAR16      *StringPtr;
  BOOLEAN     IsBufferStorage;
  CHAR16      *DstBuf;
  CHAR16      TempChar;
  UINTN       LengthStr;
  UINT8       *Dst;
  CHAR16      TemStr[5];
  UINTN       Index;
  UINT8       DigitUint8;
  BOOLEAN     IsString;
  UINTN       Length;
  EFI_STATUS  Status;
  UINT8       *Buffer;

  Buffer = NULL;

  IsString = (BOOLEAN)((QuestionValue->Type == EFI_IFR_TYPE_STRING) ?  TRUE : FALSE);
  if ((Question->Storage->Type == EFI_HII_VARSTORE_BUFFER) ||
      (Question->Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER))
  {
    IsBufferStorage = TRUE;
  } else {
    IsBufferStorage = FALSE;
  }

  //
  // Question Value is provided by Buffer Storage or NameValue Storage
  //
  if ((QuestionValue->Type == EFI_IFR_TYPE_STRING) || (QuestionValue->Type == EFI_IFR_TYPE_BUFFER)) {
    //
    // This Question is password or orderedlist
    //
    if (QuestionValue->Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Dst = QuestionValue->Buffer;
  } else {
    //
    // Other type of Questions
    //
    if (Question->QuestionReferToBitField) {
      Buffer = (UINT8 *)AllocateZeroPool (Question->StorageWidth);
      if (Buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Dst = Buffer;
    } else {
      Dst = (UINT8 *)&QuestionValue->Value;
    }
  }

  //
  // Temp cut at the end of this section, end with '\0' or '&'.
  //
  StringPtr = Value;
  while (*StringPtr != L'\0' && *StringPtr != L'&') {
    StringPtr++;
  }

  TempChar   = *StringPtr;
  *StringPtr = L'\0';

  LengthStr = StrLen (Value);

  //
  // Value points to a Unicode hexadecimal string, we need to convert the string to the value with CHAR16/UINT8...type.
  // When generating the Value string, we follow this rule: 1 byte -> 2 Unicode characters (for string: 2 byte(CHAR16) ->4 Unicode characters).
  // So the maximum value string length of a question is : Question->StorageWidth * 2.
  // If the value string length > Question->StorageWidth * 2, only set the string length as Question->StorageWidth * 2, then convert.
  //
  if (LengthStr > (UINTN)Question->StorageWidth * 2) {
    Length = (UINTN)Question->StorageWidth * 2;
  } else {
    Length = LengthStr;
  }

  Status = EFI_SUCCESS;
  if (!IsBufferStorage && IsString) {
    //
    // Convert Config String to Unicode String, e.g "0041004200430044" => "ABCD"
    // Add string tail char L'\0' into Length
    //
    DstBuf = (CHAR16 *)Dst;
    ZeroMem (TemStr, sizeof (TemStr));
    for (Index = 0; Index < Length; Index += 4) {
      StrnCpyS (TemStr, sizeof (TemStr) / sizeof (CHAR16), Value + Index, 4);
      DstBuf[Index/4] = (CHAR16)StrHexToUint64 (TemStr);
    }

    //
    // Add tailing L'\0' character
    //
    DstBuf[Index/4] = L'\0';
  } else {
    ZeroMem (TemStr, sizeof (TemStr));
    for (Index = 0; Index < Length; Index++) {
      TemStr[0]  = Value[LengthStr - Index - 1];
      DigitUint8 = (UINT8)StrHexToUint64 (TemStr);
      if ((Index & 1) == 0) {
        Dst[Index/2] = DigitUint8;
      } else {
        Dst[Index/2] = (UINT8)((DigitUint8 << 4) + Dst[Index/2]);
      }
    }
  }

  *StringPtr = TempChar;

  if ((Buffer != NULL) && Question->QuestionReferToBitField) {
    GetBitsQuestionValue (Question, Buffer, QuestionValue);
    FreePool (Buffer);
  }

  return Status;
}

/**
  Get the string based on the StringId and HII Package List Handle.

  @param[in]  Token                  The String's ID.
  @param[in]  HiiHandle              The package list in the HII database to search for
                                 the specified string.

  @return The output string.

**/
CHAR16 *
GetTokenString (
  IN EFI_STRING_ID   Token,
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STRING  String;

  if (HiiHandle == NULL) {
    return NULL;
  }

  String = HiiGetString (HiiHandle, Token, NULL);
  if (String == NULL) {
    String = AllocateCopyPool (StrSize (mUnknownString), mUnknownString);
    if (String == NULL) {
      return NULL;
    }
  }

  return (CHAR16 *)String;
}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param[in] ConfigString  String to be converted

**/
VOID
EFIAPI
HiiStringToLowercase (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && (*String >= L'A') && (*String <= L'F')) {
      *String = (CHAR16)(*String - L'A' + L'a');
    }
  }
}

/**
  Evaluate if the result is a non-zero value.

  @param[in]  Result       The result to be evaluated.

  @retval TRUE             It is a non-zero value.
  @retval FALSE            It is a zero value.

**/
BOOLEAN
IsHiiValueTrue (
  IN EFI_HII_VALUE  *Result
  )
{
  switch (Result->Type) {
    case EFI_IFR_TYPE_BOOLEAN:
      return Result->Value.b;

    case EFI_IFR_TYPE_NUM_SIZE_8:
      return (BOOLEAN)(Result->Value.u8 != 0);

    case EFI_IFR_TYPE_NUM_SIZE_16:
      return (BOOLEAN)(Result->Value.u16 != 0);

    case EFI_IFR_TYPE_NUM_SIZE_32:
      return (BOOLEAN)(Result->Value.u32 != 0);

    case EFI_IFR_TYPE_NUM_SIZE_64:
      return (BOOLEAN)(Result->Value.u64 != 0);

    default:
      return FALSE;
  }
}

/**
  Set a new string to string package.

  @param[in]  String              A pointer to the Null-terminated Unicode string
                                  to add or update in the String Package associated
                                  with HiiHandle.
  @param[in]  HiiHandle           A handle that was previously registered in the
                                  HII Database.

  @return the Id for this new string.

**/
EFI_STRING_ID
NewHiiString (
  IN CHAR16          *String,
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STRING_ID  StringId;

  StringId = HiiSetString (HiiHandle, 0, String, NULL);
  return StringId;
}

/**
  Perform nosubmitif check for a Form.

  @param[in]  FormSet                FormSet data structure.
  @param[in]  Form                   Form data structure.
  @param[in]  Question               The Question to be validated.

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
ValidateNoSubmit (
  IN HII_FORMSET    *FormSet,
  IN HII_FORM       *Form,
  IN HII_STATEMENT  *Question
  )
{
  EFI_STATUS      Status;
  LIST_ENTRY      *Link;
  LIST_ENTRY      *ListHead;
  HII_EXPRESSION  *Expression;

  ListHead = &Question->NoSubmitListHead;
  Link     = GetFirstNode (ListHead);
  while (!IsNull (ListHead, Link)) {
    Expression = HII_EXPRESSION_FROM_LINK (Link);

    //
    // Evaluate the expression
    //
    Status = EvaluateHiiExpression (FormSet, Form, Expression);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (IsHiiValueTrue (&Expression->Result)) {
      return EFI_NOT_READY;
    }

    Link = GetNextNode (ListHead, Link);
  }

  return EFI_SUCCESS;
}

/**
  Perform NoSubmit check for each Form in FormSet.

  @param[in]     FormSet                FormSet data structure.
  @param[in,out] CurrentForm            Current input form data structure.
  @param[out]    Statement              The statement for this check.

  @retval EFI_SUCCESS            Form validation pass.
  @retval other                  Form validation failed.

**/
EFI_STATUS
NoSubmitCheck (
  IN     HII_FORMSET  *FormSet,
  IN OUT HII_FORM     **CurrentForm,
  OUT HII_STATEMENT   **Statement
  )
{
  EFI_STATUS     Status;
  LIST_ENTRY     *Link;
  HII_STATEMENT  *Question;
  HII_FORM       *Form;
  LIST_ENTRY     *LinkForm;

  LinkForm = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, LinkForm)) {
    Form     = HII_FORM_FROM_LINK (LinkForm);
    LinkForm = GetNextNode (&FormSet->FormListHead, LinkForm);

    if ((*CurrentForm != NULL) && (*CurrentForm != Form)) {
      continue;
    }

    Link = GetFirstNode (&Form->StatementListHead);
    while (!IsNull (&Form->StatementListHead, Link)) {
      Question = HII_STATEMENT_FROM_LINK (Link);
      Status   = ValidateNoSubmit (FormSet, Form, Question);
      if (EFI_ERROR (Status)) {
        if (*CurrentForm == NULL) {
          *CurrentForm = Form;
        }

        if (Statement != NULL) {
          *Statement = Question;
        }

        return Status;
      }

      Link = GetNextNode (&Form->StatementListHead, Link);
    }
  }

  return EFI_SUCCESS;
}

/**
  Allocate new memory and concatenate Source on the end of Destination.

  @param  Dest                   String to added to the end of.
  @param  Src                    String to concatenate.

**/
VOID
NewStringCat (
  IN OUT CHAR16  **Dest,
  IN     CHAR16  *Src
  )
{
  CHAR16  *NewHiiString;
  UINTN   MaxLen;

  if (*Dest == NULL) {
    NewStringCopy (Dest, Src);
    return;
  }

  MaxLen       = (StrSize (*Dest) + StrSize (Src) - 2) / sizeof (CHAR16);
  NewHiiString = AllocatePool (MaxLen * sizeof (CHAR16));
  if (NewHiiString == NULL) {
    return;
  }

  StrCpyS (NewHiiString, MaxLen, *Dest);
  StrCatS (NewHiiString, MaxLen, Src);

  FreePool (*Dest);
  *Dest = NewHiiString;
}

/**
  Convert setting of Buffer Storage or NameValue Storage to <ConfigResp>.

  @param[in]  Storage                The Storage to be converted.
  @param[in]  ConfigResp             The returned <ConfigResp>.
  @param[in]  ConfigRequest          The ConfigRequest string.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
StorageToConfigResp (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               **ConfigResp,
  IN CHAR16               *ConfigRequest
  )
{
  EFI_STATUS                       Status;
  EFI_STRING                       Progress;
  LIST_ENTRY                       *Link;
  HII_NAME_VALUE_NODE              *Node;
  UINT8                            *SourceBuf;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;
  EFI_STRING                       TempConfigRequest;
  UINTN                            RequestStrSize;

  Status = EFI_SUCCESS;

  if (Storage->ConfigHdr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ConfigRequest != NULL) {
    TempConfigRequest = AllocateCopyPool (StrSize (ConfigRequest), ConfigRequest);
    if (TempConfigRequest == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    RequestStrSize    = (StrLen (Storage->ConfigHdr) + StrLen (Storage->ConfigRequest) + 1) * sizeof (CHAR16);
    TempConfigRequest = AllocatePool (RequestStrSize);
    if (TempConfigRequest == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    UnicodeSPrint (
      TempConfigRequest,
      RequestStrSize,
      L"%s%s",
      Storage->ConfigHdr,
      Storage->ConfigRequest
      );
  }

  switch (Storage->Type) {
    case EFI_HII_VARSTORE_BUFFER:
    case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:

      Status = gBS->LocateProtocol (
                      &gEfiHiiConfigRoutingProtocolGuid,
                      NULL,
                      (VOID **)&HiiConfigRouting
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      SourceBuf = Storage->Buffer;
      Status    = HiiConfigRouting->BlockToConfig (
                                      HiiConfigRouting,
                                      TempConfigRequest,
                                      SourceBuf,
                                      Storage->Size,
                                      ConfigResp,
                                      &Progress
                                      );
      break;

    case EFI_HII_VARSTORE_NAME_VALUE:

      *ConfigResp = NULL;
      NewStringCat (ConfigResp, Storage->ConfigHdr);

      Link = GetFirstNode (&Storage->NameValueList);
      while (!IsNull (&Storage->NameValueList, Link)) {
        Node = HII_NAME_VALUE_NODE_FROM_LINK (Link);

        if (StrStr (TempConfigRequest, Node->Name) != NULL) {
          NewStringCat (ConfigResp, L"&");
          NewStringCat (ConfigResp, Node->Name);
          NewStringCat (ConfigResp, L"=");
          NewStringCat (ConfigResp, Node->Value);
        }

        Link = GetNextNode (&Storage->NameValueList, Link);
      }

      break;

    case EFI_HII_VARSTORE_EFI_VARIABLE:
    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/**
  Convert <ConfigResp> to settings in Buffer Storage or NameValue Storage.

  @param[in]  Storage                The Storage to receive the settings.
  @param[in]  ConfigResp             The <ConfigResp> to be converted.

  @retval EFI_SUCCESS            Convert success.
  @retval EFI_INVALID_PARAMETER  Incorrect storage type.

**/
EFI_STATUS
ConfigRespToStorage (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               *ConfigResp
  )
{
  EFI_STATUS                       Status;
  EFI_STRING                       Progress;
  UINTN                            BufferSize;
  CHAR16                           *StrPtr;
  CHAR16                           *Name;
  CHAR16                           *Value;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  Status = EFI_SUCCESS;

  switch (Storage->Type) {
    case EFI_HII_VARSTORE_BUFFER:
    case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:

      Status = gBS->LocateProtocol (
                      &gEfiHiiConfigRoutingProtocolGuid,
                      NULL,
                      (VOID **)&HiiConfigRouting
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      BufferSize = Storage->Size;
      Status     = HiiConfigRouting->ConfigToBlock (
                                       HiiConfigRouting,
                                       ConfigResp,
                                       Storage->Buffer,
                                       &BufferSize,
                                       &Progress
                                       );
      break;

    case EFI_HII_VARSTORE_NAME_VALUE:
      StrPtr = StrStr (ConfigResp, L"PATH");
      if (StrPtr == NULL) {
        break;
      }

      StrPtr = StrStr (ConfigResp, L"&");
      while (StrPtr != NULL) {
        //
        // Skip '&'
        //
        StrPtr = StrPtr + 1;
        Name   = StrPtr;
        StrPtr = StrStr (StrPtr, L"=");
        if (StrPtr == NULL) {
          break;
        }

        *StrPtr = 0;

        //
        // Skip '='
        //
        StrPtr = StrPtr + 1;
        Value  = StrPtr;
        StrPtr = StrStr (StrPtr, L"&");
        if (StrPtr != NULL) {
          *StrPtr = 0;
        }

        SetValueByName (Storage, Name, Value, NULL);
      }

      break;

    case EFI_HII_VARSTORE_EFI_VARIABLE:
    default:
      Status = EFI_INVALID_PARAMETER;
      break;
  }

  return Status;
}

/**
  Fetch the Ifr binary data of a FormSet.

  @param[in]  Handle             PackageList Handle
  @param[in,out]  FormSetGuid    On input, GUID or class GUID of a formset. If not
                                 specified (NULL or zero GUID), take the first
                                 FormSet with class GUID EFI_HII_PLATFORM_SETUP_FORMSET_GUID
                                 found in package list.
                                 On output, GUID of the formset found(if not NULL).
  @param[out]  BinaryLength      The length of the FormSet IFR binary.
  @param[out]  BinaryData        The buffer designed to receive the FormSet.

  @retval EFI_SUCCESS            Buffer filled with the requested FormSet.
                                 BufferLength was updated.
  @retval EFI_INVALID_PARAMETER  The handle is unknown.
  @retval EFI_NOT_FOUND          A form or FormSet on the requested handle cannot
                                 be found with the requested FormId.

**/
EFI_STATUS
GetIfrBinaryData (
  IN     EFI_HII_HANDLE  Handle,
  IN OUT EFI_GUID        *FormSetGuid,
  OUT UINTN              *BinaryLength,
  OUT UINT8              **BinaryData
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  UINT8                        Index;
  UINT8                        NumberOfClassGuid;
  BOOLEAN                      ClassGuidMatch;
  EFI_GUID                     *ClassGuid;
  EFI_GUID                     *ComparingGuid;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;

  OpCodeData = NULL;
  Package    = NULL;
  ZeroMem (&PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));

  //
  // if FormSetGuid is NULL or zero GUID, return first Setup FormSet in the package list
  //
  if (FormSetGuid == NULL) {
    ComparingGuid = &gZeroGuid;
  } else {
    ComparingGuid = FormSetGuid;
  }

  //
  // Get HII PackageList
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    *BinaryData = NULL;
    return Status;
  }

  BufferSize     = 0;
  HiiPackageList = NULL;
  Status         = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    if (HiiPackageList == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  }

  if (EFI_ERROR (Status)) {
    if (HiiPackageList != NULL) {
      FreePool (HiiPackageList);
    }

    *BinaryData = NULL;
    return Status;
  }

  //
  // Get Form package from this HII package List
  //
  Offset  = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2 = 0;
  CopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));

  ClassGuidMatch = FALSE;
  while (Offset < PackageListLength) {
    Package = ((UINT8 *)HiiPackageList) + Offset;
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));

    if (PackageHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet in this Form Package
      //

      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader.Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          //
          // Try to compare against formset GUID
          //

          if (IsZeroGuid (FormSetGuid) ||
              CompareGuid (ComparingGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER))))
          {
            break;
          }

          if (((EFI_IFR_OP_HEADER *)OpCodeData)->Length > OFFSET_OF (EFI_IFR_FORM_SET, Flags)) {
            //
            // Try to compare against formset class GUID
            //
            NumberOfClassGuid = (UINT8)(((EFI_IFR_FORM_SET *)OpCodeData)->Flags & 0x3);
            ClassGuid         = (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_FORM_SET));
            for (Index = 0; Index < NumberOfClassGuid; Index++) {
              if (CompareGuid (ComparingGuid, ClassGuid + Index)) {
                ClassGuidMatch = TRUE;
                break;
              }
            }

            if (ClassGuidMatch) {
              break;
            }
          } else if (ComparingGuid == &gEfiHiiPlatformSetupFormsetGuid) {
            ClassGuidMatch = TRUE;
            break;
          }
        }

        Offset2 += ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;
      }

      if (Offset2 < PackageHeader.Length) {
        //
        // Target formset found
        //
        break;
      }
    }

    Offset += PackageHeader.Length;
  }

  if (Offset >= PackageListLength) {
    //
    // Form package not found in this Package List
    //
    FreePool (HiiPackageList);
    *BinaryData = NULL;
    return EFI_NOT_FOUND;
  }

  if (FormSetGuid != NULL) {
    //
    // Return the FormSet GUID
    //
    CopyMem (FormSetGuid, &((EFI_IFR_FORM_SET *)OpCodeData)->Guid, sizeof (EFI_GUID));
  }

  //
  // To determine the length of a whole FormSet IFR binary, one have to parse all the Opcodes
  // in this FormSet; So, here just simply copy the data from start of a FormSet to the end
  // of the Form Package.
  //

  *BinaryLength = PackageHeader.Length - Offset2;
  *BinaryData   = AllocateCopyPool (*BinaryLength, OpCodeData);
  FreePool (HiiPackageList);
  if (*BinaryData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Check if the requested element is in storage.

  @param  Storage                   The storage contains elements.
  @param  RequestElement            The element to be searched.

**/
BOOLEAN
ElementValidation (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               *RequestElement
  )
{
  return StrStr (Storage->ConfigRequest, RequestElement) != NULL ? TRUE : FALSE;
}

/**
  Append the Request element to the Config Request.

  @param  ConfigRequest          Current ConfigRequest info.
  @param  SpareStrLen            Current remain free buffer for config request.
  @param  RequestElement         New Request element.

**/
VOID
AppendConfigRequest (
  IN OUT CHAR16  **ConfigRequest,
  IN OUT UINTN   *SpareStrLen,
  IN     CHAR16  *RequestElement
  )
{
  CHAR16  *NewStr;
  UINTN   StringSize;
  UINTN   StrLength;
  UINTN   MaxLen;

  StrLength  = StrLen (RequestElement);
  StringSize = (*ConfigRequest != NULL) ? StrSize (*ConfigRequest) : sizeof (CHAR16);
  MaxLen     = StringSize / sizeof (CHAR16) + *SpareStrLen;

  //
  // Append <RequestElement> to <ConfigRequest>
  //
  if (StrLength > *SpareStrLen) {
    //
    // Old String buffer is not sufficient for RequestElement, allocate a new one
    //
    MaxLen = StringSize / sizeof (CHAR16) + CONFIG_REQUEST_STRING_INCREMENTAL;
    NewStr = AllocatePool (MaxLen * sizeof (CHAR16));
    if (NewStr == NULL) {
      return;
    }

    if (*ConfigRequest != NULL) {
      CopyMem (NewStr, *ConfigRequest, StringSize);
      FreePool (*ConfigRequest);
    } else {
      NewStr[0] = L'\0';
    }

    *ConfigRequest = NewStr;
    *SpareStrLen   = CONFIG_REQUEST_STRING_INCREMENTAL;
  }

  StrCatS (*ConfigRequest, MaxLen, RequestElement);
  *SpareStrLen -= StrLength;
}

/**
  Adjust the config request info, remove the request elements which already in AllConfigRequest string.

  @param  Storage                Form set Storage.
  @param  Request                The input request string.
  @param  RespString             Whether the input is ConfigRequest or ConfigResp format.

  @retval TRUE                   Has element not covered by current used elements, need to continue to call ExtractConfig
  @retval FALSE                  All elements covered by current used elements.

**/
BOOLEAN
ConfigRequestAdjust (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               *Request,
  IN BOOLEAN              RespString
  )
{
  CHAR16   *RequestElement;
  CHAR16   *NextRequestElement;
  CHAR16   *NextElementBackup;
  CHAR16   *SearchKey;
  CHAR16   *ValueKey;
  BOOLEAN  RetVal;
  CHAR16   *ConfigRequest;

  RetVal            = FALSE;
  NextElementBackup = NULL;
  ValueKey          = NULL;

  if (Request != NULL) {
    ConfigRequest = Request;
  } else {
    ConfigRequest = Storage->ConfigRequest;
  }

  if (Storage->ConfigRequest == NULL) {
    Storage->ConfigRequest = AllocateCopyPool (StrSize (ConfigRequest), ConfigRequest);
    if (Storage->ConfigRequest == NULL) {
      return FALSE;
    }

    return TRUE;
  }

  if (Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) {
    //
    // "&Name1&Name2" section for EFI_HII_VARSTORE_NAME_VALUE storage
    //
    SearchKey = L"&";
  } else {
    //
    // "&OFFSET=####&WIDTH=####" section for EFI_HII_VARSTORE_BUFFER storage
    //
    SearchKey = L"&OFFSET";
    ValueKey  = L"&VALUE";
  }

  //
  // Find SearchKey storage
  //
  if (Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) {
    RequestElement = StrStr (ConfigRequest, L"PATH");
    if (RequestElement == NULL) {
      return FALSE;
    }

    RequestElement = StrStr (RequestElement, SearchKey);
  } else {
    RequestElement = StrStr (ConfigRequest, SearchKey);
  }

  while (RequestElement != NULL) {
    //
    // +1 to avoid find header itself.
    //
    NextRequestElement = StrStr (RequestElement + 1, SearchKey);

    //
    // The last Request element in configRequest string.
    //
    if (NextRequestElement != NULL) {
      if (RespString && (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER)) {
        NextElementBackup  = NextRequestElement;
        NextRequestElement = StrStr (RequestElement, ValueKey);
        if (NextRequestElement == NULL) {
          return FALSE;
        }
      }

      //
      // Replace "&" with '\0'.
      //
      *NextRequestElement = L'\0';
    } else {
      if (RespString && (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER)) {
        NextElementBackup  = NextRequestElement;
        NextRequestElement = StrStr (RequestElement, ValueKey);
        if (NextRequestElement == NULL) {
          return FALSE;
        }

        //
        // Replace "&" with '\0'.
        //
        *NextRequestElement = L'\0';
      }
    }

    if (!ElementValidation (Storage, RequestElement)) {
      //
      // Add this element to the Storage->BrowserStorage->AllRequestElement.
      //
      AppendConfigRequest (&Storage->ConfigRequest, &Storage->SpareStrLen, RequestElement);
      RetVal = TRUE;
    }

    if (NextRequestElement != NULL) {
      //
      // Restore '&' with '\0' for later used.
      //
      *NextRequestElement = L'&';
    }

    if (RespString && (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER)) {
      RequestElement = NextElementBackup;
    } else {
      RequestElement = NextRequestElement;
    }
  }

  return RetVal;
}

/**
  Fill storage with settings requested from Configuration Driver.

  @param[in] FormSet                FormSet data structure.
  @param[in] Storage                Buffer Storage.

**/
VOID
LoadFormSetStorage (
  IN HII_FORMSET          *FormSet,
  IN HII_FORMSET_STORAGE  *Storage
  )
{
  EFI_STATUS                       Status;
  EFI_STRING                       Progress;
  EFI_STRING                       Result;
  CHAR16                           *StrPtr;
  EFI_STRING                       ConfigRequest;
  UINTN                            RequestStrSize;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  ConfigRequest = NULL;

  switch (Storage->Type) {
    case EFI_HII_VARSTORE_EFI_VARIABLE:
      return;

    case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:
 #if 0 // bug fix for efivarstore
      if (Storage->ConfigRequest != NULL) {
        ConfigRequestAdjust (Storage, Storage->ConfigRequest, FALSE);
        return;
      }

 #endif
      break;

    case EFI_HII_VARSTORE_BUFFER:
    case EFI_HII_VARSTORE_NAME_VALUE:
      //
      // Skip if there is no RequestElement.
      //
      if (Storage->ElementCount == 0) {
        return;
      }

      break;

    default:
      return;
  }

  if (Storage->Type != EFI_HII_VARSTORE_NAME_VALUE) {
    //
    // Create the config request string to get all fields for this storage.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWW"followed by a Null-terminator
    //
    RequestStrSize = StrSize (Storage->ConfigHdr) + 20 * sizeof (CHAR16);
    ConfigRequest  = AllocatePool (RequestStrSize);
    if (ConfigRequest == NULL) {
      return;
    }

    UnicodeSPrint (
      ConfigRequest,
      RequestStrSize,
      L"%s&OFFSET=0&WIDTH=%04x",
      Storage->ConfigHdr,
      Storage->Size
      );
  } else {
    RequestStrSize = (StrLen (Storage->ConfigHdr) + StrLen (Storage->ConfigRequest) + 1) * sizeof (CHAR16);
    ConfigRequest  = AllocatePool (RequestStrSize);
    if (ConfigRequest == NULL) {
      return;
    }

    UnicodeSPrint (
      ConfigRequest,
      RequestStrSize,
      L"%s%s",
      Storage->ConfigHdr,
      Storage->ConfigRequest
      );
  }

  //
  // Request current settings from Configuration Driver
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiConfigRoutingProtocolGuid,
                  NULL,
                  (VOID **)&HiiConfigRouting
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  Status = HiiConfigRouting->ExtractConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               &Progress,
                               &Result
                               );
  if (!EFI_ERROR (Status)) {
    //
    // Convert Result from <ConfigAltResp> to <ConfigResp>
    //
    StrPtr = StrStr (Result, L"&GUID=");
    if (StrPtr != NULL) {
      *StrPtr = L'\0';
    }

    Status = ConfigRespToStorage (Storage, Result);
    FreePool (Result);
  }

  Storage->ConfigRequest = AllocateCopyPool (StrSize (ConfigRequest), ConfigRequest);
  if (Storage->ConfigRequest == NULL) {
    if (ConfigRequest != NULL) {
      FreePool (ConfigRequest);
    }

    return;
  }

  if (Storage->Type != EFI_HII_VARSTORE_NAME_VALUE) {
    if (ConfigRequest != NULL) {
      FreePool (ConfigRequest);
    }
  }
}

/**
  Zero extend integer/boolean/date/time to UINT64 for comparing.

  @param[in]  Value                  HII Value to be converted.

**/
VOID
ExtendValueToU64 (
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
      Temp = Value->Value.u32 & 0xffffff;
      break;

    case EFI_IFR_TYPE_DATE:
      Temp = Value->Value.u32;
      break;

    default:
      return;
  }

  Value->Value.u64 = Temp;
}

/**
  Pop an element from the stack.

  @param  Stack                  On input: old stack
  @param  StackPtr               On input: old stack pointer; On output: new stack pointer
  @param  Data                   Data to pop.

  @retval EFI_SUCCESS            The value was popped onto the stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack

**/
EFI_STATUS
PopStack (
  IN     EFI_HII_VALUE  *Stack,
  IN OUT EFI_HII_VALUE  **StackPtr,
  OUT EFI_HII_VALUE     *Data
  );

/**
  Push an element onto the Boolean Stack.

  @param  Stack                  On input: old stack; On output: new stack
  @param  StackPtr               On input: old stack pointer; On output: new stack
                                 pointer
  @param  StackEnd               On input: old stack end; On output: new stack end
  @param  Data                   Data to push.

  @retval EFI_SUCCESS            Push stack success.

**/
EFI_STATUS
PushStack (
  IN OUT EFI_HII_VALUE  **Stack,
  IN OUT EFI_HII_VALUE  **StackPtr,
  IN OUT EFI_HII_VALUE  **StackEnd,
  IN     EFI_HII_VALUE  *Data
  );

/**
  Initialize the internal data structure of a FormSet.

  @param[in]      Handle         PackageList Handle
  @param[in,out]  FormSetGuid    On input, GUID or class GUID of a formset. If not
                                 specified (NULL or zero GUID), take the first
                                 FormSet with class GUID EFI_HII_PLATFORM_SETUP_FORMSET_GUID
                                 found in package list.
                                 On output, GUID of the formset found(if not NULL).
  @param[out]     FormSet        FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The specified FormSet could not be found.

**/
EFI_STATUS
CreateFormSetFromHiiHandle (
  IN     EFI_HII_HANDLE  Handle,
  IN OUT EFI_GUID        *FormSetGuid,
  OUT HII_FORMSET        *FormSet
  );

/**
  Initialize a Formset and get current setting for Questions.

  @param[in,out]  FormSet                FormSet data structure.

**/
VOID
InitializeFormSet (
  IN OUT HII_FORMSET  *FormSet
  );

/**
  Get Value for given Name from a NameValue Storage.

  @param[in]      Storage        The NameValue Storage.
  @param[in]      Name           The Name.
  @param[in,out]  Value          The returned Value.

  @retval EFI_SUCCESS            Value found for given Name.
  @retval EFI_NOT_FOUND          No such Name found in NameValue storage.
  @retval EFI_INVALID_PARAMETER  Storage or Value is NULL.

**/
EFI_STATUS
GetValueByName (
  IN HII_FORMSET_STORAGE  *Storage,
  IN CHAR16               *Name,
  IN OUT CHAR16           **Value
  )
{
  LIST_ENTRY           *Link;
  HII_NAME_VALUE_NODE  *Node;

  if ((Storage == NULL) || (Value == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *Value = NULL;

  Link = GetFirstNode (&Storage->NameValueList);
  while (!IsNull (&Storage->NameValueList, Link)) {
    Node = HII_NAME_VALUE_NODE_FROM_LINK (Link);

    if (StrCmp (Name, Node->Name) == 0) {
      NewStringCopy (Value, Node->Value);
      return EFI_SUCCESS;
    }

    Link = GetNextNode (&Storage->NameValueList, Link);
  }

  return EFI_NOT_FOUND;
}

/**
  Get Question's current Value.

  @param[in]      FormSet        FormSet data structure.
  @param[in]      Form           Form data structure.
  @param[in,out]  Question       Question to be initialized.
  @param[in]      GetValueFrom   Where to get value, may from editbuffer, buffer or hii driver.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Formset, Form or Question is NULL.

**/
EFI_STATUS
GetQuestionValue (
  IN HII_FORMSET                  *FormSet,
  IN HII_FORM                     *Form,
  IN OUT HII_STATEMENT            *Question,
  IN GET_SET_QUESTION_VALUE_WITH  GetValueFrom
  )
{
  EFI_STATUS                       Status;
  BOOLEAN                          Enabled;
  BOOLEAN                          Pending;
  UINT8                            *Dst;
  UINTN                            StorageWidth;
  EFI_TIME                         EfiTime;
  HII_FORMSET_STORAGE              *Storage;
  HII_FORMSET_STORAGE              *FormsetStorage;
  EFI_IFR_TYPE_VALUE               *QuestionValue;
  CHAR16                           *ConfigRequest;
  CHAR16                           *Progress;
  CHAR16                           *Result;
  CHAR16                           *Value;
  UINTN                            Length;
  BOOLEAN                          IsBufferStorage;
  UINTN                            MaxLen;
  EFI_HII_CONFIG_ROUTING_PROTOCOL  *HiiConfigRouting;

  if ((FormSet == NULL) || (Form == NULL) || (Question == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  Value  = NULL;
  Result = NULL;

  if (GetValueFrom >= GetSetValueWithMax) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Question value is provided by an Expression, evaluate it
  //
  if (Question->ValueExpression != NULL) {
    Status = EvaluateHiiExpression (FormSet, Form, Question->ValueExpression);
    if (!EFI_ERROR (Status)) {
      if (Question->ValueExpression->Result.Type == EFI_IFR_TYPE_BUFFER) {
        ASSERT (Question->Value.Type == EFI_IFR_TYPE_BUFFER && Question->Value.Buffer != NULL);
        if (Question->StorageWidth > Question->ValueExpression->Result.BufferLen) {
          CopyMem (Question->Value.Buffer, Question->ValueExpression->Result.Buffer, Question->ValueExpression->Result.BufferLen);
          Question->Value.BufferLen = Question->ValueExpression->Result.BufferLen;
        } else {
          CopyMem (Question->Value.Buffer, Question->ValueExpression->Result.Buffer, Question->StorageWidth);
          Question->Value.BufferLen = Question->StorageWidth;
        }

        FreePool (Question->ValueExpression->Result.Buffer);
      }

      Question->Value.Type = Question->ValueExpression->Result.Type;
      CopyMem (&Question->Value.Value, &Question->ValueExpression->Result.Value, sizeof (EFI_IFR_TYPE_VALUE));
    }

    return Status;
  }

  //
  // Get question value by read expression.
  //
  if ((Question->ReadExpression != NULL) && (Form->FormType == STANDARD_MAP_FORM_TYPE)) {
    Status = EvaluateHiiExpression (FormSet, Form, Question->ReadExpression);
    if (!EFI_ERROR (Status) &&
        ((Question->ReadExpression->Result.Type < EFI_IFR_TYPE_OTHER) || (Question->ReadExpression->Result.Type == EFI_IFR_TYPE_BUFFER)))
    {
      //
      // Only update question value to the valid result.
      //
      if (Question->ReadExpression->Result.Type == EFI_IFR_TYPE_BUFFER) {
        ASSERT (Question->Value.Type == EFI_IFR_TYPE_BUFFER && Question->Value.Buffer != NULL);
        if (Question->StorageWidth > Question->ReadExpression->Result.BufferLen) {
          CopyMem (Question->Value.Buffer, Question->ReadExpression->Result.Buffer, Question->ReadExpression->Result.BufferLen);
          Question->Value.BufferLen = Question->ReadExpression->Result.BufferLen;
        } else {
          CopyMem (Question->Value.Buffer, Question->ReadExpression->Result.Buffer, Question->StorageWidth);
          Question->Value.BufferLen = Question->StorageWidth;
        }

        FreePool (Question->ReadExpression->Result.Buffer);
      }

      Question->Value.Type = Question->ReadExpression->Result.Type;
      CopyMem (&Question->Value.Value, &Question->ReadExpression->Result.Value, sizeof (EFI_IFR_TYPE_VALUE));
      return EFI_SUCCESS;
    }
  }

  //
  // Question value is provided by RTC
  //
  Storage       = Question->Storage;
  QuestionValue = &Question->Value.Value;
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
          //
          // For date/time without storage
          //
          return EFI_SUCCESS;
      }

      if (EFI_ERROR (Status)) {
        if (Question->Operand == EFI_IFR_DATE_OP) {
          QuestionValue->date.Year  = 0xff;
          QuestionValue->date.Month = 0xff;
          QuestionValue->date.Day   = 0xff;
        } else {
          QuestionValue->time.Hour   = 0xff;
          QuestionValue->time.Minute = 0xff;
          QuestionValue->time.Second = 0xff;
        }

        return EFI_SUCCESS;
      }

      if (Question->Operand == EFI_IFR_DATE_OP) {
        QuestionValue->date.Year  = EfiTime.Year;
        QuestionValue->date.Month = EfiTime.Month;
        QuestionValue->date.Day   = EfiTime.Day;
      } else {
        QuestionValue->time.Hour   = EfiTime.Hour;
        QuestionValue->time.Minute = EfiTime.Minute;
        QuestionValue->time.Second = EfiTime.Second;
      }
    }

    return EFI_SUCCESS;
  }

  //
  // Question value is provided by EFI variable
  //
  StorageWidth = Question->StorageWidth;
  if (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
    if (Question->Value.Buffer != NULL) {
      Dst = Question->Value.Buffer;
    } else {
      Dst = (UINT8 *)QuestionValue;
    }

    Status = gRT->GetVariable (
                    Question->VariableName,
                    &Storage->Guid,
                    NULL,
                    &StorageWidth,
                    Dst
                    );
    //
    // Always return success, even this EFI variable doesn't exist
    //
    return EFI_SUCCESS;
  }

  //
  // Question Value is provided by Buffer Storage or NameValue Storage
  //
  if (Question->Value.Buffer != NULL) {
    //
    // This Question is password or orderedlist
    //
    Dst = Question->Value.Buffer;
  } else {
    //
    // Other type of Questions
    //
    Dst = (UINT8 *)&Question->Value.Value;
  }

  if ((Storage->Type == EFI_HII_VARSTORE_BUFFER) ||
      (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER))
  {
    IsBufferStorage = TRUE;
  } else {
    IsBufferStorage = FALSE;
  }

  if (GetValueFrom == GetSetValueWithBuffer ) {
    if (IsBufferStorage) {
      //
      // Copy from storage Edit buffer
      // If the Question refer to bit filed, get the value in the related bit filed.
      //
      if (Question->QuestionReferToBitField) {
        GetBitsQuestionValue (Question, Storage->Buffer + Question->VarStoreInfo.VarOffset, &Question->Value);
      } else {
        CopyMem (Dst, Storage->Buffer + Question->VarStoreInfo.VarOffset, StorageWidth);
      }
    } else {
      Value  = NULL;
      Status = GetValueByName (Storage, Question->VariableName, &Value);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      ASSERT (Value != NULL);
      Status = BufferToQuestionValue (Question, Value, &Question->Value);
      FreePool (Value);
    }
  } else {
    FormsetStorage = GetFstStgFromVarId (FormSet, Question->VarStoreId);
    ASSERT (FormsetStorage != NULL);
    //
    // <ConfigRequest> ::= <ConfigHdr> + <BlockName> ||
    //                   <ConfigHdr> + "&" + <VariableName>
    //
    if (IsBufferStorage) {
      Length  = StrLen (FormsetStorage->ConfigHdr);
      Length += StrLen (Question->BlockName);
    } else {
      Length  = StrLen (FormsetStorage->ConfigHdr);
      Length += StrLen (Question->VariableName) + 1;
    }

    // Allocate buffer include '\0'
    MaxLen        = Length + 1;
    ConfigRequest = AllocatePool (MaxLen * sizeof (CHAR16));
    ASSERT (ConfigRequest != NULL);

    StrCpyS (ConfigRequest, MaxLen, FormsetStorage->ConfigHdr);
    if (IsBufferStorage) {
      StrCatS (ConfigRequest, MaxLen, Question->BlockName);
    } else {
      StrCatS (ConfigRequest, MaxLen, L"&");
      StrCatS (ConfigRequest, MaxLen, Question->VariableName);
    }

    Status = gBS->LocateProtocol (
                    &gEfiHiiConfigRoutingProtocolGuid,
                    NULL,
                    (VOID **)&HiiConfigRouting
                    );
    if (EFI_ERROR (Status)) {
      return Status;
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
      return Status;
    }

    //
    // Skip <ConfigRequest>
    //
    if (IsBufferStorage) {
      Value = StrStr (Result, L"&VALUE");
      if (Value == NULL) {
        FreePool (Result);
        return EFI_NOT_FOUND;
      }

      //
      // Skip "&VALUE"
      //
      Value = Value + 6;
    } else {
      Value = Result + Length;
    }

    if (*Value != '=') {
      FreePool (Result);
      return EFI_NOT_FOUND;
    }

    //
    // Skip '=', point to value
    //
    Value = Value + 1;

    Status = BufferToQuestionValue (Question, Value, &Question->Value);
    if (EFI_ERROR (Status)) {
      FreePool (Result);
      return Status;
    }

    //
    // Synchronize Buffer
    //
    if (IsBufferStorage) {
      CopyMem (Storage->Buffer + Question->VarStoreInfo.VarOffset, Dst, StorageWidth);
    } else {
      SetValueByName (Storage, Question->VariableName, Value, NULL);
    }

    if (Result != NULL) {
      FreePool (Result);
    }
  }

  return Status;
}

/**
  Convert the input Unicode character to upper.

  @param[in] String  Th Unicode character to be converted.

**/
VOID
IfrStrToUpper (
  IN CHAR16  *String
  )
{
  if (String == NULL) {
    return;
  }

  while (*String != 0) {
    if ((*String >= 'a') && (*String <= 'z')) {
      *String = (UINT16)((*String) & ((UINT16) ~0x20));
    }

    String++;
  }
}

/**
  Check whether this value type can be transfer to EFI_IFR_TYPE_BUFFER type.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param[in]  Value              Expression value to compare on.

  @retval TRUE                   This value type can be transferred to EFI_IFR_TYPE_BUFFER type.
  @retval FALSE                  This value type can't be transferred to EFI_IFR_TYPE_BUFFER type.

**/
BOOLEAN
IsTypeInBuffer (
  IN  EFI_HII_VALUE  *Value
  )
{
  if (Value == NULL) {
    return FALSE;
  }

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

  @param[in]  Value              Expression value to compare on.

  @retval TRUE                   This value type can be transferred to EFI_IFR_TYPE_BUFFER type.
  @retval FALSE                  This value type can't be transferred to EFI_IFR_TYPE_BUFFER type.

**/
BOOLEAN
IsTypeInUINT64 (
  IN  EFI_HII_VALUE  *Value
  )
{
  if (Value == NULL) {
    return FALSE;
  }

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
  Return the buffer pointer for this value.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param[in]  Value              Expression value to compare on.

  @retval Buf                    Return the buffer pointer.

**/
UINT8 *
GetBufferForValue (
  IN  EFI_HII_VALUE  *Value
  )
{
  if (Value == NULL) {
    return NULL;
  }

  switch (Value->Type) {
    case EFI_IFR_TYPE_BUFFER:
      return Value->Buffer;

    case EFI_IFR_TYPE_DATE:
      return (UINT8 *)(&Value->Value.date);

    case EFI_IFR_TYPE_TIME:
      return (UINT8 *)(&Value->Value.time);

    case EFI_IFR_TYPE_REF:
      return (UINT8 *)(&Value->Value.ref);

    default:
      return NULL;
  }
}

/**
  Return the buffer length for this value.

  EFI_IFR_TYPE_REF, EFI_IFR_TYPE_DATE and EFI_IFR_TYPE_TIME are converted to
  EFI_IFR_TYPE_BUFFER when do the value compare.

  @param[in] Value                Expression value to compare on.

  @retval  BufLen                 Return the buffer length.

**/
UINT16
GetLengthForValue (
  IN  EFI_HII_VALUE  *Value
  )
{
  if (Value == NULL) {
    return 0;
  }

  switch (Value->Type) {
    case EFI_IFR_TYPE_BUFFER:
      return Value->BufferLen;

    case EFI_IFR_TYPE_DATE:
      return (UINT16)sizeof (EFI_HII_DATE);

    case EFI_IFR_TYPE_TIME:
      return (UINT16)sizeof (EFI_HII_TIME);

    case EFI_IFR_TYPE_REF:
      return (UINT16)sizeof (EFI_HII_REF);

    default:
      return 0;
  }
}

/**
  Get UINT64 type value.

  @param[in]  Value              Input Hii value.

  @retval UINT64                 Return the UINT64 type value.

**/
UINT64
HiiValueToUINT64 (
  IN EFI_HII_VALUE  *Value
  )
{
  UINT64  RetVal;

  if (Value == NULL) {
    return 0;
  }

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
  Compare two Hii value.

  @param[in]  Value1             Expression value to compare on left-hand.
  @param[in]  Value2             Expression value to compare on right-hand.
  @param[out] Result             Return value after compare.
                                 retval 0                      Two operators equal.
                                 return Positive value if Value1 is greater than Value2.
                                 retval Negative value if Value1 is less than Value2.
  @param[in]  HiiHandle          Only required for string compare.

  @retval other                  Could not perform compare on two values.
  @retval EFI_SUCCESS            Compare the value success.
  @retval EFI_INVALID_PARAMETER  Value1, Value2 or Result is NULL.

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

  if ((Value1 == NULL) || (Value2 == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

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

    Str1 = GetTokenString (Value1->Value.string, HiiHandle);
    if (Str1 == NULL) {
      //
      // String not found
      //
      return EFI_NOT_FOUND;
    }

    Str2 = GetTokenString (Value2->Value.string, HiiHandle);
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
    Buf1    = GetBufferForValue (Value1);
    Buf1Len = GetLengthForValue (Value1);
    Buf2    = GetBufferForValue (Value2);
    Buf2Len = GetLengthForValue (Value2);

    Len     = Buf1Len > Buf2Len ? Buf2Len : Buf1Len;
    *Result = CompareMem (Buf1, Buf2, Len);
    if ((*Result == 0) && (Buf1Len != Buf2Len)) {
      //
      // In this case, means base on small number buffer, the data is same
      // So which value has more data, which value is bigger.
      //
      *Result = Buf1Len > Buf2Len ? 1 : -1;
    }

    return EFI_SUCCESS;
  }

  //
  // Take types(integer, boolean) as integer
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
  Check if current user has the privilege specified by the permissions GUID.

  @param[in] Guid  A GUID specifying setup access permissions.

  @retval TRUE     Current user has the privilege.
  @retval FALSE    Current user does not have the privilege.
**/
BOOLEAN
CheckUserPrivilege (
  IN EFI_GUID  *Guid
  )
{
  EFI_STATUS                    Status;
  EFI_USER_PROFILE_HANDLE       UserProfileHandle;
  EFI_USER_INFO_HANDLE          UserInfoHandle;
  EFI_USER_INFO                 *UserInfo;
  EFI_GUID                      *UserPermissionsGuid;
  UINTN                         UserInfoSize;
  UINTN                         AccessControlDataSize;
  EFI_USER_INFO_ACCESS_CONTROL  *AccessControl;
  UINTN                         RemainSize;

  if (mUserManager == NULL) {
    Status = gBS->LocateProtocol (
                    &gEfiUserManagerProtocolGuid,
                    NULL,
                    (VOID **)&mUserManager
                    );
    if (EFI_ERROR (Status)) {
      ///
      /// If the system does not support user management, then it is assumed that
      /// all users have admin privilege and evaluation of each EFI_IFR_SECURITY
      /// op-code is always TRUE.
      ///
      return TRUE;
    }
  }

  Status = mUserManager->Current (mUserManager, &UserProfileHandle);
  ASSERT_EFI_ERROR (Status);

  ///
  /// Enumerate all user information of the current user profile
  /// to look for any EFI_USER_INFO_ACCESS_SETUP record.
  ///

  for (UserInfoHandle = NULL; ;) {
    Status = mUserManager->GetNextInfo (mUserManager, UserProfileHandle, &UserInfoHandle);
    if (EFI_ERROR (Status)) {
      break;
    }

    UserInfoSize = 0;
    Status       = mUserManager->GetInfo (mUserManager, UserProfileHandle, UserInfoHandle, NULL, &UserInfoSize);
    if (Status != EFI_BUFFER_TOO_SMALL) {
      continue;
    }

    UserInfo = (EFI_USER_INFO *)AllocatePool (UserInfoSize);
    if (UserInfo == NULL) {
      break;
    }

    Status = mUserManager->GetInfo (mUserManager, UserProfileHandle, UserInfoHandle, UserInfo, &UserInfoSize);
    if (EFI_ERROR (Status) ||
        (UserInfo->InfoType != EFI_USER_INFO_ACCESS_POLICY_RECORD) ||
        (UserInfo->InfoSize <= sizeof (EFI_USER_INFO)))
    {
      FreePool (UserInfo);
      continue;
    }

    RemainSize    = UserInfo->InfoSize - sizeof (EFI_USER_INFO);
    AccessControl = (EFI_USER_INFO_ACCESS_CONTROL *)(UserInfo + 1);
    while (RemainSize >= sizeof (EFI_USER_INFO_ACCESS_CONTROL)) {
      if ((RemainSize < AccessControl->Size) || (AccessControl->Size < sizeof (EFI_USER_INFO_ACCESS_CONTROL))) {
        break;
      }

      if (AccessControl->Type == EFI_USER_INFO_ACCESS_SETUP) {
        ///
        /// Check if current user has the privilege specified by the permissions GUID.
        ///

        UserPermissionsGuid   = (EFI_GUID *)(AccessControl + 1);
        AccessControlDataSize = AccessControl->Size - sizeof (EFI_USER_INFO_ACCESS_CONTROL);
        while (AccessControlDataSize >= sizeof (EFI_GUID)) {
          if (CompareGuid (Guid, UserPermissionsGuid)) {
            FreePool (UserInfo);
            return TRUE;
          }

          UserPermissionsGuid++;
          AccessControlDataSize -= sizeof (EFI_GUID);
        }
      }

      RemainSize   -= AccessControl->Size;
      AccessControl = (EFI_USER_INFO_ACCESS_CONTROL *)((UINT8 *)AccessControl + AccessControl->Size);
    }

    FreePool (UserInfo);
  }

  return FALSE;
}

/**
  Search a Question in Form scope using its QuestionId.

  @param[in]  Form               The form which contains this Question.
  @param[in]  QuestionId         Id of this Question.

  @retval Pointer                The Question.
  @retval NULL                   Specified Question not found in the form.

**/
HII_STATEMENT *
QuestionIdInForm (
  IN HII_FORM  *Form,
  IN UINT16    QuestionId
  )
{
  LIST_ENTRY     *Link;
  HII_STATEMENT  *Question;

  if ((QuestionId == 0) || (Form == NULL)) {
    //
    // The value of zero is reserved
    //
    return NULL;
  }

  Link = GetFirstNode (&Form->StatementListHead);
  while (!IsNull (&Form->StatementListHead, Link)) {
    Question = HII_STATEMENT_FROM_LINK (Link);

    if (Question->QuestionId == QuestionId) {
      return Question;
    }

    Link = GetNextNode (&Form->StatementListHead, Link);
  }

  return NULL;
}

/**
  Search a Question in Formset scope using its QuestionId.

  @param[in]  FormSet            The formset which contains this form.
  @param[in]  Form               The form which contains this Question.
  @param[in]  QuestionId         Id of this Question.

  @retval Pointer                The Question.
  @retval NULL                   Specified Question not found in the form.

**/
HII_STATEMENT *
QuestionIdInFormset (
  IN HII_FORMSET  *FormSet,
  IN HII_FORM     *Form,
  IN UINT16       QuestionId
  )
{
  LIST_ENTRY     *Link;
  HII_STATEMENT  *Question;

  if ((FormSet == NULL) || (Form == NULL)) {
    return NULL;
  }

  //
  // Search in the form scope first
  //
  Question = QuestionIdInForm (Form, QuestionId);
  if (Question != NULL) {
    return Question;
  }

  //
  // Search in the formset scope
  //
  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = HII_FORM_FROM_LINK (Link);

    Question = QuestionIdInForm (Form, QuestionId);
    if (Question != NULL) {
      //
      // EFI variable storage may be updated by Callback() asynchronous,
      // to keep synchronous, always reload the Question Value.
      //
      if (Question->Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE) {
        GetQuestionValue (FormSet, Form, Question, GetSetValueWithHiiDriver);
      }

      return Question;
    }

    Link = GetNextNode (&FormSet->FormListHead, Link);
  }

  return NULL;
}

/**
  Push an Expression value onto the Stack

  @param[in]  Value              Expression value to push.

  @retval EFI_SUCCESS            The value was pushed onto the stack.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.
  @retval EFI_INVALID_PARAMETER  Value is NULL.

**/
EFI_STATUS
PushExpression (
  IN EFI_HII_VALUE  *Value
  )
{
  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PushStack (
           &mExpressionEvaluationStack,
           &mExpressionEvaluationStackPointer,
           &mExpressionEvaluationStackEnd,
           Value
           );
}

/**
  Pop an Expression value from the stack.

  @param[out]  Value              Expression value to pop.

  @retval EFI_SUCCESS            The value was popped onto the stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack
  @retval EFI_INVALID_PARAMETER  Value is NULL.

**/
EFI_STATUS
PopExpression (
  OUT EFI_HII_VALUE  *Value
  )
{
  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return PopStack (
           mExpressionEvaluationStack + mExpressionEvaluationStackOffset,
           &mExpressionEvaluationStackPointer,
           Value
           );
}

/**
  Get current stack offset from stack start.

  @return Stack offset to stack start.
**/
UINTN
SaveExpressionEvaluationStackOffset (
  VOID
  )
{
  UINTN  TempStackOffset;

  TempStackOffset                  = mExpressionEvaluationStackOffset;
  mExpressionEvaluationStackOffset = mExpressionEvaluationStackPointer - mExpressionEvaluationStack;
  return TempStackOffset;
}

/**
  Restore stack offset based on input stack offset

  @param[in]  StackOffset  Offset to stack start.

**/
VOID
RestoreExpressionEvaluationStackOffset (
  UINTN  StackOffset
  )
{
  mExpressionEvaluationStackOffset = StackOffset;
}

/**
  Evaluate opcode EFI_IFR_TO_STRING.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[in]  Format             String format in EFI_IFR_TO_STRING.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrToString (
  IN HII_FORMSET      *FormSet,
  IN UINT8            Format,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value;
  CHAR16         *String;
  CHAR16         *PrintFormat;
  CHAR16         Buffer[MAXIMUM_VALUE_CHARACTERS];
  UINT8          *SrcBuf;
  UINTN          BufferSize;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PopExpression (&Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (Value.Type) {
    case EFI_IFR_TYPE_NUM_SIZE_8:
    case EFI_IFR_TYPE_NUM_SIZE_16:
    case EFI_IFR_TYPE_NUM_SIZE_32:
    case EFI_IFR_TYPE_NUM_SIZE_64:
      BufferSize = MAXIMUM_VALUE_CHARACTERS * sizeof (CHAR16);
      switch (Format) {
        case EFI_IFR_STRING_UNSIGNED_DEC:
        case EFI_IFR_STRING_SIGNED_DEC:
          PrintFormat = L"%ld";
          break;

        case EFI_IFR_STRING_LOWERCASE_HEX:
          PrintFormat = L"%lx";
          break;

        case EFI_IFR_STRING_UPPERCASE_HEX:
          PrintFormat = L"%lX";
          break;

        default:
          Result->Type = EFI_IFR_TYPE_UNDEFINED;
          return EFI_SUCCESS;
      }

      UnicodeSPrint (Buffer, BufferSize, PrintFormat, Value.Value.u64);
      String = Buffer;
      break;

    case EFI_IFR_TYPE_STRING:
      CopyMem (Result, &Value, sizeof (EFI_HII_VALUE));
      return EFI_SUCCESS;

    case EFI_IFR_TYPE_BOOLEAN:
      String = (Value.Value.b) ? L"True" : L"False";
      break;

    case EFI_IFR_TYPE_BUFFER:
    case EFI_IFR_TYPE_DATE:
    case EFI_IFR_TYPE_TIME:
    case EFI_IFR_TYPE_REF:
      //
      // + 3 is base on the unicode format, the length may be odd number,
      // so need 1 byte to align, also need 2 bytes for L'\0'.
      //
      if (Value.Type == EFI_IFR_TYPE_BUFFER) {
        SrcBuf = Value.Buffer;
      } else {
        SrcBuf = GetBufferForValue (&Value);
      }

      if (Format == EFI_IFR_STRING_ASCII) {
        PrintFormat = L"%a";
      } else {
        // Format == EFI_IFR_STRING_UNICODE
        PrintFormat = L"%s";
      }

      UnicodeSPrint (Buffer, sizeof (Buffer), PrintFormat, SrcBuf);
      String = Buffer;
      if (Value.Type == EFI_IFR_TYPE_BUFFER) {
        FreePool (Value.Buffer);
      }

      break;

    default:
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      return EFI_SUCCESS;
  }

  Result->Type         = EFI_IFR_TYPE_STRING;
  Result->Value.string = NewHiiString (String, FormSet->HiiHandle);
  return EFI_SUCCESS;
}

/**
  Evaluate opcode EFI_IFR_TO_UINT.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrToUint (
  IN HII_FORMSET      *FormSet,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value;
  CHAR16         *String;
  CHAR16         *StringPtr;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PopExpression (&Value);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Value.Type >= EFI_IFR_TYPE_OTHER) && !IsTypeInBuffer (&Value)) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  Status = EFI_SUCCESS;
  if (Value.Type == EFI_IFR_TYPE_STRING) {
    String = GetTokenString (Value.Value.string, FormSet->HiiHandle);
    if (String == NULL) {
      return EFI_NOT_FOUND;
    }

    IfrStrToUpper (String);
    StringPtr = StrStr (String, L"0X");
    if (StringPtr != NULL) {
      //
      // Hex string
      //
      Result->Value.u64 = StrHexToUint64 (String);
    } else {
      //
      // decimal string
      //
      Result->Value.u64 = StrDecimalToUint64 (String);
    }

    FreePool (String);
  } else if (IsTypeInBuffer (&Value)) {
    if (GetLengthForValue (&Value) > 8) {
      if (Value.Type == EFI_IFR_TYPE_BUFFER) {
        FreePool (Value.Buffer);
      }

      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      return EFI_SUCCESS;
    }

    Result->Value.u64 = *(UINT64 *)GetBufferForValue (&Value);
    if (Value.Type == EFI_IFR_TYPE_BUFFER) {
      FreePool (Value.Buffer);
    }
  } else {
    CopyMem (Result, &Value, sizeof (EFI_HII_VALUE));
  }

  Result->Type = EFI_IFR_TYPE_NUM_SIZE_64;
  return Status;
}

/**
  Evaluate opcode EFI_IFR_CATENATE.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrCatenate (
  IN HII_FORMSET      *FormSet,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value[2];
  CHAR16         *String[2];
  UINTN          Index;
  CHAR16         *StringPtr;
  UINTN          Size;
  UINT16         Length0;
  UINT16         Length1;
  UINT8          *TmpBuf;
  UINTN          MaxLen;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // String[0] - The second string
  // String[1] - The first string
  //
  String[0] = NULL;
  String[1] = NULL;
  StringPtr = NULL;
  Status    = EFI_SUCCESS;
  ZeroMem (Value, sizeof (Value));

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  for (Index = 0; Index < 2; Index++) {
    if ((Value[Index].Type != EFI_IFR_TYPE_STRING) && !IsTypeInBuffer (&Value[Index])) {
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    if (Value[Index].Type == EFI_IFR_TYPE_STRING) {
      String[Index] = GetTokenString (Value[Index].Value.string, FormSet->HiiHandle);
      if (String[Index] == NULL) {
        Status = EFI_NOT_FOUND;
        goto Done;
      }
    }
  }

  if (Value[0].Type == EFI_IFR_TYPE_STRING) {
    Size      = StrSize (String[0]);
    MaxLen    = (StrSize (String[1]) + Size) / sizeof (CHAR16);
    StringPtr = AllocatePool (MaxLen * sizeof (CHAR16));
    ASSERT (StringPtr != NULL);
    StrCpyS (StringPtr, MaxLen, String[1]);
    StrCatS (StringPtr, MaxLen, String[0]);

    Result->Type         = EFI_IFR_TYPE_STRING;
    Result->Value.string = NewHiiString (StringPtr, FormSet->HiiHandle);
  } else {
    Result->Type      = EFI_IFR_TYPE_BUFFER;
    Length0           = GetLengthForValue (&Value[0]);
    Length1           = GetLengthForValue (&Value[1]);
    Result->BufferLen = (UINT16)(Length0 + Length1);

    Result->Buffer = AllocatePool (Result->BufferLen);
    ASSERT (Result->Buffer != NULL);

    TmpBuf = GetBufferForValue (&Value[0]);
    ASSERT (TmpBuf != NULL);
    CopyMem (Result->Buffer, TmpBuf, Length0);
    TmpBuf = GetBufferForValue (&Value[1]);
    ASSERT (TmpBuf != NULL);
    CopyMem (&Result->Buffer[Length0], TmpBuf, Length1);
  }

Done:
  if (Value[0].Buffer != NULL) {
    FreePool (Value[0].Buffer);
  }

  if (Value[1].Buffer != NULL) {
    FreePool (Value[1].Buffer);
  }

  if (String[0] != NULL) {
    FreePool (String[0]);
  }

  if (String[1] != NULL) {
    FreePool (String[1]);
  }

  if (StringPtr != NULL) {
    FreePool (StringPtr);
  }

  return Status;
}

/**
  Evaluate opcode EFI_IFR_MATCH.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrMatch (
  IN   HII_FORMSET    *FormSet,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value[2];
  CHAR16         *String[2];
  UINTN          Index;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // String[0] - The string to search
  // String[1] - pattern
  //
  String[0] = NULL;
  String[1] = NULL;
  Status    = EFI_SUCCESS;
  ZeroMem (Value, sizeof (Value));

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  for (Index = 0; Index < 2; Index++) {
    if (Value[Index].Type != EFI_IFR_TYPE_STRING) {
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    String[Index] = GetTokenString (Value[Index].Value.string, FormSet->HiiHandle);
    if (String[Index] == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
  }

  Result->Type    = EFI_IFR_TYPE_BOOLEAN;
  Result->Value.b = mUnicodeCollation->MetaiMatch (mUnicodeCollation, String[0], String[1]);

Done:
  if (String[0] != NULL) {
    FreePool (String[0]);
  }

  if (String[1] != NULL) {
    FreePool (String[1]);
  }

  return Status;
}

/**
  Evaluate opcode EFI_IFR_MATCH2.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[in]  SyntaxType         Syntax type for match2.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrMatch2 (
  IN HII_FORMSET      *FormSet,
  IN EFI_GUID         *SyntaxType,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS                       Status;
  EFI_HII_VALUE                    Value[2];
  CHAR16                           *String[2];
  UINTN                            Index;
  UINTN                            GuidIndex;
  EFI_HANDLE                       *HandleBuffer;
  UINTN                            BufferSize;
  EFI_REGULAR_EXPRESSION_PROTOCOL  *RegularExpressionProtocol;
  UINTN                            RegExSyntaxTypeListSize;
  EFI_REGEX_SYNTAX_TYPE            *RegExSyntaxTypeList;
  UINTN                            CapturesCount;

  if ((FormSet == NULL) || (SyntaxType == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // String[0] - The string to search
  // String[1] - pattern
  //
  String[0]           = NULL;
  String[1]           = NULL;
  HandleBuffer        = NULL;
  RegExSyntaxTypeList = NULL;
  Status              = EFI_SUCCESS;
  ZeroMem (Value, sizeof (Value));

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  for (Index = 0; Index < 2; Index++) {
    if (Value[Index].Type != EFI_IFR_TYPE_STRING) {
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    String[Index] = GetTokenString (Value[Index].Value.string, FormSet->HiiHandle);
    if (String[Index] == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
  }

  BufferSize   = 0;
  HandleBuffer = NULL;
  Status       = gBS->LocateHandle (
                        ByProtocol,
                        &gEfiRegularExpressionProtocolGuid,
                        NULL,
                        &BufferSize,
                        HandleBuffer
                        );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuffer = AllocatePool (BufferSize);
    if (HandleBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    Status = gBS->LocateHandle (
                    ByProtocol,
                    &gEfiRegularExpressionProtocolGuid,
                    NULL,
                    &BufferSize,
                    HandleBuffer
                    );
  }

  if (EFI_ERROR (Status)) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    Status       = EFI_SUCCESS;
    goto Done;
  }

  ASSERT (HandleBuffer != NULL);
  for ( Index = 0; Index < BufferSize / sizeof (EFI_HANDLE); Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Index],
                    &gEfiRegularExpressionProtocolGuid,
                    (VOID **)&RegularExpressionProtocol
                    );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    RegExSyntaxTypeListSize = 0;
    RegExSyntaxTypeList     = NULL;

    Status = RegularExpressionProtocol->GetInfo (
                                          RegularExpressionProtocol,
                                          &RegExSyntaxTypeListSize,
                                          RegExSyntaxTypeList
                                          );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      RegExSyntaxTypeList = AllocatePool (RegExSyntaxTypeListSize);
      if (RegExSyntaxTypeList == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      Status = RegularExpressionProtocol->GetInfo (
                                            RegularExpressionProtocol,
                                            &RegExSyntaxTypeListSize,
                                            RegExSyntaxTypeList
                                            );
    } else if (EFI_ERROR (Status)) {
      goto Done;
    }

    for (GuidIndex = 0; GuidIndex < RegExSyntaxTypeListSize / sizeof (EFI_GUID); GuidIndex++) {
      if (CompareGuid (&RegExSyntaxTypeList[GuidIndex], SyntaxType)) {
        //
        // Find the match type, return the value.
        //
        Result->Type = EFI_IFR_TYPE_BOOLEAN;
        Status       = RegularExpressionProtocol->MatchString (
                                                    RegularExpressionProtocol,
                                                    String[0],
                                                    String[1],
                                                    SyntaxType,
                                                    &Result->Value.b,
                                                    NULL,
                                                    &CapturesCount
                                                    );
        goto Done;
      }
    }

    if (RegExSyntaxTypeList != NULL) {
      FreePool (RegExSyntaxTypeList);
    }
  }

  //
  // Type specified by SyntaxType is not supported
  // in any of the EFI_REGULAR_EXPRESSION_PROTOCOL instances.
  //
  Result->Type = EFI_IFR_TYPE_UNDEFINED;
  Status       = EFI_SUCCESS;

Done:
  if (String[0] != NULL) {
    FreePool (String[0]);
  }

  if (String[1] != NULL) {
    FreePool (String[1]);
  }

  if (RegExSyntaxTypeList != NULL) {
    FreePool (RegExSyntaxTypeList);
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return Status;
}

/**
  Evaluate opcode EFI_IFR_FIND.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[in]  Format             Case sensitive or insensitive.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrFind (
  IN HII_FORMSET      *FormSet,
  IN UINT8            Format,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value[3];
  CHAR16         *String[2];
  UINTN          Base;
  CHAR16         *StringPtr;
  UINTN          Index;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Value, sizeof (Value));

  if (Format > EFI_IFR_FF_CASE_INSENSITIVE) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[2]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Value[0].Type > EFI_IFR_TYPE_NUM_SIZE_64) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  Base = (UINTN)Value[0].Value.u64;

  //
  // String[0] - sub-string
  // String[1] - The string to search
  //
  String[0] = NULL;
  String[1] = NULL;
  for (Index = 0; Index < 2; Index++) {
    if (Value[Index + 1].Type != EFI_IFR_TYPE_STRING) {
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    String[Index] = GetTokenString (Value[Index + 1].Value.string, FormSet->HiiHandle);
    if (String[Index] == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    if (Format == EFI_IFR_FF_CASE_INSENSITIVE) {
      //
      // Case insensitive, convert both string to upper case
      //
      IfrStrToUpper (String[Index]);
    }
  }

  Result->Type = EFI_IFR_TYPE_NUM_SIZE_64;
  if (Base >= StrLen (String[1])) {
    Result->Value.u64 = 0xFFFFFFFFFFFFFFFFULL;
  } else {
    StringPtr         = StrStr (String[1] + Base, String[0]);
    Result->Value.u64 = (StringPtr == NULL) ? 0xFFFFFFFFFFFFFFFFULL : (StringPtr - String[1]);
  }

Done:
  if (String[0] != NULL) {
    FreePool (String[0]);
  }

  if (String[1] != NULL) {
    FreePool (String[1]);
  }

  return Status;
}

/**
  Evaluate opcode EFI_IFR_MID.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrMid (
  IN   HII_FORMSET    *FormSet,
  OUT  EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value[3];
  CHAR16         *String;
  UINTN          Base;
  UINTN          Length;
  CHAR16         *SubString;
  UINT16         BufferLen;
  UINT8          *Buffer;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Value, sizeof (Value));

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[2]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Value[0].Type > EFI_IFR_TYPE_NUM_SIZE_64) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  Length = (UINTN)Value[0].Value.u64;

  if (Value[1].Type > EFI_IFR_TYPE_NUM_SIZE_64) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  Base = (UINTN)Value[1].Value.u64;

  if ((Value[2].Type != EFI_IFR_TYPE_STRING) && !IsTypeInBuffer (&Value[2])) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  if (Value[2].Type == EFI_IFR_TYPE_STRING) {
    String = GetTokenString (Value[2].Value.string, FormSet->HiiHandle);
    if (String == NULL) {
      return EFI_NOT_FOUND;
    }

    if ((Length == 0) || (Base >= StrLen (String))) {
      SubString = gEmptyString;
    } else {
      SubString = String + Base;
      if ((Base + Length) < StrLen (String)) {
        SubString[Length] = L'\0';
      }
    }

    Result->Type         = EFI_IFR_TYPE_STRING;
    Result->Value.string = NewHiiString (SubString, FormSet->HiiHandle);

    FreePool (String);
  } else {
    BufferLen = GetLengthForValue (&Value[2]);
    Buffer    = GetBufferForValue (&Value[2]);

    Result->Type = EFI_IFR_TYPE_BUFFER;
    if ((Length == 0) || (Base >= BufferLen)) {
      Result->BufferLen = 0;
      Result->Buffer    = NULL;
    } else {
      Result->BufferLen = (UINT16)((BufferLen - Base) < Length ? (BufferLen - Base) : Length);
      Result->Buffer    = AllocatePool (Result->BufferLen);
      ASSERT (Result->Buffer != NULL);
      CopyMem (Result->Buffer, &Buffer[Base], Result->BufferLen);
    }

    if (Value[2].Type == EFI_IFR_TYPE_BUFFER) {
      FreePool (Value[2].Buffer);
    }
  }

  return Status;
}

/**
  Evaluate opcode EFI_IFR_TOKEN.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrToken (
  IN  HII_FORMSET    *FormSet,
  OUT EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value[3];
  CHAR16         *String[2];
  UINTN          Count;
  CHAR16         *Delimiter;
  CHAR16         *SubString;
  CHAR16         *StringPtr;
  UINTN          Index;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Value, sizeof (Value));

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[2]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Value[0].Type > EFI_IFR_TYPE_NUM_SIZE_64) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  Count = (UINTN)Value[0].Value.u64;

  //
  // String[0] - Delimiter
  // String[1] - The string to search
  //
  String[0] = NULL;
  String[1] = NULL;
  for (Index = 0; Index < 2; Index++) {
    if (Value[Index + 1].Type != EFI_IFR_TYPE_STRING) {
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    String[Index] = GetTokenString (Value[Index + 1].Value.string, FormSet->HiiHandle);
    if (String[Index] == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
  }

  Delimiter = String[0];
  SubString = String[1];
  while (Count > 0) {
    SubString = StrStr (SubString, Delimiter);
    if (SubString != NULL) {
      //
      // Skip over the delimiter
      //
      SubString = SubString + StrLen (Delimiter);
    } else {
      break;
    }

    Count--;
  }

  if (SubString == NULL) {
    //
    // nth delimited sub-string not found, push an empty string
    //
    SubString = gEmptyString;
  } else {
    //
    // Put a NULL terminator for nth delimited sub-string
    //
    StringPtr = StrStr (SubString, Delimiter);
    if (StringPtr != NULL) {
      *StringPtr = L'\0';
    }
  }

  Result->Type         = EFI_IFR_TYPE_STRING;
  Result->Value.string = NewHiiString (SubString, FormSet->HiiHandle);

Done:
  if (String[0] != NULL) {
    FreePool (String[0]);
  }

  if (String[1] != NULL) {
    FreePool (String[1]);
  }

  return Status;
}

/**
  Evaluate opcode EFI_IFR_SPAN.

  @param[in]  FormSet            Formset which contains this opcode.
  @param[in]  Flags              FIRST_MATCHING or FIRST_NON_MATCHING.
  @param[out] Result             Evaluation result for this opcode.

  @retval EFI_SUCCESS            Opcode evaluation success.
  @retval Other                  Opcode evaluation failed.

**/
EFI_STATUS
IfrSpan (
  IN  HII_FORMSET    *FormSet,
  IN  UINT8          Flags,
  OUT EFI_HII_VALUE  *Result
  )
{
  EFI_STATUS     Status;
  EFI_HII_VALUE  Value[3];
  CHAR16         *String[2];
  CHAR16         *Charset;
  UINTN          Base;
  UINTN          Index;
  CHAR16         *StringPtr;
  BOOLEAN        Found;

  if ((FormSet == NULL) || (Result == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (Value, sizeof (Value));

  Status = PopExpression (&Value[0]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[1]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = PopExpression (&Value[2]);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Value[0].Type > EFI_IFR_TYPE_NUM_SIZE_64) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    return EFI_SUCCESS;
  }

  Base = (UINTN)Value[0].Value.u64;

  //
  // String[0] - Charset
  // String[1] - The string to search
  //
  String[0] = NULL;
  String[1] = NULL;
  for (Index = 0; Index < 2; Index++) {
    if (Value[Index + 1].Type != EFI_IFR_TYPE_STRING) {
      Result->Type = EFI_IFR_TYPE_UNDEFINED;
      Status       = EFI_SUCCESS;
      goto Done;
    }

    String[Index] = GetTokenString (Value[Index + 1].Value.string, FormSet->HiiHandle);
    if (String[Index] == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
  }

  if (Base >= StrLen (String[1])) {
    Result->Type = EFI_IFR_TYPE_UNDEFINED;
    Status       = EFI_SUCCESS;
    goto Done;
  }

  Found     = FALSE;
  StringPtr = String[1] + Base;
  Charset   = String[0];
  while (*StringPtr != 0 && !Found) {
    Index = 0;
    while (Charset[Index] != 0) {
      if ((*StringPtr >= Charset[Index]) && (*StringPtr <= Charset[Index + 1])) {
        if (Flags == EFI_IFR_FLAGS_FIRST_MATCHING) {
          Found = TRUE;
          break;
        }
      } else {
        if (Flags == EFI_IFR_FLAGS_FIRST_NON_MATCHING) {
          Found = TRUE;
          break;
        }
      }

      //
      // Skip characters pair representing low-end of a range and high-end of a range
      //
      Index += 2;
    }

    if (!Found) {
      StringPtr++;
    }
  }

  Result->Type      = EFI_IFR_TYPE_NUM_SIZE_64;
  Result->Value.u64 = StringPtr - String[1];

Done:
  if (String[0] != NULL) {
    FreePool (String[0]);
  }

  if (String[1] != NULL) {
    FreePool (String[1]);
  }

  return Status;
}

/**
  Get Expression given its RuleId.

  @param[in]  Form               The form which contains this Expression.
  @param[in]  RuleId             Id of this Expression.

  @retval Pointer                The Expression.
  @retval NULL                   Specified Expression not found in the form.

**/
HII_EXPRESSION *
RuleIdToExpression (
  IN HII_FORM  *Form,
  IN UINT8     RuleId
  )
{
  LIST_ENTRY      *Link;
  HII_EXPRESSION  *Expression;

  if (Form == NULL) {
    return NULL;
  }

  Link = GetFirstNode (&Form->RuleListHead);
  while (!IsNull (&Form->RuleListHead, Link)) {
    Expression = HII_EXPRESSION_FROM_LINK (Link);

    if ((Expression->Type == EFI_HII_EXPRESSION_RULE) && (Expression->ExtraData.RuleId == RuleId)) {
      return Expression;
    }

    Link = GetNextNode (&Form->RuleListHead, Link);
  }

  return NULL;
}

/**
  Locate the Unicode Collation Protocol interface for later use.

  @retval EFI_SUCCESS            Protocol interface initialize success.
  @retval Other                  Protocol interface initialize failed.

**/
EFI_STATUS
InitializeUnicodeCollationProtocol (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mUnicodeCollation != NULL) {
    return EFI_SUCCESS;
  }

  //
  // BUGBUG: Proper implementation is to locate all Unicode Collation Protocol
  // instances first and then select one which support English language.
  // Current implementation just pick the first instance.
  //
  Status = gBS->LocateProtocol (
                  &gEfiUnicodeCollation2ProtocolGuid,
                  NULL,
                  (VOID **)&mUnicodeCollation
                  );
  return Status;
}

/**
  Check whether the formset guid is in this Hii package list.

  @param[in]  HiiHandle          The HiiHandle for this HII package list.
  @param[in]  FormSetGuid        The formset guid for the request formset.

  @retval TRUE                   Find the formset guid.
  @retval FALSE                  Not found the formset guid.

**/
BOOLEAN
IsFormsetGuidInHiiHandle (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_GUID        *FormSetGuid
  )
{
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINTN                        BufferSize;
  UINT32                       Offset;
  UINT32                       Offset2;
  UINT32                       PackageListLength;
  EFI_HII_PACKAGE_HEADER       *PackageHeader;
  UINT8                        *Package;
  UINT8                        *OpCodeData;
  EFI_STATUS                   Status;
  BOOLEAN                      FindGuid;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;

  if (FormSetGuid == NULL) {
    return FALSE;
  }

  BufferSize     = 0;
  HiiPackageList = NULL;
  FindGuid       = FALSE;

  //
  // Locate required Hii Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  Status = HiiDatabase->ExportPackageLists (HiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = AllocatePool (BufferSize);
    if (HiiPackageList == NULL) {
      ASSERT (HiiPackageList != NULL);
      return FALSE;
    }

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  } else {
    //
    // Fix coverity: 14253 Explicit null dereferenced.
    // When calling ExportPackageLists with BufferSize = 0, only EFI_BUFFER_TOO_SMALL is expected.
    // Otherwise, return FALSE immediately.
    //
    return FALSE;
  }

  if (EFI_ERROR (Status)) {
    if (HiiPackageList != NULL) {
      FreePool (HiiPackageList);
    }

    return FALSE;
  }

  //
  // Get Form package from this HII package List
  //
  Offset            = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  Offset2           = 0;
  PackageListLength = HiiPackageList->PackageLength;

  while (Offset < PackageListLength) {
    Package       = ((UINT8 *)HiiPackageList) + Offset;
    PackageHeader = (EFI_HII_PACKAGE_HEADER *)Package;
    Offset       += PackageHeader->Length;

    if (PackageHeader->Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Search FormSet in this Form Package
      //
      Offset2 = sizeof (EFI_HII_PACKAGE_HEADER);
      while (Offset2 < PackageHeader->Length) {
        OpCodeData = Package + Offset2;

        if (((EFI_IFR_OP_HEADER *)OpCodeData)->OpCode == EFI_IFR_FORM_SET_OP) {
          if (CompareGuid (FormSetGuid, (EFI_GUID *)(OpCodeData + sizeof (EFI_IFR_OP_HEADER)))) {
            FindGuid = TRUE;
            break;
          }
        }

        Offset2 += ((EFI_IFR_OP_HEADER *)OpCodeData)->Length;
      }
    }

    if (FindGuid) {
      break;
    }
  }

  FreePool (HiiPackageList);

  return FindGuid;
}

/**
  Find HII Handle in the HII database associated with given Device Path.

  If DevicePath is NULL, then ASSERT.

  @param[in]  DevicePath         Device Path associated with the HII package list
                                 handle.
  @param[in]  FormsetGuid        The formset guid for this formset.

  @retval Handle                 HII package list Handle associated with the Device
                                        Path.
  @retval NULL                   Hii Package list handle is not found.

**/
EFI_HII_HANDLE
DevicePathToHiiHandle (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_GUID                  *FormsetGuid
  )
{
  EFI_STATUS                 Status;
  EFI_DEVICE_PATH_PROTOCOL   *TmpDevicePath;
  UINTN                      Index;
  EFI_HANDLE                 Handle;
  EFI_HANDLE                 DriverHandle;
  EFI_HII_HANDLE             *HiiHandles;
  EFI_HII_HANDLE             HiiHandle;
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;

  if ((DevicePath == NULL) || (FormsetGuid == NULL)) {
    return NULL;
  }

  TmpDevicePath = DevicePath;
  //
  // Locate Device Path Protocol handle buffer
  //
  Status = gBS->LocateDevicePath (
                  &gEfiDevicePathProtocolGuid,
                  &TmpDevicePath,
                  &DriverHandle
                  );
  if (EFI_ERROR (Status) || !IsDevicePathEnd (TmpDevicePath)) {
    return NULL;
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
    return NULL;
  }

  //
  // Retrieve all HII Handles from HII database
  //
  HiiHandles = HiiGetHiiHandles (NULL);
  if (HiiHandles == NULL) {
    return NULL;
  }

  //
  // Search Hii Handle by Driver Handle
  //
  HiiHandle = NULL;
  for (Index = 0; HiiHandles[Index] != NULL; Index++) {
    Status = HiiDatabase->GetPackageListHandle (
                            HiiDatabase,
                            HiiHandles[Index],
                            &Handle
                            );
    if (!EFI_ERROR (Status) && (Handle == DriverHandle)) {
      if (IsFormsetGuidInHiiHandle (HiiHandles[Index], FormsetGuid)) {
        HiiHandle = HiiHandles[Index];
        break;
      }

      if (HiiHandle != NULL) {
        break;
      }
    }
  }

  FreePool (HiiHandles);
  return HiiHandle;
}

/**
  Get question value from the predefined formset.

  @param[in]  DevicePath         The driver's device path which produce the formset data.
  @param[in]  InputHiiHandle     The hii handle associate with the formset data.
  @param[in]  FormSetGuid        The formset guid which include the question.
  @param[in]  QuestionId         The question id which need to get value from.
  @param[out] Value              The return data about question's value.

  @retval TRUE                   Get the question value success.
  @retval FALSE                  Get the question value failed.
**/
BOOLEAN
GetQuestionValueFromForm (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN EFI_HII_HANDLE            InputHiiHandle,
  IN EFI_GUID                  *FormSetGuid,
  IN EFI_QUESTION_ID           QuestionId,
  OUT EFI_HII_VALUE            *Value
  )
{
  EFI_STATUS      Status;
  EFI_HII_HANDLE  HiiHandle;
  HII_STATEMENT   *Question;
  HII_FORMSET     *FormSet;
  HII_FORM        *Form;
  BOOLEAN         GetTheVal;
  LIST_ENTRY      *Link;

  //
  // The input parameter DevicePath or InputHiiHandle must have one valid input.
  //
  if ((DevicePath == NULL) && (InputHiiHandle == NULL)) {
    ASSERT (
      (DevicePath != NULL && InputHiiHandle == NULL) ||
      (DevicePath == NULL && InputHiiHandle != NULL)
      );
    return FALSE;
  }

  if ((FormSetGuid == NULL) || (Value == NULL)) {
    return FALSE;
  }

  GetTheVal = TRUE;
  HiiHandle = NULL;
  Question  = NULL;
  Form      = NULL;

  //
  // Get HiiHandle.
  //
  if (DevicePath != NULL) {
    HiiHandle = DevicePathToHiiHandle (DevicePath, FormSetGuid);
    if (HiiHandle == NULL) {
      return FALSE;
    }
  } else {
    HiiHandle = InputHiiHandle;
  }

  ASSERT (HiiHandle != NULL);

  //
  // Get the formset data include this question.
  //
  FormSet = AllocateZeroPool (sizeof (HII_FORMSET));
  ASSERT (FormSet != NULL);

  Status = CreateFormSetFromHiiHandle (HiiHandle, FormSetGuid, FormSet);
  if (EFI_ERROR (Status)) {
    GetTheVal = FALSE;
    goto Done;
  }

  InitializeFormSet (FormSet);

  //
  // Base on the Question Id to get the question info.
  //
  Question = QuestionIdInFormset (FormSet, NULL, QuestionId);
  if (Question == NULL) {
    GetTheVal = FALSE;
    goto Done;
  }

  //
  // Search form in the formset scope
  //
  Link = GetFirstNode (&FormSet->FormListHead);
  while (!IsNull (&FormSet->FormListHead, Link)) {
    Form = HII_FORM_FROM_LINK (Link);

    Question = QuestionIdInForm (Form, QuestionId);
    if (Question != NULL) {
      break;
    }

    Link = GetNextNode (&FormSet->FormListHead, Link);
    Form = NULL;
  }

  ASSERT (Form != NULL);

  //
  // Get the question value.
  //
  Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
  if (EFI_ERROR (Status)) {
    GetTheVal = FALSE;
    goto Done;
  }

  CopyMem (Value, &Question->Value, sizeof (EFI_HII_VALUE));

Done:
  //
  // Clean the formset structure and restore the global parameter.
  //
  if (FormSet != NULL) {
    DestroyFormSet (FormSet);
  }

  return GetTheVal;
}

/**
  Covert HII_STATEMENT_VALUE to EFI_HII_VALUE.

  The HiiValue->Buffer is allocated from EFI boot services memory. It is the
  responsibility of the caller to free the memory allocated.

  @param[in]  StatementValue     Source to be converted.
  @param[out] HiiValue           The buffer that is converted from StatementValue

  @retval EFI_SUCCESS            Convert successfully.
  @retval EFI_INVALID_PARAMETER  StatementValue is NULL or HiiValue is NULL.

**/
EFI_STATUS
HiiStatementValueToHiiValue (
  IN  HII_STATEMENT_VALUE  *StatementValue,
  OUT EFI_HII_VALUE        *HiiValue
  )
{
  if ((StatementValue == NULL) || (HiiValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (HiiValue, sizeof (EFI_HII_VALUE));

  HiiValue->Type      = StatementValue->Type;
  HiiValue->BufferLen = StatementValue->BufferLen;
  if ((StatementValue->Buffer != NULL) && (StatementValue->BufferLen > 0)) {
    HiiValue->Buffer = AllocateCopyPool (HiiValue->BufferLen, StatementValue->Buffer);
  }

  CopyMem (&HiiValue->Value, &StatementValue->Value, sizeof (EFI_IFR_TYPE_VALUE));

  return EFI_SUCCESS;
}

/**
  Release the buffer in EFI_HII_VALUE if the buffer is not NULL.

  @param[in] HiiValue           The buffer to be released.

  @retval EFI_SUCCESS            release HiiValue successfully.
  @retval EFI_INVALID_PARAMETER  HiiValue is NULL.

**/
EFI_STATUS
ReleaseHiiValue (
  IN EFI_HII_VALUE  *HiiValue
  )
{
  if (HiiValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (HiiValue->Buffer == NULL) {
    return EFI_SUCCESS;
  }

  FreePool (HiiValue->Buffer);
  ZeroMem (HiiValue, sizeof (EFI_HII_VALUE));

  return EFI_SUCCESS;
}

/**
  Evaluate the result of a HII expression.

  If Expression is NULL, then ASSERT.

  @param[in]      FormSet        FormSet associated with this expression.
  @param[in]      Form           Form associated with this expression.
  @param[in,out]  Expression     Expression to be evaluated.

  @retval EFI_SUCCESS            The expression evaluated successfully.
  @retval EFI_NOT_FOUND          The Question which referenced by a QuestionId
                                 could not be found.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack.
  @retval EFI_INVALID_PARAMETER  Syntax error with the Expression.
  @retval EFI_INVALID_PARAMETER  Formset, Form or Expression is NULL.

**/
EFI_STATUS
EvaluateHiiExpression (
  IN     HII_FORMSET     *FormSet,
  IN     HII_FORM        *Form,
  IN OUT HII_EXPRESSION  *Expression
  )
{
  EFI_STATUS                          Status;
  LIST_ENTRY                          *Link;
  HII_EXPRESSION_OPCODE               *OpCode;
  HII_STATEMENT                       *Question;
  HII_STATEMENT                       *Question2;
  UINT16                              Index;
  EFI_HII_VALUE                       Data1;
  EFI_HII_VALUE                       Data2;
  EFI_HII_VALUE                       Data3;
  HII_EXPRESSION                      *RuleExpression;
  EFI_HII_VALUE                       *Value;
  INTN                                Result;
  CHAR16                              *StrPtr;
  CHAR16                              *NameValue;
  UINT32                              TempValue;
  LIST_ENTRY                          *SubExpressionLink;
  HII_EXPRESSION                      *SubExpression;
  UINTN                               StackOffset;
  UINTN                               TempLength;
  CHAR16                              TempStr[5];
  UINT8                               DigitUint8;
  UINT8                               *TempBuffer;
  EFI_TIME                            EfiTime;
  EFI_HII_VALUE                       QuestionVal;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *PathFromText;

  if ((FormSet == NULL) || (Form == NULL) || (Expression == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  StrPtr       = NULL;
  DevicePath   = NULL;
  PathFromText = NULL;

  //
  // Save current stack offset.
  //
  StackOffset = SaveExpressionEvaluationStackOffset ();

  ASSERT (Expression != NULL);
  Expression->Result.Type = EFI_IFR_TYPE_OTHER;

  Link = GetFirstNode (&Expression->OpCodeListHead);
  while (!IsNull (&Expression->OpCodeListHead, Link)) {
    OpCode = HII_EXPRESSION_OPCODE_FROM_LINK (Link);

    Link = GetNextNode (&Expression->OpCodeListHead, Link);

    ZeroMem (&Data1, sizeof (EFI_HII_VALUE));
    ZeroMem (&Data2, sizeof (EFI_HII_VALUE));
    ZeroMem (&Data3, sizeof (EFI_HII_VALUE));

    Value       = &Data3;
    Value->Type = EFI_IFR_TYPE_BOOLEAN;
    Status      = EFI_SUCCESS;

    switch (OpCode->Operand) {
      //
      // Built-in functions
      //
      case EFI_IFR_EQ_ID_VAL_OP:
        Question = QuestionIdInFormset (FormSet, Form, OpCode->ExtraData.EqIdValData.QuestionId);
        if (Question == NULL) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        //
        // Load value from storage.
        //
        Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Status = HiiStatementValueToHiiValue (&Question->Value, &Data1);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Status = CompareHiiValue (&Data1, &OpCode->ExtraData.EqIdValData.Value, &Result, NULL);
        ReleaseHiiValue (&Data1);
        if (Status == EFI_UNSUPPORTED) {
          Status      = EFI_SUCCESS;
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Value->Value.b = (BOOLEAN)((Result == 0) ? TRUE : FALSE);
        break;

      case EFI_IFR_EQ_ID_ID_OP:
        Question = QuestionIdInFormset (FormSet, Form, OpCode->ExtraData.EqIdIdData.QuestionId1);
        if (Question == NULL) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        //
        // Load value from storage.
        //
        Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Status = HiiStatementValueToHiiValue (&Question->Value, &Data1);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Question2 = QuestionIdInFormset (FormSet, Form, OpCode->ExtraData.EqIdIdData.QuestionId2);
        if (Question2 == NULL) {
          ReleaseHiiValue (&Data1);
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        //
        // Load value from storage.
        //
        Status = GetQuestionValue (FormSet, Form, Question2, GetSetValueWithBuffer);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Status = HiiStatementValueToHiiValue (&Question2->Value, &Data2);
        if (EFI_ERROR (Status)) {
          ReleaseHiiValue (&Data1);
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Status = CompareHiiValue (&Data1, &Data2, &Result, FormSet->HiiHandle);
        ReleaseHiiValue (&Data1);
        ReleaseHiiValue (&Data2);
        if (Status == EFI_UNSUPPORTED) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          Status      = EFI_SUCCESS;
          break;
        }

        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Value->Value.b = (BOOLEAN)((Result == 0) ? TRUE : FALSE);
        break;

      case EFI_IFR_EQ_ID_VAL_LIST_OP:
        Question = QuestionIdInFormset (FormSet, Form, OpCode->ExtraData.EqIdListData.QuestionId);
        if (Question == NULL) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        //
        // Load value from storage.
        //
        Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value->Value.b = FALSE;
        for (Index = 0; Index < OpCode->ExtraData.EqIdListData.ListLength; Index++) {
          if (Question->Value.Value.u16 == OpCode->ExtraData.EqIdListData.ValueList[Index]) {
            Value->Value.b = TRUE;
            break;
          }
        }

        break;

      case EFI_IFR_DUP_OP:
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Status = PushExpression (Value);
        break;

      case EFI_IFR_QUESTION_REF1_OP:
      case EFI_IFR_THIS_OP:
        Question = QuestionIdInFormset (FormSet, Form, OpCode->ExtraData.QuestionRef1Data.QuestionId);
        if (Question == NULL) {
          Status = EFI_NOT_FOUND;
          goto Done;
        }

        //
        // Load value from storage.
        //
        Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Value = (EFI_HII_VALUE *)&Question->Value;
        break;

      case EFI_IFR_SECURITY_OP:
        Value->Value.b = CheckUserPrivilege (&OpCode->ExtraData.Guid);
        break;

      case EFI_IFR_GET_OP:
        //
        // Get Value from VarStore buffer, EFI VarStore, Name/Value VarStore.
        //
        Value->Type     = EFI_IFR_TYPE_UNDEFINED;
        Value->Value.u8 = 0;
        if (OpCode->ExtraData.GetSetData.VarStorage != NULL) {
          switch (OpCode->ExtraData.GetSetData.VarStorage->Type) {
            case EFI_HII_VARSTORE_BUFFER:
            case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:
              //
              // Get value from Edit Buffer
              //
              Value->Type = OpCode->ExtraData.GetSetData.ValueType;
              CopyMem (&Value->Value, OpCode->ExtraData.GetSetData.VarStorage->EditBuffer + OpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset, OpCode->ExtraData.GetSetData.ValueWidth);
              break;
            case EFI_HII_VARSTORE_NAME_VALUE:
              if (OpCode->ExtraData.GetSetData.ValueType != EFI_IFR_TYPE_STRING) {
                //
                // Get value from string except for STRING value.
                //
                Status = GetValueByName (OpCode->ExtraData.GetSetData.VarStorage, OpCode->ExtraData.GetSetData.ValueName, &StrPtr);
                if (!EFI_ERROR (Status)) {
                  ASSERT (StrPtr != NULL);
                  TempLength = StrLen (StrPtr);
                  if (OpCode->ExtraData.GetSetData.ValueWidth >= ((TempLength + 1) / 2)) {
                    Value->Type = OpCode->ExtraData.GetSetData.ValueType;
                    TempBuffer  = (UINT8 *)&Value->Value;
                    ZeroMem (TempStr, sizeof (TempStr));
                    for (Index = 0; Index < TempLength; Index++) {
                      TempStr[0] = StrPtr[TempLength - Index - 1];
                      DigitUint8 = (UINT8)StrHexToUint64 (TempStr);
                      if ((Index & 1) == 0) {
                        TempBuffer[Index/2] = DigitUint8;
                      } else {
                        TempBuffer[Index/2] = (UINT8)((DigitUint8 << 4) + TempBuffer[Index/2]);
                      }
                    }
                  }
                }
              }

              break;
            case EFI_HII_VARSTORE_EFI_VARIABLE:
              //
              // Get value from variable.
              //
              TempLength  = OpCode->ExtraData.GetSetData.ValueWidth;
              Value->Type = OpCode->ExtraData.GetSetData.ValueType;
              Status      = gRT->GetVariable (
                                   OpCode->ExtraData.GetSetData.ValueName,
                                   &OpCode->ExtraData.GetSetData.VarStorage->Guid,
                                   NULL,
                                   &TempLength,
                                   &Value->Value
                                   );
              if (EFI_ERROR (Status)) {
                Value->Type     = EFI_IFR_TYPE_UNDEFINED;
                Value->Value.u8 = 0;
              }

              break;
            default:
              //
              // Not recognize storage.
              //
              Status = EFI_UNSUPPORTED;
              goto Done;
          }
        } else {
          //
          // For Time/Date Data
          //
          if ((OpCode->ExtraData.GetSetData.ValueType != EFI_IFR_TYPE_DATE) && (OpCode->ExtraData.GetSetData.ValueType != EFI_IFR_TYPE_TIME)) {
            //
            // Only support Data/Time data when storage doesn't exist.
            //
            Status = EFI_UNSUPPORTED;
            goto Done;
          }

          Status = gRT->GetTime (&EfiTime, NULL);
          if (!EFI_ERROR (Status)) {
            if (OpCode->ExtraData.GetSetData.ValueType == EFI_IFR_TYPE_DATE) {
              switch (OpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset) {
                case 0x00:
                  Value->Type      = EFI_IFR_TYPE_NUM_SIZE_16;
                  Value->Value.u16 = EfiTime.Year;
                  break;
                case 0x02:
                  Value->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
                  Value->Value.u8 = EfiTime.Month;
                  break;
                case 0x03:
                  Value->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
                  Value->Value.u8 = EfiTime.Day;
                  break;
                default:
                  //
                  // Invalid Date field.
                  //
                  Status = EFI_INVALID_PARAMETER;
                  goto Done;
              }
            } else {
              switch (OpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset) {
                case 0x00:
                  Value->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
                  Value->Value.u8 = EfiTime.Hour;
                  break;
                case 0x01:
                  Value->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
                  Value->Value.u8 = EfiTime.Minute;
                  break;
                case 0x02:
                  Value->Type     = EFI_IFR_TYPE_NUM_SIZE_8;
                  Value->Value.u8 = EfiTime.Second;
                  break;
                default:
                  //
                  // Invalid Time field.
                  //
                  Status = EFI_INVALID_PARAMETER;
                  goto Done;
              }
            }
          }
        }

        break;

      case EFI_IFR_QUESTION_REF3_OP:
        //
        // EFI_IFR_QUESTION_REF3
        // Pop an expression from the expression stack
        //
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Validate the expression value
        //
        if ((Value->Type > EFI_IFR_TYPE_NUM_SIZE_64) || (Value->Value.u64 > 0xffff)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (OpCode->ExtraData.QuestionRef3Data.DevicePath != 0) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;

          Status = gBS->LocateProtocol (
                          &gEfiDevicePathFromTextProtocolGuid,
                          NULL,
                          (VOID **)&PathFromText
                          );
          if (EFI_ERROR (Status)) {
            goto Done;
          }

          StrPtr = GetTokenString (OpCode->ExtraData.QuestionRef3Data.DevicePath, FormSet->HiiHandle);
          if ((StrPtr != NULL) && (PathFromText != NULL)) {
            DevicePath = PathFromText->ConvertTextToDevicePath (StrPtr);
            if ((DevicePath != NULL) && GetQuestionValueFromForm (DevicePath, NULL, &OpCode->ExtraData.Guid, Value->Value.u16, &QuestionVal)) {
              Value = &QuestionVal;
            }

            if (DevicePath != NULL) {
              FreePool (DevicePath);
            }
          }

          if (StrPtr != NULL) {
            FreePool (StrPtr);
          }
        } else if (IsZeroGuid (&OpCode->ExtraData.Guid)) {
          if (!GetQuestionValueFromForm (NULL, FormSet->HiiHandle, &OpCode->ExtraData.Guid, Value->Value.u16, &QuestionVal)) {
            Value->Type = EFI_IFR_TYPE_UNDEFINED;
            break;
          }

          Value = &QuestionVal;
        } else {
          Question = QuestionIdInFormset (FormSet, Form, Value->Value.u16);
          if (Question == NULL) {
            Value->Type = EFI_IFR_TYPE_UNDEFINED;
            break;
          }

          //
          // Load value from storage.
          //
          Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
          if (EFI_ERROR (Status)) {
            Value->Type = EFI_IFR_TYPE_UNDEFINED;
            break;
          }

          //
          // push the questions' value on to the expression stack
          //
          Value = (EFI_HII_VALUE *)&Question->Value;
        }

        break;

      case EFI_IFR_RULE_REF_OP:
        //
        // Find expression for this rule
        //
        RuleExpression = RuleIdToExpression (Form, OpCode->ExtraData.RuleId);
        if (RuleExpression == NULL) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        //
        // Evaluate this rule expression
        //
        Status = EvaluateHiiExpression (FormSet, Form, RuleExpression);
        if (EFI_ERROR (Status) || (RuleExpression->Result.Type == EFI_IFR_TYPE_UNDEFINED)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value = &RuleExpression->Result;
        break;

      case EFI_IFR_STRING_REF1_OP:
        Value->Type         = EFI_IFR_TYPE_STRING;
        Value->Value.string = OpCode->ExtraData.Value.Value.string;
        break;

      //
      // Constant
      //
      case EFI_IFR_TRUE_OP:
      case EFI_IFR_FALSE_OP:
      case EFI_IFR_ONE_OP:
      case EFI_IFR_ONES_OP:
      case EFI_IFR_UINT8_OP:
      case EFI_IFR_UINT16_OP:
      case EFI_IFR_UINT32_OP:
      case EFI_IFR_UINT64_OP:
      case EFI_IFR_UNDEFINED_OP:
      case EFI_IFR_VERSION_OP:
      case EFI_IFR_ZERO_OP:
        Value = &OpCode->ExtraData.Value;
        break;

      //
      // unary-op
      //
      case EFI_IFR_LENGTH_OP:
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if ((Value->Type != EFI_IFR_TYPE_STRING) && !IsTypeInBuffer (Value)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (Value->Type == EFI_IFR_TYPE_STRING) {
          StrPtr = GetTokenString (Value->Value.string, FormSet->HiiHandle);
          if (StrPtr == NULL) {
            Status = EFI_INVALID_PARAMETER;
            goto Done;
          }

          Value->Type      = EFI_IFR_TYPE_NUM_SIZE_64;
          Value->Value.u64 = StrLen (StrPtr);
          FreePool (StrPtr);
        } else {
          Value->Type      = EFI_IFR_TYPE_NUM_SIZE_64;
          Value->Value.u64 = GetLengthForValue (Value);
          FreePool (Value->Buffer);
        }

        break;

      case EFI_IFR_NOT_OP:
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if (Value->Type != EFI_IFR_TYPE_BOOLEAN) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value->Value.b = (BOOLEAN)(!Value->Value.b);
        break;

      case EFI_IFR_QUESTION_REF2_OP:
        //
        // Pop an expression from the expression stack
        //
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Validate the expression value
        //
        if ((Value->Type > EFI_IFR_TYPE_NUM_SIZE_64) || (Value->Value.u64 > 0xffff)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Question = QuestionIdInFormset (FormSet, Form, Value->Value.u16);
        if (Question == NULL) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        //
        // Load value from storage.
        //
        Status = GetQuestionValue (FormSet, Form, Question, GetSetValueWithBuffer);
        if (EFI_ERROR (Status)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value = (EFI_HII_VALUE *)&Question->Value;
        break;

      case EFI_IFR_STRING_REF2_OP:
        //
        // Pop an expression from the expression stack
        //
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Validate the expression value
        //
        if ((Value->Type > EFI_IFR_TYPE_NUM_SIZE_64) || (Value->Value.u64 > 0xffff)) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value->Type = EFI_IFR_TYPE_STRING;
        StrPtr      = GetTokenString (Value->Value.u16, FormSet->HiiHandle);
        if (StrPtr == NULL) {
          //
          // If String not exit, push an empty string
          //
          Value->Value.string = NewHiiString (gEmptyString, FormSet->HiiHandle);
        } else {
          Index               = (UINT16)Value->Value.u64;
          Value->Value.string = Index;
          FreePool (StrPtr);
        }

        break;

      case EFI_IFR_TO_BOOLEAN_OP:
        //
        // Pop an expression from the expression stack
        //
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Convert an expression to a Boolean
        //
        if (Value->Type <= EFI_IFR_TYPE_DATE) {
          //
          // When converting from an unsigned integer, zero will be converted to
          // FALSE and any other value will be converted to TRUE.
          //
          Value->Value.b = (BOOLEAN)(HiiValueToUINT64 (Value) != 0);

          Value->Type = EFI_IFR_TYPE_BOOLEAN;
        } else if (Value->Type == EFI_IFR_TYPE_STRING) {
          //
          // When converting from a string, if case-insensitive compare
          // with "true" is True, then push True. If a case-insensitive compare
          // with "false" is True, then push False. Otherwise, push Undefined.
          //
          StrPtr = GetTokenString (Value->Value.string, FormSet->HiiHandle);
          if (StrPtr == NULL) {
            Status = EFI_INVALID_PARAMETER;
            goto Done;
          }

          IfrStrToUpper (StrPtr);
          if (StrCmp (StrPtr, L"TRUE") == 0) {
            Value->Value.b = TRUE;
            Value->Type    = EFI_IFR_TYPE_BOOLEAN;
          } else if (StrCmp (StrPtr, L"FALSE") == 0) {
            Value->Value.b = FALSE;
            Value->Type    = EFI_IFR_TYPE_BOOLEAN;
          } else {
            Value->Type = EFI_IFR_TYPE_UNDEFINED;
          }

          FreePool (StrPtr);
        } else if (Value->Type == EFI_IFR_TYPE_BUFFER) {
          //
          // When converting from a buffer, if the buffer is all zeroes,
          // then push False. Otherwise push True.
          //
          for (Index = 0; Index < Value->BufferLen; Index++) {
            if (Value->Buffer[Index] != 0) {
              break;
            }
          }

          if (Index >= Value->BufferLen) {
            Value->Value.b = FALSE;
          } else {
            Value->Value.b = TRUE;
          }

          Value->Type = EFI_IFR_TYPE_BOOLEAN;
          FreePool (Value->Buffer);
        }

        break;

      case EFI_IFR_TO_STRING_OP:
        Status = IfrToString (FormSet, OpCode->ExtraData.Format, Value);
        break;

      case EFI_IFR_TO_UINT_OP:
        Status = IfrToUint (FormSet, Value);
        break;

      case EFI_IFR_TO_LOWER_OP:
      case EFI_IFR_TO_UPPER_OP:
        Status = InitializeUnicodeCollationProtocol ();
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if (Value->Type != EFI_IFR_TYPE_STRING) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        StrPtr = GetTokenString (Value->Value.string, FormSet->HiiHandle);
        if (StrPtr == NULL) {
          Status = EFI_NOT_FOUND;
          goto Done;
        }

        if (OpCode->Operand == EFI_IFR_TO_LOWER_OP) {
          mUnicodeCollation->StrLwr (mUnicodeCollation, StrPtr);
        } else {
          mUnicodeCollation->StrUpr (mUnicodeCollation, StrPtr);
        }

        Value->Value.string = NewHiiString (StrPtr, FormSet->HiiHandle);
        FreePool (StrPtr);
        break;

      case EFI_IFR_BITWISE_NOT_OP:
        //
        // Pop an expression from the expression stack
        //
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if (Value->Type > EFI_IFR_TYPE_DATE) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value->Type      = EFI_IFR_TYPE_NUM_SIZE_64;
        Value->Value.u64 = ~HiiValueToUINT64(Value);
        break;

      case EFI_IFR_SET_OP:
        //
        // Pop an expression from the expression stack
        //
        Status = PopExpression (Value);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Data1.Type    = EFI_IFR_TYPE_BOOLEAN;
        Data1.Value.b = FALSE;
        //
        // Set value to var storage buffer
        //
        if (OpCode->ExtraData.GetSetData.VarStorage != NULL) {
          switch (OpCode->ExtraData.GetSetData.VarStorage->Type) {
            case EFI_HII_VARSTORE_BUFFER:
            case EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER:
              CopyMem (OpCode->ExtraData.GetSetData.VarStorage->EditBuffer + OpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset, &Value->Value, OpCode->ExtraData.GetSetData.ValueWidth);
              Data1.Value.b = TRUE;
              break;
            case EFI_HII_VARSTORE_NAME_VALUE:
              if (OpCode->ExtraData.GetSetData.ValueType != EFI_IFR_TYPE_STRING) {
                NameValue = AllocatePool ((OpCode->ExtraData.GetSetData.ValueWidth * 2 + 1) * sizeof (CHAR16));
                ASSERT (NameValue != NULL);
                //
                // Convert Buffer to Hex String
                //
                TempBuffer = (UINT8 *)&Value->Value + OpCode->ExtraData.GetSetData.ValueWidth - 1;
                StrPtr     = NameValue;
                for (Index = 0; Index < OpCode->ExtraData.GetSetData.ValueWidth; Index++, TempBuffer--) {
                  UnicodeValueToStringS (
                    StrPtr,
                    (OpCode->ExtraData.GetSetData.ValueWidth * 2 + 1) * sizeof (CHAR16) - ((UINTN)StrPtr - (UINTN)NameValue),
                    PREFIX_ZERO | RADIX_HEX,
                    *TempBuffer,
                    2
                    );
                  StrPtr += StrnLenS (StrPtr, OpCode->ExtraData.GetSetData.ValueWidth * 2 + 1 - ((UINTN)StrPtr - (UINTN)NameValue) / sizeof (CHAR16));
                }

                Status = SetValueByName (OpCode->ExtraData.GetSetData.VarStorage, OpCode->ExtraData.GetSetData.ValueName, NameValue, NULL);
                FreePool (NameValue);
                if (!EFI_ERROR (Status)) {
                  Data1.Value.b = TRUE;
                }
              }

              break;
            case EFI_HII_VARSTORE_EFI_VARIABLE:
              Status = gRT->SetVariable (
                              OpCode->ExtraData.GetSetData.ValueName,
                              &OpCode->ExtraData.GetSetData.VarStorage->Guid,
                              OpCode->ExtraData.GetSetData.VarStorage->Attributes,
                              OpCode->ExtraData.GetSetData.ValueWidth,
                              &Value->Value
                              );
              if (!EFI_ERROR (Status)) {
                Data1.Value.b = TRUE;
              }

              break;
            default:
              //
              // Not recognize storage.
              //
              Status = EFI_UNSUPPORTED;
              goto Done;
          }
        } else {
          //
          // For Time/Date Data
          //
          if ((OpCode->ExtraData.GetSetData.ValueType != EFI_IFR_TYPE_DATE) && (OpCode->ExtraData.GetSetData.ValueType != EFI_IFR_TYPE_TIME)) {
            //
            // Only support Data/Time data when storage doesn't exist.
            //
            Status = EFI_UNSUPPORTED;
            goto Done;
          }

          Status = gRT->GetTime (&EfiTime, NULL);
          if (!EFI_ERROR (Status)) {
            if (OpCode->ExtraData.GetSetData.ValueType == EFI_IFR_TYPE_DATE) {
              switch (OpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset) {
                case 0x00:
                  EfiTime.Year = Value->Value.u16;
                  break;
                case 0x02:
                  EfiTime.Month = Value->Value.u8;
                  break;
                case 0x03:
                  EfiTime.Day = Value->Value.u8;
                  break;
                default:
                  //
                  // Invalid Date field.
                  //
                  Status = EFI_INVALID_PARAMETER;
                  goto Done;
              }
            } else {
              switch (OpCode->ExtraData.GetSetData.VarStoreInfo.VarOffset) {
                case 0x00:
                  EfiTime.Hour = Value->Value.u8;
                  break;
                case 0x01:
                  EfiTime.Minute = Value->Value.u8;
                  break;
                case 0x02:
                  EfiTime.Second = Value->Value.u8;
                  break;
                default:
                  //
                  // Invalid Time field.
                  //
                  Status = EFI_INVALID_PARAMETER;
                  goto Done;
              }
            }

            Status = gRT->SetTime (&EfiTime);
            if (!EFI_ERROR (Status)) {
              Data1.Value.b = TRUE;
            }
          }
        }

        Value = &Data1;
        break;

      //
      // binary-op
      //
      case EFI_IFR_ADD_OP:
      case EFI_IFR_SUBTRACT_OP:
      case EFI_IFR_MULTIPLY_OP:
      case EFI_IFR_DIVIDE_OP:
      case EFI_IFR_MODULO_OP:
      case EFI_IFR_BITWISE_AND_OP:
      case EFI_IFR_BITWISE_OR_OP:
      case EFI_IFR_SHIFT_LEFT_OP:
      case EFI_IFR_SHIFT_RIGHT_OP:
        //
        // Pop an expression from the expression stack
        //
        Status = PopExpression (&Data2);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Pop another expression from the expression stack
        //
        Status = PopExpression (&Data1);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if (Data2.Type > EFI_IFR_TYPE_DATE) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (Data1.Type > EFI_IFR_TYPE_DATE) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Value->Type = EFI_IFR_TYPE_NUM_SIZE_64;

        switch (OpCode->Operand) {
          case EFI_IFR_ADD_OP:
            Value->Value.u64 = HiiValueToUINT64 (&Data1) + HiiValueToUINT64 (&Data2);
            break;

          case EFI_IFR_SUBTRACT_OP:
            Value->Value.u64 = HiiValueToUINT64 (&Data1) - HiiValueToUINT64 (&Data2);
            break;

          case EFI_IFR_MULTIPLY_OP:
            Value->Value.u64 = MultU64x32 (HiiValueToUINT64 (&Data1), (UINT32)HiiValueToUINT64 (&Data2));
            break;

          case EFI_IFR_DIVIDE_OP:
            Value->Value.u64 = DivU64x32 (HiiValueToUINT64 (&Data1), (UINT32)HiiValueToUINT64 (&Data2));
            break;

          case EFI_IFR_MODULO_OP:
            DivU64x32Remainder (HiiValueToUINT64 (&Data1), (UINT32)HiiValueToUINT64 (&Data2), &TempValue);
            Value->Value.u64 = TempValue;
            break;

          case EFI_IFR_BITWISE_AND_OP:
            Value->Value.u64 = HiiValueToUINT64 (&Data1) & HiiValueToUINT64 (&Data2);
            break;

          case EFI_IFR_BITWISE_OR_OP:
            Value->Value.u64 = HiiValueToUINT64 (&Data1) | HiiValueToUINT64 (&Data2);
            break;

          case EFI_IFR_SHIFT_LEFT_OP:
            Value->Value.u64 = LShiftU64 (HiiValueToUINT64 (&Data1), (UINTN)HiiValueToUINT64 (&Data2));
            break;

          case EFI_IFR_SHIFT_RIGHT_OP:
            Value->Value.u64 = RShiftU64 (HiiValueToUINT64 (&Data1), (UINTN)HiiValueToUINT64 (&Data2));
            break;

          default:
            break;
        }

        break;

      case EFI_IFR_AND_OP:
      case EFI_IFR_OR_OP:
        //
        // Two Boolean operator
        //
        Status = PopExpression (&Data2);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Pop another expression from the expression stack
        //
        Status = PopExpression (&Data1);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if (Data2.Type != EFI_IFR_TYPE_BOOLEAN) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (Data1.Type != EFI_IFR_TYPE_BOOLEAN) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (OpCode->Operand == EFI_IFR_AND_OP) {
          Value->Value.b = (BOOLEAN)(Data1.Value.b && Data2.Value.b);
        } else {
          Value->Value.b = (BOOLEAN)(Data1.Value.b || Data2.Value.b);
        }

        break;

      case EFI_IFR_EQUAL_OP:
      case EFI_IFR_NOT_EQUAL_OP:
      case EFI_IFR_GREATER_EQUAL_OP:
      case EFI_IFR_GREATER_THAN_OP:
      case EFI_IFR_LESS_EQUAL_OP:
      case EFI_IFR_LESS_THAN_OP:
        //
        // Compare two integer, string, boolean or date/time
        //
        Status = PopExpression (&Data2);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Pop another expression from the expression stack
        //
        Status = PopExpression (&Data1);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if ((Data2.Type > EFI_IFR_TYPE_BOOLEAN) &&
            (Data2.Type != EFI_IFR_TYPE_STRING) &&
            !IsTypeInBuffer (&Data2))
        {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if ((Data1.Type > EFI_IFR_TYPE_BOOLEAN) &&
            (Data1.Type != EFI_IFR_TYPE_STRING) &&
            !IsTypeInBuffer (&Data1))
        {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        Status = CompareHiiValue (&Data1, &Data2, &Result, FormSet->HiiHandle);
        if (Data1.Type == EFI_IFR_TYPE_BUFFER) {
          FreePool (Data1.Buffer);
        }

        if (Data2.Type == EFI_IFR_TYPE_BUFFER) {
          FreePool (Data2.Buffer);
        }

        if (Status == EFI_UNSUPPORTED) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          Status      = EFI_SUCCESS;
          break;
        }

        if (EFI_ERROR (Status)) {
          goto Done;
        }

        switch (OpCode->Operand) {
          case EFI_IFR_EQUAL_OP:
            Value->Value.b = (BOOLEAN)((Result == 0) ? TRUE : FALSE);
            break;

          case EFI_IFR_NOT_EQUAL_OP:
            Value->Value.b = (BOOLEAN)((Result != 0) ? TRUE : FALSE);
            break;

          case EFI_IFR_GREATER_EQUAL_OP:
            Value->Value.b = (BOOLEAN)((Result >= 0) ? TRUE : FALSE);
            break;

          case EFI_IFR_GREATER_THAN_OP:
            Value->Value.b = (BOOLEAN)((Result > 0) ? TRUE : FALSE);
            break;

          case EFI_IFR_LESS_EQUAL_OP:
            Value->Value.b = (BOOLEAN)((Result <= 0) ? TRUE : FALSE);
            break;

          case EFI_IFR_LESS_THAN_OP:
            Value->Value.b = (BOOLEAN)((Result < 0) ? TRUE : FALSE);
            break;

          default:
            break;
        }

        break;

      case EFI_IFR_MATCH_OP:
        Status = InitializeUnicodeCollationProtocol ();
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        Status = IfrMatch (FormSet, Value);
        break;

      case EFI_IFR_MATCH2_OP:
        Status = IfrMatch2 (FormSet, &OpCode->ExtraData.Guid, Value);
        break;

      case EFI_IFR_CATENATE_OP:
        Status = IfrCatenate (FormSet, Value);
        break;

      //
      // ternary-op
      //
      case EFI_IFR_CONDITIONAL_OP:
        //
        // Pop third expression from the expression stack
        //
        Status = PopExpression (&Data3);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Pop second expression from the expression stack
        //
        Status = PopExpression (&Data2);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Pop first expression from the expression stack
        //
        Status = PopExpression (&Data1);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        if (Data1.Type != EFI_IFR_TYPE_BOOLEAN) {
          Value->Type = EFI_IFR_TYPE_UNDEFINED;
          break;
        }

        if (Data1.Value.b) {
          Value = &Data3;
        } else {
          Value = &Data2;
        }

        break;

      case EFI_IFR_FIND_OP:
        Status = IfrFind (FormSet, OpCode->ExtraData.Format, Value);
        break;

      case EFI_IFR_MID_OP:
        Status = IfrMid (FormSet, Value);
        break;

      case EFI_IFR_TOKEN_OP:
        Status = IfrToken (FormSet, Value);
        break;

      case EFI_IFR_SPAN_OP:
        Status = IfrSpan (FormSet, OpCode->ExtraData.Flags, Value);
        break;

      case EFI_IFR_MAP_OP:
        //
        // Pop the check value
        //
        Status = PopExpression (&Data1);
        if (EFI_ERROR (Status)) {
          goto Done;
        }

        //
        // Check MapExpression list is valid.
        //
        if (OpCode->MapExpressionList.ForwardLink == NULL) {
          Status = EFI_INVALID_PARAMETER;
          goto Done;
        }

        //
        // Go through map expression list.
        //
        SubExpressionLink = GetFirstNode (&OpCode->MapExpressionList);
        while (!IsNull (&OpCode->MapExpressionList, SubExpressionLink)) {
          SubExpression = HII_EXPRESSION_FROM_LINK (SubExpressionLink);
          //
          // Evaluate the first expression in this pair.
          //
          Status = EvaluateHiiExpression (FormSet, Form, SubExpression);
          if (EFI_ERROR (Status)) {
            goto Done;
          }

          //
          // Compare the expression value with current value
          //
          if ((CompareHiiValue (&Data1, &SubExpression->Result, &Result, NULL) == EFI_SUCCESS) && (Result == 0)) {
            //
            // Try get the map value.
            //
            SubExpressionLink = GetNextNode (&OpCode->MapExpressionList, SubExpressionLink);
            if (IsNull (&OpCode->MapExpressionList, SubExpressionLink)) {
              Status = EFI_INVALID_PARAMETER;
              goto Done;
            }

            SubExpression = HII_EXPRESSION_FROM_LINK (SubExpressionLink);
            Status        = EvaluateHiiExpression (FormSet, Form, SubExpression);
            if (EFI_ERROR (Status)) {
              goto Done;
            }

            Value = &SubExpression->Result;
            break;
          }

          //
          // Skip the second expression on this pair.
          //
          SubExpressionLink = GetNextNode (&OpCode->MapExpressionList, SubExpressionLink);
          if (IsNull (&OpCode->MapExpressionList, SubExpressionLink)) {
            Status = EFI_INVALID_PARAMETER;
            goto Done;
          }

          //
          // Goto the first expression on next pair.
          //
          SubExpressionLink = GetNextNode (&OpCode->MapExpressionList, SubExpressionLink);
        }

        //
        // No map value is found.
        //
        if (IsNull (&OpCode->MapExpressionList, SubExpressionLink)) {
          Value->Type     = EFI_IFR_TYPE_UNDEFINED;
          Value->Value.u8 = 0;
        }

        break;

      default:
        break;
    }

    if (EFI_ERROR (Status) || (Value->Type == EFI_IFR_TYPE_UNDEFINED)) {
      goto Done;
    }

    Status = PushExpression (Value);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
  }

  //
  // Pop the final result from expression stack
  //
  Value  = &Data1;
  Status = PopExpression (Value);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // After evaluating an expression, there should be only one value left on the expression stack
  //
  if (PopExpression (Value) != EFI_ACCESS_DENIED) {
    Status = EFI_INVALID_PARAMETER;
  }

Done:
  RestoreExpressionEvaluationStackOffset (StackOffset);
  if (!EFI_ERROR (Status)) {
    CopyMem (&Expression->Result, Value, sizeof (EFI_HII_VALUE));
  }

  return Status;
}

/**
  Set value of a data element in an Array by its Index.

  @param[in]  Array       The data array.
  @param[in]  Type        Type of the data in this array.
  @param[in]  Index       Zero based index for data in this array.
  @param[in]  Value       The value to be set.

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
  Search an Option of a Question by its value.

  @param[in]  Question           The Question
  @param[in]  OptionValue        Value for Option to be searched.

  @retval Pointer                Pointer to the found Option.
  @retval NULL                   Option not found.

**/
HII_QUESTION_OPTION *
ValueToOption (
  IN HII_STATEMENT        *Question,
  IN HII_STATEMENT_VALUE  *OptionValue
  )
{
  LIST_ENTRY           *Link;
  HII_QUESTION_OPTION  *Option;
  EFI_HII_VALUE        Data1;
  EFI_HII_VALUE        Data2;
  INTN                 Result;
  EFI_STATUS           Status;

  Result = 0;
  ZeroMem (&Data1, sizeof (EFI_HII_VALUE));
  ZeroMem (&Data2, sizeof (EFI_HII_VALUE));

  Status = HiiStatementValueToHiiValue (OptionValue, &Data1);
  ASSERT_EFI_ERROR (Status);

  Link = GetFirstNode (&Question->OptionListHead);
  while (!IsNull (&Question->OptionListHead, Link)) {
    Option = HII_QUESTION_OPTION_FROM_LINK (Link);

    Status = HiiStatementValueToHiiValue (&Option->Value, &Data2);
    ASSERT_EFI_ERROR (Status);

    if ((CompareHiiValue (&Data1, &Data2, &Result, NULL) == EFI_SUCCESS) && (Result == 0)) {
      //
      // Check the suppressif condition, only a valid option can be return.
      //
      if ((Option->SuppressExpression == NULL) ||
          ((EvaluateExpressionList (Option->SuppressExpression, FALSE, NULL, NULL) == ExpressFalse)))
      {
        return Option;
      }
    }

    Link = GetNextNode (&Question->OptionListHead, Link);
  }

  return NULL;
}

/**
  Find the point in the ConfigResp string for this question.

  @param[in]  Question         The question.
  @param[in]  ConfigResp       Get ConfigResp string.

  @retval  point to the offset where is for this question.

**/
CHAR16 *
GetOffsetFromConfigResp (
  IN HII_STATEMENT  *Question,
  IN CHAR16         *ConfigResp
  )
{
  CHAR16  *RequestElement;
  CHAR16  *BlockData;

  //
  // Type is EFI_HII_VARSTORE_NAME_VALUE.
  //
  if (Question->Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) {
    RequestElement = StrStr (ConfigResp, Question->VariableName);
    if (RequestElement != NULL) {
      //
      // Skip the "VariableName=" field.
      //
      RequestElement += StrLen (Question->VariableName) + 1;
    }

    return RequestElement;
  }

  //
  // Type is EFI_HII_VARSTORE_EFI_VARIABLE or EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER
  //

  //
  // Convert all hex digits in ConfigResp to lower case before searching.
  //
  HiiStringToLowercase (ConfigResp);

  //
  // 1. Directly use Question->BlockName to find.
  //
  RequestElement = StrStr (ConfigResp, Question->BlockName);
  if (RequestElement != NULL) {
    //
    // Skip the "Question->BlockName&VALUE=" field.
    //
    RequestElement += StrLen (Question->BlockName) + StrLen (L"&VALUE=");
    return RequestElement;
  }

  //
  // 2. Change all hex digits in Question->BlockName to lower and compare again.
  //
  BlockData = AllocateCopyPool (StrSize (Question->BlockName), Question->BlockName);
  ASSERT (BlockData != NULL);
  HiiStringToLowercase (BlockData);
  RequestElement = StrStr (ConfigResp, BlockData);
  FreePool (BlockData);

  if (RequestElement != NULL) {
    //
    // Skip the "Question->BlockName&VALUE=" field.
    //
    RequestElement += StrLen (Question->BlockName) + StrLen (L"&VALUE=");
  }

  return RequestElement;
}

/**
  Get Question default value from AltCfg string.

  @param[in]  FormSet            The form set.
  @param[in]  Form               The form
  @param[in]  Question           The question.
  @param[out] DefaultValue       Default value.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetDefaultValueFromAltCfg (
  IN     HII_FORMSET          *FormSet,
  IN     HII_FORM             *Form,
  IN     HII_STATEMENT        *Question,
  OUT    HII_STATEMENT_VALUE  *DefaultValue
  )
{
  HII_FORMSET_STORAGE      *Storage;
  CHAR16                   *ConfigResp;
  CHAR16                   *Value;
  LIST_ENTRY               *Link;
  HII_FORM_CONFIG_REQUEST  *ConfigInfo;

  Storage = Question->Storage;
  if ((Storage == NULL) || (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE)) {
    return EFI_NOT_FOUND;
  }

  //
  // Try to get AltCfg string from form. If not found it, then
  // try to get it from formset.
  //
  ConfigResp = NULL;
  Link       = GetFirstNode (&Form->ConfigRequestHead);
  while (!IsNull (&Form->ConfigRequestHead, Link)) {
    ConfigInfo = HII_FORM_CONFIG_REQUEST_FROM_LINK (Link);
    Link       = GetNextNode (&Form->ConfigRequestHead, Link);

    if (Storage == ConfigInfo->Storage) {
      ConfigResp = ConfigInfo->ConfigAltResp;
      break;
    }
  }

  if (ConfigResp == NULL) {
    return EFI_NOT_FOUND;
  }

  Value = GetOffsetFromConfigResp (Question, ConfigResp);
  if (Value == NULL) {
    return EFI_NOT_FOUND;
  }

  return BufferToQuestionValue (Question, Value, DefaultValue);
}

/**
  Get default Id value used for browser.

  @param[in]  DefaultId     The default id value used by hii.

  @retval Browser used default value.

**/
INTN
GetDefaultIdForCallBack (
  IN UINTN  DefaultId
  )
{
  if (DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) {
    return EFI_BROWSER_ACTION_DEFAULT_STANDARD;
  } else if (DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
    return EFI_BROWSER_ACTION_DEFAULT_MANUFACTURING;
  } else if (DefaultId == EFI_HII_DEFAULT_CLASS_SAFE) {
    return EFI_BROWSER_ACTION_DEFAULT_SAFE;
  } else if ((DefaultId >= EFI_HII_DEFAULT_CLASS_PLATFORM_BEGIN) && (DefaultId < EFI_HII_DEFAULT_CLASS_PLATFORM_BEGIN + 0x1000)) {
    return EFI_BROWSER_ACTION_DEFAULT_PLATFORM + DefaultId - EFI_HII_DEFAULT_CLASS_PLATFORM_BEGIN;
  } else if ((DefaultId >= EFI_HII_DEFAULT_CLASS_HARDWARE_BEGIN) && (DefaultId < EFI_HII_DEFAULT_CLASS_HARDWARE_BEGIN + 0x1000)) {
    return EFI_BROWSER_ACTION_DEFAULT_HARDWARE + DefaultId - EFI_HII_DEFAULT_CLASS_HARDWARE_BEGIN;
  } else if ((DefaultId >= EFI_HII_DEFAULT_CLASS_FIRMWARE_BEGIN) && (DefaultId < EFI_HII_DEFAULT_CLASS_FIRMWARE_BEGIN + 0x1000)) {
    return EFI_BROWSER_ACTION_DEFAULT_FIRMWARE + DefaultId - EFI_HII_DEFAULT_CLASS_FIRMWARE_BEGIN;
  } else {
    return -1;
  }
}

/**
  Get default value of question.

  @param[in]  FormSet            The form set.
  @param[in]  Form               The form.
  @param[in]  Question           The question.
  @param[in]  DefaultId          The Class of the default.
  @param[out] DefaultValue       The default value of given question.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetQuestionDefault (
  IN HII_FORMSET           *FormSet,
  IN HII_FORM              *Form,
  IN HII_STATEMENT         *Question,
  IN UINT16                DefaultId,
  OUT HII_STATEMENT_VALUE  *DefaultValue
  )
{
  EFI_STATUS                      Status;
  LIST_ENTRY                      *Link;
  HII_QUESTION_DEFAULT            *Default;
  HII_QUESTION_OPTION             *Option;
  HII_STATEMENT_VALUE             *HiiValue;
  UINT8                           Index;
  EFI_STRING                      StrValue;
  EFI_HII_CONFIG_ACCESS_PROTOCOL  *ConfigAccess;
  EFI_BROWSER_ACTION_REQUEST      ActionRequest;
  INTN                            Action;
  EFI_IFR_TYPE_VALUE              *TypeValue;
  UINT16                          OriginalDefaultId;
  HII_FORMSET_DEFAULTSTORE        *DefaultStore;
  LIST_ENTRY                      *DefaultLink;

  if ((FormSet == NULL) || (Form == NULL) || (Question == NULL) || (DefaultValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status            = EFI_NOT_FOUND;
  StrValue          = NULL;
  ConfigAccess      = NULL;
  OriginalDefaultId = DefaultId;
  DefaultLink       = GetFirstNode (&FormSet->DefaultStoreListHead);

  //
  // Statement don't have storage, skip them
  //
  if (Question->QuestionId == 0) {
    DEBUG ((DEBUG_ERROR, "%a: Question has no storage, skip it\n", __func__));
    return Status;
  }

  //
  // There are Five ways to specify default value for a Question:
  //  1, use call back function (highest priority)
  //  2, use ExtractConfig function
  //  3, use nested EFI_IFR_DEFAULT
  //  4, set flags of EFI_ONE_OF_OPTION (provide Standard and Manufacturing default)
  //  5, set flags of EFI_IFR_CHECKBOX (provide Standard and Manufacturing default) (lowest priority)
  //
  CopyMem (DefaultValue, &Question->Value, sizeof (HII_STATEMENT_VALUE));
ReGetDefault:
  HiiValue  = DefaultValue;
  TypeValue = &HiiValue->Value;
  if (HiiValue->Type == EFI_IFR_TYPE_BUFFER) {
    //
    // For orderedlist, need to pass the BufferValue to Callback function.
    //
    DefaultValue->BufferLen = Question->Value.BufferLen;
    DefaultValue->Buffer    = AllocateZeroPool (DefaultValue->BufferLen);
    ASSERT (DefaultValue->Buffer != NULL);
    if (DefaultValue->Buffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    TypeValue = (EFI_IFR_TYPE_VALUE *)DefaultValue->Buffer;
  }

  //
  // Get Question default value from call back function.
  // The string type of question cause HII driver to set string to its default value.
  // So, we don't do this otherwise it will actually set question to default value.
  // We only want to get default value of question.
  //
  if (HiiValue->Type != EFI_IFR_TYPE_STRING) {
    ConfigAccess = FormSet->ConfigAccess;
    Action       = GetDefaultIdForCallBack (DefaultId);
    if ((Action > 0) && ((Question->QuestionFlags & EFI_IFR_FLAG_CALLBACK) != 0) && (ConfigAccess != NULL)) {
      ActionRequest = EFI_BROWSER_ACTION_REQUEST_NONE;
      Status        = ConfigAccess->Callback (
                                      ConfigAccess,
                                      Action,
                                      Question->QuestionId,
                                      HiiValue->Type,
                                      TypeValue,
                                      &ActionRequest
                                      );
      if (!EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  //
  // Get default value from altcfg string.
  //
  if (ConfigAccess != NULL) {
    Status = GetDefaultValueFromAltCfg (FormSet, Form, Question, DefaultValue);
    if (!EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // EFI_IFR_DEFAULT has highest priority
  //
  if (!IsListEmpty (&Question->DefaultListHead)) {
    Link = GetFirstNode (&Question->DefaultListHead);
    while (!IsNull (&Question->DefaultListHead, Link)) {
      Default = HII_QUESTION_DEFAULT_FROM_LINK (Link);

      if (Default->DefaultId == DefaultId) {
        if (Default->ValueExpression != NULL) {
          //
          // Default is provided by an Expression, evaluate it
          //
          Status = EvaluateHiiExpression (FormSet, Form, Default->ValueExpression);
          if (EFI_ERROR (Status)) {
            return Status;
          }

          if (Default->ValueExpression->Result.Type == EFI_IFR_TYPE_BUFFER) {
            ASSERT (HiiValue->Type == EFI_IFR_TYPE_BUFFER && DefaultValue->Buffer != NULL);
            if (DefaultValue->BufferLen > Default->ValueExpression->Result.BufferLen) {
              CopyMem (DefaultValue->Buffer, Default->ValueExpression->Result.Buffer, Default->ValueExpression->Result.BufferLen);
              DefaultValue->BufferLen = Default->ValueExpression->Result.BufferLen;
            } else {
              CopyMem (DefaultValue->Buffer, Default->ValueExpression->Result.Buffer, DefaultValue->BufferLen);
            }

            FreePool (Default->ValueExpression->Result.Buffer);
          }

          HiiValue->Type = Default->ValueExpression->Result.Type;
          CopyMem (&HiiValue->Value, &Default->ValueExpression->Result.Value, sizeof (EFI_IFR_TYPE_VALUE));
        } else {
          //
          // Default value is embedded in EFI_IFR_DEFAULT
          //
          if (Default->Value.Type == EFI_IFR_TYPE_BUFFER) {
            ASSERT (HiiValue->Buffer != NULL);
            CopyMem (HiiValue->Buffer, Default->Value.Buffer, Default->Value.BufferLen);
          } else {
            CopyMem (HiiValue, &Default->Value, sizeof (EFI_HII_VALUE));
          }
        }

        if (HiiValue->Type == EFI_IFR_TYPE_STRING) {
          StrValue = HiiGetString (FormSet->HiiHandle, HiiValue->Value.string, NULL);
          if (StrValue == NULL) {
            return EFI_NOT_FOUND;
          }

          if (DefaultValue->BufferLen > StrSize (StrValue)) {
            ZeroMem (DefaultValue->Buffer, DefaultValue->BufferLen);
            CopyMem (DefaultValue->Buffer, StrValue, StrSize (StrValue));
          } else {
            CopyMem (DefaultValue->Buffer, StrValue, DefaultValue->BufferLen);
          }
        }

        return EFI_SUCCESS;
      }

      Link = GetNextNode (&Question->DefaultListHead, Link);
    }
  }

  //
  // EFI_ONE_OF_OPTION
  //
  if ((Question->Operand == EFI_IFR_ONE_OF_OP) && !IsListEmpty (&Question->OptionListHead)) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
      //
      // OneOfOption could only provide Standard and Manufacturing default
      //
      Link = GetFirstNode (&Question->OptionListHead);
      while (!IsNull (&Question->OptionListHead, Link)) {
        Option = HII_QUESTION_OPTION_FROM_LINK (Link);
        Link   = GetNextNode (&Question->OptionListHead, Link);

        if ((Option->SuppressExpression != NULL) &&
            (EvaluateExpressionList (Option->SuppressExpression, FALSE, NULL, NULL) != ExpressFalse))
        {
          continue;
        }

        if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT) != 0)) ||
            ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Option->Flags & EFI_IFR_OPTION_DEFAULT_MFG) != 0))
            )
        {
          CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));

          return EFI_SUCCESS;
        }
      }
    }
  }

  //
  // EFI_IFR_CHECKBOX - lowest priority
  //
  if (Question->Operand == EFI_IFR_CHECKBOX_OP) {
    if (DefaultId <= EFI_HII_DEFAULT_CLASS_MANUFACTURING) {
      //
      // Checkbox could only provide Standard and Manufacturing default
      //
      if (((DefaultId == EFI_HII_DEFAULT_CLASS_STANDARD) && ((Question->ExtraData.Flags & EFI_IFR_CHECKBOX_DEFAULT) != 0)) ||
          ((DefaultId == EFI_HII_DEFAULT_CLASS_MANUFACTURING) && ((Question->ExtraData.Flags & EFI_IFR_CHECKBOX_DEFAULT_MFG) != 0))
          )
      {
        HiiValue->Value.b = TRUE;
      }

      return EFI_SUCCESS;
    }
  }

  //
  // For question without default value for current default Id, we try to re-get the default value form other default id in the DefaultStoreList.
  // If get, will exit the function, if not, will choose next default id in the DefaultStoreList.
  // The default id in DefaultStoreList are in ascending order to make sure choose the smallest default id every time.
  //
  while (!IsNull (&FormSet->DefaultStoreListHead, DefaultLink)) {
    DefaultStore = HII_FORMSET_DEFAULTSTORE_FROM_LINK (DefaultLink);
    DefaultLink  = GetNextNode (&FormSet->DefaultStoreListHead, DefaultLink);
    DefaultId    = DefaultStore->DefaultId;
    if (DefaultId == OriginalDefaultId) {
      continue;
    }

    goto ReGetDefault;
  }

  //
  // For Questions without default value for all the default id in the DefaultStoreList.
  //
  Status = EFI_NOT_FOUND;
  switch (Question->Operand) {
    case EFI_IFR_CHECKBOX_OP:
      HiiValue->Value.b = FALSE;
      Status            = EFI_SUCCESS;
      break;

    case EFI_IFR_NUMERIC_OP:
      //
      // Take minimum value as numeric default value
      //
      if ((Question->ExtraData.NumData.Flags & EFI_IFR_DISPLAY) == 0) {
        //
        // In EFI_IFR_DISPLAY_INT_DEC type, should check value with int* type.
        //
        switch (Question->ExtraData.NumData.Flags & EFI_IFR_NUMERIC_SIZE) {
          case EFI_IFR_NUMERIC_SIZE_1:
            if (((INT8)HiiValue->Value.u8 < (INT8)Question->ExtraData.NumData.Minimum) || ((INT8)HiiValue->Value.u8 > (INT8)Question->ExtraData.NumData.Maximum)) {
              HiiValue->Value.u8 = (UINT8)Question->ExtraData.NumData.Minimum;
              Status             = EFI_SUCCESS;
            }

            break;
          case EFI_IFR_NUMERIC_SIZE_2:
            if (((INT16)HiiValue->Value.u16 < (INT16)Question->ExtraData.NumData.Minimum) || ((INT16)HiiValue->Value.u16 > (INT16)Question->ExtraData.NumData.Maximum)) {
              HiiValue->Value.u16 = (UINT16)Question->ExtraData.NumData.Minimum;
              Status              = EFI_SUCCESS;
            }

            break;
          case EFI_IFR_NUMERIC_SIZE_4:
            if (((INT32)HiiValue->Value.u32 < (INT32)Question->ExtraData.NumData.Minimum) || ((INT32)HiiValue->Value.u32 > (INT32)Question->ExtraData.NumData.Maximum)) {
              HiiValue->Value.u32 = (UINT32)Question->ExtraData.NumData.Minimum;
              Status              = EFI_SUCCESS;
            }

            break;
          case EFI_IFR_NUMERIC_SIZE_8:
            if (((INT64)HiiValue->Value.u64 < (INT64)Question->ExtraData.NumData.Minimum) || ((INT64)HiiValue->Value.u64 > (INT64)Question->ExtraData.NumData.Maximum)) {
              HiiValue->Value.u64 = Question->ExtraData.NumData.Minimum;
              Status              = EFI_SUCCESS;
            }

            break;
          default:
            break;
        }
      } else {
        if ((HiiValue->Value.u64 < Question->ExtraData.NumData.Minimum) || (HiiValue->Value.u64 > Question->ExtraData.NumData.Maximum)) {
          HiiValue->Value.u64 = Question->ExtraData.NumData.Minimum;
          Status              = EFI_SUCCESS;
        }
      }

      break;

    case EFI_IFR_ONE_OF_OP:
      //
      // Take first oneof option as oneof's default value
      //
      Link = GetFirstNode (&Question->OptionListHead);
      while (!IsNull (&Question->OptionListHead, Link)) {
        Option = HII_QUESTION_OPTION_FROM_LINK (Link);
        Link   = GetNextNode (&Question->OptionListHead, Link);

        if ((Option->SuppressExpression != NULL) &&
            (EvaluateExpressionList (Option->SuppressExpression, FALSE, NULL, NULL) != ExpressFalse))
        {
          continue;
        }

        CopyMem (HiiValue, &Option->Value, sizeof (EFI_HII_VALUE));
        Status = EFI_SUCCESS;
        break;
      }

      break;

    case EFI_IFR_ORDERED_LIST_OP:
      //
      // Take option sequence in IFR as ordered list's default value
      //
      Index = 0;
      Link  = GetFirstNode (&Question->OptionListHead);
      while (!IsNull (&Question->OptionListHead, Link)) {
        Status = EFI_SUCCESS;
        Option = HII_QUESTION_OPTION_FROM_LINK (Link);
        Link   = GetNextNode (&Question->OptionListHead, Link);

        if ((Option->SuppressExpression != NULL) &&
            (EvaluateExpressionList (Option->SuppressExpression, FALSE, NULL, NULL) != ExpressFalse))
        {
          continue;
        }

        SetArrayData (DefaultValue->Buffer, Question->Value.Type, Index, Option->Value.Value.u64);

        Index++;
        if (Index >= Question->ExtraData.OrderListData.MaxContainers) {
          break;
        }
      }

      break;

    default:
      break;
  }

  return Status;
}
