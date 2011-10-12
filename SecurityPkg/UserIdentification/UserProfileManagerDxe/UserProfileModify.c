/** @file
  The functions to modify a user profile.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserProfileManager.h"

EFI_USER_PROFILE_HANDLE           mModifyUser = NULL;

/**
  Display user select form, cab select a user to modify.

**/
VOID
SelectUserToModify  (
  VOID
  )
{
  EFI_STATUS              Status;
  UINT8                   Index;
  EFI_USER_PROFILE_HANDLE User;
  EFI_USER_PROFILE_HANDLE CurrentUser;
  UINT32                  CurrentAccessRight;
  VOID                    *StartOpCodeHandle;
  VOID                    *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL      *StartLabel;
  EFI_IFR_GUID_LABEL      *EndLabel;

  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABEL_USER_MOD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add each user can be modified.
  //
  User  = NULL;
  Index = 1;
  mUserManager->Current (mUserManager, &CurrentUser);
  while (TRUE) {
    Status = mUserManager->GetNext (mUserManager, &User);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = GetAccessRight (&CurrentAccessRight);
    if (EFI_ERROR (Status)) {
      CurrentAccessRight = EFI_USER_INFO_ACCESS_ENROLL_SELF;
    }

    if ((CurrentAccessRight == EFI_USER_INFO_ACCESS_MANAGE) || (User == CurrentUser)) {
      AddUserToForm (User, (UINT16)(KEY_MODIFY_USER | KEY_SELECT_USER | Index), StartOpCodeHandle);
    }
    Index++;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_MODIFY_USER,       // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Get all the user info from mModifyUser in the user manager, and save on the
  global variable.

**/
VOID
GetAllUserInfo (
  VOID
  )
{
  EFI_STATUS            Status;
  EFI_USER_INFO_HANDLE  UserInfo;
  EFI_USER_INFO         *Info;
  UINTN                 InfoSize;
  UINTN                 MemSize;
  UINTN                 DataLen;

  //
  // Init variable to default value.
  //
  mProviderChoice                   = 0;
  mConncetLogical                   = 0;

  mUserInfo.CreateDateExist         = FALSE;
  mUserInfo.UsageDateExist          = FALSE;
  mUserInfo.UsageCount              = 0;
  
  mUserInfo.AccessPolicyLen         = 0;
  mUserInfo.AccessPolicyModified    = FALSE;
  if (mUserInfo.AccessPolicy != NULL) {
    FreePool (mUserInfo.AccessPolicy);
    mUserInfo.AccessPolicy = NULL;
  }
  mUserInfo.IdentityPolicyLen       = 0;
  mUserInfo.IdentityPolicyModified  = FALSE;
  if (mUserInfo.IdentityPolicy != NULL) {
    FreePool (mUserInfo.IdentityPolicy);
    mUserInfo.IdentityPolicy = NULL;
  }
  
  //
  // Allocate user information memory.
  //
  MemSize = sizeof (EFI_USER_INFO) + 63;
  Info    = AllocateZeroPool (MemSize);
  if (Info == NULL) {
    return ;
  }
  
  //
  // Get each user information.
  //
  UserInfo = NULL;
  while (TRUE) {
    Status = mUserManager->GetNextInfo (mUserManager, mModifyUser, &UserInfo);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Get information.
    //
    InfoSize  = MemSize;
    Status    = mUserManager->GetInfo (
                                mUserManager, 
                                mModifyUser, 
                                UserInfo, 
                                Info, 
                                &InfoSize
                                );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MemSize = InfoSize;
      FreePool (Info);
      Info = AllocateZeroPool (MemSize);
      if (Info == NULL) {
        return ;
      }

      Status = mUserManager->GetInfo (
                               mUserManager,
                               mModifyUser,
                               UserInfo,
                               Info,
                               &InfoSize
                               );
    }

    if (Status == EFI_SUCCESS) {
      //
      // Deal with each information according to informaiton type.
      //
      DataLen = Info->InfoSize - sizeof (EFI_USER_INFO);
      switch (Info->InfoType) {
      case EFI_USER_INFO_NAME_RECORD:
        CopyMem (&mUserInfo.UserName, (UINT8 *) (Info + 1), DataLen);
        break;

      case EFI_USER_INFO_CREATE_DATE_RECORD:
        CopyMem (&mUserInfo.CreateDate, (UINT8 *) (Info + 1), DataLen);
        mUserInfo.CreateDateExist = TRUE;
        break;

      case EFI_USER_INFO_USAGE_DATE_RECORD:
        CopyMem (&mUserInfo.UsageDate, (UINT8 *) (Info + 1), DataLen);
        mUserInfo.UsageDateExist = TRUE;
        break;

      case EFI_USER_INFO_USAGE_COUNT_RECORD:
        CopyMem (&mUserInfo.UsageCount, (UINT8 *) (Info + 1), DataLen);
        break;

      case EFI_USER_INFO_ACCESS_POLICY_RECORD:
        mUserInfo.AccessPolicy = AllocateZeroPool (DataLen);
        if (mUserInfo.AccessPolicy == NULL) {
          break;
        }

        CopyMem (mUserInfo.AccessPolicy, (UINT8 *) (Info + 1), DataLen);
        mUserInfo.AccessPolicyLen = DataLen;
        break;

      case EFI_USER_INFO_IDENTITY_POLICY_RECORD:
        mUserInfo.IdentityPolicy = AllocateZeroPool (DataLen);
        if (mUserInfo.IdentityPolicy == NULL) {
          break;
        }

        CopyMem (mUserInfo.IdentityPolicy, (UINT8 *) (Info + 1), DataLen);
        mUserInfo.IdentityPolicyLen = DataLen;
        break;

      default:
        break;
      }
    }
  }
  FreePool (Info);
}


/**
  Convert the Date to a string, and update the Hii database DateID string with it.

  @param[in] Date       Points to the date to be converted.
  @param[in] DateId     String ID in the HII database to be replaced.

**/
VOID
ResolveDate (
  IN EFI_TIME                                   *Date,
  IN EFI_STRING_ID                              DateId
  )
{
  CHAR16  *Str;
  UINTN   DateBufLen;

  //
  // Convert date to string.
  //
  DateBufLen = 64;
  Str        = AllocateZeroPool (DateBufLen);
  if (Str == NULL) {
    return ;
  }

  UnicodeSPrint (
    Str,
    DateBufLen,
    L"%4d-%2d-%2d ",
    Date->Year,
    Date->Month,
    Date->Day
    );

  //
  // Convert time to string.
  //
  DateBufLen -= StrLen (Str);
  UnicodeSPrint (
    Str + StrLen (Str),
    DateBufLen,
    L"%2d:%2d:%2d", 
    Date->Hour,
    Date->Minute,
    Date->Second
    );
  
  HiiSetString (mCallbackInfo->HiiHandle, DateId, Str, NULL);
  FreePool (Str);
}


/**
  Convert the CountVal to a string, and update the Hii database CountId string
  with it.

  @param[in]  CountVal   The hex value to convert.
  @param[in]  CountId    String ID in the HII database to be replaced.

**/
VOID
ResolveCount (
  IN UINT32                                     CountVal,
  IN EFI_STRING_ID                              CountId
  )
{
  CHAR16  Count[10];

  UnicodeSPrint (Count, 20, L"%d", CountVal);  
  HiiSetString (mCallbackInfo->HiiHandle, CountId, Count, NULL);
}


/**
  Concatenates one Null-terminated Unicode string to another Null-terminated
  Unicode string.

  @param[in, out]  Source1      On entry, point to a Null-terminated Unicode string.
                                On exit, point to a new concatenated Unicode string                                
  @param[in]       Source2      Pointer to a Null-terminated Unicode string.

**/
VOID
AddStr (
  IN OUT  CHAR16                  **Source1,
  IN      CONST CHAR16            *Source2
  )
{
  CHAR16                        *TmpStr;
  UINTN                         StrLength;

  ASSERT (Source1 != NULL);
  ASSERT (Source2 != NULL);

  if (*Source1 == NULL) {
    StrLength = StrSize (Source2);
  } else {
    StrLength  = StrSize (*Source1);
    StrLength += StrSize (Source2) -1;
  }

  TmpStr     = AllocateZeroPool (StrLength);
  ASSERT (TmpStr != NULL);

  if (*Source1 == NULL) {
    StrCpy (TmpStr, Source2);;
  } else {
    StrCpy (TmpStr, *Source1);
    FreePool (*Source1);
    StrCat (TmpStr, Source2);
  }

  *Source1 = TmpStr;
}


/**
  Convert the identity policy to a unicode string and update the Hii database
  IpStringId string with it.

  @param[in]  Ip         Points to identity policy.
  @param[in]  IpLen      The identity policy length.
  @param[in]  IpStringId String ID in the HII database to be replaced.

**/
VOID
ResolveIdentityPolicy (
  IN  UINT8                                     *Ip,
  IN  UINTN                                     IpLen,
  IN  EFI_STRING_ID                             IpStringId
  )
{
  CHAR16                        *TmpStr;
  UINTN                         ChkLen;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  UINT16                        Index;
  CHAR16                        *ProvStr;
  EFI_STRING_ID                 ProvId;
  EFI_HII_HANDLE                HiiHandle;
  EFI_USER_CREDENTIAL2_PROTOCOL *UserCredential;
 
  TmpStr = NULL;
  
  //
  // Resolve each policy.
  //
  ChkLen  = 0;
  while (ChkLen < IpLen) {
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (Ip + ChkLen);
    switch (Identity->Type) {
    case EFI_USER_INFO_IDENTITY_FALSE:
      AddStr (&TmpStr, L"False");
      break;

    case EFI_USER_INFO_IDENTITY_TRUE:
      AddStr (&TmpStr, L"None");
      break;

    case EFI_USER_INFO_IDENTITY_NOT:
      AddStr (&TmpStr, L"! ");
      break;

    case EFI_USER_INFO_IDENTITY_AND:
      AddStr (&TmpStr, L" && ");
      break;

    case EFI_USER_INFO_IDENTITY_OR:
      AddStr (&TmpStr, L" || ");
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_TYPE:
      for (Index = 0; Index < mProviderInfo->Count; Index++) {
        UserCredential = mProviderInfo->Provider[Index];
        if (CompareGuid ((EFI_GUID *) (Identity + 1), &UserCredential->Type)) {     
          UserCredential->Title (
                            UserCredential, 
                            &HiiHandle, 
                            &ProvId
                            );
          ProvStr = HiiGetString (HiiHandle, ProvId, NULL);
          if (ProvStr != NULL) {
            AddStr (&TmpStr, ProvStr);
            FreePool (ProvStr);
          }
          break;
        }
      }
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER:
      for (Index = 0; Index < mProviderInfo->Count; Index++) {
        UserCredential = mProviderInfo->Provider[Index];
        if (CompareGuid ((EFI_GUID *) (Identity + 1), &UserCredential->Identifier)) {          
          UserCredential->Title (
                            UserCredential,
                            &HiiHandle,
                            &ProvId
                            );
          ProvStr = HiiGetString (HiiHandle, ProvId, NULL);
          if (ProvStr != NULL) {
            AddStr (&TmpStr, ProvStr);
            FreePool (ProvStr);
          }
          break;
        }
      }
      break;
    }

    ChkLen += Identity->Length;
  }

  if (TmpStr != NULL) {
    HiiSetString (mCallbackInfo->HiiHandle, IpStringId, TmpStr, NULL);
    FreePool (TmpStr);
  }
}


/**
  Display modify user information form.

  This form displays, username, create Date, usage date, usage count, identity policy,
  and access policy.

  @param[in] UserIndex       The index of the user in display list to modify.
  
**/
VOID
ModifyUserInfo (
  IN UINT8                                      UserIndex
  )
{
  EFI_STATUS               Status;
  EFI_USER_PROFILE_HANDLE  CurrentUser;
  UINT32                   CurrentAccessRight;
  VOID                     *StartOpCodeHandle;
  VOID                     *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL       *StartLabel;
  EFI_IFR_GUID_LABEL       *EndLabel;

  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABEL_USER_INFO_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Find the user profile to be modified.
  //
  mModifyUser = NULL;
  Status      = mUserManager->GetNext (mUserManager, &mModifyUser);
  if (EFI_ERROR (Status)) {
    return ;
  }

  while (UserIndex > 1) {
    Status = mUserManager->GetNext (mUserManager, &mModifyUser);
    if (EFI_ERROR (Status)) {
      return ;
    }
    UserIndex--;
  }
  
  //
  // Get user profile information.
  //
  GetAllUserInfo ();

  //
  // Update user name.
  HiiSetString (
    mCallbackInfo->HiiHandle,
    STRING_TOKEN (STR_USER_NAME_VAL),
    mUserInfo.UserName,
    NULL
    );
  
  //
  // Update create date.
  //
  if (mUserInfo.CreateDateExist) {
    ResolveDate (&mUserInfo.CreateDate, STRING_TOKEN (STR_CREATE_DATE_VAL));
  } else {
    HiiSetString (
      mCallbackInfo->HiiHandle,
      STRING_TOKEN (STR_CREATE_DATE_VAL),
      L"",
      NULL
      );
  }
  
  //
  // Add usage date.
  //
  if (mUserInfo.UsageDateExist) {
    ResolveDate (&mUserInfo.UsageDate, STRING_TOKEN (STR_USAGE_DATE_VAL));
  } else {
    HiiSetString (
      mCallbackInfo->HiiHandle,
      STRING_TOKEN (STR_USAGE_DATE_VAL),
      L"",
      NULL
      );
  }
  
  //
  // Add usage count.
  //
  ResolveCount ((UINT32) mUserInfo.UsageCount, STRING_TOKEN (STR_USAGE_COUNT_VAL));
  
  //
  // Add identity policy.
  //
  mUserManager->Current (mUserManager, &CurrentUser);
  if (mModifyUser == CurrentUser) {
    ResolveIdentityPolicy (
      mUserInfo.IdentityPolicy,
      mUserInfo.IdentityPolicyLen,
      STRING_TOKEN (STR_IDENTIFY_POLICY_VAL)
      );
    HiiCreateGotoOpCode (
      StartOpCodeHandle,                                  // Container for opcodes
      FORMID_MODIFY_IP,                                   // Target Form ID
      STRING_TOKEN (STR_IDENTIFY_POLICY),                 // Prompt text
      STRING_TOKEN (STR_IDENTIFY_POLICY_VAL),             // Help text
      EFI_IFR_FLAG_CALLBACK,                              // Question flag
      KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_IP   // Question ID
      );
  }
  
  //
  // Add access policy.
  //
  Status = GetAccessRight (&CurrentAccessRight);
  if (EFI_ERROR (Status)) {
    CurrentAccessRight = EFI_USER_INFO_ACCESS_ENROLL_SELF;
  }

  if (CurrentAccessRight == EFI_USER_INFO_ACCESS_MANAGE) {
    HiiCreateGotoOpCode (
      StartOpCodeHandle,                                  // Container for opcodes
      FORMID_MODIFY_AP,                                   // Target Form ID
      STRING_TOKEN (STR_ACCESS_POLICY),                   // Prompt text
      STRING_TOKEN (STR_NULL_STRING),                     // Help text
      EFI_IFR_FLAG_CALLBACK,                              // Question flag
      KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_AP   // Question ID
      );
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle,                             // HII handle
    &gUserProfileManagerGuid,                             // Formset GUID
    FORMID_USER_INFO,                                     // Form ID
    StartOpCodeHandle,                                    // Label
    EndOpCodeHandle                                       // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Get all the access policy info from current user info, and save in the global
  variable.

**/
VOID
ResolveAccessPolicy (
  VOID
  )
{
  UINTN                         OffSet;
  EFI_USER_INFO_ACCESS_CONTROL  Control;
  UINTN                         ValLen;
  UINT8                         *AccessData;

  //
  // Set default value 
  //
  mAccessInfo.AccessRight       = EFI_USER_INFO_ACCESS_ENROLL_SELF;
  mAccessInfo.AccessSetup       = ACCESS_SETUP_RESTRICTED;
  mAccessInfo.AccessBootOrder   = EFI_USER_INFO_ACCESS_BOOT_ORDER_INSERT;

  mAccessInfo.LoadPermitLen     = 0;
  mAccessInfo.LoadForbidLen     = 0;
  mAccessInfo.ConnectPermitLen  = 0;
  mAccessInfo.ConnectForbidLen  = 0;
  
  //
  // Get each user access policy.
  //
  OffSet = 0;
  while (OffSet < mUserInfo.AccessPolicyLen) {
    CopyMem (&Control, mUserInfo.AccessPolicy + OffSet, sizeof (Control));    
    ValLen = Control.Size - sizeof (Control);
    switch (Control.Type) {
    case EFI_USER_INFO_ACCESS_ENROLL_SELF:
      mAccessInfo.AccessRight = EFI_USER_INFO_ACCESS_ENROLL_SELF;
      break;

    case EFI_USER_INFO_ACCESS_ENROLL_OTHERS:
      mAccessInfo.AccessRight = EFI_USER_INFO_ACCESS_ENROLL_OTHERS;
      break;

    case EFI_USER_INFO_ACCESS_MANAGE:
      mAccessInfo.AccessRight = EFI_USER_INFO_ACCESS_MANAGE;
      break;

    case EFI_USER_INFO_ACCESS_SETUP:
      AccessData = mUserInfo.AccessPolicy + OffSet + sizeof (Control);
      if (CompareGuid ((EFI_GUID *) AccessData, &gEfiUserInfoAccessSetupNormalGuid)) {
        mAccessInfo.AccessSetup = ACCESS_SETUP_NORMAL;
      } else if (CompareGuid ((EFI_GUID *) AccessData, &gEfiUserInfoAccessSetupRestrictedGuid)) {
        mAccessInfo.AccessSetup = ACCESS_SETUP_RESTRICTED;
      } else if (CompareGuid ((EFI_GUID *) AccessData, &gEfiUserInfoAccessSetupAdminGuid)) {
        mAccessInfo.AccessSetup = ACCESS_SETUP_ADMIN;
      }
      break;

    case EFI_USER_INFO_ACCESS_BOOT_ORDER:
      AccessData = mUserInfo.AccessPolicy + OffSet + sizeof (Control);
      CopyMem (&mAccessInfo.AccessBootOrder, AccessData, sizeof (UINT32));
      break;

    case EFI_USER_INFO_ACCESS_FORBID_LOAD:
      if (mAccessInfo.LoadForbid != NULL) {
        FreePool (mAccessInfo.LoadForbid);
      }

      mAccessInfo.LoadForbid = AllocateZeroPool (ValLen);
      if (mAccessInfo.LoadForbid != NULL) {
        AccessData = mUserInfo.AccessPolicy + OffSet + sizeof (Control);
        CopyMem (mAccessInfo.LoadForbid, AccessData, ValLen);
        mAccessInfo.LoadForbidLen = ValLen;
      }
      break;

    case EFI_USER_INFO_ACCESS_PERMIT_LOAD:
      if (mAccessInfo.LoadPermit != NULL) {
        FreePool (mAccessInfo.LoadPermit);
      }

      mAccessInfo.LoadPermit = AllocateZeroPool (ValLen);
      if (mAccessInfo.LoadPermit != NULL) {
        AccessData = mUserInfo.AccessPolicy + OffSet + sizeof (Control);
        CopyMem (mAccessInfo.LoadPermit, AccessData, ValLen);
        mAccessInfo.LoadPermitLen = ValLen;
      }
      break;

    case EFI_USER_INFO_ACCESS_FORBID_CONNECT:
      if (mAccessInfo.ConnectForbid != NULL) {
        FreePool (mAccessInfo.ConnectForbid);
      }

      mAccessInfo.ConnectForbid = AllocateZeroPool (ValLen);
      if (mAccessInfo.ConnectForbid != NULL) {
        AccessData = mUserInfo.AccessPolicy + OffSet + sizeof (Control);
        CopyMem (mAccessInfo.ConnectForbid, AccessData, ValLen);
        mAccessInfo.ConnectForbidLen = ValLen;
      }
      break;

    case EFI_USER_INFO_ACCESS_PERMIT_CONNECT:
      if (mAccessInfo.ConnectPermit != NULL) {
        FreePool (mAccessInfo.ConnectPermit);
      }

      mAccessInfo.ConnectPermit = AllocateZeroPool (ValLen);
      if (mAccessInfo.ConnectPermit != NULL) {
        AccessData = mUserInfo.AccessPolicy + OffSet + sizeof (Control);
        CopyMem (mAccessInfo.ConnectPermit, AccessData, ValLen);
        mAccessInfo.ConnectPermitLen = ValLen;
      }
      break;
    }

    OffSet += Control.Size;
  }
}


/**
  Find the specified info in profile mModifyUser by the InfoType.

  @param[in]  InfoType     The user information type to find.
  @param[out] UserInfo     Points to user information handle found.
  
  @retval EFI_SUCCESS      Find the user information successfully.
  @retval Others           Fail to find the user information.

**/
EFI_STATUS
FindInfoByType (
  IN  UINT8                                     InfoType,
  OUT EFI_USER_INFO_HANDLE                      *UserInfo
  )
{
  EFI_STATUS    Status;
  EFI_USER_INFO *Info;
  UINTN         InfoSize;
  UINTN         MemSize;

  if (UserInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *UserInfo = NULL;
  //
  // Allocate user information memory.
  //
  MemSize = sizeof (EFI_USER_INFO) + 63;
  Info    = AllocateZeroPool (MemSize);
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Get each user information.
  //
  while (TRUE) {
    Status = mUserManager->GetNextInfo (mUserManager, mModifyUser, UserInfo);
    if (EFI_ERROR (Status)) {
      break;
    }
    //
    // Get information.
    //
    InfoSize  = MemSize;
    Status    = mUserManager->GetInfo (
                                mUserManager,
                                mModifyUser,
                                *UserInfo,
                                Info,
                                &InfoSize
                                );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MemSize = InfoSize;
      FreePool (Info);
      Info = AllocateZeroPool (MemSize);
      if (Info == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      Status = mUserManager->GetInfo (
                               mUserManager,
                               mModifyUser,
                               *UserInfo,
                               Info,
                               &InfoSize
                               );
    }
    if (Status == EFI_SUCCESS) {
      if (Info->InfoType == InfoType) {
        break;
      }
    }
  }

  FreePool (Info);
  return Status;
}


/**
  Display modify user access policy form.

  In this form, access right, access setup and access boot order are dynamically
  added. Load devicepath and connect devicepath are displayed too.
  
**/
VOID
ModidyAccessPolicy (
  VOID
  )
{
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  VOID                *OptionsOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;
  VOID                *DefaultOpCodeHandle;
  
  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABEL_AP_MOD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;


  //
  // Resolve access policy information.
  //
  ResolveAccessPolicy ();

  //
  // Add access right one-of-code.
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);
  DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (DefaultOpCodeHandle != NULL);
  
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_NORMAL),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    EFI_USER_INFO_ACCESS_ENROLL_SELF
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_ENROLL),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    EFI_USER_INFO_ACCESS_ENROLL_OTHERS
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_MANAGE),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    EFI_USER_INFO_ACCESS_MANAGE
    );

  HiiCreateDefaultOpCode (
    DefaultOpCodeHandle, 
    EFI_HII_DEFAULT_CLASS_STANDARD, 
    EFI_IFR_NUMERIC_SIZE_1, 
    mAccessInfo.AccessRight
    );
 
  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                    // Container for dynamic created opcodes
    KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_AP | KEY_MODIFY_RIGHT, // Question ID
    0,                                    // VarStore ID
    0,                                    // Offset in Buffer Storage
    STRING_TOKEN (STR_ACCESS_RIGHT),      // Question prompt text
    STRING_TOKEN (STR_ACCESS_RIGHT_HELP), // Question help text
    EFI_IFR_FLAG_CALLBACK,                // Question flag
    EFI_IFR_NUMERIC_SIZE_1,               // Data type of Question Value
    OptionsOpCodeHandle,                  // Option Opcode list
    DefaultOpCodeHandle                   // Default Opcode
    );
  HiiFreeOpCodeHandle (DefaultOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);


  //
  // Add setup type one-of-code.
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);
  DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (DefaultOpCodeHandle != NULL);
  
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_RESTRICTED),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    ACCESS_SETUP_RESTRICTED
    );
    
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_NORMAL),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    ACCESS_SETUP_NORMAL
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_ADMIN),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    ACCESS_SETUP_ADMIN
    );

  HiiCreateDefaultOpCode (
    DefaultOpCodeHandle, 
    EFI_HII_DEFAULT_CLASS_STANDARD, 
    EFI_IFR_NUMERIC_SIZE_1, 
    mAccessInfo.AccessSetup
    );    

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                    // Container for dynamic created opcodes
    KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_AP | KEY_MODIFY_SETUP, // Question ID
    0,                                    // VarStore ID
    0,                                    // Offset in Buffer Storage
    STRING_TOKEN (STR_ACCESS_SETUP),      // Question prompt text
    STRING_TOKEN (STR_ACCESS_SETUP_HELP), // Question help text
    EFI_IFR_FLAG_CALLBACK,                // Question flag
    EFI_IFR_NUMERIC_SIZE_1,               // Data type of Question Value
    OptionsOpCodeHandle,                  // Option Opcode list
    DefaultOpCodeHandle                   // Default Opcode
    );
  HiiFreeOpCodeHandle (DefaultOpCodeHandle);
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  
  //
  // Add boot order one-of-code.
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);
  DefaultOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (DefaultOpCodeHandle != NULL);
  
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_INSERT),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    EFI_USER_INFO_ACCESS_BOOT_ORDER_INSERT
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_APPEND),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    EFI_USER_INFO_ACCESS_BOOT_ORDER_APPEND
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_REPLACE),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    EFI_USER_INFO_ACCESS_BOOT_ORDER_REPLACE
    );
    
  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_NODEFAULT),
    0,
    EFI_IFR_NUMERIC_SIZE_4,
    EFI_USER_INFO_ACCESS_BOOT_ORDER_NODEFAULT
    );

  HiiCreateDefaultOpCode (
    DefaultOpCodeHandle, 
    EFI_HII_DEFAULT_CLASS_STANDARD, 
    EFI_IFR_NUMERIC_SIZE_4, 
    mAccessInfo.AccessBootOrder
    );
    
  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                  // Container for dynamic created opcodes
    KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_AP | KEY_MODIFY_BOOT, // Question ID
    0,                                  // VarStore ID
    0,                                  // Offset in Buffer Storage
    STRING_TOKEN (STR_BOOR_ORDER),      // Question prompt text
    STRING_TOKEN (STR_BOOT_ORDER_HELP), // Question help text
    EFI_IFR_FLAG_CALLBACK,              // Question flag
    EFI_IFR_NUMERIC_SIZE_1,             // Data type of Question Value
    OptionsOpCodeHandle,                // Option Opcode list
    DefaultOpCodeHandle                 // Default Opcode
    );
  HiiFreeOpCodeHandle (DefaultOpCodeHandle);    
  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  //
  // Update Form.
  //
  HiiUpdateForm (
    mCallbackInfo->HiiHandle,           // HII handle
    &gUserProfileManagerGuid,           // Formset GUID
    FORMID_MODIFY_AP,                   // Form ID
    StartOpCodeHandle,                  // Label for where to insert opcodes
    EndOpCodeHandle                     // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Expand access policy memory size.

  @param[in] ValidLen       The valid access policy length.
  @param[in] ExpandLen      The length that is needed to expand.
    
**/
VOID
ExpandMemory (
  IN      UINTN                                 ValidLen,
  IN      UINTN                                 ExpandLen
  )
{
  UINT8 *Mem;
  UINTN Len;

  //
  // Expand memory.
  //
  Len = mUserInfo.AccessPolicyLen + (ExpandLen / 64 + 1) * 64;
  Mem = AllocateZeroPool (Len);
  ASSERT (Mem != NULL);

  if (mUserInfo.AccessPolicy != NULL) {
    CopyMem (Mem, mUserInfo.AccessPolicy, ValidLen);
    FreePool (mUserInfo.AccessPolicy);
  }

  mUserInfo.AccessPolicy    = Mem;
  mUserInfo.AccessPolicyLen = Len;
}


/**
  Collect all the access policy data to mUserInfo.AccessPolicy, 
  and save it to user profile.

**/
VOID
SaveAccessPolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINTN                         OffSet;
  UINTN                         Size;
  EFI_USER_INFO_ACCESS_CONTROL  Control;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *Info;

  if (mUserInfo.AccessPolicy != NULL) {
    FreePool (mUserInfo.AccessPolicy);
  }
  mUserInfo.AccessPolicy          = NULL;
  mUserInfo.AccessPolicyLen       = 0;
  mUserInfo.AccessPolicyModified  = TRUE;
  OffSet                          = 0;
  
  //
  // Save access right.
  //
  Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL);
  if (mUserInfo.AccessPolicyLen - OffSet < Size) {
    ExpandMemory (OffSet, Size);
  }

  Control.Type = mAccessInfo.AccessRight;
  Control.Size = (UINT32) Size;
  CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
  OffSet += sizeof (Control);
  
  //
  // Save access setup.
  //
  Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + sizeof (EFI_GUID);
  if (mUserInfo.AccessPolicyLen - OffSet < Size) {
    ExpandMemory (OffSet, Size);
  }

  Control.Type = EFI_USER_INFO_ACCESS_SETUP;
  Control.Size = (UINT32) Size;  
  CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
  OffSet += sizeof (Control);
  
  if (mAccessInfo.AccessSetup == ACCESS_SETUP_NORMAL) {
    CopyGuid ((EFI_GUID *) (mUserInfo.AccessPolicy + OffSet), &gEfiUserInfoAccessSetupNormalGuid);
  } else if (mAccessInfo.AccessSetup == ACCESS_SETUP_RESTRICTED) {
    CopyGuid ((EFI_GUID *) (mUserInfo.AccessPolicy + OffSet), &gEfiUserInfoAccessSetupRestrictedGuid);
  } else if (mAccessInfo.AccessSetup == ACCESS_SETUP_ADMIN) {
    CopyGuid ((EFI_GUID *) (mUserInfo.AccessPolicy + OffSet), &gEfiUserInfoAccessSetupAdminGuid);
  }
  OffSet += sizeof (EFI_GUID);
  
  //
  // Save access of boot order.
  //
  Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + sizeof (UINT32);
  if (mUserInfo.AccessPolicyLen - OffSet < Size) {
    ExpandMemory (OffSet, Size);
  }

  Control.Type = EFI_USER_INFO_ACCESS_BOOT_ORDER;
  Control.Size = (UINT32) Size;  
  CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
  OffSet += sizeof (Control);

  CopyMem ((UINT8 *) (mUserInfo.AccessPolicy + OffSet), &mAccessInfo.AccessBootOrder, sizeof (UINT32));
  OffSet += sizeof (UINT32);
  
  //
  // Save permit load.
  //
  if (mAccessInfo.LoadPermitLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.LoadPermitLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_PERMIT_LOAD;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
  
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.LoadPermit, mAccessInfo.LoadPermitLen);
    OffSet += mAccessInfo.LoadPermitLen;
  }
  
  //
  // Save forbid load.
  //
  if (mAccessInfo.LoadForbidLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.LoadForbidLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_FORBID_LOAD;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
    
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.LoadForbid, mAccessInfo.LoadForbidLen);
    OffSet += mAccessInfo.LoadForbidLen;
  }
  
  //
  // Save permit connect.
  //
  if (mAccessInfo.ConnectPermitLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.ConnectPermitLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_PERMIT_CONNECT;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
    
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.ConnectPermit, mAccessInfo.ConnectPermitLen);
    OffSet += mAccessInfo.ConnectPermitLen;
  }
  
  //
  // Save forbid connect.
  //
  if (mAccessInfo.ConnectForbidLen > 0) {
    Size = sizeof (EFI_USER_INFO_ACCESS_CONTROL) + mAccessInfo.ConnectForbidLen;
    if (mUserInfo.AccessPolicyLen - OffSet < Size) {
      ExpandMemory (OffSet, Size);
    }

    Control.Type = EFI_USER_INFO_ACCESS_FORBID_CONNECT;
    Control.Size = (UINT32) Size;  
    CopyMem (mUserInfo.AccessPolicy + OffSet, &Control, sizeof (Control));
    OffSet += sizeof (Control);
    
    CopyMem (mUserInfo.AccessPolicy + OffSet, mAccessInfo.ConnectForbid, mAccessInfo.ConnectForbidLen);
    OffSet += mAccessInfo.ConnectForbidLen;
  }

  mUserInfo.AccessPolicyLen = OffSet;

  //
  // Save access policy.
  //
  if (mUserInfo.AccessPolicyModified && (mUserInfo.AccessPolicyLen > 0)) {
    Info = AllocateZeroPool (sizeof (EFI_USER_INFO) + mUserInfo.AccessPolicyLen);
    if (Info == NULL) {
      return ;
    }

    Status = FindInfoByType (EFI_USER_INFO_ACCESS_POLICY_RECORD, &UserInfo);
    if (!EFI_ERROR (Status)) {
      Info->InfoType    = EFI_USER_INFO_ACCESS_POLICY_RECORD;
      Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV |
                          EFI_USER_INFO_PUBLIC |
                          EFI_USER_INFO_EXCLUSIVE;
      Info->InfoSize    = (UINT32) (sizeof (EFI_USER_INFO) + mUserInfo.AccessPolicyLen);
      CopyMem ((UINT8 *) (Info + 1), mUserInfo.AccessPolicy, mUserInfo.AccessPolicyLen);
      Status = mUserManager->SetInfo (
                               mUserManager,
                               mModifyUser,
                               &UserInfo,
                               Info,
                               Info->InfoSize
                               );
      mUserInfo.AccessPolicyModified = FALSE;
    }
    FreePool (Info);
  }

  if (mAccessInfo.ConnectForbid != NULL) {
    FreePool (mAccessInfo.ConnectForbid);
    mAccessInfo.ConnectForbid = NULL;
  }

  if (mAccessInfo.ConnectPermit != NULL) {
    FreePool (mAccessInfo.ConnectPermit);
    mAccessInfo.ConnectPermit = NULL;
  }

  if (mAccessInfo.LoadForbid != NULL) {
    FreePool (mAccessInfo.LoadForbid);
    mAccessInfo.LoadForbid = NULL;
  }

  if (mAccessInfo.LoadPermit != NULL) {
    FreePool (mAccessInfo.LoadPermit);
    mAccessInfo.LoadPermit = NULL;
  }
}


