/** @file
  Sample platform variable cleanup library implementation.

Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PlatVarCleanup.h"

VAR_ERROR_FLAG              mLastVarErrorFlag = VAR_ERROR_FLAG_NO_ERROR;
EDKII_VAR_CHECK_PROTOCOL    *mVarCheck = NULL;

///
/// The flag to indicate whether the platform has left the DXE phase of execution.
///
BOOLEAN                     mEndOfDxe = FALSE;

EFI_EVENT                   mPlatVarCleanupLibEndOfDxeEvent = NULL;

LIST_ENTRY                  mUserVariableList = INITIALIZE_LIST_HEAD_VARIABLE (mUserVariableList);
UINT16                      mUserVariableCount = 0;
UINT16                      mMarkedUserVariableCount = 0;

EFI_GUID                    mVariableCleanupHiiGuid = VARIABLE_CLEANUP_HII_GUID;
CHAR16                      mVarStoreName[] = L"VariableCleanup";

HII_VENDOR_DEVICE_PATH      mVarCleanupHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    VARIABLE_CLEANUP_HII_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (sizeof (EFI_DEVICE_PATH_PROTOCOL)),
      (UINT8) ((sizeof (EFI_DEVICE_PATH_PROTOCOL)) >> 8)
    }
  }
};

/**
  Internal get variable error flag.

  @return   Variable error flag.

**/
VAR_ERROR_FLAG
InternalGetVarErrorFlag (
  VOID
  )
{
  EFI_STATUS        Status;
  UINTN             Size;
  VAR_ERROR_FLAG    ErrorFlag;

  Size = sizeof (ErrorFlag);
  Status = gRT->GetVariable (
                  VAR_ERROR_FLAG_NAME,
                  &gEdkiiVarErrorFlagGuid,
                  NULL,
                  &Size,
                  &ErrorFlag
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_INFO, "%s - not found\n", VAR_ERROR_FLAG_NAME));
    return VAR_ERROR_FLAG_NO_ERROR;
  }
  return ErrorFlag;
}

/**
  Is user variable?

  @param[in] Name   Pointer to variable name.
  @param[in] Guid   Pointer to vendor guid.

  @retval TRUE      User variable.
  @retval FALSE     System variable.

**/
BOOLEAN
IsUserVariable (
  IN CHAR16                     *Name,
  IN EFI_GUID                   *Guid
  )
{
  EFI_STATUS                    Status;
  VAR_CHECK_VARIABLE_PROPERTY   Property;

  if (mVarCheck == NULL) {
    gBS->LocateProtocol (
           &gEdkiiVarCheckProtocolGuid,
           NULL,
           (VOID **) &mVarCheck
           );
  }
  ASSERT (mVarCheck != NULL);

  ZeroMem (&Property, sizeof (Property));
  Status = mVarCheck->VariablePropertyGet (
                        Name,
                        Guid,
                        &Property
                        );
  if (EFI_ERROR (Status)) {
    //
    // No property, it is user variable.
    //
    DEBUG ((EFI_D_INFO, "PlatformVarCleanup - User variable: %g:%s\n", Guid, Name));
    return TRUE;
  }

//  DEBUG ((EFI_D_INFO, "PlatformVarCleanup - Variable Property: %g:%s\n", Guid, Name));
//  DEBUG ((EFI_D_INFO, "  Revision  - 0x%04x\n", Property.Revision));
//  DEBUG ((EFI_D_INFO, "  Property  - 0x%04x\n", Property.Property));
//  DEBUG ((EFI_D_INFO, "  Attribute - 0x%08x\n", Property.Attributes));
//  DEBUG ((EFI_D_INFO, "  MinSize   - 0x%x\n", Property.MinSize));
//  DEBUG ((EFI_D_INFO, "  MaxSize   - 0x%x\n", Property.MaxSize));

  return FALSE;
}

/**
  Find user variable node by variable GUID.

  @param[in] Guid   Pointer to vendor guid.

  @return Pointer to user variable node.

**/
USER_VARIABLE_NODE *
FindUserVariableNodeByGuid (
  IN EFI_GUID   *Guid
  )
{
  USER_VARIABLE_NODE    *UserVariableNode;
  LIST_ENTRY            *Link;

  for (Link = mUserVariableList.ForwardLink
       ;Link != &mUserVariableList
       ;Link = Link->ForwardLink) {
    UserVariableNode = USER_VARIABLE_FROM_LINK (Link);

    if (CompareGuid (Guid, &UserVariableNode->Guid)) {
      //
      // Found it.
      //
      return UserVariableNode;
    }
  }

  //
  // Create new one if not found.
  //
  UserVariableNode = AllocateZeroPool (sizeof (*UserVariableNode));
  ASSERT (UserVariableNode != NULL);
  UserVariableNode->Signature = USER_VARIABLE_NODE_SIGNATURE;
  CopyGuid (&UserVariableNode->Guid, Guid);
  //
  // (36 chars of "########-####-####-####-############" + 1 space + 1 terminator) * sizeof (CHAR16).
  //
  UserVariableNode->PromptString = AllocatePool ((36 + 2) * sizeof (CHAR16));
  ASSERT (UserVariableNode->PromptString != NULL);
  UnicodeSPrint (UserVariableNode->PromptString, (36 + 2) * sizeof (CHAR16), L" %g", &UserVariableNode->Guid);
  InitializeListHead (&UserVariableNode->NameLink);
  InsertTailList (&mUserVariableList, &UserVariableNode->Link);
  return UserVariableNode;
}

