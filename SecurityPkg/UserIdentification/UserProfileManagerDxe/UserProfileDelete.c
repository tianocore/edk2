/** @file
  The functions to delete a user profile.
    
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
  Get the username from the specified user.

  @param[in]   User              Handle of a user profile. 

  @retval EFI_STRING_ID          The String Id of the user's username.

**/
EFI_STRING_ID 
GetUserName (
  IN  EFI_USER_PROFILE_HANDLE                   User
  )
{
  EFI_STATUS            Status;
  EFI_USER_INFO_HANDLE  UserInfo;
  EFI_USER_INFO         *Info;
  UINTN                 InfoSize;
  UINTN                 MemSize;
  UINTN                 NameLen;
  CHAR16                UserName[USER_NAME_LENGTH];
  EFI_STRING_ID         UserId;
  
  //
  // Allocate user information memory.
  //
  MemSize = sizeof (EFI_USER_INFO) + 63;
  Info    = AllocateZeroPool (MemSize);
  ASSERT (Info != NULL);
  
  //
  // Get user name information.
  //
  UserInfo = NULL;
  while (TRUE) {
    InfoSize = MemSize;
    //
    // Get next user information.
    //
    Status = mUserManager->GetNextInfo (
                             mUserManager,
                             User,
                             &UserInfo
                             );
    if (EFI_ERROR (Status)) {
      break;
    }

    Status = mUserManager->GetInfo (
                             mUserManager,
                             User,
                             UserInfo,
                             Info,
                             &InfoSize
                             );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MemSize = InfoSize;
      FreePool (Info);
      Info = AllocateZeroPool (MemSize);
      ASSERT (Info != NULL);

      Status = mUserManager->GetInfo (
                               mUserManager,
                               User,
                               UserInfo,
                               Info,
                               &InfoSize
                               );
    }
    //
    // Check user information.
    //
    if (Status == EFI_SUCCESS) {
      if (Info->InfoType == EFI_USER_INFO_NAME_RECORD) {
        NameLen = Info->InfoSize - sizeof (EFI_USER_INFO);
        if (NameLen > USER_NAME_LENGTH * sizeof (CHAR16)) {
          NameLen = USER_NAME_LENGTH * sizeof (CHAR16);
        }
        ASSERT (NameLen >= sizeof (CHAR16));
        CopyMem (UserName, (UINT8 *) (Info + 1), NameLen);
        UserName[NameLen / sizeof (CHAR16) - 1] = 0;
        UserId = HiiSetString (
                   mCallbackInfo->HiiHandle,
                   0,
                   UserName,
                   NULL
                   );
        if (UserId != 0) {
          FreePool (Info);
          return UserId;
        }
      }
    }
  }

  FreePool (Info);
  return 0;
}


/**
  Add a username item in form.

  @param[in]  User          Points to the user profile whose username is added. 
  @param[in]  Index         The index of the user in the user name list
  @param[in]  OpCodeHandle  Points to container for dynamic created opcodes.

**/
VOID
AddUserToForm (
  IN  EFI_USER_PROFILE_HANDLE                   User,
  IN  UINT16                                    Index,
  IN  VOID                                      *OpCodeHandle
  )
{
  EFI_STRING_ID NameId;

  //
  // Get user name
  //
  NameId = GetUserName (User);
  if (NameId == 0) {
    return ;
  }
  
  //
  // Create user name option.
  //
  switch (Index & KEY_FIRST_FORM_MASK) {
  case KEY_MODIFY_USER:
    HiiCreateGotoOpCode (
      OpCodeHandle,                   // Container for dynamic created opcodes
      FORMID_USER_INFO,               // Target Form ID
      NameId,                         // Prompt text
      STRING_TOKEN (STR_NULL_STRING), // Help text
      EFI_IFR_FLAG_CALLBACK,          // Question flag
      Index                           // Question ID
      );
    break;

  case KEY_DEL_USER:
    HiiCreateActionOpCode (
      OpCodeHandle,                   // Container for dynamic created opcodes
      Index,                          // Question ID
      NameId,                         // Prompt text
      STRING_TOKEN (STR_NULL_STRING), // Help text
      EFI_IFR_FLAG_CALLBACK,          // Question flag
      0                               // Action String ID
      );
    break;

  default:
    break;
  }
}