/**
  Get the username from user input, and update username string in the Hii 
  database with it.

**/
VOID
ModifyUserName (
  VOID
  )
{
  EFI_STATUS              Status;
  CHAR16                  UserName[USER_NAME_LENGTH];
  UINTN                   Len;
  EFI_INPUT_KEY           Key;
  EFI_USER_INFO_HANDLE    UserInfo;
  EFI_USER_INFO           *Info;
  EFI_USER_PROFILE_HANDLE TempUser;

  //
  // Get the new user name.
  //
  Len = sizeof (UserName);
  Status = GetUserNameInput (&Len, UserName);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_ABORTED) {
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Failed To Get User Name.",
        L"",
        L"Please Press Any Key to Continue ...",
        NULL
        );
    }
    return ;
  }
  
  //
  // Check whether the username had been used or not.
  //
  Info = AllocateZeroPool (sizeof (EFI_USER_INFO) + Len);
  if (Info == NULL) {
    return ;
  }

  Info->InfoType    = EFI_USER_INFO_NAME_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV |
                      EFI_USER_INFO_PUBLIC |
                      EFI_USER_INFO_EXCLUSIVE;
  Info->InfoSize    = (UINT32) (sizeof (EFI_USER_INFO) + Len);
  CopyMem ((UINT8 *) (Info + 1), UserName, Len);

  TempUser  = NULL;
  Status    = mUserManager->Find (
                              mUserManager,
                              &TempUser,
                              NULL,
                              Info,
                              Info->InfoSize
                              );
  if (!EFI_ERROR (Status)) {
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      L"The User Name Had Been Used.",
      L"",
      L"Please Use Other User Name",
      NULL
      );
    FreePool (Info);
    return ;
  }
  
  //
  // Update username display in the form.
  //
  CopyMem (mUserInfo.UserName, UserName, Len);
  HiiSetString (
    mCallbackInfo->HiiHandle, 
    STRING_TOKEN (STR_USER_NAME_VAL), 
    mUserInfo.UserName, 
    NULL
    );

  //
  // Save the user name.
  //
  Status = FindInfoByType (EFI_USER_INFO_NAME_RECORD, &UserInfo);
  if (!EFI_ERROR (Status)) {
    mUserManager->SetInfo (
                    mUserManager,
                    mModifyUser,
                    &UserInfo,
                    Info,
                    Info->InfoSize
                    );
  }
  FreePool (Info);
}


