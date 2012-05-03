/** @file
  The functions to add a user profile.
    
Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UserProfileManager.h"


/**
  Get user name from the popup windows.
  
  @param[in, out]  UserNameLen  On entry, point to UserName buffer lengh, in bytes.
                                On exit, point to input user name length, in bytes.
  @param[out]      UserName     The buffer to hold the input user name.
 
  @retval EFI_ABORTED           It is given up by pressing 'ESC' key.
  @retval EFI_NOT_READY         Not a valid input at all.
  @retval EFI_SUCCESS           Get a user name successfully.

**/
EFI_STATUS
GetUserNameInput (
  IN OUT  UINTN         *UserNameLen,
     OUT  CHAR16        *UserName
  )
{
  EFI_INPUT_KEY Key;
  UINTN         NameLen;
  CHAR16        Name[USER_NAME_LENGTH];

  NameLen = 0;
  while (TRUE) {
    Name[NameLen]     = L'_';
    Name[NameLen + 1] = L'\0';
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      L"Input User Name",
      L"---------------------",
      Name,
      NULL
      );
    //
    // Check key.
    //
    if (Key.ScanCode == SCAN_NULL) {
      if (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
        //
        // Add the null terminator.
        //
        Name[NameLen] = 0;
        NameLen++;
        break;
      } else if ((Key.UnicodeChar == CHAR_NULL) ||
                 (Key.UnicodeChar == CHAR_TAB) ||
                 (Key.UnicodeChar == CHAR_LINEFEED)
                 ) {
        continue;
      } else {
        if (Key.UnicodeChar == CHAR_BACKSPACE) {
          if (NameLen > 0) {
            NameLen--;
          }
        } else {
          Name[NameLen] = Key.UnicodeChar;
          NameLen++;
          if (NameLen + 1 == USER_NAME_LENGTH) {
            //
            // Add the null terminator.
            //
            Name[NameLen] = 0;
            NameLen++;
            break;
          }
        }
      }
    }

    if (Key.ScanCode == SCAN_ESC) {
      return EFI_ABORTED;
    }
  }

  if (NameLen <= 1) {
    return EFI_NOT_READY;
  }

  if (*UserNameLen < NameLen * sizeof (CHAR16)) {
    return EFI_NOT_READY;
  }

  *UserNameLen = NameLen * sizeof (CHAR16);
  CopyMem (UserName, Name, *UserNameLen);
  
  return EFI_SUCCESS;
}

/**
  Set a user's username.

  @param[in]   User          Handle of a user profile .
  @param[in]   UserNameLen   The lengh of UserName.
  @param[in]   UserName      Point to the buffer of user name.

  @retval EFI_NOT_READY      The usernme in mAddUserName had been used.
  @retval EFI_SUCCESS        Change the user's username successfully with 
                             username in mAddUserName.

**/
EFI_STATUS
SetUserName (
  IN  EFI_USER_PROFILE_HANDLE    User,
  IN  UINTN                      UserNameLen,
  IN  CHAR16                     *UserName
  )
{
  EFI_STATUS              Status;
  EFI_USER_INFO_HANDLE    UserInfo;
  EFI_USER_PROFILE_HANDLE TempUser;
  EFI_USER_INFO           *NewUserInfo;
   
  NewUserInfo = AllocateZeroPool (sizeof (EFI_USER_INFO) + UserNameLen);
  ASSERT (NewUserInfo != NULL);

  NewUserInfo->InfoType    = EFI_USER_INFO_NAME_RECORD;
  NewUserInfo->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | 
                             EFI_USER_INFO_PUBLIC | 
                             EFI_USER_INFO_EXCLUSIVE;
  NewUserInfo->InfoSize    = (UINT32) (sizeof (EFI_USER_INFO) + UserNameLen);
  CopyMem ((UINT8 *) (NewUserInfo + 1), UserName, UserNameLen);
  TempUser  = NULL;
  Status    = mUserManager->Find (
                              mUserManager,
                              &TempUser,
                              NULL,
                              NewUserInfo,
                              NewUserInfo->InfoSize
                              );
  if (!EFI_ERROR (Status)) {
    //
    // The user name had been used, return error.
    //
    FreePool (NewUserInfo);
    return EFI_NOT_READY;
  }

  UserInfo = NULL;
  mUserManager->SetInfo (
                  mUserManager,
                  User,
                  &UserInfo,
                  NewUserInfo,
                  NewUserInfo->InfoSize
                  );
  FreePool (NewUserInfo);
  return EFI_SUCCESS;
}