/**
  Delete the user specified by UserIndex in user profile database.

  @param[in]  UserIndex       The index of user in the user name list 
                              to be deleted.

**/
VOID
DeleteUser (
  IN UINT8                                      UserIndex
  )
{
  EFI_STATUS              Status;
  EFI_USER_PROFILE_HANDLE User;
  EFI_INPUT_KEY           Key;
  EFI_USER_INFO_HANDLE    UserInfo;
  EFI_USER_INFO           *Info;
  UINTN                   InfoSize;

  //
  // Find specified user profile and delete it.
  //
  User    = NULL;
  Status  = mUserManager->GetNext (mUserManager, &User);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  while (UserIndex > 1) {
    Status = mUserManager->GetNext (mUserManager, &User);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    UserIndex--;
  }

  if (UserIndex == 1) {
    //
    // Get the identification policy.
    //
    Status = FindInfoByType (User, EFI_USER_INFO_IDENTITY_POLICY_RECORD, &UserInfo);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    InfoSize = 0;
    Info = NULL;
    Status   = mUserManager->GetInfo (mUserManager, User, UserInfo, Info, &InfoSize);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Info = AllocateZeroPool (InfoSize);
      if (Info == NULL) {
        goto Done;
      }
      Status = mUserManager->GetInfo (mUserManager, User, UserInfo, Info, &InfoSize);
    }

    //
    // Delete the user on the credential providers by its identification policy.
    //
    ASSERT (Info != NULL);
    DeleteCredentialFromProviders ((UINT8 *)(Info + 1), Info->InfoSize - sizeof (EFI_USER_INFO), User);
    FreePool (Info);
    
    Status = mUserManager->Delete (mUserManager, User);
    if (EFI_ERROR (Status)) {
      goto Done;
    }
    CreatePopUp (
      EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
      &Key,
      L"Delete User Succeed!",
      L"",
      L"Please Press Any Key to Continue ...",
      NULL
      );
    return ;  
  }

Done:
  CreatePopUp (
    EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE,
    &Key,
    L"Delete User Failed!",
    L"",
    L"Please Press Any Key to Continue ...",
    NULL
    );
}


/**
  Display user select form, cab select a user to delete.

**/
VOID
SelectUserToDelete (
  VOID
  )
{
  EFI_STATUS              Status;
  UINT8                   Index;
  EFI_USER_PROFILE_HANDLE User;
  EFI_USER_PROFILE_HANDLE CurrentUser;
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
  StartLabel->Number        = LABEL_USER_DEL_FUNC;

  EndLabel = (EFI_IFR_GUID_LABEL *) HiiCreateGuidOpCode (
                                      EndOpCodeHandle,
                                      &gEfiIfrTianoGuid,
                                      NULL,
                                      sizeof (EFI_IFR_GUID_LABEL)
                                      );
  EndLabel->ExtendOpCode  = EFI_IFR_EXTEND_OP_LABEL;
  EndLabel->Number        = LABEL_END;

  //
  // Add each user can be deleted.
  //
  User  = NULL;
  Index = 1;
  mUserManager->Current (mUserManager, &CurrentUser);
  while (TRUE) {
    Status = mUserManager->GetNext (mUserManager, &User);
    if (EFI_ERROR (Status)) {
      break;
    }

    if (User != CurrentUser) {
      AddUserToForm (
        User,
        (UINT16)(KEY_DEL_USER | KEY_SELECT_USER | Index),
        StartOpCodeHandle
        );
    }
    Index++;
  }

  HiiUpdateForm (
    mCallbackInfo->HiiHandle, // HII handle
    &gUserProfileManagerGuid, // Formset GUID
    FORMID_DEL_USER,          // Form ID
    StartOpCodeHandle,        // Label for where to insert opcodes
    EndOpCodeHandle           // Replace data
    );

  HiiFreeOpCodeHandle (StartOpCodeHandle);
  HiiFreeOpCodeHandle (EndOpCodeHandle);
}