/**
  Display the form of the modifying user identity policy.

**/
VOID
ModifyIdentityPolicy (
  VOID
  )
{
  UINTN               Index;
  CHAR16              *ProvStr;
  EFI_STRING_ID       ProvID;
  EFI_HII_HANDLE      HiiHandle;
  VOID                *OptionsOpCodeHandle;
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;

  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABEL_IP_MOD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add credential providers
  //.
  if (mProviderInfo->Count > 0) {
    OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
    ASSERT (OptionsOpCodeHandle != NULL);

    //
    // Add credential provider Option OpCode.
    //
    for (Index = 0; Index < mProviderInfo->Count; Index++) {
      mProviderInfo->Provider[Index]->Title (
                                        mProviderInfo->Provider[Index],
                                        &HiiHandle,
                                        &ProvID
                                        );
      ProvStr = HiiGetString (HiiHandle, ProvID, NULL);
      ProvID  = HiiSetString (mCallbackInfo->HiiHandle, 0, ProvStr, NULL);
      FreePool (ProvStr);
      if (ProvID == 0) {
        return ;
      }

      HiiCreateOneOfOptionOpCode (
        OptionsOpCodeHandle,
        ProvID,
        0,
        EFI_IFR_NUMERIC_SIZE_1,
        (UINT8) Index
        );
    }

    HiiCreateOneOfOpCode (
      StartOpCodeHandle,                // Container for dynamic created opcodes
      KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_IP | KEY_MODIFY_PROV,  // Question ID
      0,                                // VarStore ID
      0,                                // Offset in Buffer Storage
      STRING_TOKEN (STR_PROVIDER),      // Question prompt text
      STRING_TOKEN (STR_PROVIDER_HELP), // Question help text
      EFI_IFR_FLAG_CALLBACK,            // Question flag
      EFI_IFR_NUMERIC_SIZE_1,           // Data type of Question Value
      OptionsOpCodeHandle,              // Option Opcode list
      NULL                              // Default Opcode is NULl
      );

    HiiFreeOpCodeHandle (OptionsOpCodeHandle);
  }
  
  //
  // Add logical connector Option OpCode.
  //
  OptionsOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (OptionsOpCodeHandle != NULL);

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_AND_CON),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    0
    );

  HiiCreateOneOfOptionOpCode (
    OptionsOpCodeHandle,
    STRING_TOKEN (STR_OR_CON),
    0,
    EFI_IFR_NUMERIC_SIZE_1,
    1
    );

  HiiCreateOneOfOpCode (
    StartOpCodeHandle,                  // Container for dynamic created opcodes
    KEY_MODIFY_USER | KEY_SELECT_USER | KEY_MODIFY_IP | KEY_MODIFY_CONN,  // Question ID
    0,                                  // VarStore ID
    0,                                  // Offset in Buffer Storage
    STRING_TOKEN (STR_CONNECTOR),       // Question prompt text
    STRING_TOKEN (STR_CONNECTOR_HELP),  // Question help text
    EFI_IFR_FLAG_CALLBACK,              // Question flag
    EFI_IFR_NUMERIC_SIZE_1,             // Data type of Question Value
    OptionsOpCodeHandle,                // Option Opcode list
    NULL                                // Default Opcode is NULl
    );

  HiiFreeOpCodeHandle (OptionsOpCodeHandle);

  //
  // Update identity policy in the form.
  //
  ResolveIdentityPolicy (
    mUserInfo.IdentityPolicy, 
    mUserInfo.IdentityPolicyLen, 
    STRING_TOKEN (STR_IDENTIFY_POLICY_VALUE)
    );

  if (mUserInfo.NewIdentityPolicy != NULL) {
    FreePool (mUserInfo.NewIdentityPolicy);
    mUserInfo.NewIdentityPolicy         = NULL;
    mUserInfo.NewIdentityPolicyLen      = 0;
    mUserInfo.NewIdentityPolicyModified = FALSE;
  }
  mProviderChoice = 0;
  mConncetLogical = 0;

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_MODIFY_IP,         // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Save the identity policy and update UI with it.
  
  This funciton will verify the new identity policy, in current implementation, 
  the identity policy can be:  T, P & P & P & ..., P | P | P | ...
  Here, "T" means "True", "P" means "Credential Provider", "&" means "and", "|" means "or".
  Other identity policies are not supported.  