/**
  Set create date of the specified user.

  @param[in]  User               Handle of a user profile.

**/
VOID
SetCreateDate (
  IN        EFI_USER_PROFILE_HANDLE   User
  )
{
  EFI_STATUS                Status;
  EFI_USER_INFO_HANDLE      UserInfo;
  EFI_USER_INFO_CREATE_DATE Date;
  EFI_USER_INFO             *NewUserInfo;
  
  NewUserInfo = AllocateZeroPool (
                  sizeof (EFI_USER_INFO) +
                  sizeof (EFI_USER_INFO_CREATE_DATE)
                  );
  ASSERT (NewUserInfo != NULL);

  NewUserInfo->InfoType    = EFI_USER_INFO_CREATE_DATE_RECORD;
  NewUserInfo->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | 
                             EFI_USER_INFO_PUBLIC | 
                             EFI_USER_INFO_EXCLUSIVE;
  NewUserInfo->InfoSize    = sizeof (EFI_USER_INFO) + sizeof (EFI_USER_INFO_CREATE_DATE);
  Status                   = gRT->GetTime (&Date, NULL);
  if (EFI_ERROR (Status)) {
    FreePool (NewUserInfo);
    return ;
  }

  CopyMem ((UINT8 *) (NewUserInfo + 1), &Date, sizeof (EFI_USER_INFO_CREATE_DATE));
  UserInfo = NULL;
  mUserManager->SetInfo (
                  mUserManager,
                  User,
                  &UserInfo,
                  NewUserInfo,
                  NewUserInfo->InfoSize
                  );
  FreePool (NewUserInfo);
}


/**
  Set the default identity policy of the specified user.

  @param[in]  User               Handle of a user profile. 

**/
VOID
SetIdentityPolicy (
  IN        EFI_USER_PROFILE_HANDLE   User
  )
{
  EFI_USER_INFO_IDENTITY_POLICY *Policy;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *NewUserInfo;
  
  NewUserInfo = AllocateZeroPool (
                  sizeof (EFI_USER_INFO) + 
                  sizeof (EFI_USER_INFO_IDENTITY_POLICY)
                  );
  ASSERT (NewUserInfo != NULL);
  
  Policy                   = (EFI_USER_INFO_IDENTITY_POLICY *) (NewUserInfo + 1);
  Policy->Type             = EFI_USER_INFO_IDENTITY_TRUE;
  Policy->Length           = sizeof (EFI_USER_INFO_IDENTITY_POLICY);

  NewUserInfo->InfoType    = EFI_USER_INFO_IDENTITY_POLICY_RECORD;
  NewUserInfo->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | 
                             EFI_USER_INFO_PUBLIC | 
                             EFI_USER_INFO_EXCLUSIVE;
  NewUserInfo->InfoSize    = sizeof (EFI_USER_INFO) + Policy->Length;
  UserInfo                 = NULL;
  mUserManager->SetInfo (
                  mUserManager,
                  User,
                  &UserInfo,
                  NewUserInfo,
                  NewUserInfo->InfoSize
                  );
  FreePool (NewUserInfo);
}