/**
  Create user variable node.

**/
VOID
CreateUserVariableNode (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_STATUS                    GetVariableStatus;
  CHAR16                        *VarName;
  UINTN                         MaxVarNameSize;
  UINTN                         VarNameSize;
  UINTN                         MaxDataSize;
  UINTN                         DataSize;
  VOID                          *Data;
  UINT32                        Attributes;
  EFI_GUID                      Guid;
  USER_VARIABLE_NODE            *UserVariableNode;
  USER_VARIABLE_NAME_NODE       *UserVariableNameNode;
  UINT16                        Index;
  UINTN                         StringSize;

  //
  // Initialize 128 * sizeof (CHAR16) variable name size.
  //
  MaxVarNameSize = 128 * sizeof (CHAR16);
  VarName = AllocateZeroPool (MaxVarNameSize);
  ASSERT (VarName != NULL);

  //
  // Initialize 0x1000 variable data size.
  //
  MaxDataSize = 0x1000;
  Data = AllocateZeroPool (MaxDataSize);
  ASSERT (Data != NULL);

  Index = 0;
  do {
    VarNameSize = MaxVarNameSize;
    Status = gRT->GetNextVariableName (&VarNameSize, VarName, &Guid);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      VarName = ReallocatePool (MaxVarNameSize, VarNameSize, VarName);
      ASSERT (VarName != NULL);
      MaxVarNameSize = VarNameSize;
      Status = gRT->GetNextVariableName (&VarNameSize, VarName, &Guid);
    }

    if (!EFI_ERROR (Status)) {
      if (IsUserVariable (VarName, &Guid)) {
        DataSize = MaxDataSize;
        GetVariableStatus = gRT->GetVariable (VarName, &Guid, &Attributes, &DataSize, Data);
        if (GetVariableStatus == EFI_BUFFER_TOO_SMALL) {
          Data = ReallocatePool (MaxDataSize, DataSize, Data);
          ASSERT (Data != NULL);
          MaxDataSize = DataSize;
          GetVariableStatus = gRT->GetVariable (VarName, &Guid, &Attributes, &DataSize, Data);
        }
        ASSERT_EFI_ERROR (GetVariableStatus);

        if ((Attributes & EFI_VARIABLE_NON_VOLATILE) != 0) {
          UserVariableNode = FindUserVariableNodeByGuid (&Guid);
          ASSERT (UserVariableNode != NULL);

          //
          // Different variables that have same variable GUID share same user variable node.
          //
          UserVariableNameNode = AllocateZeroPool (sizeof (*UserVariableNameNode));
          ASSERT (UserVariableNameNode != NULL);
          UserVariableNameNode->Signature = USER_VARIABLE_NAME_NODE_SIGNATURE;
          UserVariableNameNode->Name = AllocateCopyPool (VarNameSize, VarName);
          UserVariableNameNode->Attributes = Attributes;
          UserVariableNameNode->DataSize = DataSize;
          UserVariableNameNode->Index = Index;
          UserVariableNameNode->QuestionId = (EFI_QUESTION_ID) (USER_VARIABLE_QUESTION_ID + Index);
          //
          // 2 space * sizeof (CHAR16) + StrSize.
          //
          StringSize = 2 * sizeof (CHAR16) + StrSize (UserVariableNameNode->Name);
          UserVariableNameNode->PromptString = AllocatePool (StringSize);
          ASSERT (UserVariableNameNode->PromptString != NULL);
          UnicodeSPrint (UserVariableNameNode->PromptString, StringSize, L"  %s", UserVariableNameNode->Name);
          //
          // (33 chars of "Attribtues = 0x and DataSize = 0x" + 1 terminator + (sizeof (UINT32) + sizeof (UINTN)) * 2) * sizeof (CHAR16).
          //
          StringSize = (33 + 1 + (sizeof (UINT32) + sizeof (UINTN)) * 2) * sizeof (CHAR16);
          UserVariableNameNode->HelpString = AllocatePool (StringSize);
          ASSERT (UserVariableNameNode->HelpString != NULL);
          UnicodeSPrint (UserVariableNameNode->HelpString, StringSize, L"Attribtues = 0x%08x and DataSize = 0x%x", UserVariableNameNode->Attributes, UserVariableNameNode->DataSize);
          UserVariableNameNode->Deleted = FALSE;
          InsertTailList (&UserVariableNode->NameLink, &UserVariableNameNode->Link);
          Index++;
        }
      }
    }
  } while (Status != EFI_NOT_FOUND);

  mUserVariableCount = Index;
  ASSERT (mUserVariableCount <= MAX_USER_VARIABLE_COUNT);
  DEBUG ((EFI_D_INFO, "PlatformVarCleanup - User variable count: 0x%04x\n", mUserVariableCount));

  FreePool (VarName);
  FreePool (Data);
}