**/
VOID
SaveIdentityPolicy (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *Info;
  EFI_INPUT_KEY                 Key;
  UINTN                         Offset;
  UINT32                        OpCode;
  UINTN                         InfoSize;

  if (!mUserInfo.NewIdentityPolicyModified || (mUserInfo.NewIdentityPolicyLen == 0)) {
    return;
  }
  
  //
  // Check policy expression.
  //
  OpCode  = EFI_USER_INFO_IDENTITY_FALSE;
  Offset  = 0;
  while (Offset < mUserInfo.NewIdentityPolicyLen) {
    //
    // Check access policy according to type
    //
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (mUserInfo.NewIdentityPolicy + Offset);
    switch (Identity->Type) {

    case EFI_USER_INFO_IDENTITY_TRUE:
      break;

    case EFI_USER_INFO_IDENTITY_OR:
      if (OpCode == EFI_USER_INFO_IDENTITY_AND) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid Identity Policy, Mixed Connector Unsupport!",
          L"",
          L"Press Any Key to Continue ...",
          NULL
          );
        return ;
      }

      OpCode = EFI_USER_INFO_IDENTITY_OR;
      break;

    case EFI_USER_INFO_IDENTITY_AND:
      if (OpCode == EFI_USER_INFO_IDENTITY_OR) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"Invalid Identity Policy, Mixed Connector Unsupport!",
          L"",
          L"Press Any Key to Continue ...",
          NULL
          );
        return ;
      }

      OpCode = EFI_USER_INFO_IDENTITY_AND;
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER:
      break;

    default:
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Unsupport parameter",
        L"",
        L"Press Any Key to Continue ...",
        NULL
        );
      return ;
    }
    Offset += Identity->Length;
  }

  //
  // Save identity policy.
  //
  Info = AllocateZeroPool (
           sizeof (EFI_USER_INFO) + 
           mUserInfo.NewIdentityPolicyLen
           );
  if (Info == NULL) {
    return ;
  }

  Status = FindInfoByType (EFI_USER_INFO_IDENTITY_POLICY_RECORD, &UserInfo);
  if (EFI_ERROR (Status)) {
    FreePool (Info);
    return ;
  }
  
  Info->InfoType    = EFI_USER_INFO_IDENTITY_POLICY_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV |
                      EFI_USER_INFO_PRIVATE |
                      EFI_USER_INFO_EXCLUSIVE;
  Info->InfoSize    = (UINT32) (sizeof (EFI_USER_INFO) + mUserInfo.NewIdentityPolicyLen);
  CopyMem ((UINT8 *) (Info + 1), mUserInfo.NewIdentityPolicy, mUserInfo.NewIdentityPolicyLen);
  Status = mUserManager->SetInfo (
                           mUserManager,
                           mModifyUser,
                           &UserInfo,
                           Info,
                           Info->InfoSize
                           );
  FreePool (Info);                           
  if (EFI_ERROR (Status)) {
    //
    // Get the user information again, it may be changed during saving it.
    //
    InfoSize = 0;
    Status = mUserManager->GetInfo (
                             mUserManager,
                             mModifyUser,
                             UserInfo,
                             Info,
                             &InfoSize
                             );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (InfoSize);
      ASSERT (Info != NULL);
      Status = mUserManager->GetInfo (
                               mUserManager,
                               mModifyUser,
                               UserInfo,
                               Info,
                               &InfoSize
                               );
    }
    ASSERT_EFI_ERROR (Status);
    
    //
    // Save current identification policy to mUserInfo.IdentityPolicy. 
    //
    ASSERT (Info != NULL);
    if (mUserInfo.IdentityPolicy != NULL) {
      FreePool (mUserInfo.IdentityPolicy);
    }

    mUserInfo.IdentityPolicyLen = Info->InfoSize - sizeof (EFI_USER_INFO);
    mUserInfo.IdentityPolicy    = AllocateCopyPool (mUserInfo.IdentityPolicyLen, Info + 1);    
    ASSERT (mUserInfo.IdentityPolicy != NULL);  

    //
    // Free the memory
    //
    FreePool (Info);
    FreePool (mUserInfo.NewIdentityPolicy);
  } else {  
    //
    // Update the mUserInfo.IdentityPolicy by mUserInfo.NewIdentityPolicy
    //
    if (mUserInfo.IdentityPolicy != NULL) {
      FreePool (mUserInfo.IdentityPolicy);
    }
    mUserInfo.IdentityPolicy    = mUserInfo.NewIdentityPolicy;
    mUserInfo.IdentityPolicyLen = mUserInfo.NewIdentityPolicyLen;
  }

  mUserInfo.NewIdentityPolicy         = NULL;
  mUserInfo.NewIdentityPolicyLen      = 0;
  mUserInfo.NewIdentityPolicyModified = FALSE;   

  //
  // Update identity policy choice.
  //
  ResolveIdentityPolicy (
    mUserInfo.IdentityPolicy, 
    mUserInfo.IdentityPolicyLen, 
    STRING_TOKEN (STR_IDENTIFY_POLICY_VAL)
    );
}