/**
  Set the default access policy of the specified user.

  @param[in]  User               Handle of a user profile. 

**/
VOID
SetAccessPolicy (
  IN        EFI_USER_PROFILE_HANDLE   User
  )
{
  EFI_USER_INFO_ACCESS_CONTROL  *Control;
  EFI_USER_INFO_HANDLE          UserInfo;
  EFI_USER_INFO                 *NewUserInfo;
  
  NewUserInfo = AllocateZeroPool (
                  sizeof (EFI_USER_INFO) + 
                  sizeof (EFI_USER_INFO_ACCESS_CONTROL)
                  );
  ASSERT (NewUserInfo != NULL);
  
  Control                  = (EFI_USER_INFO_ACCESS_CONTROL *) (NewUserInfo + 1);
  Control->Type            = EFI_USER_INFO_ACCESS_ENROLL_SELF;
  Control->Size            = sizeof (EFI_USER_INFO_ACCESS_CONTROL);

  NewUserInfo->InfoType    = EFI_USER_INFO_ACCESS_POLICY_RECORD;
  NewUserInfo->InfoAttribs = EFI_USER_INFO_STORAGE_PLATFORM_NV | 
                             EFI_USER_INFO_PUBLIC | 
                             EFI_USER_INFO_EXCLUSIVE;
  NewUserInfo->InfoSize    = sizeof (EFI_USER_INFO) + Control->Size;
  UserInfo                 = NULL;
  mUserManager->SetInfo (
                  mUserManager,
                  User,
                  &UserInfo,
                  NewUserInfo,
                  NewUserInfo->InfoSize
                  );
  FreePool (NewUserInfo);
}


/**
  Add a new user profile into the user profile database.

**/
VOID
CallAddUser (
  VOID
  )
{
  EFI_STATUS              Status;
  EFI_INPUT_KEY           Key;
  EFI_USER_PROFILE_HANDLE User;
  UINTN                   UserNameLen;
  CHAR16                  UserName[USER_NAME_LENGTH];
  CHAR16                  *QuestionStr;
  CHAR16                  *PromptStr;

  QuestionStr = NULL;
  PromptStr   = NULL;
  
  //
  // Get user name to add.
  //
  UserNameLen = sizeof (UserName);
  Status = GetUserNameInput (&UserNameLen, UserName);
  if (EFI_ERROR (Status)) {
    if (Status != EFI_ABORTED) {
      QuestionStr = GetStringById (STRING_TOKEN (STR_GET_USERNAME_FAILED));
      PromptStr   = GetStringById (STRING_TOKEN (STR_STROKE_KEY_CONTINUE)); 
      goto Done;
    }
    return ;
  }

  //
  // Create a new user profile.
  //
  User    = NULL;
  Status  = mUserManager->Create (mUserManager, &User);
  if (EFI_ERROR (Status)) {
    QuestionStr = GetStringById (STRING_TOKEN (STR_CREATE_PROFILE_FAILED));
    PromptStr   = GetStringById (STRING_TOKEN (STR_STROKE_KEY_CONTINUE)); 
  } else {
    //
    // Add default user information.
    //
    Status = SetUserName (User, UserNameLen, UserName);
    if (EFI_ERROR (Status)) {
      QuestionStr = GetStringById (STRING_TOKEN (STR_USER_ALREADY_EXISTED));
      PromptStr   = GetStringById (STRING_TOKEN (STR_STROKE_KEY_CONTINUE)); 
      goto Done;
    }

    SetCreateDate (User);
    SetIdentityPolicy (User);
    SetAccessPolicy (User);

    QuestionStr = GetStringById (STRING_TOKEN (STR_CREATE_PROFILE_SUCCESS));
    PromptStr   = GetStringById (STRING_TOKEN (STR_STROKE_KEY_CONTINUE)); 
  }

Done:
  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
    &Key,
    QuestionStr,
    L"",
    PromptStr,
    NULL
    );
  FreePool (QuestionStr);
  FreePool (PromptStr);
}