/**
  Destroy user variable nodes.

**/
VOID
DestroyUserVariableNode (
  VOID
  )
{
  USER_VARIABLE_NODE        *UserVariableNode;
  LIST_ENTRY                *Link;
  USER_VARIABLE_NAME_NODE   *UserVariableNameNode;
  LIST_ENTRY                *NameLink;

  while (mUserVariableList.ForwardLink != &mUserVariableList) {
    Link = mUserVariableList.ForwardLink;
    UserVariableNode = USER_VARIABLE_FROM_LINK (Link);

    RemoveEntryList (&UserVariableNode->Link);

    while (UserVariableNode->NameLink.ForwardLink != &UserVariableNode->NameLink) {
      NameLink = UserVariableNode->NameLink.ForwardLink;
      UserVariableNameNode = USER_VARIABLE_NAME_FROM_LINK (NameLink);

      RemoveEntryList (&UserVariableNameNode->Link);

      FreePool (UserVariableNameNode->Name);
      FreePool (UserVariableNameNode->PromptString);
      FreePool (UserVariableNameNode->HelpString);
      FreePool (UserVariableNameNode);
    }

    FreePool (UserVariableNode->PromptString);
    FreePool (UserVariableNode);
  }
}

/**
  Create a time based data payload by concatenating the EFI_VARIABLE_AUTHENTICATION_2
  descriptor with the input data. NO authentication is required in this function.

  @param[in, out] DataSize          On input, the size of Data buffer in bytes.
                                    On output, the size of data returned in Data
                                    buffer in bytes.
  @param[in, out] Data              On input, Pointer to data buffer to be wrapped or
                                    pointer to NULL to wrap an empty payload.
                                    On output, Pointer to the new payload date buffer allocated from pool,
                                    it's caller's responsibility to free the memory after using it.

  @retval EFI_SUCCESS               Create time based payload successfully.
  @retval EFI_OUT_OF_RESOURCES      There are not enough memory resourses to create time based payload.
  @retval EFI_INVALID_PARAMETER     The parameter is invalid.
  @retval Others                    Unexpected error happens.

**/
EFI_STATUS
CreateTimeBasedPayload (
  IN OUT UINTN      *DataSize,
  IN OUT UINT8      **Data
  )
{
  EFI_STATUS                        Status;
  UINT8                             *NewData;
  UINT8                             *Payload;
  UINTN                             PayloadSize;
  EFI_VARIABLE_AUTHENTICATION_2     *DescriptorData;
  UINTN                             DescriptorSize;
  EFI_TIME                          Time;

  if (Data == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // At user physical presence, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED_2 descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize = OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo) + OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  NewData = (UINT8 *) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION_2 *) (NewData);

  ZeroMem (&Time, sizeof (EFI_TIME));
  Status = gRT->GetTime (&Time, NULL);
  if (EFI_ERROR (Status)) {
    FreePool (NewData);
    return Status;
  }
  Time.Pad1       = 0;
  Time.Nanosecond = 0;
  Time.TimeZone   = 0;
  Time.Daylight   = 0;
  Time.Pad2       = 0;
  CopyMem (&DescriptorData->TimeStamp, &Time, sizeof (EFI_TIME));

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertPkcs7Guid);

  if (Payload != NULL) {
    FreePool (Payload);
  }

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

/**
  Create a counter based data payload by concatenating the EFI_VARIABLE_AUTHENTICATION
  descriptor with the input data. NO authentication is required in this function.

  @param[in, out] DataSize          On input, the size of Data buffer in bytes.
                                    On output, the size of data returned in Data
                                    buffer in bytes.
  @param[in, out] Data              On input, Pointer to data buffer to be wrapped or
                                    pointer to NULL to wrap an empty payload.
                                    On output, Pointer to the new payload date buffer allocated from pool,
                                    it's caller's responsibility to free the memory after using it.

  @retval EFI_SUCCESS               Create counter based payload successfully.
  @retval EFI_OUT_OF_RESOURCES      There are not enough memory resourses to create time based payload.
  @retval EFI_INVALID_PARAMETER     The parameter is invalid.
  @retval Others                    Unexpected error happens.

**/
EFI_STATUS
CreateCounterBasedPayload (
  IN OUT UINTN      *DataSize,
  IN OUT UINT8      **Data
  )
{
  EFI_STATUS                        Status;
  UINT8                             *NewData;
  UINT8                             *Payload;
  UINTN                             PayloadSize;
  EFI_VARIABLE_AUTHENTICATION       *DescriptorData;
  UINTN                             DescriptorSize;
  UINT64                            MonotonicCount;

  if (Data == NULL || DataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // At user physical presence, the variable does not need to be signed but the
  // parameters to the SetVariable() call still need to be prepared as authenticated
  // variable. So we create EFI_VARIABLE_AUTHENTICATED descriptor without certificate
  // data in it.
  //
  Payload     = *Data;
  PayloadSize = *DataSize;

  DescriptorSize = (OFFSET_OF (EFI_VARIABLE_AUTHENTICATION, AuthInfo)) + \
                   (OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)) + \
                   sizeof (EFI_CERT_BLOCK_RSA_2048_SHA256);
  NewData = (UINT8 *) AllocateZeroPool (DescriptorSize + PayloadSize);
  if (NewData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((Payload != NULL) && (PayloadSize != 0)) {
    CopyMem (NewData + DescriptorSize, Payload, PayloadSize);
  }

  DescriptorData = (EFI_VARIABLE_AUTHENTICATION *) (NewData);

  Status = gBS->GetNextMonotonicCount (&MonotonicCount);
  if (EFI_ERROR (Status)) {
    FreePool (NewData);
    return Status;
  }
  DescriptorData->MonotonicCount = MonotonicCount;

  DescriptorData->AuthInfo.Hdr.dwLength         = OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData) + sizeof (EFI_CERT_BLOCK_RSA_2048_SHA256);
  DescriptorData->AuthInfo.Hdr.wRevision        = 0x0200;
  DescriptorData->AuthInfo.Hdr.wCertificateType = WIN_CERT_TYPE_EFI_GUID;
  CopyGuid (&DescriptorData->AuthInfo.CertType, &gEfiCertTypeRsa2048Sha256Guid);

  if (Payload != NULL) {
    FreePool (Payload);
  }

  *DataSize = DescriptorSize + PayloadSize;
  *Data     = NewData;
  return EFI_SUCCESS;
}

/**
  Delete user variable.

  @param[in] DeleteAll              Delete all user variables.
  @param[in] VariableCleanupData    Pointer to variable cleanup data.

**/
VOID
DeleteUserVariable (
  IN BOOLEAN                DeleteAll,
  IN VARIABLE_CLEANUP_DATA  *VariableCleanupData OPTIONAL
  )
{
  EFI_STATUS                Status;
  USER_VARIABLE_NODE        *UserVariableNode;
  LIST_ENTRY                *Link;
  USER_VARIABLE_NAME_NODE   *UserVariableNameNode;
  LIST_ENTRY                *NameLink;
  UINTN                     DataSize;
  UINT8                     *Data;

  for (Link = mUserVariableList.ForwardLink
       ;Link != &mUserVariableList
       ;Link = Link->ForwardLink) {
    UserVariableNode = USER_VARIABLE_FROM_LINK (Link);

    for (NameLink = UserVariableNode->NameLink.ForwardLink
        ;NameLink != &UserVariableNode->NameLink
        ;NameLink = NameLink->ForwardLink) {
      UserVariableNameNode = USER_VARIABLE_NAME_FROM_LINK (NameLink);

      if (!UserVariableNameNode->Deleted && (DeleteAll || ((VariableCleanupData != NULL) && (VariableCleanupData->UserVariable[UserVariableNameNode->Index] == TRUE)))) {
        DEBUG ((EFI_D_INFO, "PlatformVarCleanup - Delete variable: %g:%s\n", &UserVariableNode->Guid, UserVariableNameNode->Name));
        if ((UserVariableNameNode->Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) != 0) {
          DataSize = 0;
          Data = NULL;
          Status = CreateTimeBasedPayload (&DataSize, &Data);
          if (!EFI_ERROR (Status)) {
            Status = gRT->SetVariable (UserVariableNameNode->Name, &UserVariableNode->Guid, UserVariableNameNode->Attributes, DataSize, Data);
            FreePool (Data);
          }
        } else if ((UserVariableNameNode->Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
          DataSize = 0;
          Data = NULL;
          Status = CreateCounterBasedPayload (&DataSize, &Data);
          if (!EFI_ERROR (Status)) {
            Status = gRT->SetVariable (UserVariableNameNode->Name, &UserVariableNode->Guid, UserVariableNameNode->Attributes, DataSize, Data);
            FreePool (Data);
          }
        } else {
          Status = gRT->SetVariable (UserVariableNameNode->Name, &UserVariableNode->Guid, 0, 0, NULL);
        }
        if (!EFI_ERROR (Status)) {
          UserVariableNameNode->Deleted = TRUE;
        } else {
          DEBUG ((EFI_D_INFO, "PlatformVarCleanup - Delete variable fail: %g:%s\n", &UserVariableNode->Guid, UserVariableNameNode->Name));
        }
      }
    }
  }
}

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]  This          Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Request       A null-terminated Unicode string in <ConfigRequest> format.
  @param[out] Progress      On return, points to a character in the Request string.
                            Points to the string's null terminator if request was successful.
                            Points to the most recent '&' before the first failing name/value
                            pair (or the beginning of the string if the failure is in the
                            first name/value pair) if the request was not successful.
  @param[out] Results       A null-terminated Unicode string in <ConfigAltResp> format which
                            has all values filled in for the names in the Request string.
                            String to be allocated by the called function.

  @retval EFI_SUCCESS               The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES      Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER     Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND             Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
VariableCleanupHiiExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN  CONST EFI_STRING                          Request,
  OUT EFI_STRING                                *Progress,
  OUT EFI_STRING                                *Results
  )
{
  EFI_STATUS                        Status;
  VARIABLE_CLEANUP_HII_PRIVATE_DATA *Private;
  UINTN                             BufferSize;
  EFI_STRING                        ConfigRequestHdr;
  EFI_STRING                        ConfigRequest;
  BOOLEAN                           AllocatedRequest;
  UINTN                             Size;

  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *Progress = Request;
  if ((Request != NULL) && !HiiIsConfigHdrMatch (Request, &mVariableCleanupHiiGuid, mVarStoreName)) {
    return EFI_NOT_FOUND;
  }

  ConfigRequestHdr = NULL;
  ConfigRequest    = NULL;
  AllocatedRequest = FALSE;
  Size             = 0;

  Private = VARIABLE_CLEANUP_HII_PRIVATE_FROM_THIS (This);
  //
  // Convert buffer data to <ConfigResp> by helper function BlockToConfig().
  //
  BufferSize = sizeof (VARIABLE_CLEANUP_DATA);
  ConfigRequest = Request;
  if ((Request == NULL) || (StrStr (Request, L"OFFSET") == NULL)) {
    //
    // Request has no request element, construct full request string.
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator.
    //
    ConfigRequestHdr = HiiConstructConfigHdr (&mVariableCleanupHiiGuid, mVarStoreName, Private->HiiHandle);
    Size = (StrLen (ConfigRequestHdr) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    ASSERT (ConfigRequest != NULL);
    AllocatedRequest = TRUE;
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", ConfigRequestHdr, (UINT64)BufferSize);
    FreePool (ConfigRequestHdr);
  }

  Status = Private->ConfigRouting->BlockToConfig (
                                     Private->ConfigRouting,
                                     ConfigRequest,
                                     (UINT8 *) &Private->VariableCleanupData,
                                     BufferSize,
                                     Results,
                                     Progress
                                     );
  ASSERT_EFI_ERROR (Status);

  //
  // Free the allocated config request string.
  //
  if (AllocatedRequest) {
    FreePool (ConfigRequest);
    ConfigRequest = NULL;
  }
  //
  // Set Progress string to the original request string or the string's null terminator.
  //
  if (Request == NULL) {
    *Progress = NULL;
  } else if (StrStr (Request, L"OFFSET") == NULL) {
    *Progress = Request + StrLen (Request);
  }

  return Status;
}

/**
  Update user variable form.

  @param[in] Private    Points to the VARIABLE_CLEANUP_HII_PRIVATE_DATA.

**/
VOID
UpdateUserVariableForm (
  IN VARIABLE_CLEANUP_HII_PRIVATE_DATA  *Private
  )
{
  EFI_STRING_ID             PromptStringToken;
  EFI_STRING_ID             HelpStringToken;
  VOID                      *StartOpCodeHandle;
  VOID                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL        *StartLabel;
  EFI_IFR_GUID_LABEL        *EndLabel;
  USER_VARIABLE_NODE        *UserVariableNode;
  LIST_ENTRY                *Link;
  USER_VARIABLE_NAME_NODE   *UserVariableNameNode;
  LIST_ENTRY                *NameLink;
  BOOLEAN                   Created;

  //
  // Init OpCode Handle.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode as the start opcode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (StartOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  StartLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number = LABEL_START;

  //
  // Create Hii Extend Label OpCode as the end opcode.
  //
  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (EndOpCodeHandle, &gEfiIfrTianoGuid, NULL, sizeof (EFI_IFR_GUID_LABEL));
  EndLabel->ExtendOpCode = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number = LABEL_END;

  HiiUpdateForm (
    Private->HiiHandle,
    &mVariableCleanupHiiGuid,
    FORM_ID_VARIABLE_CLEANUP,
    StartOpCodeHandle, // LABEL_START
    EndOpCodeHandle    // LABEL_END
    );

  for (Link = mUserVariableList.ForwardLink
      ;Link != &mUserVariableList
      ;Link = Link->ForwardLink) {
    UserVariableNode = USER_VARIABLE_FROM_LINK (Link);

    //
    // Create checkbox opcode for variables in the same variable GUID space.
    //
    Created = FALSE;
    for (NameLink = UserVariableNode->NameLink.ForwardLink
        ;NameLink != &UserVariableNode->NameLink
        ;NameLink = NameLink->ForwardLink) {
      UserVariableNameNode = USER_VARIABLE_NAME_FROM_LINK (NameLink);

      if (!UserVariableNameNode->Deleted) {
        if (!Created) {
          //
          // Create subtitle opcode for variable GUID.
          //
          PromptStringToken = HiiSetString (Private->HiiHandle, 0, UserVariableNode->PromptString, NULL);
          HiiCreateSubTitleOpCode (StartOpCodeHandle, PromptStringToken, 0, 0, 0);
          Created = TRUE;
        }

        //
        // Only create opcode for the non-deleted variables.
        //
        PromptStringToken = HiiSetString (Private->HiiHandle, 0, UserVariableNameNode->PromptString, NULL);
        HelpStringToken = HiiSetString (Private->HiiHandle, 0, UserVariableNameNode->HelpString, NULL);
        HiiCreateCheckBoxOpCode (
          StartOpCodeHandle,
          UserVariableNameNode->QuestionId,
          VARIABLE_CLEANUP_VARSTORE_ID,
          (UINT16) (USER_VARIABLE_VAR_OFFSET + UserVariableNameNode->Index),
          PromptStringToken,
          HelpStringToken,
          EFI_IFR_FLAG_CALLBACK,
          Private->VariableCleanupData.UserVariable[UserVariableNameNode->Index],
          NULL
          );
      }
    }
  }

  HiiCreateSubTitleOpCode (
    StartOpCodeHandle,
    STRING_TOKEN (STR_NULL_STRING),
    0,
    0,
    0
    );

  //
  // Create the "Apply changes" and "Discard changes" tags.
  //
  HiiCreateActionOpCode (
    StartOpCodeHandle,
    SAVE_AND_EXIT_QUESTION_ID,
    STRING_TOKEN (STR_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    EFI_IFR_FLAG_CALLBACK,
    0
    );
  HiiCreateActionOpCode (
    StartOpCodeHandle,
    NO_SAVE_AND_EXIT_QUESTION_ID,
    STRING_TOKEN (STR_NO_SAVE_AND_EXIT),
    STRING_TOKEN (STR_NULL_STRING),
    EFI_IFR_FLAG_CALLBACK,
    0
    );

  HiiUpdateForm (
    Private->HiiHandle,
    &mVariableCleanupHiiGuid,
    FORM_ID_VARIABLE_CLEANUP,
    StartOpCodeHandle, // LABEL_START
    EndOpCodeHandle    // LABEL_END
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}

/**
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver's name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job. Currently not implemented.

  @param[in]  This                  Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration         A null-terminated Unicode string in
                                    <ConfigString> format.
  @param[out] Progress              A pointer to a string filled in with the
                                    offset of the most recent '&' before the
                                    first failing name / value pair (or the
                                    beginn ing of the string if the failure
                                    is in the first name / value pair) or
                                    the terminating NULL if all was
                                    successful.

  @retval EFI_SUCCESS               The results have been distributed or are
                                    awaiting distribution.
  @retval EFI_OUT_OF_RESOURCES      Not enough memory to store the
                                    parts of the results that must be
                                    stored awaiting possible future
                                    protocols.
  @retval EFI_INVALID_PARAMETERS    Passing in a NULL for the
                                    Results parameter would result
                                    in this type of error.
  @retval EFI_NOT_FOUND             Target for the specified routing data
                                    was not found.

**/
EFI_STATUS
EFIAPI
VariableCleanupHiiRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN  CONST EFI_STRING                          Configuration,
  OUT EFI_STRING                                *Progress
  )
{
  EFI_STATUS                        Status;
  VARIABLE_CLEANUP_HII_PRIVATE_DATA *Private;
  UINTN                             BufferSize;

  if (Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Configuration;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check routing data in <ConfigHdr>.
  // Note: there is no name for Name/Value storage, only GUID will be checked.
  //
  if (!HiiIsConfigHdrMatch (Configuration, &mVariableCleanupHiiGuid, mVarStoreName)) {
    return EFI_NOT_FOUND;
  }

  Private = VARIABLE_CLEANUP_HII_PRIVATE_FROM_THIS (This);
  //
  // Get Buffer Storage data.
  //
  BufferSize = sizeof (VARIABLE_CLEANUP_DATA);
  //
  // Convert <ConfigResp> to buffer data by helper function ConfigToBlock().
  //
  Status = Private->ConfigRouting->ConfigToBlock (
                            Private->ConfigRouting,
                            Configuration,
                            (UINT8 *) &Private->VariableCleanupData,
                            &BufferSize,
                            Progress
                            );
  ASSERT_EFI_ERROR (Status);

  DeleteUserVariable (FALSE, &Private->VariableCleanupData);
  //
  // For "F10" hotkey to refresh the form.
  //
//  UpdateUserVariableForm (Private);

  return EFI_SUCCESS;
}

/**
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param[in]  This                  Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action                Specifies the type of action taken by the browser.
  @param[in]  QuestionId            A unique value which is sent to the original
                                    exporting driver so that it can identify the type
                                    of data to expect. The format of the data tends to
                                    vary based on the opcode that generated the callback.
  @param[in]  Type                  The type of value for the question.
  @param[in]  Value                 A pointer to the data being sent to the original
                                    exporting driver.
  @param[out] ActionRequest         On return, points to the action requested by the
                                    callback function.

  @retval EFI_SUCCESS               The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES      Not enough storage is available to hold the
                                    variable and its data.
  @retval EFI_DEVICE_ERROR          The variable could not be saved.
  @retval EFI_UNSUPPORTED           The specified Action is not supported by the
                                    callback.
**/
EFI_STATUS
EFIAPI
VariableCleanupHiiCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
{
  VARIABLE_CLEANUP_HII_PRIVATE_DATA *Private;
  VARIABLE_CLEANUP_DATA             *VariableCleanupData;

  Private = VARIABLE_CLEANUP_HII_PRIVATE_FROM_THIS (This);

  if ((Action != EFI_BROWSER_ACTION_CHANGING) && (Action != EFI_BROWSER_ACTION_CHANGED)) {
    //
    // All other action return unsupported.
    //
    return EFI_UNSUPPORTED;
  }

  //
  // Retrieve uncommitted data from Form Browser.
  //
  VariableCleanupData = &Private->VariableCleanupData;
  HiiGetBrowserData (&mVariableCleanupHiiGuid, mVarStoreName, sizeof (VARIABLE_CLEANUP_DATA), (UINT8 *) VariableCleanupData);
  if (Action == EFI_BROWSER_ACTION_CHANGING) {
    if (Value == NULL) {
      return EFI_INVALID_PARAMETER;
    }
  } else if (Action == EFI_BROWSER_ACTION_CHANGED) {
    if ((Value == NULL) || (ActionRequest == NULL)) {
      return EFI_INVALID_PARAMETER;
    }
    if ((QuestionId >= USER_VARIABLE_QUESTION_ID) && (QuestionId < USER_VARIABLE_QUESTION_ID + MAX_USER_VARIABLE_COUNT)) {
      if (Value->b){
        //
        // Means one user variable checkbox is marked to delete but not press F10 or "Commit Changes and Exit" menu.
        //
        mMarkedUserVariableCount++;
        ASSERT (mMarkedUserVariableCount <= mUserVariableCount);
        if (mMarkedUserVariableCount == mUserVariableCount) {
          //
          // All user variables have been marked, then also mark the SelectAll checkbox.
          //
          VariableCleanupData->SelectAll = TRUE;
        }
      } else {
        //
        // Means one user variable checkbox is unmarked.
        //
        mMarkedUserVariableCount--;
        //
        // Also unmark the SelectAll checkbox.
        //
        VariableCleanupData->SelectAll = FALSE;
      }
    } else {
      switch (QuestionId) {
        case SELECT_ALL_QUESTION_ID:
         if (Value->b){
            //
            // Means the SelectAll checkbox is marked to delete all user variables but not press F10 or "Commit Changes and Exit" menu.
            //
            SetMem (VariableCleanupData->UserVariable, sizeof (VariableCleanupData->UserVariable), TRUE);
            mMarkedUserVariableCount = mUserVariableCount;
          } else {
            //
            // Means the SelectAll checkbox is unmarked.
            //
            SetMem (VariableCleanupData->UserVariable, sizeof (VariableCleanupData->UserVariable), FALSE);
            mMarkedUserVariableCount = 0;
          }
          break;
        case SAVE_AND_EXIT_QUESTION_ID:
          DeleteUserVariable (FALSE, VariableCleanupData);
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_SUBMIT_EXIT;
          break;

        case NO_SAVE_AND_EXIT_QUESTION_ID:
          //
          // Restore local maintain data.
          //
          *ActionRequest = EFI_BROWSER_ACTION_REQUEST_FORM_DISCARD_EXIT;
          break;

        default:
          break;
      }
    }
  }

  //
  // Pass changed uncommitted data back to Form Browser.
  //
  HiiSetBrowserData (&mVariableCleanupHiiGuid, mVarStoreName, sizeof (VARIABLE_CLEANUP_DATA), (UINT8 *) VariableCleanupData, NULL);
  return EFI_SUCCESS;
}

/**
  Platform variable cleanup.

  @param[in] Flag                   Variable error flag.
  @param[in] Type                   Variable cleanup type.
                                    If it is VarCleanupManually, the interface must be called after console connected.

  @retval EFI_SUCCESS               No error or error processed.
  @retval EFI_UNSUPPORTED           The specified Flag or Type is not supported.
                                    For example, system error may be not supported to process and Platform should have mechanism to reset system to manufacture mode.
                                    Another, if system and user variables are wanted to be distinguished to process, the interface must be called after EndOfDxe.
  @retval EFI_OUT_OF_RESOURCES      Not enough resource to process the error.
  @retval EFI_INVALID_PARAMETER     The specified Flag or Type is an invalid value.
  @retval Others                    Other failure occurs.

**/
EFI_STATUS
EFIAPI
PlatformVarCleanup (
  IN VAR_ERROR_FLAG     Flag,
  IN VAR_CLEANUP_TYPE   Type
  )
{
  EFI_STATUS                            Status;
  EFI_FORM_BROWSER2_PROTOCOL            *FormBrowser2;
  VARIABLE_CLEANUP_HII_PRIVATE_DATA     *Private;

  if (!mEndOfDxe) {
    //
    // This implementation must be called after EndOfDxe.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Type >= VarCleanupMax) || ((Flag & ((VAR_ERROR_FLAG) (VAR_ERROR_FLAG_SYSTEM_ERROR & VAR_ERROR_FLAG_USER_ERROR))) == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if (Flag == VAR_ERROR_FLAG_NO_ERROR) {
    //
    // Just return success if no error.
    //
    return EFI_SUCCESS;
  }

  if ((Flag & (~((VAR_ERROR_FLAG) VAR_ERROR_FLAG_SYSTEM_ERROR))) == 0) {
    //
    // This sample does not support system variables cleanup.
    //
    DEBUG ((EFI_D_ERROR, "NOTICE - VAR_ERROR_FLAG_SYSTEM_ERROR\n"));
    DEBUG ((EFI_D_ERROR, "Platform should have mechanism to reset system to manufacture mode\n"));
    return EFI_UNSUPPORTED;
  }

  //
  // Continue to process VAR_ERROR_FLAG_USER_ERROR.
  //

  //
  // Create user variable nodes for the following processing.
  //
  CreateUserVariableNode ();

  switch (Type) {
    case VarCleanupAll:
      DeleteUserVariable (TRUE, NULL);
      //
      // Destroyed the created user variable nodes
      //
      DestroyUserVariableNode ();
      return EFI_SUCCESS;
      break;

    case VarCleanupManually:
      //
      // Locate FormBrowser2 protocol.
      //
      Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Private = AllocateZeroPool (sizeof (VARIABLE_CLEANUP_HII_PRIVATE_DATA));
      if (Private == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }

      Private->Signature = VARIABLE_CLEANUP_HII_PRIVATE_SIGNATURE;
      Private->ConfigAccess.ExtractConfig = VariableCleanupHiiExtractConfig;
      Private->ConfigAccess.RouteConfig   = VariableCleanupHiiRouteConfig;
      Private->ConfigAccess.Callback      = VariableCleanupHiiCallback;

      Status = gBS->LocateProtocol (
                      &gEfiHiiConfigRoutingProtocolGuid,
                      NULL,
                      (VOID **) &Private->ConfigRouting
                      );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Install Device Path Protocol and Config Access protocol to driver handle.
      //
      Status = gBS->InstallMultipleProtocolInterfaces (
                      &Private->DriverHandle,
                      &gEfiDevicePathProtocolGuid,
                      &mVarCleanupHiiVendorDevicePath,
                      &gEfiHiiConfigAccessProtocolGuid,
                      &Private->ConfigAccess,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Publish our HII data.
      //
      Private->HiiHandle = HiiAddPackages (
                             &mVariableCleanupHiiGuid,
                             Private->DriverHandle,
                             PlatformVarCleanupLibStrings,
                             PlatVarCleanupBin,
                             NULL
                             );
      if (Private->HiiHandle == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }

      UpdateUserVariableForm (Private);

      Status = FormBrowser2->SendForm (
                               FormBrowser2,
                               &Private->HiiHandle,
                               1,
                               NULL,
                               0,
                               NULL,
                               NULL
                               );
      break;

    default:
      return EFI_UNSUPPORTED;
      break;
  }

Done:
  if (Private->DriverHandle != NULL) {
    gBS->UninstallMultipleProtocolInterfaces (
           Private->DriverHandle,
           &gEfiDevicePathProtocolGuid,
           &mVarCleanupHiiVendorDevicePath,
           &gEfiHiiConfigAccessProtocolGuid,
           &Private->ConfigAccess,
           NULL
           );
  }
  if (Private->HiiHandle != NULL) {
    HiiRemovePackages (Private->HiiHandle);
  }

  FreePool (Private);

  //
  // Destroyed the created user variable nodes
  //
  DestroyUserVariableNode ();
  return Status;
}

/**
  Get last boot variable error flag.

  @return   Last boot variable error flag.

**/
VAR_ERROR_FLAG
EFIAPI
GetLastBootVarErrorFlag (
  VOID
  )
{
  return mLastVarErrorFlag;
}

/**
  Notification function of END_OF_DXE.

  This is a notification function registered on END_OF_DXE event.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    Pointer to the notification function's context.

**/
VOID
EFIAPI
PlatformVarCleanupEndOfDxeEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mEndOfDxe = TRUE;
}

/**
  The constructor function caches the pointer to VarCheck protocol and last boot variable error flag.

  The constructor function locates VarCheck protocol from protocol database.
  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PlatformVarCleanupLibConstructor (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;

  mLastVarErrorFlag = InternalGetVarErrorFlag ();
  DEBUG ((EFI_D_INFO, "mLastVarErrorFlag - 0x%02x\n", mLastVarErrorFlag));

  //
  // Register EFI_END_OF_DXE_EVENT_GROUP_GUID event.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  PlatformVarCleanupEndOfDxeEvent,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &mPlatVarCleanupLibEndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  The destructor function closes the End of DXE event.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.

**/
EFI_STATUS
EFIAPI
PlatformVarCleanupLibDestructor (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS    Status;

  //
  // Close the End of DXE event.
  //
  Status = gBS->CloseEvent (mPlatVarCleanupLibEndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