/**
  Verify the new identity policy in the current implementation. The same credential
  provider can't appear twice in one identity policy.

  @param[in] NewGuid       Points to the credential provider guid.
  
  @retval TRUE     The NewGuid was found in the identity policy.
  @retval FALSE    The NewGuid was not found.

**/
BOOLEAN
CheckIdentityPolicy (
  IN EFI_GUID                                      *NewGuid
  )
{
  UINTN                         Offset;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  EFI_INPUT_KEY                 Key;

  Offset = 0;
  while (Offset < mUserInfo.NewIdentityPolicyLen) {
    //
    // Check access policy according to type.
    //
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) (mUserInfo.NewIdentityPolicy + Offset);
    switch (Identity->Type) {

    case EFI_USER_INFO_IDENTITY_TRUE:
    case EFI_USER_INFO_IDENTITY_OR:
    case EFI_USER_INFO_IDENTITY_AND:
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER:
      if (CompareGuid (NewGuid, (EFI_GUID *) (Identity + 1))) {
        CreatePopUp (
          EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
          &Key,
          L"This Credential Provider Are Already Used!",
          L"",
          L"Press Any Key to Continue ...",
          NULL
          );
        return FALSE;
      }
      break;

    default:
      CreatePopUp (
        EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
        &Key,
        L"Unsupport parameter",
        L"",
        L"Press Any Key to Continue ...",
        NULL
        );
      return FALSE;
    }

    Offset += Identity->Length;
  }
  return TRUE;
}


