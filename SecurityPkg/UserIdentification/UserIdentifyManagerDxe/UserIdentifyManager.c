/** @file
  This driver manages user information and produces user manager protocol.
  
Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserIdentifyManager.h"

//
// Default user name.
//
CHAR16                      mUserName[]       = L"Administrator";

//
// Points to the user profile database.
//
USER_PROFILE_DB             *mUserProfileDb   = NULL;

//
// Points to the credential providers found in system.
//
CREDENTIAL_PROVIDER_INFO    *mProviderDb      = NULL;

//
// Current user shared in multi function.
//
EFI_USER_PROFILE_HANDLE     mCurrentUser      = NULL;

//
// Flag indicates a user is identified.
//
BOOLEAN                     mIdentified       = FALSE;
USER_MANAGER_CALLBACK_INFO  *mCallbackInfo    = NULL;
HII_VENDOR_DEVICE_PATH      mHiiVendorDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    USER_IDENTIFY_MANAGER_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};


EFI_USER_MANAGER_PROTOCOL gUserIdentifyManager = {
  UserProfileCreate,
  UserProfileDelete,
  UserProfileGetNext,
  UserProfileCurrent,
  UserProfileIdentify,
  UserProfileFind,
  UserProfileNotify,
  UserProfileGetInfo,
  UserProfileSetInfo,
  UserProfileDeleteInfo,
  UserProfileGetNextInfo,
};


/**
  Find the specified user in the user database.

  This function searches the specified user from the beginning of the user database. 
  And if NextUser is TRUE, return the next User in the user database.  
  
  @param[in, out] User         On entry, points to the user profile entry to search. 
                               On return, points to the user profile entry or NULL if not found.
  @param[in]      NextUser     If FALSE, find the user in user profile database specifyed by User
                               If TRUE, find the next user in user profile database specifyed 
                               by User. 
  @param[out]     ProfileIndex A pointer to the index of user profile database that matches the 
                               user specifyed by User.

  @retval EFI_NOT_FOUND        User was NULL, or User was not found, or the next user was not found.
  @retval EFI_SUCCESS          User or the next user are found in user profile database
  
**/
EFI_STATUS
FindUserProfile (
  IN OUT  USER_PROFILE_ENTRY                    **User,
  IN      BOOLEAN                               NextUser,
     OUT  UINTN                                 *ProfileIndex OPTIONAL
  )
{
  UINTN               Index;

  //
  // Check parameters
  //
  if ((mUserProfileDb == NULL) || (User == NULL)) {
    return EFI_NOT_FOUND;
  }
  
  //
  // Check whether the user profile is in the user profile database.
  //
  for (Index = 0; Index < mUserProfileDb->UserProfileNum; Index++) {
    if (mUserProfileDb->UserProfile[Index] == *User) {
      if (ProfileIndex != NULL) {
        *ProfileIndex = Index;
      }
      break;
    }
  }

  if (NextUser) {
    //
    // Find the next user profile.
    //
    Index++;
    if (Index < mUserProfileDb->UserProfileNum) {
      *User = mUserProfileDb->UserProfile[Index];
    } else if (Index == mUserProfileDb->UserProfileNum) {
      *User = NULL;
      return EFI_NOT_FOUND;
    } else {
      if ((mUserProfileDb->UserProfileNum > 0) && (*User == NULL)) {
        *User = mUserProfileDb->UserProfile[0];
      } else {
        *User = NULL;
        return EFI_NOT_FOUND;
      }
    }
  } else if (Index == mUserProfileDb->UserProfileNum) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Find the specified user information record in the specified User profile.

  This function searches the specified user information record from the beginning of the user 
  profile. And if NextInfo is TRUE, return the next info in the user profile.  
  
  @param[in]      User     Points to the user profile entry.                             
  @param[in, out] Info     On entry, points to the user information record or NULL to start
                           searching with the first user information record.
                           On return, points to the user information record or NULL if not found.                           
  @param[in]      NextInfo If FALSE, find the user information record in profile specifyed by User.
                           If TRUE, find the next user information record in profile specifyed 
                           by User. 
  @param[out]     Offset   A pointer to the offset of the information record in the user profile.

  @retval EFI_INVALID_PARAMETER Info is NULL
  @retval EFI_NOT_FOUND         Info was not found, or the next Info was not found.
  @retval EFI_SUCCESS           Info or the next info are found in user profile.
  
**/
EFI_STATUS
FindUserInfo (
  IN     USER_PROFILE_ENTRY                    * User,
  IN OUT EFI_USER_INFO                         **Info,
  IN     BOOLEAN                               NextInfo,
     OUT UINTN                                 *Offset OPTIONAL
  )
{
  EFI_STATUS    Status;
  EFI_USER_INFO *UserInfo;
  UINTN         InfoLen;

  if (Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check user profile entry
  //
  Status = FindUserProfile (&User, FALSE, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Find user information in the specified user record.
  //
  InfoLen = 0;
  while (InfoLen < User->UserProfileSize) {
    UserInfo = (EFI_USER_INFO *) (User->ProfileInfo + InfoLen);
    if (UserInfo == *Info) {
      if (Offset != NULL) {
        *Offset = InfoLen;
      }
      break;
    }
    InfoLen += ALIGN_VARIABLE (UserInfo->InfoSize);
  }
  
  //
  // Check whether to find the next user information.
  //
  if (NextInfo) {
    if (InfoLen < User->UserProfileSize) {
      UserInfo = (EFI_USER_INFO *) (User->ProfileInfo + InfoLen);
      InfoLen += ALIGN_VARIABLE (UserInfo->InfoSize);
      if (InfoLen < User->UserProfileSize) {
        *Info = (EFI_USER_INFO *) (User->ProfileInfo + InfoLen);
        if (Offset != NULL) {
          *Offset = InfoLen;
        }
      } else if (InfoLen == User->UserProfileSize) {
        *Info = NULL;
        return EFI_NOT_FOUND;
      }
    } else {
      if (*Info == NULL) {
        *Info = (EFI_USER_INFO *) User->ProfileInfo;
        if (Offset != NULL) {
          *Offset = 0;
        }
      } else {
        *Info = NULL;
        return EFI_NOT_FOUND;
      }
    }
  } else if (InfoLen == User->UserProfileSize) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  Find a user infomation record by the information record type.

  This function searches all user information records of User. The search starts with the 
  user information record following Info and continues until either the information is found 
  or there are no more user infomation record.
  A match occurs when a Info.InfoType field matches the user information record type.

  @param[in]      User     Points to the user profile record to search.                          
  @param[in, out] Info     On entry, points to the user information record or NULL to start
                           searching with the first user information record.
                           On return, points to the user information record or NULL if not found.
  @param[in]      InfoType The infomation type to be searched.

  @retval EFI_SUCCESS           User information was found. Info points to the user information record.
  @retval EFI_NOT_FOUND         User information was not found.                           
  @retval EFI_INVALID_PARAMETER User is NULL or Info is NULL.
  
**/
EFI_STATUS
FindUserInfoByType (
  IN      USER_PROFILE_ENTRY                    *User,
  IN OUT  EFI_USER_INFO                         **Info,
  IN      UINT8                                 InfoType
  )
{
  EFI_STATUS    Status;
  EFI_USER_INFO *UserInfo;
  UINTN         InfoLen;

  if (Info == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check whether the user has the specified user information.
  //
  InfoLen = 0;
  if (*Info == NULL) {
    Status = FindUserProfile (&User, FALSE, NULL);
  } else {
    Status = FindUserInfo (User, Info, TRUE, &InfoLen);
  }

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }
  
  while (InfoLen < User->UserProfileSize) {
    UserInfo = (EFI_USER_INFO *) (User->ProfileInfo + InfoLen);
    if (UserInfo->InfoType == InfoType) {
      if (UserInfo != *Info) {
        *Info = UserInfo;
        return EFI_SUCCESS;
      }
    }

    InfoLen += ALIGN_VARIABLE (UserInfo->InfoSize);
  }

  *Info = NULL;
  return EFI_NOT_FOUND;
}

/**
  Find a user using a user information record.

  This function searches all user profiles for the specified user information record. The 
  search starts with the user information record handle following UserInfo and continues 
  until either the information is found or there are no more user profiles.
  A match occurs when the Info.InfoType field matches the user information record type and the 
  user information record data matches the portion of Info passed the EFI_USER_INFO header.

  @param[in, out] User     On entry, points to the previously returned user profile record, 
                           or NULL to start searching with the first user profile. 
                           On return, points to the user profile entry, or NULL if not found.
  @param[in, out] UserInfo On entry, points to the previously returned user information record, 
                           or NULL to start searching with the first. 
                           On return, points to the user information record, or NULL if not found.
  @param[in]      Info     Points to the buffer containing the user information to be compared 
                           to the user information record.
  @param[in]      InfoSize The size of Info, in bytes. Same as Info->InfoSize.

  @retval EFI_SUCCESS           User information was found. User points to the user profile record, 
                                and UserInfo points to the user information record.
  @retval EFI_NOT_FOUND         User information was not found.                           
  @retval EFI_INVALID_PARAMETER User is NULL; Info is NULL; or, InfoSize is too small.
  
**/
EFI_STATUS
FindUserProfileByInfo (
  IN OUT  USER_PROFILE_ENTRY                    **User,
  IN OUT  EFI_USER_INFO                         **UserInfo, OPTIONAL
  IN      EFI_USER_INFO                         *Info,
  IN      UINTN                                 InfoSize
  )
{
  EFI_STATUS    Status;
  EFI_USER_INFO *InfoEntry;


  if ((User == NULL) || (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (InfoSize < sizeof (EFI_USER_INFO)) {
    return EFI_INVALID_PARAMETER;
  }

  if (UserInfo != NULL) {
    InfoEntry = *UserInfo;
  } else {
    InfoEntry = NULL;
  }
  //
  // Find user profile according to information.
  //
  if (*User == NULL) {
    *User = mUserProfileDb->UserProfile[0];
  }
  
  //
  // Check user profile handle.
  //
  Status = FindUserProfile (User, FALSE, NULL);

  while (!EFI_ERROR (Status)) {
    //
    // Find the user information in a user profile.
    //
    while (TRUE) {
      Status = FindUserInfoByType (*User, &InfoEntry, Info->InfoType);
      if (EFI_ERROR (Status)) {
        break;
      }
      
      if (InfoSize == Info->InfoSize) {
        if (CompareMem ((UINT8 *) (InfoEntry + 1), (UINT8 *) (Info + 1), InfoSize - sizeof (EFI_USER_INFO)) == 0) {
          //
          // Found the infomation record.
          //
          if (UserInfo != NULL) {
            *UserInfo = InfoEntry;
          }
          return EFI_SUCCESS;
        }
      }      
    }
    
    //
    // Get next user profile.
    //
    InfoEntry = NULL;
    Status    = FindUserProfile (User, TRUE, NULL);
  }

  return EFI_NOT_FOUND;
}


/**
  Check whether the access policy is valid.

  @param[in]  PolicyInfo          Point to the access policy.
  @param[in]  InfoLen             The policy length.

  @retval TRUE     The policy is a valid access policy.
  @retval FALSE    The access policy is not a valid access policy.
  
**/
BOOLEAN
CheckAccessPolicy (
  IN  UINT8                                     *PolicyInfo,
  IN  UINTN                                     InfoLen
  )
{
  UINTN                         TotalLen;
  UINTN                         ValueLen;
  UINTN                         OffSet;
  EFI_USER_INFO_ACCESS_CONTROL  Access;
  EFI_DEVICE_PATH_PROTOCOL      *Path;
  UINTN                         PathSize;

  TotalLen = 0;
  while (TotalLen < InfoLen) {
    //
    // Check access policy according to type.
    //
    CopyMem (&Access, PolicyInfo + TotalLen, sizeof (Access));    
    ValueLen = Access.Size - sizeof (EFI_USER_INFO_ACCESS_CONTROL);
    switch (Access.Type) {
    case EFI_USER_INFO_ACCESS_FORBID_LOAD:
    case EFI_USER_INFO_ACCESS_PERMIT_LOAD:
    case EFI_USER_INFO_ACCESS_FORBID_CONNECT:
    case EFI_USER_INFO_ACCESS_PERMIT_CONNECT:
      OffSet = 0;
      while (OffSet < ValueLen) {
        Path      = (EFI_DEVICE_PATH_PROTOCOL *) (PolicyInfo + TotalLen + sizeof (Access) + OffSet);
        PathSize  = GetDevicePathSize (Path);
        OffSet += PathSize;
      }
      if (OffSet != ValueLen) {
        return FALSE;
      }
      break;

    case EFI_USER_INFO_ACCESS_SETUP:
      if (ValueLen % sizeof (EFI_GUID) != 0) {
        return FALSE;
      }
      break;

    case EFI_USER_INFO_ACCESS_BOOT_ORDER:
      if (ValueLen % sizeof (EFI_USER_INFO_ACCESS_BOOT_ORDER_HDR) != 0) {
        return FALSE;
      }
      break;

    case EFI_USER_INFO_ACCESS_ENROLL_SELF:
    case EFI_USER_INFO_ACCESS_ENROLL_OTHERS:
    case EFI_USER_INFO_ACCESS_MANAGE:
      if (ValueLen != 0) {
        return FALSE;
      }
      break;

    default:
      return FALSE;
      break;
    }

    TotalLen += Access.Size;
  }

  if (TotalLen != InfoLen) {
    return FALSE;
  }

  return TRUE;
}


/**
  Check whether the identity policy is valid.

  @param[in]  PolicyInfo          Point to the identity policy.
  @param[in]  InfoLen             The policy length.

  @retval TRUE     The policy is a valid identity policy.
  @retval FALSE    The access policy is not a valid identity policy.
  
**/
BOOLEAN
CheckIdentityPolicy (
  IN  UINT8                                     *PolicyInfo,
  IN  UINTN                                     InfoLen
  )
{
  UINTN                         TotalLen;
  UINTN                         ValueLen;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;

  TotalLen  = 0;

  //
  // Check each part of policy expression.
  //
  while (TotalLen < InfoLen) {
    //
    // Check access polisy according to type.
    //
    Identity  = (EFI_USER_INFO_IDENTITY_POLICY *) (PolicyInfo + TotalLen);
    ValueLen  = Identity->Length - sizeof (EFI_USER_INFO_IDENTITY_POLICY);
    switch (Identity->Type) {
    //
    // Check False option.
    //
    case EFI_USER_INFO_IDENTITY_FALSE:
      if (ValueLen != 0) {
        return FALSE;
      }
      break;

    //
    // Check True option.
    //
    case EFI_USER_INFO_IDENTITY_TRUE:
      if (ValueLen != 0) {
        return FALSE;
      }
      break;

    //
    // Check negative operation.
    //
    case EFI_USER_INFO_IDENTITY_NOT:
      if (ValueLen != 0) {
        return FALSE;
      }
      break;

    //
    // Check and operation.
    //
    case EFI_USER_INFO_IDENTITY_AND:
      if (ValueLen != 0) {
        return FALSE;
      }
      break;

    //
    // Check or operation.
    //
    case EFI_USER_INFO_IDENTITY_OR:
      if (ValueLen != 0) {
        return FALSE;
      }
      break;

    //
    // Check credential provider by type.
    //
    case EFI_USER_INFO_IDENTITY_CREDENTIAL_TYPE:
      if (ValueLen != sizeof (EFI_GUID)) {
        return FALSE;
      }
      break;

    //
    // Check credential provider by ID.
    //
    case EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER:
      if (ValueLen != sizeof (EFI_GUID)) {
        return FALSE;
      }
      break;

    default:
      return FALSE;
      break;
    }

    TotalLen += Identity->Length;
  }

  if (TotalLen != InfoLen) {
    return FALSE;
  }

  return TRUE;
}


/**
  Check whether the user information is a valid user information record.

  @param[in]  Info points to the user information.

  @retval TRUE     The info is a valid user information record.
  @retval FALSE    The info is not a valid user information record.
  
**/
BOOLEAN
CheckUserInfo (
  IN CONST  EFI_USER_INFO                       *Info
  )
{
  UINTN       InfoLen;

  if (Info == NULL) {
    return FALSE;
  }
  //
  // Check user information according to information type.
  //
  InfoLen = Info->InfoSize - sizeof (EFI_USER_INFO);
  switch (Info->InfoType) {
  case EFI_USER_INFO_EMPTY_RECORD:
    if (InfoLen != 0) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_NAME_RECORD:
  case EFI_USER_INFO_CREDENTIAL_TYPE_NAME_RECORD:
  case EFI_USER_INFO_CREDENTIAL_PROVIDER_NAME_RECORD:
    break;

  case EFI_USER_INFO_CREATE_DATE_RECORD:
  case EFI_USER_INFO_USAGE_DATE_RECORD:
    if (InfoLen != sizeof (EFI_TIME)) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_USAGE_COUNT_RECORD:
    if (InfoLen != sizeof (UINT64)) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_IDENTIFIER_RECORD:
    if (InfoLen != 16) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_CREDENTIAL_TYPE_RECORD:
  case EFI_USER_INFO_CREDENTIAL_PROVIDER_RECORD:
  case EFI_USER_INFO_GUID_RECORD:
    if (InfoLen != sizeof (EFI_GUID)) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_PKCS11_RECORD:
  case EFI_USER_INFO_CBEFF_RECORD:
    break;

  case EFI_USER_INFO_FAR_RECORD:
  case EFI_USER_INFO_RETRY_RECORD:
    if (InfoLen != 1) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_ACCESS_POLICY_RECORD:
    if(!CheckAccessPolicy ((UINT8 *) (Info + 1), InfoLen)) {
      return FALSE;
    }
    break;

  case EFI_USER_INFO_IDENTITY_POLICY_RECORD:
    if (!CheckIdentityPolicy ((UINT8 *) (Info + 1), InfoLen)) {
      return FALSE;
    }
    break;

  default:
    return FALSE;
    break;
  }

  return TRUE;
}


/**
  Check the user profile data format to be added.

  @param[in]  UserProfileInfo     Points to the user profile data.
  @param[in]  UserProfileSize     The length of user profile data.

  @retval TRUE     It is a valid user profile.
  @retval FALSE    It is not a valid user profile.
  
**/
BOOLEAN
CheckProfileInfo (
  IN  UINT8                                     *UserProfileInfo,
  IN  UINTN                                     UserProfileSize
  )
{
  UINTN         ChkLen;
  EFI_USER_INFO *Info;

  if (UserProfileInfo == NULL) {
    return FALSE;
  }
  
  //
  // Check user profile information length.
  //
  ChkLen = 0;
  while (ChkLen < UserProfileSize) {
    Info = (EFI_USER_INFO *) (UserProfileInfo + ChkLen);
    //
    // Check user information format.
    //
    if (!CheckUserInfo (Info)) {
      return FALSE;
    }

    ChkLen += ALIGN_VARIABLE (Info->InfoSize);
  }

  if (ChkLen != UserProfileSize) {
    return FALSE;
  }

  return TRUE;
}


/**
  Find the specified RightType in current user profile.

  @param[in]  RightType      Could be EFI_USER_INFO_ACCESS_MANAGE,
                             EFI_USER_INFO_ACCESS_ENROLL_OTHERS or
                             EFI_USER_INFO_ACCESS_ENROLL_SELF.
  
  @retval TRUE     Find the specified RightType in current user profile.
  @retval FALSE    Can't find the right in the profile.
  
**/
BOOLEAN
CheckCurrentUserAccessRight (
  IN        UINT32                              RightType
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO                 *Info;
  UINTN                         TotalLen;
  UINTN                         CheckLen;
  EFI_USER_INFO_ACCESS_CONTROL  Access;

  //
  // Get user access right information.
  //
  Info = NULL;
  Status = FindUserInfoByType (
            (USER_PROFILE_ENTRY *) mCurrentUser,
            &Info,
            EFI_USER_INFO_ACCESS_POLICY_RECORD
            );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  ASSERT (Info != NULL);
  TotalLen  = Info->InfoSize - sizeof (EFI_USER_INFO);
  CheckLen  = 0;
  while (CheckLen < TotalLen) {
    //
    // Check right according to access type.
    //
    CopyMem (&Access, (UINT8 *) (Info + 1) + CheckLen, sizeof (Access));
    if (Access.Type == RightType) {
      return TRUE;;
    }

    CheckLen += Access.Size;
  }

  return FALSE;
}


/**
  Create a unique user identifier.

  @param[out]  Identifier     This points to the identifier.

**/
VOID
GenerateIdentifier (
   OUT    UINT8                               *Identifier
  )
{
  EFI_TIME  Time;
  UINT64    MonotonicCount;
  UINT32    *MonotonicPointer;
  UINTN     Index;

  //
  // Create a unique user identifier.
  //
  gRT->GetTime (&Time, NULL);
  CopyMem (Identifier, &Time, sizeof (EFI_TIME));
  //
  // Remove zeros.
  //
  for (Index = 0; Index < sizeof (EFI_TIME); Index++) {
    if (Identifier[Index] == 0) {
      Identifier[Index] = 0x5a;
    }
  }

  MonotonicPointer = (UINT32 *) Identifier;
  gBS->GetNextMonotonicCount (&MonotonicCount);
  MonotonicPointer[0] += (UINT32) MonotonicCount;
  MonotonicPointer[1] += (UINT32) MonotonicCount;
  MonotonicPointer[2] += (UINT32) MonotonicCount;
  MonotonicPointer[3] += (UINT32) MonotonicCount;
}


/**
  Generate unique user ID.

  @param[out]  UserId                 Points to the user identifer.

**/
VOID
GenerateUserId (
  OUT    UINT8                               *UserId
  )
{
  EFI_STATUS              Status;
  USER_PROFILE_ENTRY      *UserProfile;
  EFI_USER_INFO           *UserInfo;
  UINTN                   Index;

  //
  // Generate unique user ID
  //
  while (TRUE) {
    GenerateIdentifier (UserId);
    //
    // Check whether it's unique in user profile database.
    //
    if (mUserProfileDb == NULL) {
      return ;
    }

    for (Index = 0; Index < mUserProfileDb->UserProfileNum; Index++) {
      UserProfile = (USER_PROFILE_ENTRY *) (mUserProfileDb->UserProfile[Index]);
      UserInfo    = NULL;
      Status      = FindUserInfoByType (UserProfile, &UserInfo, EFI_USER_INFO_IDENTIFIER_RECORD);
      if (EFI_ERROR (Status)) {
        continue;
      }

      if (CompareMem ((UINT8 *) (UserInfo + 1), UserId, sizeof (EFI_USER_INFO_IDENTIFIER)) == 0) {
        break;
      }
    }

    if (Index == mUserProfileDb->UserProfileNum) {
      return ;
    }
  }
}


/**
  Expand user profile database.

  @retval TRUE     Success to expand user profile database.
  @retval FALSE    Fail to expand user profile database.
  
**/
BOOLEAN
ExpandUsermUserProfileDb (
  VOID
  )
{
  UINTN               MaxNum;
  USER_PROFILE_DB     *NewDataBase;

  //
  // Create new user profile database.
  //
  if (mUserProfileDb == NULL) {
    MaxNum = USER_NUMBER_INC;
  } else {
    MaxNum = mUserProfileDb->MaxProfileNum + USER_NUMBER_INC;
  }

  NewDataBase = AllocateZeroPool (
                  sizeof (USER_PROFILE_DB) - sizeof (EFI_USER_PROFILE_HANDLE) +
                  MaxNum * sizeof (EFI_USER_PROFILE_HANDLE)
                  );
  if (NewDataBase == NULL) {
    return FALSE;
  }

  NewDataBase->MaxProfileNum = MaxNum;

  //
  // Copy old user profile database value
  //
  if (mUserProfileDb == NULL) {
    NewDataBase->UserProfileNum = 0;
  } else {
    NewDataBase->UserProfileNum = mUserProfileDb->UserProfileNum;
    CopyMem (
      NewDataBase->UserProfile,
      mUserProfileDb->UserProfile,
      NewDataBase->UserProfileNum * sizeof (EFI_USER_PROFILE_HANDLE)
      );
    FreePool (mUserProfileDb);
  }

  mUserProfileDb = NewDataBase;
  return TRUE;
}


/**
  Expand user profile

  @param[in]  User                    Points to user profile.
  @param[in]  ExpandSize              The size of user profile. 

  @retval TRUE     Success to expand user profile size.
  @retval FALSE    Fail to expand user profile size.
  
**/
BOOLEAN
ExpandUserProfile (
  IN USER_PROFILE_ENTRY                         *User,
  IN UINTN                                      ExpandSize
  )
{
  UINT8 *Info;
  UINTN InfoSizeInc;

  //
  // Allocate new memory.
  //
  InfoSizeInc = 128;
  User->MaxProfileSize += ((ExpandSize + InfoSizeInc - 1) / InfoSizeInc) * InfoSizeInc;
  Info = AllocateZeroPool (User->MaxProfileSize);
  if (Info == NULL) {
    return FALSE;
  }
  
  //
  // Copy exist information.
  //
  if (User->UserProfileSize > 0) {
    CopyMem (Info, User->ProfileInfo, User->UserProfileSize);
    FreePool (User->ProfileInfo);
  }

  User->ProfileInfo = Info;
  return TRUE;
}


/**
  Save the user profile to non-volatile memory, or delete it from non-volatile memory.

  @param[in]  User         Point to the user profile
  @param[in]  Delete       If TRUE, delete the found user profile.
                           If FALSE, save the user profile.
  @retval EFI_SUCCESS      Save or delete user profile successfully.
  @retval Others           Fail to change the profile.
  
**/
EFI_STATUS
SaveNvUserProfile (
  IN  USER_PROFILE_ENTRY                        *User,
  IN  BOOLEAN                                   Delete
  )
{
  EFI_STATUS  Status;

  //
  // Check user profile entry.
  //
  Status = FindUserProfile (&User, FALSE, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Save the user profile to non-volatile memory.
  //
  Status = gRT->SetVariable (
                  User->UserVarName,
                  &gUserIdentifyManagerGuid,
                  EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                  Delete ? 0 : User->UserProfileSize,
                  User->ProfileInfo
                  );
  return Status;
}

/**
  Add one new user info into the user's profile.

  @param[in]   User        point to the user profile
  @param[in]   Info        Points to the user information payload.
  @param[in]   InfoSize    The size of the user information payload, in bytes.
  @param[out]  UserInfo    Point to the new info in user profile
  @param[in]   Save        If TRUE, save the profile to NV flash.
                           If FALSE, don't need to save the profile to NV flash.

  @retval EFI_SUCCESS      Add user info to user profile successfully.
  @retval Others           Fail to add user info to user profile.

**/
EFI_STATUS
AddUserInfo (
  IN  USER_PROFILE_ENTRY                        *User,
  IN  UINT8                                     *Info,
  IN  UINTN                                     InfoSize,
  OUT EFI_USER_INFO                             **UserInfo, OPTIONAL
  IN  BOOLEAN                                   Save
  )
{
  EFI_STATUS  Status;

  if ((Info == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check user profile handle.
  //
  Status = FindUserProfile (&User, FALSE, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Check user information memory size.
  //
  if (User->MaxProfileSize - User->UserProfileSize < ALIGN_VARIABLE (InfoSize)) {
    if (!ExpandUserProfile (User, ALIGN_VARIABLE (InfoSize))) {
      return EFI_OUT_OF_RESOURCES;
    }
  }
  
  //
  // Add new user information.
  //
  CopyMem (User->ProfileInfo + User->UserProfileSize, Info, InfoSize);
  if (UserInfo != NULL) {
    *UserInfo = (EFI_USER_INFO *) (User->ProfileInfo + User->UserProfileSize);
  }
  User->UserProfileSize += ALIGN_VARIABLE (InfoSize);

  //
  // Save user profile information.
  //
  if (Save) {
    Status = SaveNvUserProfile (User, FALSE);
  }

  return Status;
}


/**
  Get the user info from the specified user info handle.

  @param[in]      User            Point to the user profile.
  @param[in]      UserInfo        Point to the user information record to get.
  @param[out]     Info            On entry, points to a buffer of at least *InfoSize bytes. 
                                  On exit, holds the user information.
  @param[in, out] InfoSize        On entry, points to the size of Info. 
                                  On return, points to the size of the user information.
  @param[in]      ChkRight        If TRUE, check the user info attribute.
                                  If FALSE, don't check the user info attribute.


  @retval EFI_ACCESS_DENIED       The information cannot be accessed by the current user.
  @retval EFI_INVALID_PARAMETER   InfoSize is NULL or UserInfo is NULL.
  @retval EFI_BUFFER_TOO_SMALL    The number of bytes specified by *InfoSize is too small to hold the 
                                  returned data. The actual size required is returned in *InfoSize.
  @retval EFI_SUCCESS             Information returned successfully.

**/
EFI_STATUS
GetUserInfo (
  IN        USER_PROFILE_ENTRY                  *User,
  IN        EFI_USER_INFO                       *UserInfo,
  OUT       EFI_USER_INFO                       *Info,
  IN OUT    UINTN                               *InfoSize,
  IN        BOOLEAN                             ChkRight
  )
{
  EFI_STATUS  Status;

  if ((InfoSize == NULL) || (UserInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*InfoSize != 0) && (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Find the user information to get.
  //
  Status = FindUserInfo (User, &UserInfo, FALSE, NULL);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Check information attributes.
  //
  if (ChkRight) {
    switch (UserInfo->InfoAttribs & EFI_USER_INFO_ACCESS) {
    case EFI_USER_INFO_PRIVATE:
    case EFI_USER_INFO_PROTECTED:
      if (User != mCurrentUser) {
        return EFI_ACCESS_DENIED;
      }
      break;

    case EFI_USER_INFO_PUBLIC:
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
    }
  }
  
  //
  // Get user information.
  //
  if (UserInfo->InfoSize > *InfoSize) {
    *InfoSize = UserInfo->InfoSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *InfoSize = UserInfo->InfoSize;
  if (Info != NULL) {
    CopyMem (Info, UserInfo, *InfoSize);
  }

  return EFI_SUCCESS;
}


/**
  Delete the specified user information from user profile.

  @param[in]  User        Point to the user profile.
  @param[in]  Info        Point to the user information record to delete.
  @param[in]  Save        If TRUE, save the profile to NV flash.
                          If FALSE, don't need to save the profile to NV flash.

  @retval EFI_SUCCESS     Delete user info from user profile successfully.
  @retval Others          Fail to delete user info from user profile.

**/
EFI_STATUS
DelUserInfo (
  IN  USER_PROFILE_ENTRY                        *User,
  IN  EFI_USER_INFO                             *Info,
  IN  BOOLEAN                                   Save
  )
{
  EFI_STATUS  Status;
  UINTN       Offset;
  UINTN       NextOffset;

  //
  // Check user information handle.
  //
  Status = FindUserInfo (User, &Info, FALSE, &Offset);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Info->InfoType == EFI_USER_INFO_IDENTIFIER_RECORD) {
    return EFI_ACCESS_DENIED;
  }
  
  //
  // Delete the specified user information.
  //
  NextOffset = Offset + ALIGN_VARIABLE (Info->InfoSize);
  User->UserProfileSize -= ALIGN_VARIABLE (Info->InfoSize);
  if (Offset < User->UserProfileSize) {
    CopyMem (User->ProfileInfo + Offset, User->ProfileInfo + NextOffset, User->UserProfileSize - Offset);
  }

  if (Save) {
    Status = SaveNvUserProfile (User, FALSE);
  }

  return Status;
}


/**
  Add or update user information.

  @param[in]      User           Point to the user profile.
  @param[in, out] UserInfo       On entry, points to the user information to modify,
                                 or NULL to add a new UserInfo. 
                                 On return, points to the modified user information.
  @param[in]      Info           Points to the new user information.
  @param[in]      InfoSize       The size of Info,in bytes.

  @retval EFI_INVALID_PARAMETER  UserInfo is NULL or Info is NULL.
  @retval EFI_ACCESS_DENIED      The record is exclusive.
  @retval EFI_SUCCESS            User information was successfully changed/added.

**/
EFI_STATUS
ModifyUserInfo (
  IN        USER_PROFILE_ENTRY                  *User,
  IN OUT    EFI_USER_INFO                       **UserInfo,
  IN CONST  EFI_USER_INFO                       *Info,
  IN        UINTN                               InfoSize
  )
{
  EFI_STATUS    Status;
  UINTN         PayloadLen;
  EFI_USER_INFO *OldInfo;

  if ((UserInfo == NULL) || (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (InfoSize < sizeof (EFI_USER_INFO) || InfoSize != Info->InfoSize) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check user information.
  //
  if (Info->InfoType == EFI_USER_INFO_IDENTIFIER_RECORD) {
    return EFI_ACCESS_DENIED;
  }
  
  if (!CheckUserInfo (Info)) {
    return EFI_INVALID_PARAMETER;
  }


  if (*UserInfo == NULL) {
    //
    // Add new user information.
    //
    OldInfo = NULL;
    do {
      Status = FindUserInfoByType (User, &OldInfo, Info->InfoType);
      if (EFI_ERROR (Status)) {
        break;
      }
      ASSERT (OldInfo != NULL);

      if (((OldInfo->InfoAttribs & EFI_USER_INFO_EXCLUSIVE) != 0) || 
           ((Info->InfoAttribs & EFI_USER_INFO_EXCLUSIVE) != 0)) {
        //
        //  Same type can not co-exist for exclusive information.
        //
        return EFI_ACCESS_DENIED;
      }

      //
      // Check whether it exists in DB.
      //
      if (Info->InfoSize != OldInfo->InfoSize) {
        continue;
      }

      if (!CompareGuid (&OldInfo->Credential, &Info->Credential)) {
        continue;
      }
  
      PayloadLen = Info->InfoSize - sizeof (EFI_USER_INFO);
      if (PayloadLen == 0) {
        continue;
      }

      if (CompareMem ((UINT8 *)(OldInfo + 1), (UINT8 *)(Info + 1), PayloadLen) != 0) {
        continue;
      }

      //
      // Yes. The new info is as same as the one in profile.
      //
      return EFI_SUCCESS;
    } while (!EFI_ERROR (Status));

    Status = AddUserInfo (User, (UINT8 *) Info, InfoSize, UserInfo, TRUE);
    return Status;
  }
  
  //
  // Modify existing user information.
  //
  OldInfo = *UserInfo;
  if (OldInfo->InfoType != Info->InfoType) {
    return EFI_INVALID_PARAMETER;
  }
  
  if (((Info->InfoAttribs & EFI_USER_INFO_EXCLUSIVE) != 0) && 
       (OldInfo->InfoAttribs & EFI_USER_INFO_EXCLUSIVE) == 0) {
    //
    // Try to add exclusive attrib in new info. 
    // Check whether there is another information with the same type in profile.
    //
    OldInfo = NULL;
    do {
      Status = FindUserInfoByType (User, &OldInfo, Info->InfoType);
      if (EFI_ERROR (Status)) {
        break;
      }
      if (OldInfo != *UserInfo) {
        //
        // There is another information with the same type in profile.
        // Therefore, can't modify existing user information to add exclusive attribute.
        //
        return EFI_ACCESS_DENIED;
      }
    } while (TRUE);    
  }

  Status = DelUserInfo (User, *UserInfo, FALSE);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return AddUserInfo (User, (UINT8 *) Info, InfoSize, UserInfo, TRUE);
}


/**
  Delete the user profile from non-volatile memory and database.

  @param[in]  User              Points to the user profile.

  @retval EFI_SUCCESS      Delete user from the user profile successfully.
  @retval Others           Fail to delete user from user profile
  
**/
EFI_STATUS
DelUserProfile (
  IN  USER_PROFILE_ENTRY                        *User
  )
{
  EFI_STATUS          Status;
  UINTN               Index;

  //
  // Check whether it is in the user profile database.
  //
  Status = FindUserProfile (&User, FALSE, &Index);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check whether it is the current user.
  //
  if (User == mCurrentUser) {
    return EFI_ACCESS_DENIED;
  }
  
  //
  // Delete user profile from the non-volatile memory.
  //
  Status    = SaveNvUserProfile (mUserProfileDb->UserProfile[mUserProfileDb->UserProfileNum - 1], TRUE);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  mUserProfileDb->UserProfileNum--;

  //
  // Modify user profile database.
  //
  if (Index != mUserProfileDb->UserProfileNum) {
    mUserProfileDb->UserProfile[Index] = mUserProfileDb->UserProfile[mUserProfileDb->UserProfileNum];
    CopyMem (
      ((USER_PROFILE_ENTRY *) mUserProfileDb->UserProfile[Index])->UserVarName,
      User->UserVarName,
      sizeof (User->UserVarName)
      );
    Status = SaveNvUserProfile (mUserProfileDb->UserProfile[Index], FALSE);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
  //
  // Delete user profile information.
  //
  if (User->ProfileInfo != NULL) {
    FreePool (User->ProfileInfo);
  }

  FreePool (User);
  return EFI_SUCCESS;
}


/**
  Add user profile to user profile database.

  @param[out]   UserProfile   Point to the newly added user profile.
  @param[in]    ProfileSize   The size of the user profile.
  @param[in]    ProfileInfo   Point to the user profie data.
  @param[in]    Save          If TRUE, save the new added profile to NV flash.
                              If FALSE, don't save the profile to NV flash.

  @retval EFI_SUCCESS         Add user profile to user profile database successfully.
  @retval Others              Fail to add user profile to user profile database.

**/
EFI_STATUS
AddUserProfile (
     OUT  USER_PROFILE_ENTRY                    **UserProfile, OPTIONAL
  IN      UINTN                                 ProfileSize,
  IN      UINT8                                 *ProfileInfo,
  IN      BOOLEAN                               Save
  )
{
  EFI_STATUS              Status;
  USER_PROFILE_ENTRY      *User;

  //
  // Check the data format to be added.
  //
  if (!CheckProfileInfo (ProfileInfo, ProfileSize)) {
    return EFI_SECURITY_VIOLATION;
  }
  
  //
  // Create user profile entry.
  //
  User = AllocateZeroPool (sizeof (USER_PROFILE_ENTRY));
  if (User == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // Add the entry to the user profile database.
  //
  if (mUserProfileDb->UserProfileNum == mUserProfileDb->MaxProfileNum) {
    if (!ExpandUsermUserProfileDb ()) {
      FreePool (User);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  UnicodeSPrint (
    User->UserVarName, 
    sizeof (User->UserVarName),
    L"User%04x", 
    mUserProfileDb->UserProfileNum
    );
  User->UserProfileSize = 0;
  User->MaxProfileSize  = 0;
  User->ProfileInfo     = NULL;
  mUserProfileDb->UserProfile[mUserProfileDb->UserProfileNum] = (EFI_USER_PROFILE_HANDLE) User;
  mUserProfileDb->UserProfileNum++;

  //
  // Add user profile information.
  //
  Status = AddUserInfo (User, ProfileInfo, ProfileSize, NULL, Save);
  if (EFI_ERROR (Status)) {
    DelUserProfile (User);
    return Status;
  }
  //
  // Set new user profile handle.
  //
  if (UserProfile != NULL) {
    *UserProfile = User;
  }

  return EFI_SUCCESS;
}


/**
  This function creates a new user profile with only a new user identifier
  attached and returns its handle. The user profile is non-volatile, but the
  handle User can change across reboots.

  @param[out]  User               Handle of a new user profile.

  @retval EFI_SUCCESS             User profile was successfully created.
  @retval Others                  Fail to create user profile

**/
EFI_STATUS
CreateUserProfile (
  OUT USER_PROFILE_ENTRY                        **User
  )
{
  EFI_STATUS    Status;
  EFI_USER_INFO *UserInfo;

  if (User == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Generate user id information.
  //
  UserInfo = AllocateZeroPool (sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_IDENTIFIER));
  if (UserInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UserInfo->InfoType    = EFI_USER_INFO_IDENTIFIER_RECORD;
  UserInfo->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_IDENTIFIER);
  UserInfo->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
  GenerateUserId ((UINT8 *) (UserInfo + 1));
  
  //
  // Add user profile to the user profile database.
  //
  Status = AddUserProfile (User, UserInfo->InfoSize, (UINT8 *) UserInfo, TRUE);
  FreePool (UserInfo);
  return Status;
}


/**
  Add a default user profile to user profile database.

  @retval EFI_SUCCESS             A default user profile is added successfully.
  @retval Others                  Fail to add a default user profile
  
**/
EFI_STATUS
AddDefaultUserProfile (
  VOID
  )
{
  EFI_STATUS                    Status;
  USER_PROFILE_ENTRY            *User;
  EFI_USER_INFO                 *Info;
  EFI_USER_INFO                 *NewInfo;
  EFI_USER_INFO_CREATE_DATE     CreateDate;
  EFI_USER_INFO_USAGE_COUNT     UsageCount;
  EFI_USER_INFO_ACCESS_CONTROL  *Access;
  EFI_USER_INFO_IDENTITY_POLICY *Policy;
  
  //
  // Create a user profile.
  //
  Status = CreateUserProfile (&User);
  if (EFI_ERROR (Status)) {
    return Status;
  }
 
  //
  // Allocate a buffer to add all default user information.
  //
  Info = AllocateZeroPool (sizeof (EFI_USER_INFO) + INFO_PAYLOAD_SIZE);
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Add user name.
  //
  Info->InfoType    = EFI_USER_INFO_NAME_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
  Info->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (mUserName);
  CopyMem ((UINT8 *) (Info + 1), mUserName, sizeof (mUserName));
  NewInfo = NULL;
  Status  = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  //
  // Add user profile create date record.
  //
  Info->InfoType    = EFI_USER_INFO_CREATE_DATE_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
  Info->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_CREATE_DATE);
  Status            = gRT->GetTime (&CreateDate, NULL);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  CopyMem ((UINT8 *) (Info + 1), &CreateDate, sizeof (EFI_USER_INFO_CREATE_DATE));
  NewInfo = NULL;
  Status  = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  //
  // Add user profile usage count record.
  //
  Info->InfoType    = EFI_USER_INFO_USAGE_COUNT_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
  Info->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_USAGE_COUNT);
  UsageCount        = 0;
  CopyMem ((UINT8 *) (Info + 1), &UsageCount, sizeof (EFI_USER_INFO_USAGE_COUNT));
  NewInfo = NULL;
  Status  = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
 
  //
  // Add user access right.
  //
  Info->InfoType    = EFI_USER_INFO_ACCESS_POLICY_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
  Access            = (EFI_USER_INFO_ACCESS_CONTROL *) (Info + 1);
  Access->Type      = EFI_USER_INFO_ACCESS_MANAGE;
  Access->Size      = sizeof (EFI_USER_INFO_ACCESS_CONTROL);
  Info->InfoSize    = sizeof (EFI_USER_INFO) + Access->Size;
  NewInfo = NULL;
  Status  = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  //
  // Add user identity policy.
  //
  Info->InfoType    = EFI_USER_INFO_IDENTITY_POLICY_RECORD;
  Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PRIVATE | EFI_USER_INFO_EXCLUSIVE;
  Policy            = (EFI_USER_INFO_IDENTITY_POLICY *) (Info + 1);
  Policy->Type      = EFI_USER_INFO_IDENTITY_TRUE;
  Policy->Length    = sizeof (EFI_USER_INFO_IDENTITY_POLICY);  
  Info->InfoSize    = sizeof (EFI_USER_INFO) + Policy->Length;
  NewInfo = NULL;
  Status  = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);

Done:
  FreePool (Info);
  return Status;
}


/**
  Publish current user information into EFI System Configuration Table.

  By UEFI spec, the User Identity Manager will publish the current user profile 
  into the EFI System Configuration Table. Currently, only the user identifier and user
  name are published.

  @retval EFI_SUCCESS      Current user information is published successfully.
  @retval Others           Fail to publish current user information

**/
EFI_STATUS
PublishUserTable (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_CONFIGURATION_TABLE *EfiConfigurationTable;
  EFI_USER_INFO_TABLE     *UserInfoTable;
  EFI_USER_INFO           *IdInfo;
  EFI_USER_INFO           *NameInfo;

  Status = EfiGetSystemConfigurationTable (
             &gEfiUserManagerProtocolGuid,
             (VOID **) &EfiConfigurationTable
             );
  if (!EFI_ERROR (Status)) {
    //
    // The table existed! 
    //
    return EFI_SUCCESS;
  }

  //
  // Get user ID information.
  //
  IdInfo  = NULL;
  Status  = FindUserInfoByType (mCurrentUser, &IdInfo, EFI_USER_INFO_IDENTIFIER_RECORD);
  if (EFI_ERROR (Status)) {
    return Status;

  }
  //
  // Get user name information.
  //
  NameInfo  = NULL;
  Status    = FindUserInfoByType (mCurrentUser, &NameInfo, EFI_USER_INFO_NAME_RECORD);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Allocate a buffer for user information table.
  //
  UserInfoTable = (EFI_USER_INFO_TABLE *) AllocateRuntimePool (
                                            sizeof (EFI_USER_INFO_TABLE) + 
                                            IdInfo->InfoSize + 
                                            NameInfo->InfoSize
                                            );
  if (UserInfoTable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    return Status;
  }

  UserInfoTable->Size = sizeof (EFI_USER_INFO_TABLE);  
  
  //
  // Append the user information to the user info table
  //
  CopyMem ((UINT8 *) UserInfoTable + UserInfoTable->Size, (UINT8 *) IdInfo, IdInfo->InfoSize);
  UserInfoTable->Size += IdInfo->InfoSize;

  CopyMem ((UINT8 *) UserInfoTable + UserInfoTable->Size, (UINT8 *) NameInfo, NameInfo->InfoSize);
  UserInfoTable->Size += NameInfo->InfoSize;

  Status = gBS->InstallConfigurationTable (&gEfiUserManagerProtocolGuid, (VOID *) UserInfoTable);
  return Status;
}


/**
  Get the user's identity type.

  The identify manager only supports the identity policy in which the credential 
  provider handles are connected by the operator 'AND' or 'OR'.


  @param[in]   User              Handle of a user profile.
  @param[out]  PolicyType        Point to the identity type.

  @retval EFI_SUCCESS            Get user's identity type successfully.
  @retval Others                 Fail to get user's identity type.

**/
EFI_STATUS
GetIdentifyType (
  IN      EFI_USER_PROFILE_HANDLE               User,
     OUT  UINT8                                 *PolicyType
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO                 *IdentifyInfo;
  UINTN                         TotalLen;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;

  //
  // Get user identify policy information.
  //
  IdentifyInfo  = NULL;
  Status        = FindUserInfoByType (User, &IdentifyInfo, EFI_USER_INFO_IDENTITY_POLICY_RECORD);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (IdentifyInfo != NULL);
  
  //
  // Search the user identify policy according to type.
  //
  TotalLen    = 0;
  *PolicyType = EFI_USER_INFO_IDENTITY_FALSE;
  while (TotalLen < IdentifyInfo->InfoSize - sizeof (EFI_USER_INFO)) {
    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) ((UINT8 *) (IdentifyInfo + 1) + TotalLen);
    if (Identity->Type == EFI_USER_INFO_IDENTITY_AND) {
      *PolicyType = EFI_USER_INFO_IDENTITY_AND;
      break;
    }

    if (Identity->Type == EFI_USER_INFO_IDENTITY_OR) {
      *PolicyType = EFI_USER_INFO_IDENTITY_OR;
      break;
    }
    TotalLen += Identity->Length;
  }
  return EFI_SUCCESS;
}


/**
  Identify the User by the specfied provider.

  @param[in]  User                Handle of a user profile.
  @param[in]  Provider            Points to the identifier of credential provider.

  @retval EFI_INVALID_PARAMETER   Provider is NULL.
  @retval EFI_NOT_FOUND           Fail to identify the specified user.
  @retval EFI_SUCCESS             User is identified successfully.

**/
EFI_STATUS
IdentifyByProviderId (
  IN  EFI_USER_PROFILE_HANDLE                   User,
  IN  EFI_GUID                                  *Provider
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO_IDENTIFIER      UserId;
  UINTN                         Index;
  EFI_CREDENTIAL_LOGON_FLAGS    AutoLogon;
  EFI_HII_HANDLE                HiiHandle;
  EFI_GUID                      FormSetId;
  EFI_FORM_ID                   FormId;
  EFI_USER_CREDENTIAL2_PROTOCOL *UserCredential;

  if (Provider == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check the user ID identified by the specified credential provider.
  //
  for (Index = 0; Index < mProviderDb->Count; Index++) {
    //
    // Check credential provider class.
    //
    UserCredential = mProviderDb->Provider[Index];
    if (CompareGuid (&UserCredential->Identifier, Provider)) {
      Status = UserCredential->Select (UserCredential, &AutoLogon);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      if ((AutoLogon & EFI_CREDENTIAL_LOGON_FLAG_AUTO) == 0) {
        //
        // Get credential provider form.
        //
        Status = UserCredential->Form (
                                   UserCredential, 
                                   &HiiHandle, 
                                   &FormSetId, 
                                   &FormId
                                   );
        if (!EFI_ERROR (Status)) {        
          //
          // Send form to get user input.
          //
          Status = mCallbackInfo->FormBrowser2->SendForm (
                                                  mCallbackInfo->FormBrowser2,
                                                  &HiiHandle,
                                                  1,
                                                  &FormSetId,
                                                  FormId,
                                                  NULL,
                                                  NULL
                                                  );
          if (EFI_ERROR (Status)) {
            return Status;
          }                                            
        }        
      }

      Status = UserCredential->User (UserCredential, User, &UserId);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = UserCredential->Deselect (UserCredential);
      if (EFI_ERROR (Status)) {
        return Status;
      }
      
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Update user information when user is logon on successfully.

  @param[in]  User         Points to user profile.

  @retval EFI_SUCCESS      Update user information successfully.
  @retval Others           Fail to update user information.

**/
EFI_STATUS
UpdateUserInfo (
  IN  USER_PROFILE_ENTRY                        *User
  )
{
  EFI_STATUS                Status;
  EFI_USER_INFO             *Info;
  EFI_USER_INFO             *NewInfo;
  EFI_USER_INFO_CREATE_DATE Date;
  EFI_USER_INFO_USAGE_COUNT UsageCount;
  UINTN                     InfoLen;

  //
  // Allocate a buffer to update user's date record and usage record.
  //
  InfoLen  = MAX (sizeof (EFI_USER_INFO_CREATE_DATE), sizeof (EFI_USER_INFO_USAGE_COUNT));
  Info     = AllocateZeroPool (sizeof (EFI_USER_INFO) + InfoLen);
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Check create date record.
  //
  NewInfo = NULL;
  Status  = FindUserInfoByType (User, &NewInfo, EFI_USER_INFO_CREATE_DATE_RECORD);
  if (Status == EFI_NOT_FOUND) {
    Info->InfoType    = EFI_USER_INFO_CREATE_DATE_RECORD;
    Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
    Info->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_CREATE_DATE);
    Status            = gRT->GetTime (&Date, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Info);
      return Status;
    }

    CopyMem ((UINT8 *) (Info + 1), &Date, sizeof (EFI_USER_INFO_CREATE_DATE));
    NewInfo = NULL;
    Status  = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
    if (EFI_ERROR (Status)) {
      FreePool (Info);
      return Status;
    }
  }
  
  //
  // Update usage date record.
  //
  NewInfo = NULL;
  Status  = FindUserInfoByType (User, &NewInfo, EFI_USER_INFO_USAGE_DATE_RECORD);
  if ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND)) {
    Info->InfoType    = EFI_USER_INFO_USAGE_DATE_RECORD;
    Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
    Info->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_USAGE_DATE);
    Status            = gRT->GetTime (&Date, NULL);
    if (EFI_ERROR (Status)) {
      FreePool (Info);
      return Status;
    }

    CopyMem ((UINT8 *) (Info + 1), &Date, sizeof (EFI_USER_INFO_USAGE_DATE));
    Status = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
    if (EFI_ERROR (Status)) {
      FreePool (Info);
      return Status;
    }
  }
  
  //
  // Update usage count record.
  //
  UsageCount  = 0;
  NewInfo     = NULL;
  Status      = FindUserInfoByType (User, &NewInfo, EFI_USER_INFO_USAGE_COUNT_RECORD);
  //
  // Get usage count.
  //
  if (Status == EFI_SUCCESS) {
    CopyMem (&UsageCount, (UINT8 *) (NewInfo + 1), sizeof (EFI_USER_INFO_USAGE_COUNT));
  }

  UsageCount++;
  if ((Status == EFI_SUCCESS) || (Status == EFI_NOT_FOUND)) {
    Info->InfoType    = EFI_USER_INFO_USAGE_COUNT_RECORD;
    Info->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | EFI_USER_INFO_PUBLIC | EFI_USER_INFO_EXCLUSIVE;
    Info->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_USAGE_COUNT);
    CopyMem ((UINT8 *) (Info + 1), &UsageCount, sizeof (EFI_USER_INFO_USAGE_COUNT));
    Status = ModifyUserInfo (User, &NewInfo, Info, Info->InfoSize);
    if (EFI_ERROR (Status)) {
      FreePool (Info);
      return Status;
    }
  }

  FreePool (Info);
  return EFI_SUCCESS;
}


/**
  Add a credenetial provider item in form.

  @param[in]  ProviderGuid        Points to the identifir of credential provider.
  @param[in]  OpCodeHandle        Points to container for dynamic created opcodes.

**/
VOID
AddProviderSelection (
  IN     EFI_GUID                               *ProviderGuid,
  IN     VOID                                   *OpCodeHandle
  )
{
  EFI_HII_HANDLE                HiiHandle;
  EFI_STRING_ID                 ProvID;
  CHAR16                        *ProvStr;
  UINTN                         Index;
  EFI_USER_CREDENTIAL2_PROTOCOL *UserCredential;

  for (Index = 0; Index < mProviderDb->Count; Index++) {
    UserCredential = mProviderDb->Provider[Index];
    if (CompareGuid (&UserCredential->Identifier, ProviderGuid)) {
      //
      // Add credential provider selection.
      //
      UserCredential->Title (UserCredential, &HiiHandle, &ProvID);
      ProvStr = HiiGetString (HiiHandle, ProvID, NULL);
      if (ProvStr == NULL) {
        continue ;
      }
      ProvID  = HiiSetString (mCallbackInfo->HiiHandle, 0, ProvStr, NULL);
      FreePool (ProvStr);
      HiiCreateActionOpCode (
        OpCodeHandle,                          // Container for dynamic created opcodes
        (EFI_QUESTION_ID)(LABEL_PROVIDER_NAME + Index),  // Question ID
        ProvID,                                // Prompt text
        STRING_TOKEN (STR_NULL_STRING),        // Help text
        EFI_IFR_FLAG_CALLBACK,                 // Question flag
        0                                      // Action String ID
        );
      break;
    }
  }
}


/**
  Add a username item in form.

  @param[in]  Index            The index of the user in the user name list.
  @param[in]  User             Points to the user profile whose username is added. 
  @param[in]  OpCodeHandle     Points to container for dynamic created opcodes.

  @retval EFI_SUCCESS          Add a username successfully.
  @retval Others               Fail to add a username.

**/
EFI_STATUS
AddUserSelection (
  IN     UINT16                                 Index,
  IN     USER_PROFILE_ENTRY                     *User,
  IN     VOID                                   *OpCodeHandle
  )
{
  EFI_STRING_ID UserName;
  EFI_STATUS    Status;
  EFI_USER_INFO *UserInfo;

  UserInfo  = NULL;
  Status    = FindUserInfoByType (User, &UserInfo, EFI_USER_INFO_NAME_RECORD);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Add user name selection.
  //
  UserName = HiiSetString (mCallbackInfo->HiiHandle, 0, (EFI_STRING) (UserInfo + 1), NULL);
  if (UserName == 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  HiiCreateGotoOpCode (
    OpCodeHandle,                   // Container for dynamic created opcodes
    FORMID_PROVIDER_FORM,           // Target Form ID
    UserName,                       // Prompt text
    STRING_TOKEN (STR_NULL_STRING), // Help text
    EFI_IFR_FLAG_CALLBACK,          // Question flag
    (UINT16) Index                  // Question ID
    );

  return EFI_SUCCESS;
}


/**
  Identify the user whose identity policy does not contain the operator 'OR'.
  
  @param[in]  User             Points to the user profile.

  @retval EFI_SUCCESS          The specified user is identified successfully.
  @retval Others               Fail to identify the user.
  
**/
EFI_STATUS
IdentifyAndTypeUser (
  IN  USER_PROFILE_ENTRY                        *User
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO                 *IdentifyInfo;
  BOOLEAN                       Success;
  UINTN                         TotalLen;
  UINTN                         ValueLen;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;

  //
  // Get user identify policy information.
  //
  IdentifyInfo  = NULL;
  Status        = FindUserInfoByType (User, &IdentifyInfo, EFI_USER_INFO_IDENTITY_POLICY_RECORD);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (IdentifyInfo != NULL);
  
  //
  // Check each part of identification policy expression.
  //
  Success   = FALSE;
  TotalLen  = 0;
  while (TotalLen < IdentifyInfo->InfoSize - sizeof (EFI_USER_INFO)) {
    Identity  = (EFI_USER_INFO_IDENTITY_POLICY *) ((UINT8 *) (IdentifyInfo + 1) + TotalLen);
    ValueLen  = Identity->Length - sizeof (EFI_USER_INFO_IDENTITY_POLICY);
    switch (Identity->Type) {

    case EFI_USER_INFO_IDENTITY_FALSE:
      //
      // Check False option.
      //
      Success = FALSE;
      break;

    case EFI_USER_INFO_IDENTITY_TRUE:
      //
      // Check True option.
      //
      Success = TRUE;
      break;

    case EFI_USER_INFO_IDENTITY_NOT:
      //
      // Check negative operation.
      //
      break;

    case EFI_USER_INFO_IDENTITY_AND:
      //
      // Check and operation.
      //
      if (!Success) {
        return EFI_NOT_READY;
      }

      Success = FALSE;
      break;

    case EFI_USER_INFO_IDENTITY_OR:
      //
      // Check or operation.
      //
      if (Success) {
        return EFI_SUCCESS;
      }
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_TYPE:
      //
      // Check credential provider by type.
      //
      break;

    case EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER:
      //
      // Check credential provider by ID.
      //
      if (ValueLen != sizeof (EFI_GUID)) {
        return EFI_INVALID_PARAMETER;
      }

      Status = IdentifyByProviderId (User, (EFI_GUID *) (Identity + 1));
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Success = TRUE;
      break;

    default:
      return EFI_INVALID_PARAMETER;
      break;
    }

    TotalLen += Identity->Length;
  }

  if (TotalLen != IdentifyInfo->InfoSize - sizeof (EFI_USER_INFO)) {
    return EFI_INVALID_PARAMETER;
  }

  if (!Success) {
    return EFI_NOT_READY;
  }

  return EFI_SUCCESS;
}


/**
  Identify the user whose identity policy does not contain the operator 'AND'.
  
  @param[in]  User             Points to the user profile.

  @retval EFI_SUCCESS          The specified user is identified successfully.
  @retval Others               Fail to identify the user.
  
**/
EFI_STATUS
IdentifyOrTypeUser (
  IN  USER_PROFILE_ENTRY                        *User
  )
{
  EFI_STATUS                    Status;
  EFI_USER_INFO                 *IdentifyInfo;
  UINTN                         TotalLen;
  UINTN                         ValueLen;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  VOID                          *StartOpCodeHandle;
  VOID                          *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL            *StartLabel;
  EFI_IFR_GUID_LABEL            *EndLabel;

  //
  // Get user identify policy information.
  //
  IdentifyInfo  = NULL;
  Status        = FindUserInfoByType (User, &IdentifyInfo, EFI_USER_INFO_IDENTITY_POLICY_RECORD);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  ASSERT (IdentifyInfo != NULL);
  
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
  StartLabel->Number        = LABEL_PROVIDER_NAME;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add the providers that exists in the user's policy.
  //
  TotalLen = 0;
  while (TotalLen < IdentifyInfo->InfoSize - sizeof (EFI_USER_INFO)) {
    Identity  = (EFI_USER_INFO_IDENTITY_POLICY *) ((UINT8 *) (IdentifyInfo + 1) + TotalLen);
    ValueLen  = Identity->Length - sizeof (EFI_USER_INFO_IDENTITY_POLICY);
    if (Identity->Type == EFI_USER_INFO_IDENTITY_CREDENTIAL_PROVIDER) {
      AddProviderSelection ((EFI_GUID *) (Identity + 1), StartOpCodeHandle);
    }

    TotalLen += Identity->Length;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserIdentifyManagerGuid,// Formset GUID
    FORMID_PROVIDER_FORM,     // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);

  return EFI_SUCCESS;
}


/**
  This function processes the results of changes in configuration.

  @param  This                   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param  Action                 Specifies the type of action taken by the browser.
  @param  QuestionId             A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param  Type                   The type of value for the question.
  @param  Value                  A pointer to the data being sent to the original
                                 exporting driver.
  @param  ActionRequest          On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval Others                 Fail to handle the action.

**/
EFI_STATUS
EFIAPI
UserIdentifyManagerCallback (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN  EFI_BROWSER_ACTION                        Action,
  IN  EFI_QUESTION_ID                           QuestionId,
  IN  UINT8                                     Type,
  IN  EFI_IFR_TYPE_VALUE                        *Value,
  OUT EFI_BROWSER_ACTION_REQUEST                *ActionRequest
  )
{
  EFI_STATUS              Status;
  USER_PROFILE_ENTRY      *User;
  UINT8                   PolicyType;
  UINT16                  Index;
  VOID                    *StartOpCodeHandle;
  VOID                    *EndOpCodeHandle;
  EFI_IFR_GUID_LABEL      *StartLabel;
  EFI_IFR_GUID_LABEL      *EndLabel;

  Status = EFI_SUCCESS;

  switch (Action) {
  case EFI_BROWSER_ACTION_FORM_OPEN:
    {
      //
      // Update user Form when user Form is opened.
      // This will be done only in FORM_OPEN CallBack of question with FORM_OPEN_QUESTION_ID from user Form.
      //
      if (QuestionId != FORM_OPEN_QUESTION_ID) {
        return EFI_SUCCESS;
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
      StartLabel->Number        = LABEL_USER_NAME;
  
      EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                          EndOpCodeHandle,
                                          &gEfiIfrTianoGuid,
                                          NULL,
                                          sizeof (EFI_IFR_GUID_LABEL)
                                          );
      EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
      EndLabel->Number        = LABEL_END;
  
      //
      // Add all the user profile in the user profile database.
      //
      for (Index = 0; Index < mUserProfileDb->UserProfileNum; Index++) {
        User = (USER_PROFILE_ENTRY *) mUserProfileDb->UserProfile[Index];
        AddUserSelection ((UINT16)(LABEL_USER_NAME + Index), User, StartOpCodeHandle);
      }
  
      HiiUpdateForm (
        mCallbackInfo->HiiHandle, // HII handle
        &gUserIdentifyManagerGuid,// Formset GUID
        FORMID_USER_FORM,         // Form ID
        StartOpCodeHandle,        // Label for where to insert opcodes
        EndOpCodeHandle           // Replace data
        );
  
      HiiFreeOpCodeHandle (StartOpCodeHandle);
      HiiFreeOpCodeHandle (EndOpCodeHandle);
  
      return EFI_SUCCESS;
    }
    break;

  case EFI_BROWSER_ACTION_FORM_CLOSE:
    Status = EFI_SUCCESS;
    break;

  case EFI_BROWSER_ACTION_CHANGED:
    if (QuestionId >= LABEL_PROVIDER_NAME) {
      //
      // QuestionId comes from the second Form (Select a Credential Provider if identity  
      // policy is OR type). Identify the user by the selected provider.
      //
      Status = IdentifyByProviderId (mCurrentUser, &mProviderDb->Provider[QuestionId & 0xFFF]->Identifier);
      if (Status == EFI_SUCCESS) {
        mIdentified     = TRUE;
        *ActionRequest  = EFI_BROWSER_ACTION_REQUEST_EXIT;
      }
      return EFI_SUCCESS;
    }
    break;
    
  case EFI_BROWSER_ACTION_CHANGING:
    //
    // QuestionId comes from the first Form (Select a user to identify).
    //
    if (QuestionId >= LABEL_PROVIDER_NAME) {
      return EFI_SUCCESS;
    }

    User   = (USER_PROFILE_ENTRY *) mUserProfileDb->UserProfile[QuestionId & 0xFFF];
    Status = GetIdentifyType (User, &PolicyType);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (PolicyType == EFI_USER_INFO_IDENTITY_OR) {
      //
      // Identify the user by "OR" logical.
      //
      Status = IdentifyOrTypeUser (User);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      mCurrentUser = (EFI_USER_PROFILE_HANDLE) User;
    } else {
      //
      // Identify the user by "AND" logical.
      //
      Status = IdentifyAndTypeUser (User);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      mCurrentUser    = (EFI_USER_PROFILE_HANDLE) User;
      mIdentified     = TRUE;
      if (Type == EFI_IFR_TYPE_REF) {
        Value->ref.FormId = FORMID_INVALID_FORM;
      }
    }
  break;

  default:
    //
    // All other action return unsupported.
    //
    Status = EFI_UNSUPPORTED;
    break;
  }


  return Status;
}


/**
  This function construct user profile database from user data saved in the Flash.
  If no user is found in Flash, add one default user "administrator" in the user 
  profile database.

  @retval EFI_SUCCESS            Init user profile database successfully.
  @retval Others                 Fail to init user profile database.
  
**/
EFI_STATUS
InitUserProfileDb (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *VarData;
  UINTN       VarSize;
  UINTN       CurVarSize;
  CHAR16      VarName[10];
  UINTN       Index;
  UINT32      VarAttr;

  if (mUserProfileDb != NULL) {
    //
    // The user profiles had been already initialized.
    //
    return EFI_SUCCESS;
  }

  //
  // Init user profile database structure.
  //
  if (!ExpandUsermUserProfileDb ()) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurVarSize = DEFAULT_PROFILE_SIZE;
  VarData    = AllocateZeroPool (CurVarSize);
  if (VarData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  
  //
  // Get all user proifle entries.
  //
  Index = 0;
  while (TRUE) {
    //
    // Get variable name.
    //
    UnicodeSPrint (
      VarName, 
      sizeof (VarName),
      L"User%04x", 
      Index
      );
    Index++;

    //
    // Get variable value.
    //
    VarSize = CurVarSize;
    Status  = gRT->GetVariable (VarName, &gUserIdentifyManagerGuid, &VarAttr, &VarSize, VarData);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      FreePool (VarData);
      VarData = AllocatePool (VarSize);
      if (VarData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      CurVarSize  = VarSize;
      Status      = gRT->GetVariable (VarName, &gUserIdentifyManagerGuid, &VarAttr, &VarSize, VarData);
    }

    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_FOUND) {
        Status = EFI_SUCCESS;
      }
      break;
    }
    
    //
    // Check variable attributes.
    //
    if (VarAttr != (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS)) {
      Status = gRT->SetVariable (VarName, &gUserIdentifyManagerGuid, VarAttr, 0, NULL);
      continue;
    }
    
    //
    // Add user profile to the user profile database.
    //
    Status = AddUserProfile (NULL, VarSize, VarData, FALSE);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_SECURITY_VIOLATION) {
        //
        // Delete invalid user profile
        //
        gRT->SetVariable (VarName, &gUserIdentifyManagerGuid, VarAttr, 0, NULL);
      } else if (Status == EFI_OUT_OF_RESOURCES) {
        break;
      }
    } else {
      //
      // Delete and save the profile again if some invalid profiles are deleted.
      //
      if (mUserProfileDb->UserProfileNum < Index) {
        gRT->SetVariable (VarName, &gUserIdentifyManagerGuid, VarAttr, 0, NULL);
        SaveNvUserProfile (mUserProfileDb->UserProfile[mUserProfileDb->UserProfileNum - 1], FALSE);
      }
    }
  }

  if (VarData != NULL) {
    FreePool (VarData);
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Check whether the user profile database is empty.
  //
  if (mUserProfileDb->UserProfileNum == 0) {
    Status = AddDefaultUserProfile ();
  }

  return Status;
}


/**
  This function collects all the credential providers and saves to mProviderDb.

  @retval EFI_SUCCESS            Collect credential providers successfully.
  @retval Others                 Fail to collect credential providers.

**/
EFI_STATUS
InitProviderInfo (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuf;
  UINTN       Index; 

  if (mProviderDb != NULL) {
    //
    // The credential providers had been collected before.
    //
    return EFI_SUCCESS;
  }

  //
  // Try to find all the user credential provider driver.
  //
  HandleCount = 0;
  HandleBuf   = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiUserCredential2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuf
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get provider infomation.
  //
  mProviderDb = AllocateZeroPool (
                  sizeof (CREDENTIAL_PROVIDER_INFO) - 
                  sizeof (EFI_USER_CREDENTIAL2_PROTOCOL *) + 
                  HandleCount * sizeof (EFI_USER_CREDENTIAL2_PROTOCOL *)
                  );
  if (mProviderDb == NULL) {
    FreePool (HandleBuf);
    return EFI_OUT_OF_RESOURCES;
  }

 mProviderDb->Count = HandleCount;
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    HandleBuf[Index],
                    &gEfiUserCredential2ProtocolGuid,
                    (VOID **) &mProviderDb->Provider[Index]
                    );
    if (EFI_ERROR (Status)) {
      FreePool (HandleBuf);
      FreePool (mProviderDb);
      mProviderDb = NULL;
      return Status;
    }
  }

  FreePool (HandleBuf);
  return EFI_SUCCESS;
}


/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Request         A null-terminated Unicode string in <ConfigRequest> format.
  @param Progress        On return, points to a character in the Request string.
                         Points to the string's null terminator if request was successful.
                         Points to the most recent '&' before the first failing name/value
                         pair (or the beginning of the string if the failure is in the
                         first name/value pair) if the request was not successful.
  @param Results         A null-terminated Unicode string in <ConfigAltResp> format which
                         has all values filled in for the names in the Request string.
                         String to be allocated by the called function.

  @retval  EFI_SUCCESS            The Results is filled with the requested values.
  @retval  EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval  EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeExtractConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  if (Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  *Progress = Request;
  return EFI_NOT_FOUND;
}

/**
  This function processes the results of changes in configuration.


  @param This            Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param Configuration   A null-terminated Unicode string in <ConfigResp> format.
  @param Progress        A pointer to a string filled in with the offset of the most
                         recent '&' before the first failing name/value pair (or the
                         beginning of the string if the failure is in the first
                         name/value pair) or the terminating NULL if all was successful.

  @retval  EFI_SUCCESS            The Results is processed successfully.
  @retval  EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval  EFI_NOT_FOUND          Routing data doesn't match any storage in this driver.

**/
EFI_STATUS
EFIAPI
FakeRouteConfig (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  if (Configuration == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_NOT_FOUND;
}


/**
  This function initialize the data mainly used in form browser.

  @retval EFI_SUCCESS          Initialize form data successfully.
  @retval Others               Fail to Initialize form data.

**/
EFI_STATUS
InitFormBrowser (
  VOID
  )
{
  EFI_STATUS                  Status;
  USER_MANAGER_CALLBACK_INFO  *CallbackInfo;
  EFI_HII_DATABASE_PROTOCOL   *HiiDatabase;
  EFI_HII_STRING_PROTOCOL     *HiiString;
  EFI_FORM_BROWSER2_PROTOCOL  *FormBrowser2;

  //
  // Initialize driver private data.
  //
  CallbackInfo = AllocateZeroPool (sizeof (USER_MANAGER_CALLBACK_INFO));
  if (CallbackInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CallbackInfo->Signature                   = USER_MANAGER_SIGNATURE;
  CallbackInfo->ConfigAccess.ExtractConfig  = FakeExtractConfig;
  CallbackInfo->ConfigAccess.RouteConfig    = FakeRouteConfig;
  CallbackInfo->ConfigAccess.Callback       = UserIdentifyManagerCallback;

  //
  // Locate Hii Database protocol.
  //
  Status = gBS->LocateProtocol (&gEfiHiiDatabaseProtocolGuid, NULL, (VOID **) &HiiDatabase);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  CallbackInfo->HiiDatabase = HiiDatabase;

  //
  // Locate HiiString protocol.
  //
  Status = gBS->LocateProtocol (&gEfiHiiStringProtocolGuid, NULL, (VOID **) &HiiString);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  CallbackInfo->HiiString = HiiString;

  //
  // Locate Formbrowser2 protocol.
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CallbackInfo->FormBrowser2  = FormBrowser2;
  CallbackInfo->DriverHandle  = NULL;
  
  //
  // Install Device Path Protocol and Config Access protocol to driver handle.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &CallbackInfo->DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  &mHiiVendorDevicePath,
                  &gEfiHiiConfigAccessProtocolGuid,
                  &CallbackInfo->ConfigAccess,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Publish HII data.
  //
  CallbackInfo->HiiHandle = HiiAddPackages (
                              &gUserIdentifyManagerGuid,
                              CallbackInfo->DriverHandle,
                              UserIdentifyManagerStrings,
                              UserIdentifyManagerVfrBin,
                              NULL
                              );
  if (CallbackInfo->HiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  mCallbackInfo = CallbackInfo;

  return EFI_SUCCESS;
}


/**
  Identify the user whose identification policy supports auto logon.

  @param[in]   ProviderIndex   The provider index in the provider list.
  @param[out]  User            Points to user user profile if a user is identified successfully.

  @retval EFI_SUCCESS          Identify a user with the specified provider successfully.
  @retval Others               Fail to identify a user.

**/
EFI_STATUS
IdentifyAutoLogonUser (
  IN  UINTN                                     ProviderIndex,
  OUT USER_PROFILE_ENTRY                        **User
  )
{
  EFI_STATUS    Status;
  EFI_USER_INFO *Info;
  UINT8         PolicyType;

  Info = AllocateZeroPool (sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_IDENTIFIER));
  if (Info == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Info->InfoType  = EFI_USER_INFO_IDENTIFIER_RECORD;
  Info->InfoSize  = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_IDENTIFIER);

  //
  // Identify the specified credential provider's auto logon user.
  //
  Status = mProviderDb->Provider[ProviderIndex]->User (
                                                   mProviderDb->Provider[ProviderIndex],
                                                   NULL,
                                                   (EFI_USER_INFO_IDENTIFIER *) (Info + 1)
                                                   );
  if (EFI_ERROR (Status)) {
    FreePool (Info);
    return Status;
  }
  
  //
  // Find user with the specified user ID.
  //
  *User   = NULL;
  Status  = FindUserProfileByInfo (User, NULL, Info, Info->InfoSize);
  FreePool (Info);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = GetIdentifyType ((EFI_USER_PROFILE_HANDLE) * User, &PolicyType);
  if (PolicyType == EFI_USER_INFO_IDENTITY_AND) {
    //
    // The identified user need also identified by other credential provider.
    // This can handle through select user.
    //
    return EFI_NOT_READY;
  }
  
  return Status;
}


/**
  Check whether the given console is ready.

  @param[in]   ProtocolGuid   Points to the protocol guid of sonsole .
  
  @retval TRUE     The given console is ready.
  @retval FALSE    The given console is not ready.
  
**/
BOOLEAN
CheckConsole (
  EFI_GUID                     *ProtocolGuid  
  )
{
  EFI_STATUS                   Status;
  UINTN                        HandleCount;
  EFI_HANDLE                   *HandleBuf;
  UINTN                        Index; 
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath;
  
  //
  // Try to find all the handle driver.
  //
  HandleCount = 0;
  HandleBuf   = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuf
                  );
  if (EFI_ERROR (Status)) {
    return FALSE;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    DevicePath = DevicePathFromHandle (HandleBuf[Index]);
    if (DevicePath != NULL) {
      FreePool (HandleBuf);
      return TRUE;
    }
  }
  FreePool (HandleBuf);  
  return FALSE;
}


/**
  Check whether the console is ready.

  @retval TRUE     The console is ready.
  @retval FALSE    The console is not ready.
  
**/
BOOLEAN
IsConsoleReady (
  VOID
  )
{
  if (!CheckConsole (&gEfiSimpleTextOutProtocolGuid)) {
    return FALSE;
  }

  if (!CheckConsole (&gEfiSimpleTextInProtocolGuid)) {
    if (!CheckConsole (&gEfiSimpleTextInputExProtocolGuid)) {
    return FALSE;
    }
  }
  
  return TRUE;
}


/**
  Identify a user to logon.

  @param[out]  User          Points to user user profile if a user is identified successfully.

  @retval EFI_SUCCESS        Identify a user successfully.

**/
EFI_STATUS
IdentifyUser (
  OUT USER_PROFILE_ENTRY                        **User
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_CREDENTIAL_LOGON_FLAGS    AutoLogon;
  EFI_USER_INFO                 *IdentifyInfo;
  EFI_USER_INFO_IDENTITY_POLICY *Identity;
  EFI_USER_CREDENTIAL2_PROTOCOL *UserCredential;
  USER_PROFILE_ENTRY            *UserEntry;

  //
  // Initialize credential providers.
  //
  InitProviderInfo ();

  //
  // Initialize user profile database.
  //
  InitUserProfileDb ();

  //
  // If only one user in system, and its identify policy is TRUE, then auto logon.
  //
  if (mUserProfileDb->UserProfileNum == 1) {
    UserEntry     = (USER_PROFILE_ENTRY *) mUserProfileDb->UserProfile[0];
    IdentifyInfo  = NULL;
    Status        = FindUserInfoByType (UserEntry, &IdentifyInfo, EFI_USER_INFO_IDENTITY_POLICY_RECORD);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    ASSERT (IdentifyInfo != NULL);

    Identity = (EFI_USER_INFO_IDENTITY_POLICY *) ((UINT8 *) (IdentifyInfo + 1));
    if (Identity->Type == EFI_USER_INFO_IDENTITY_TRUE) {
      mCurrentUser = (EFI_USER_PROFILE_HANDLE) UserEntry;
      UpdateUserInfo (UserEntry);
      *User = UserEntry;
      return EFI_SUCCESS;
    }
  }
  
  //
  // Find and login the default & AutoLogon user.
  //
  for (Index = 0; Index < mProviderDb->Count; Index++) {
    UserCredential = mProviderDb->Provider[Index];
    Status = UserCredential->Default (UserCredential, &AutoLogon);
    if (EFI_ERROR (Status)) {
      continue;
    }

    if ((AutoLogon & (EFI_CREDENTIAL_LOGON_FLAG_DEFAULT | EFI_CREDENTIAL_LOGON_FLAG_AUTO)) != 0) {
      Status = IdentifyAutoLogonUser (Index, &UserEntry);
      if (Status == EFI_SUCCESS) {
        mCurrentUser = (EFI_USER_PROFILE_HANDLE) UserEntry;
        UpdateUserInfo (UserEntry);
        *User = UserEntry;
        return EFI_SUCCESS;
      }
    }
  }
  
  if (!IsConsoleReady ()) {
    //
    // The console is still not ready for user selection.
    //
    return EFI_ACCESS_DENIED;
  }

  //
  // Select a user and identify it.
  //
  mCallbackInfo->FormBrowser2->SendForm (
                                 mCallbackInfo->FormBrowser2,
                                 &mCallbackInfo->HiiHandle,
                                 1,
                                 &gUserIdentifyManagerGuid,
                                 0,
                                 NULL,
                                 NULL
                                 );
  
  if (mIdentified) {
    *User = (USER_PROFILE_ENTRY *) mCurrentUser;
    UpdateUserInfo (*User);
    return EFI_SUCCESS;
  }
  
  return EFI_ACCESS_DENIED;
}


/**
  An empty function to pass error checking of CreateEventEx ().
 
  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context,
                        which is implementation-dependent.

**/
VOID
EFIAPI
InternalEmptyFuntion (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
}


/**
  Create, Signal, and Close the User Profile Changed event.

**/
VOID
SignalEventUserProfileChanged (
  VOID
  )
{
  EFI_STATUS    Status;
  EFI_EVENT     Event;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InternalEmptyFuntion,
                  NULL,
                  &gEfiEventUserProfileChangedGuid,
                  &Event
                  );
  ASSERT_EFI_ERROR (Status);
  gBS->SignalEvent (Event);
  gBS->CloseEvent (Event);
}


/**
  Create a new user profile.

  This function creates a new user profile with only a new user identifier attached and returns 
  its handle. The user profile is non-volatile, but the handle User can change across reboots.

  @param[in]  This               Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[out] User               On return, points to the new user profile handle. 
                                 The user profile handle is unique only during this boot.
 
  @retval EFI_SUCCESS            User profile was successfully created.
  @retval EFI_ACCESS_DENIED      Current user does not have sufficient permissions to create a 
                                 user profile.
  @retval EFI_UNSUPPORTED        Creation of new user profiles is not supported.
  @retval EFI_INVALID_PARAMETER  The User parameter is NULL.
  
**/
EFI_STATUS
EFIAPI
UserProfileCreate (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  OUT       EFI_USER_PROFILE_HANDLE             *User
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check the right of the current user.
  //
  if (!CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_MANAGE)) {
    if (!CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_ENROLL_OTHERS)) {
      return EFI_ACCESS_DENIED;
    }
  }
  
  //
  // Create new user profile
  //
  Status = CreateUserProfile ((USER_PROFILE_ENTRY **) User);
  if (EFI_ERROR (Status)) {
    return EFI_ACCESS_DENIED;
  }
  return EFI_SUCCESS;
}


/**
  Delete an existing user profile.

  @param[in] This                Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in] User                User profile handle. 

  @retval EFI_SUCCESS            User profile was successfully deleted.
  @retval EFI_ACCESS_DENIED      Current user does not have sufficient permissions to delete a user
                                 profile or there is only one user profile.
  @retval EFI_UNSUPPORTED        Deletion of new user profiles is not supported.
  @retval EFI_INVALID_PARAMETER  User does not refer to a valid user profile.
  
**/
EFI_STATUS
EFIAPI
UserProfileDelete (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User
  )
{
  EFI_STATUS  Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check the right of the current user.
  //
  if (!CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_MANAGE)) {
    return EFI_ACCESS_DENIED;
  }
  
  //
  // Delete user profile.
  //
  Status = DelUserProfile (User);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_INVALID_PARAMETER) {
      return EFI_ACCESS_DENIED;
    }
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}


/**
  Enumerate all of the enrolled users on the platform.

  This function returns the next enrolled user profile. To retrieve the first user profile handle,  
  point User at a NULL. Each subsequent call will retrieve another user profile handle until there 
  are no more, at which point User will point to NULL. 

  @param[in]      This           Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in, out] User           On entry, points to the previous user profile handle or NULL to 
                                 start enumeration. On exit, points to the next user profile handle
                                 or NULL if there are no more user profiles.

  @retval EFI_SUCCESS            Next enrolled user profile successfully returned. 
  @retval EFI_ACCESS_DENIED      Next enrolled user profile was not successfully returned.
  @retval EFI_INVALID_PARAMETER  The User parameter is NULL.
**/
EFI_STATUS
EFIAPI
UserProfileGetNext (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN OUT    EFI_USER_PROFILE_HANDLE             *User
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  Status = FindUserProfile ((USER_PROFILE_ENTRY **) User, TRUE, NULL);
  if (EFI_ERROR (Status)) {
    return EFI_ACCESS_DENIED;
  }
  return EFI_SUCCESS;
}


/**
  Return the current user profile handle.

  @param[in]  This               Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[out] CurrentUser        On return, points to the current user profile handle.

  @retval EFI_SUCCESS            Current user profile handle returned successfully. 
  @retval EFI_INVALID_PARAMETER  The CurrentUser parameter is NULL.
  
**/
EFI_STATUS
EFIAPI
UserProfileCurrent (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  OUT       EFI_USER_PROFILE_HANDLE             *CurrentUser
  )
{  
  //
  // Get current user profile.
  //
  if ((This == NULL) || (CurrentUser == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *CurrentUser = mCurrentUser;
  return EFI_SUCCESS;
}


/**
  Identify a user.

  Identify the user and, if authenticated, returns the user handle and changes the current
  user profile. All user information marked as private in a previously selected profile
  is no longer available for inspection. 
  Whenever the current user profile is changed then the an event with the GUID 
  EFI_EVENT_GROUP_USER_PROFILE_CHANGED is signaled.

  @param[in]  This               Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[out] User               On return, points to the user profile handle for the current
                                 user profile.

  @retval EFI_SUCCESS            User was successfully identified.
  @retval EFI_ACCESS_DENIED      User was not successfully identified.
  @retval EFI_INVALID_PARAMETER  The User parameter is NULL.
  
**/
EFI_STATUS
EFIAPI
UserProfileIdentify (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  OUT       EFI_USER_PROFILE_HANDLE             *User
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (User == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (mCurrentUser != NULL) {
    *User = mCurrentUser;
    return EFI_SUCCESS;
  }
  
  //
  // Identify user
  //
  Status = IdentifyUser ((USER_PROFILE_ENTRY **) User);
  if (EFI_ERROR (Status)) {
    return EFI_ACCESS_DENIED;
  }
  
  //
  // Publish the user info into the EFI system configuration table.
  //
  PublishUserTable ();

  //
  // Signal User Profile Changed event.
  //
  SignalEventUserProfileChanged ();
  return EFI_SUCCESS;
}

/**
  Find a user using a user information record.

  This function searches all user profiles for the specified user information record.
  The search starts with the user information record handle following UserInfo and 
  continues until either the information is found or there are no more user profiles.
  A match occurs when the Info.InfoType field matches the user information record
  type and the user information record data matches the portion of Info.

  @param[in]      This           Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in, out] User           On entry, points to the previously returned user profile 
                                 handle, or NULL to start searching with the first user profile.
                                 On return, points to the user profile handle, or NULL if not
                                 found.
  @param[in, out] UserInfo       On entry, points to the previously returned user information
                                 handle, or NULL to start searching with the first. On return, 
                                 points to the user information handle of the user information
                                 record, or NULL if not found. Can be NULL, in which case only 
                                 one user information record per user can be returned. 
  @param[in]      Info           Points to the buffer containing the user information to be 
                                 compared to the user information record. If the user information 
                                 record data is empty, then only the user information record type 
                                 is compared. If InfoSize is 0, then the user information record 
                                 must be empty.

  @param[in]      InfoSize       The size of Info, in bytes. 

  @retval EFI_SUCCESS            User information was found. User points to the user profile
                                 handle, and UserInfo points to the user information handle.
  @retval EFI_NOT_FOUND          User information was not found. User points to NULL, and  
                                 UserInfo points to NULL.
  @retval EFI_INVALID_PARAMETER  User is NULL. Or Info is NULL.                           
  
**/
EFI_STATUS
EFIAPI
UserProfileFind (
  IN     CONST EFI_USER_MANAGER_PROTOCOL        *This,
  IN OUT EFI_USER_PROFILE_HANDLE                *User,
  IN OUT EFI_USER_INFO_HANDLE                   *UserInfo OPTIONAL,
  IN     CONST EFI_USER_INFO                    *Info,
  IN     UINTN                                  InfoSize
  )
{
  EFI_STATUS  Status;
  UINTN       Size;

  if ((This == NULL) || (User == NULL) || (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (InfoSize == 0) {
    //
    // If InfoSize is 0, then the user information record must be empty.
    //
    if (Info->InfoSize != sizeof (EFI_USER_INFO)) {
      return EFI_INVALID_PARAMETER;
    }
  } else {
    if (InfoSize != Info->InfoSize) {
      return EFI_INVALID_PARAMETER;
    }
  }
  Size = Info->InfoSize;  
  
  //
  // Find user profile accdoring to user information.
  //
  Status = FindUserProfileByInfo (
            (USER_PROFILE_ENTRY **) User,
            (EFI_USER_INFO **) UserInfo,
            (EFI_USER_INFO *) Info,
            Size
            );
  if (EFI_ERROR (Status)) {
    *User = NULL;
    if (UserInfo != NULL) {
      *UserInfo = NULL;
    }
    return EFI_NOT_FOUND;
  }
  
  return EFI_SUCCESS;
}


/**
  Return information attached to the user.

  This function returns user information. The format of the information is described in User 
  Information. The function may return EFI_ACCESS_DENIED if the information is marked private 
  and the handle specified by User is not the current user profile. The function may return 
  EFI_ACCESS_DENIED if the information is marked protected and the information is associated 
  with a credential provider for which the user has not been authenticated.

  @param[in]      This           Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in]      User           Handle of the user whose profile will be retrieved. 
  @param[in]      UserInfo       Handle of the user information data record.   
  @param[out]     Info           On entry, points to a buffer of at least *InfoSize bytes. On exit,  
                                 holds the user information. If the buffer is too small to hold the  
                                 information, then EFI_BUFFER_TOO_SMALL is returned and InfoSize is 
                                 updated to contain the number of bytes actually required. 
  @param[in, out] InfoSize       On entry, points to the size of Info. On return, points to the size  
                                 of the user information. 

  @retval EFI_SUCCESS            Information returned successfully.
  @retval EFI_ACCESS_DENIED      The information about the specified user cannot be accessed by the 
                                 current user.
  @retval EFI_BUFFER_TOO_SMALL   The number of bytes specified by *InfoSize is too small to hold the 
                                 returned data. The actual size required is returned in *InfoSize.
  @retval EFI_NOT_FOUND          User does not refer to a valid user profile or UserInfo does not refer 
                                 to a valid user info handle.
  @retval EFI_INVALID_PARAMETER  Info is NULL or InfoSize is NULL.
  
**/
EFI_STATUS
EFIAPI
UserProfileGetInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN        EFI_USER_INFO_HANDLE                UserInfo,
  OUT       EFI_USER_INFO                       *Info,
  IN OUT    UINTN                               *InfoSize
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (InfoSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((*InfoSize != 0) && (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((User == NULL) || (UserInfo == NULL)) {
    return EFI_NOT_FOUND;
  }
  
  Status = GetUserInfo (User, UserInfo, Info, InfoSize, TRUE);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_BUFFER_TOO_SMALL) {
      return EFI_BUFFER_TOO_SMALL;
    }
    return EFI_ACCESS_DENIED;
  }
  return EFI_SUCCESS;
}


/**
  Add or update user information.

  This function changes user information.  If NULL is pointed to by UserInfo, then a new user 
  information record is created and its handle is returned in UserInfo. Otherwise, the existing  
  one is replaced.
  If EFI_USER_INFO_IDENITTY_POLICY_RECORD is changed, it is the caller's responsibility to keep 
  it to be synced with the information on credential providers.
  If EFI_USER_INFO_EXCLUSIVE is specified in Info and a user information record of the same 
  type already exists in the user profile, then EFI_ACCESS_DENIED will be returned and UserInfo
  will point to the handle of the existing record.

  @param[in]      This            Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in]      User            Handle of the user whose profile will be retrieved. 
  @param[in, out] UserInfo        Handle of the user information data record.   
  @param[in]      Info            On entry, points to a buffer of at least *InfoSize bytes. On exit, 
                                  holds the user information. If the buffer is too small to hold the 
                                  information, then EFI_BUFFER_TOO_SMALL is returned and InfoSize is 
                                  updated to contain the number of bytes actually required. 
  @param[in]      InfoSize        On entry, points to the size of Info. On return, points to the size 
                                  of the user information. 

  @retval EFI_SUCCESS             Information returned successfully.
  @retval EFI_ACCESS_DENIED       The record is exclusive.
  @retval EFI_SECURITY_VIOLATION  The current user does not have permission to change the specified 
                                  user profile or user information record.
  @retval EFI_NOT_FOUND           User does not refer to a valid user profile or UserInfo does not  
                                  refer to a valid user info handle.
  @retval EFI_INVALID_PARAMETER   UserInfo is NULL or Info is NULL. 
**/
EFI_STATUS
EFIAPI
UserProfileSetInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN OUT    EFI_USER_INFO_HANDLE                *UserInfo,
  IN CONST  EFI_USER_INFO                       *Info,
  IN        UINTN                               InfoSize
  )
{
  EFI_STATUS  Status;

  if ((This == NULL) || (User == NULL) || (UserInfo == NULL) || (Info == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check the right of the current user.
  //
  if (User != mCurrentUser) {
    if (!CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_MANAGE)) {
      if (*UserInfo != NULL) {
        //
        // Can't update info in other profiles without MANAGE right.
        //
        return EFI_SECURITY_VIOLATION;
      }
      
      if (!CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_ENROLL_OTHERS)) {
        //
        // Can't add info into other profiles.
        //
        return EFI_SECURITY_VIOLATION;
      }
    }
  }

  if (User == mCurrentUser) {
    if (CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_ENROLL_SELF)) {
      //
      // Only identify policy can be added/updated.
      //
      if (Info->InfoType != EFI_USER_INFO_IDENTITY_POLICY_RECORD) {
        return EFI_SECURITY_VIOLATION;
      }
    }
  }
  
  //
  // Modify user information.
  //
  Status = ModifyUserInfo (User, (EFI_USER_INFO **) UserInfo, Info, InfoSize);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_ACCESS_DENIED) {
      return EFI_ACCESS_DENIED;      
    }
    return EFI_SECURITY_VIOLATION;
  }
  return EFI_SUCCESS;
}


/**
  Called by credential provider to notify of information change.

  This function allows the credential provider to notify the User Identity Manager when user status  
  has changed.
  If the User Identity Manager doesn't support asynchronous changes in credentials, then this function 
  should return EFI_UNSUPPORTED. 
  If current user does not exist, and the credential provider can identify a user, then make the user 
  to be current user and signal the EFI_EVENT_GROUP_USER_PROFILE_CHANGED event.
  If current user already exists, and the credential provider can identify another user, then switch 
  current user to the newly identified user, and signal the EFI_EVENT_GROUP_USER_PROFILE_CHANGED event.
  If current user was identified by this credential provider and now the credential provider cannot identify 
  current user, then logout current user and signal the EFI_EVENT_GROUP_USER_PROFILE_CHANGED event.

  @param[in] This          Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in] Changed       Handle on which is installed an instance of the EFI_USER_CREDENTIAL2_PROTOCOL 
                           where the user has changed.

  @retval EFI_SUCCESS      The User Identity Manager has handled the notification.
  @retval EFI_NOT_READY    The function was called while the specified credential provider was not selected.
  @retval EFI_UNSUPPORTED  The User Identity Manager doesn't support asynchronous notifications.
  
**/
EFI_STATUS
EFIAPI
UserProfileNotify (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_HANDLE                          Changed
  )
{    
  return EFI_UNSUPPORTED;
}


/**
  Delete user information.

  Delete the user information attached to the user profile specified by the UserInfo.

  @param[in] This            Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in] User            Handle of the user whose information will be deleted.
  @param[in] UserInfo        Handle of the user information to remove.

  @retval EFI_SUCCESS        User information deleted successfully.
  @retval EFI_NOT_FOUND      User information record UserInfo does not exist in the user profile.
  @retval EFI_ACCESS_DENIED  The current user does not have permission to delete this user information. 
  
**/
EFI_STATUS
EFIAPI
UserProfileDeleteInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN        EFI_USER_INFO_HANDLE                UserInfo
  )
{
  EFI_STATUS  Status;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Check the right of the current user.
  //
  if (User != mCurrentUser) {
    if (!CheckCurrentUserAccessRight (EFI_USER_INFO_ACCESS_MANAGE)) {
      return EFI_ACCESS_DENIED;
    }
  }
  
  //
  // Delete user information.
  //
  Status = DelUserInfo (User, UserInfo, TRUE);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      return EFI_NOT_FOUND;
    }
    return EFI_ACCESS_DENIED;
  } 
  return EFI_SUCCESS;
}


/**
  Enumerate user information of all the enrolled users on the platform.

  This function returns the next user information record. To retrieve the first user   
  information record handle, point UserInfo at a NULL. Each subsequent call will retrieve 
  another user information record handle until there are no more, at which point UserInfo 
  will point to NULL. 

  @param[in]      This           Points to this instance of the EFI_USER_MANAGER_PROTOCOL.
  @param[in]      User           Handle of the user whose information will be deleted.
  @param[in, out] UserInfo       Handle of the user information to remove.

  @retval EFI_SUCCESS            User information returned.
  @retval EFI_NOT_FOUND          No more user information found.
  @retval EFI_INVALID_PARAMETER  UserInfo is NULL.
  
**/
EFI_STATUS
EFIAPI
UserProfileGetNextInfo (
  IN CONST  EFI_USER_MANAGER_PROTOCOL           *This,
  IN        EFI_USER_PROFILE_HANDLE             User,
  IN OUT    EFI_USER_INFO_HANDLE                *UserInfo
  )
{
  if ((This == NULL) || (UserInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Get next user information entry.
  //
  return FindUserInfo (User, (EFI_USER_INFO **) UserInfo, TRUE, NULL);
}


/**
  Main entry for this driver.

  @param[in] ImageHandle     Image handle this driver.
  @param[in] SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS         This function always complete successfully.

**/
EFI_STATUS
EFIAPI
UserIdentifyManagerInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{

  EFI_STATUS  Status;

  //
  // It is NOT robust enough to be included in production.
  //
  #error "This implementation is just a sample, please comment this line if you really want to use this driver."

  //
  // Initiate form browser.
  //
  InitFormBrowser ();

  //
  // Install protocol interfaces for the User Identity Manager.
  //
  Status = gBS->InstallProtocolInterface (
                  &mCallbackInfo->DriverHandle,
                  &gEfiUserManagerProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gUserIdentifyManager
                  );
  ASSERT_EFI_ERROR (Status);  

  LoadDeferredImageInit (ImageHandle);
  return EFI_SUCCESS;
}