/**
  Update the mUserInfo.NewIdentityPolicy, and UI when 'add option' is pressed.

**/
VOID
AddIdentityPolicyItem (
  VOID
  )
{
  UINT8                         *NewInfo;
  EFI_USER_INFO_IDENTITY_POLICY *Policy;

  if (mProviderInfo->Count == 0) {
    return ;
  }

  if (!mUserInfo.NewIdentityPolicyModified && (mUserInfo.NewIdentityPolicyLen > 0)) {
    FreePool (mUserInfo.NewIdentityPolicy);
    mUserInfo.NewIdentityPolicy     = NULL;
    mUserInfo.NewIdentityPolicyLen  = 0;
  }
  //
  // Expand the identity policy memory for the newly added policy info.
  //
  if (mUserInfo.NewIdentityPolicyLen > 0) {
    //
    // The new policy is not empty, expand space for connetor and provider.
    //
    if (!CheckIdentityPolicy (&mProviderInfo->Provider[mProviderChoice]->Identifier)) {
      return ;
    }
    NewInfo = AllocateZeroPool (
                mUserInfo.NewIdentityPolicyLen + 
                sizeof (EFI_USER_INFO_IDENTITY_POLICY) * 2 + 
                sizeof (EFI_GUID)
                );
  } else {
    //
    // The new policy is empty, only expand space for provider.
    //
    NewInfo = AllocateZeroPool (
                mUserInfo.NewIdentityPolicyLen + 
                sizeof (EFI_USER_INFO_IDENTITY_POLICY) + 
                sizeof (EFI_GUID)
                );
  }

  if (NewInfo == NULL) {
    return ;
  }

  if (mUserInfo.NewIdentityPolicyLen > 0) {
    CopyMem (NewInfo, mUserInfo.NewIdentityPolicy, mUserInfo.NewIdentityPolicyLen);
    FreePool (mUserInfo.NewIdentityPolicy);
  }
  mUserInfo.NewIdentityPolicy = NewInfo;

  //
  // Save logical connector.
  //
  if (mUserInfo.NewIdentityPolicyLen > 0) {
    Policy = (EFI_USER_INFO_IDENTITY_POLICY *) (mUserInfo.NewIdentityPolicy +
                                                mUserInfo.NewIdentityPolicyLen);
    if (mConncetLogical == 0) {
      Policy->Type = EFI_USER_INFO_IDENTITY_AND;
    } else {
      Policy->Type = EFI_USER_INFO_IDENTITY_OR;
    }

    Policy->Length = sizeof (EFI_USER_INFO_IDENTITY_POLICY);
    mUserInfo.NewIdentityPolicyLen += Policy->Length;
  }
  
  //
  // Save credential provider.
  //
  Policy = (EFI_USER_INFO_IDENTITY_POLICY *) (mUserInfo.NewIdentityPolicy + 
                                              mUserInfo.NewIdentityPolicyLen);
  Policy->Length = sizeof (EFI_USER_INFO_IDENTITY_POLICY) + sizeof (EFI_GUID);
  Policy->Type   = EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER;
  CopyGuid ((EFI_GUID *) (Policy + 1), &mProviderInfo->Provider[mProviderChoice]->Identifier);
  mUserInfo.NewIdentityPolicyLen += Policy->Length;

  //
  // Update identity policy choice.
  //
  mUserInfo.NewIdentityPolicyModified = TRUE;
  ResolveIdentityPolicy (
    mUserInfo.NewIdentityPolicy, 
    mUserInfo.NewIdentityPolicyLen, 
    STRING_TOKEN (STR_IDENTIFY_POLICY_VALUE)
    );
}


/**
  Create an action OpCode with QuestionID and DevicePath on a given OpCodeHandle.

  @param[in]  QuestionID            The question ID.
  @param[in]  DevicePath            Points to device path.
  @param[in]  OpCodeHandle          Points to container for dynamic created opcodes.

**/
VOID
AddDevicePath (
  IN  UINTN                                     QuestionID,
  IN  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath,
  IN     VOID                                   *OpCodeHandle
  )
{
  EFI_STATUS                        Status;
  EFI_DEVICE_PATH_PROTOCOL          *Next;
  EFI_STRING_ID                     NameID;
  EFI_STRING                        DriverName;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL  *DevicePathText;

  //
  // Locate device path to text protocol.
  //
  Status = gBS->LocateProtocol (
                  &gEfiDevicePathToTextProtocolGuid,
                  NULL,
                  (VOID **) &DevicePathText
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Get driver file name node.
  //
  Next = DevicePath;
  while (!IsDevicePathEnd (Next)) {
    DevicePath  = Next;
    Next        = NextDevicePathNode (Next);
  }

  //
  // Display the device path in form.
  //
  DriverName = DevicePathText->ConvertDevicePathToText (DevicePath, FALSE, FALSE);
  NameID = HiiSetString (mCallbackInfo->HiiHandle, 0, DriverName, NULL);
  FreePool (DriverName);
  if (NameID == 0) {
    return ;
  }

  HiiCreateActionOpCode (
    OpCodeHandle,                   // Container for dynamic created opcodes
    (UINT16) QuestionID,            // Question ID
    NameID,                         // Prompt text
    STRING_TOKEN (STR_NULL_STRING), // Help text
    EFI_IFR_FLAG_CALLBACK,          // Question flag
    0                               // Action String ID
    );
}


/**
  Check whether the DevicePath is in the device path forbid list 
  (mAccessInfo.LoadForbid).

  @param[in]  DevicePath           Points to device path.
  
  @retval TRUE     The DevicePath is in the device path forbid list.
  @retval FALSE    The DevicePath is not in the device path forbid list.

**/
BOOLEAN
IsLoadForbidden (
  IN  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath
  )
{
  UINTN                     OffSet;
  UINTN                     DPSize;
  UINTN                     Size;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;

  OffSet = 0;
  Size   = GetDevicePathSize (DevicePath);
  //
  // Check each device path.
  //
  while (OffSet < mAccessInfo.LoadForbidLen) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + OffSet);
    DPSize  = GetDevicePathSize (Dp);
    //
    // Compare device path.
    //
    if ((DPSize == Size) && (CompareMem (DevicePath, Dp, Size) == 0)) {
      return TRUE;
    }
    OffSet += DPSize;
  }
  return FALSE;
}


/**
  Display the permit load device path in the loadable device path list.

**/
VOID
DisplayLoadPermit(
  VOID
  )
{
  EFI_STATUS          Status;
  CHAR16              *Order;
  UINTN               OrderSize;
  UINTN               ListCount;
  UINTN               Index;
  UINT8               *Var;
  UINT8               *VarPtr;
  CHAR16              VarName[12];
  VOID                *StartOpCodeHandle;
  VOID                *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL  *StartLabel;
  EFI_IFR_GUID_LABEL  *EndLabel;

  //
  // Get DriverOrder.
  //
  OrderSize = 0;
  Status    = gRT->GetVariable (
                     L"DriverOrder", 
                     &gEfiGlobalVariableGuid, 
                     NULL, 
                     &OrderSize, 
                     NULL
                     );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return ;
  }

  Order = AllocateZeroPool (OrderSize);
  if (Order == NULL) {
    return ;
  }

  Status = gRT->GetVariable (
                  L"DriverOrder", 
                  &gEfiGlobalVariableGuid, 
                  NULL, 
                  &OrderSize, 
                  Order
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  
  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABEL_PERMIT_LOAD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add each driver option.
  //
  Var       = NULL;
  ListCount = OrderSize / sizeof (UINT16);
  for (Index = 0; Index < ListCount; Index++) {
    //
    // Get driver device path.
    //
    UnicodeSPrint (VarName, sizeof (VarName), L"Driver%04x", Order[Index]);
    Var = GetEfiGlobalVariable (VarName);
    if (Var == NULL) {
      continue;
    }
    
    //
    // Check whether the driver is already forbidden.
    //
    
    VarPtr = Var;
    //
    // Skip attribute.
    //
    VarPtr += sizeof (UINT32);

    //
    // Skip device path lenth.
    //
    VarPtr += sizeof (UINT16);

    //
    // Skip descript string.
    //
    VarPtr += StrSize ((UINT16 *) VarPtr);

    if (IsLoadForbidden ((EFI_DEVICE_PATH_PROTOCOL *) VarPtr)) {
      FreePool (Var);
      Var = NULL;
      continue;
    }

    AddDevicePath (
      KEY_MODIFY_USER | KEY_MODIFY_AP_DP | KEY_LOAD_PERMIT_MODIFY | Order[Index],
      (EFI_DEVICE_PATH_PROTOCOL *) VarPtr,
      StartOpCodeHandle
      );
    FreePool (Var);
    Var = NULL;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_PERMIT_LOAD_DP,    // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  //
  // Clear Environment.
  //
  if (Var != NULL) {
    FreePool (Var);
  }
  FreePool (Order);
}


/**
  Display the forbid load device path list (mAccessInfo.LoadForbid).

**/
VOID
DisplayLoadForbid (
  VOID
  )
{
  UINTN                     Offset;
  UINTN                     DPSize;
  UINTN                     Index;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;
  VOID                      *StartOpCodeHandle;
  VOID                      *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL        *StartLabel;
  EFI_IFR_GUID_LABEL        *EndLabel;

  //
  // Initialize the container for dynamic opcodes.
  //
  StartOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (StartOpCodeHandle != NULL);

  EndOpCodeHandle = HiiAllocateOpCodeHandle ();
  ASSERT (EndOpCodeHandle != NULL);

  //
  // Create Hii Extend Label OpCode.
  //
  StartLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                        StartOpCodeHandle,
                                        &gEfiIfrTianoGuid,
                                        NULL,
                                        sizeof (EFI_IFR_GUID_LABEL)
                                        );
  StartLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  StartLabel->Number        = LABLE_FORBID_LOAD_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add each forbid load drivers.
  //
  Offset  = 0;
  Index   = 0;
  while (Offset < mAccessInfo.LoadForbidLen) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + Offset);
    DPSize  = GetDevicePathSize (Dp);
    AddDevicePath (
      KEY_MODIFY_USER | KEY_MODIFY_AP_DP | KEY_LOAD_FORBID_MODIFY | Index,
      Dp,
      StartOpCodeHandle
      );
    Index++;
    Offset += DPSize;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_FORBID_LOAD_DP,    // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}


/**
  Display the permit connect device path.

**/
VOID
DisplayConnectPermit (
  VOID
  )
{
  //
  // Note: 
  // As no architect protocol/interface to be called in ConnectController()
  // to verify the device path, just add a place holder for permitted connect
  // device path.
  //
}


/**
  Display the forbid connect device path list.

**/
VOID
DisplayConnectForbid (
  VOID
  )
{
  //
  // Note: 
  // As no architect protocol/interface to be called in ConnectController()
  // to verify the device path, just add a place holder for forbidden connect
  // device path.
  //
}


/**
  Delete the specified device path by DriverIndex from the forbid device path 
  list (mAccessInfo.LoadForbid).

  @param[in]  DriverIndex   The index of driver in forbidden device path list.
  
**/
VOID
DeleteFromForbidLoad (
  IN  UINT16                                    DriverIndex
  )
{
  UINTN                     OffSet;
  UINTN                     DPSize;
  UINTN                     OffLen;
  EFI_DEVICE_PATH_PROTOCOL  *Dp;

  OffSet = 0;
  //
  // Find the specified device path.
  //
  while ((OffSet < mAccessInfo.LoadForbidLen) && (DriverIndex > 0)) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + OffSet);
    DPSize  = GetDevicePathSize (Dp);
    OffSet += DPSize;
    DriverIndex--;
  }
  
  //
  // Specified device path found.
  //
  if (DriverIndex == 0) {
    Dp      = (EFI_DEVICE_PATH_PROTOCOL *) (mAccessInfo.LoadForbid + OffSet);
    DPSize  = GetDevicePathSize (Dp);
    OffLen  = mAccessInfo.LoadForbidLen - OffSet - DPSize;
    if (OffLen > 0) {
      CopyMem (
        mAccessInfo.LoadForbid + OffSet, 
        mAccessInfo.LoadForbid + OffSet + DPSize, 
        OffLen
        );
    }
    mAccessInfo.LoadForbidLen -= DPSize;
  }
}


/**
  Add the specified device path by DriverIndex to the forbid device path 
  list (mAccessInfo.LoadForbid).

  @param[in]  DriverIndex   The index of driver saved in driver options.
  
**/
VOID
AddToForbidLoad (
  IN  UINT16                                    DriverIndex
  )
{
  UINTN       DevicePathLen;
  UINT8       *Var;
  UINT8       *VarPtr;
  UINTN       NewLen;
  UINT8       *NewFL;
  CHAR16      VarName[13];

  //
  // Get loadable driver device path.
  //
  UnicodeSPrint  (VarName, sizeof (VarName), L"Driver%04x", DriverIndex);
  Var = GetEfiGlobalVariable (VarName);
  if (Var == NULL) {
    return;
  }
  
  //
  // Save forbid load driver.
  //
  
  VarPtr = Var;
  //
  // Skip attribute.
  //
  VarPtr += sizeof (UINT32);

  DevicePathLen = *(UINT16 *) VarPtr;
  //
  // Skip device path length.
  //
  VarPtr += sizeof (UINT16);

  //
  // Skip description string.
  //
  VarPtr += StrSize ((UINT16 *) VarPtr);

  NewLen  = mAccessInfo.LoadForbidLen + DevicePathLen;
  NewFL   = AllocateZeroPool (NewLen);
  if (NewFL == NULL) {
    FreePool (Var);
    return ;
  }

  if (mAccessInfo.LoadForbidLen > 0) {
    CopyMem (NewFL, mAccessInfo.LoadForbid, mAccessInfo.LoadForbidLen);
    FreePool (mAccessInfo.LoadForbid);
  }

  CopyMem (NewFL + mAccessInfo.LoadForbidLen, VarPtr, DevicePathLen);
  mAccessInfo.LoadForbidLen = NewLen;
  mAccessInfo.LoadForbid    = NewFL;
  FreePool (Var);
}


/**
  Get current user's access right.

  @param[out]  AccessRight  Points to the buffer used for user's access right.

  @retval EFI_SUCCESS       Get current user access right successfully.
  @retval others            Fail to get current user access right.

**/
EFI_STATUS
GetAccessRight (
  OUT  UINT32                                    *AccessRight
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *Info;
  UINTN                         InfoSize;
  UINTN                         MemSize;
  EFI_USER_INFO_ACCESS_CONTROL  Access;
  EFI_USER_PROFILE_HANDLE       CurrentUser;
  UINTN                         TotalLen;
  UINTN                         CheckLen;

  //
  // Allocate user information memory.
  //
  MemSize = sizeof (EFI_USER_INFO) + 63;
  Info    = AllocateZeroPool (MemSize);
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
 
  //
  // Get user access information.
  //
  UserInfo = NULL;
  mUserManager->Current (mUserManager, &CurrentUser);
  while (TRUE) {
    InfoSize = MemSize;
    //
    // Get next user information.
    //
    Status = mUserManager->GetNextInfo (mUserManager, CurrentUser, &UserInfo);
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = mUserManager->GetInfo (
                             mUserManager,
                             CurrentUser,
                             UserInfo,
                             Info,
                             &InfoSize
                             );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MemSize = InfoSize;
      FreePool (Info);
      Info = AllocateZeroPool (MemSize);
      if (Info == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      Status = mUserManager->GetInfo (
                               mUserManager,
                               CurrentUser,
                               UserInfo,
                               Info,
                               &InfoSize
                               );
    }
    if (EFI_ERROR (Status)) {
      break;
    }
    
    //
    // Check user information.
    //
    if (Info->InfoType == EFI_USER_INFO_ACCESS_POLICY_RECORD) {
      TotalLen  = Info->InfoSize - sizeof (EFI_USER_INFO);
      CheckLen  = 0;
      //
      // Get specified access information.
      //
      while (CheckLen < TotalLen) {
        CopyMem (&Access, (UINT8 *) (Info + 1) + CheckLen, sizeof (Access));
        if ((Access.Type == EFI_USER_INFO_ACCESS_ENROLL_SELF) ||
            (Access.Type == EFI_USER_INFO_ACCESS_ENROLL_OTHERS) ||
            (Access.Type == EFI_USER_INFO_ACCESS_MANAGE)
            ) {
          *AccessRight = Access.Type;
          FreePool (Info);
          return EFI_SUCCESS;
        }
        CheckLen += Access.Size;
      }
    }
  }
  FreePool (Info);
  return EFI_NOT_FOUND;
}



